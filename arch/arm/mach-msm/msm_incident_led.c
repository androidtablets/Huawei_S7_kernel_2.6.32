/* 
 * 
 * Copyright (C) 2010 Huawei, Inc.
 *
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <../../../drivers/staging/android/timed_output.h>

#include <mach/gpio.h>
#include <linux/delay.h>

#define INCIDENT_LED_DEVICE_DUG 0
#if (INCIDENT_LED_DEVICE_DUG)
#define INCIDENT_LED_DEV_DUG(format,arg...)printk(KERN_ALERT format, ## arg); 
#define INCIDENT_LED_DEV_INFO(format,arg...)printk(KERN_ALERT format, ## arg); 
#define INCIDENT_LED_DEV_ERR(format,arg...)printk(KERN_ALERT format, ## arg); 
#else
#define INCIDENT_LED_DEV_DUG(format,arg...)do { (void)(format); } while (0)
#define INCIDENT_LED_DEV_INFO(format,arg...)do { (void)(format); } while (0)
#define INCIDENT_LED_DEV_ERR(format,arg...)do { (void)(format); } while (0)
#endif

struct incident_led_blink_data {
  struct timed_output_dev dev;
  struct hrtimer timer;
  spinlock_t lock;
  struct work_struct work_incident_led_blink_on;
  struct work_struct work_incident_led_blink_off;
  struct work_struct work_incident_led_on_all_long;
};

#define LED_GPIO_INCIDENT_LED   99

#define INCIDENT_FRE_LEVEL_0  0   // high frequence level, 2Hz
#define INCIDENT_FRE_LEVEL_1  1   // middle frequence level, 1Hz
#define INCIDENT_FRE_LEVEL_2  2  // low frequence level, 0.5Hz

// NOTICE: get led blink frequence from /proc/led/fre_incident_led, refering to leds_base.c 
extern unsigned int fre_incident_level;
static int blink = 0;

// TODO: only blink 30 times HERE. Maybe a death loop in the future.
static void set_incident_led_blink(int on)
{
  int rc;
  int i=0;
  //int freq = fre_incident_level;
  int t_sleep_ms = 500;
  // NOTICE: sleep time between every jump
  switch(fre_incident_level) {
  case INCIDENT_FRE_LEVEL_0 :  // high frequence level, 2Hz
    t_sleep_ms = 250;
    break;
  case INCIDENT_FRE_LEVEL_1 :  // middle frequence level, 1Hz
    t_sleep_ms = 500;  
    break;
  case INCIDENT_FRE_LEVEL_2 : // low frequence level, 0.5Hz
    t_sleep_ms = 1000;
    break;
  }
  
  rc = gpio_request(LED_GPIO_INCIDENT_LED, " INCIDENT_LED_GPIO RESET Enable");
  if (rc) {
    //return rc;
  }

  rc = gpio_tlmm_config(GPIO_CFG(LED_GPIO_INCIDENT_LED, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), GPIO_ENABLE);
  if(rc < 0) {
    gpio_free(LED_GPIO_INCIDENT_LED);
    //return rc;
  }

  if (on == 1) {
    // NOTICE: infinite loop, unless blink is set to false.
    while(blink) {
      gpio_set_value(LED_GPIO_INCIDENT_LED, 1);
      msleep(t_sleep_ms);
      gpio_set_value(LED_GPIO_INCIDENT_LED, 0);
      msleep(t_sleep_ms);
    }
  }
  else if (on == 2) {
    mdelay(50);
    gpio_set_value(LED_GPIO_INCIDENT_LED, 1);
  }
  else {
    mdelay(50);
    gpio_set_value(LED_GPIO_INCIDENT_LED, 0);
  }
  gpio_free(LED_GPIO_INCIDENT_LED);
}

static void incident_led_blink_on(struct work_struct *work)
{
  set_incident_led_blink(1);
}

static void incident_led_blink_off(struct work_struct *work)
{
  set_incident_led_blink(0);
}

static void incident_led_on_all_long(struct work_struct *work)
{
  set_incident_led_blink(2);
}

static void timed_incident_led_blink_on(struct timed_output_dev *dev)
{
  // TODO: should get a lock to access the variable
  blink = 1; 
  struct incident_led_blink_data *data = container_of(dev, struct incident_led_blink_data, dev);
  schedule_work(&data->work_incident_led_blink_on);
}

static void timed_incident_led_on_all_long(struct timed_output_dev *dev)
{
  // TODO: should get a lock to access the variable
  blink = 0;
  struct incident_led_blink_data *data = container_of(dev, struct incident_led_blink_data, dev);
  schedule_work(&data->work_incident_led_on_all_long);
}

static void timed_incident_led_blink_off(struct timed_output_dev *dev)
{
  // TODO: should get a lock to access the variable
  blink = 0;
  struct incident_led_blink_data *data = container_of(dev, struct incident_led_blink_data, dev);
  schedule_work(&data->work_incident_led_blink_off);
}

static void incident_led_blink_enable(struct timed_output_dev *dev, int value)
{
  struct incident_led_blink_data *data = 
    container_of(dev, struct incident_led_blink_data, dev);
  unsigned long	flags;
	
  spin_lock_irqsave(&data->lock, flags);
	
  hrtimer_cancel(&data->timer);

  // NOTICE: 0 - off; >3600000(60 minutes) - on; >0 - blink as time lapses.
  if (value == 0)
    timed_incident_led_blink_off(dev);
  else if (value == -1) {
    // stop 
    timed_incident_led_blink_off(dev);
    // blink all along
    timed_incident_led_blink_on(dev);
  }
  else if (value > 3600000 ){
    timed_incident_led_on_all_long(dev);
  }
  else if (value > 0 ) {
    value = (value > 15000 ? 15000 : value);
    timed_incident_led_blink_on(dev);
    
    hrtimer_start(&data->timer,
		  ktime_set(value / 1000, (value % 1000) * 1000000),
		  HRTIMER_MODE_REL);
  }
  spin_unlock_irqrestore(&data->lock, flags);
}

static int incident_led_blink_get_time(struct timed_output_dev *dev)
{
  struct incident_led_blink_data *data =
    container_of(dev, struct incident_led_blink_data, dev);
  if (hrtimer_active(&data->timer)) {
    ktime_t r = hrtimer_get_remaining(&data->timer);
    return r.tv.sec * 1000 + r.tv.nsec / 1000000;   
  } 
  else
    return 0;
}

static enum hrtimer_restart incident_led_blink_timer_func(struct hrtimer *timer)
{
  struct incident_led_blink_data	*data =
    container_of(timer, struct incident_led_blink_data, timer);
  timed_incident_led_blink_off(&data->dev);
  return HRTIMER_NORESTART;
}

static int incident_led_blink_probe(struct platform_device *pdev)
{
  int ret =0;
  struct incident_led_blink_data *data;
  data = kzalloc(sizeof(struct incident_led_blink_data) ,GFP_KERNEL);
  if (!data)
    return -ENOMEM;
  
  INIT_WORK(&data->work_incident_led_blink_on, incident_led_blink_on);
  INIT_WORK(&data->work_incident_led_blink_off, incident_led_blink_off);
  INIT_WORK(&data->work_incident_led_on_all_long, incident_led_on_all_long);

  hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  data->timer.function = incident_led_blink_timer_func;
  spin_lock_init(&data->lock);

  data->dev.name = "incident_led";
  data->dev.get_time = incident_led_blink_get_time;
  data->dev.enable = incident_led_blink_enable;
	
  ret = timed_output_dev_register(&data->dev);
  if (ret < 0){
    timed_output_dev_unregister(&data->dev);
    kfree(data);
    return ret;
  }
  platform_set_drvdata(pdev, data);
  return 0;
}

static int incident_led_blink_remove(struct platform_device *pdev)
{
  struct incident_led_blink_data *data = platform_get_drvdata(pdev);
  timed_output_dev_unregister(&data->dev);
  kfree(data);
  return 0;
}

static struct platform_device incident_led_device = {
	.name		= "time_incident_led",
	.id		= -1,
};

static struct platform_driver incident_led = {
	.probe		= incident_led_blink_probe,
	.remove		= incident_led_blink_remove,
	.driver		= {
		.name		= "time_incident_led",
		.owner		= THIS_MODULE,
	},
};

int init_incident_led_device(void)
{
   return platform_device_register(&incident_led_device);
}


static int __init msm_init_incident_led(void)
{
  return platform_driver_register(&incident_led);
}

static void __exit msm_exit_incident_led(void)
{
  platform_driver_unregister(&incident_led);
}


module_init(msm_init_incident_led);
module_exit(msm_exit_incident_led);

MODULE_AUTHOR("sKF21980@huawei.com>");
MODULE_DESCRIPTION("timed output incident led device");
MODULE_LICENSE("GPL");

