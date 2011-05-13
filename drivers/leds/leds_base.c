/************************************************************
  Copyright (C), 2009-2010, Huawei Tech. Co., Ltd.
  FileName: leds_base.c
  Description:     接口底座led灯驱动,包括HDMI指示灯和CVBS指示灯      
  Version:         v1.0
  Function List:   
    1. driver HDMI led light
    2. driver CVBS led light
  History:        
       <author>      <time>     <version >   <desc>
***********************************************************/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/uaccess.h>

#include <mach/gpio.h>

#define INCIDENT_LED_DEVICE_DUG 0
#if (INCIDENT_LED_DEVICE_DUG)
#define INCIDENT_LED_DEV_DUG(format,arg...)printk(KERN_ALERT format, ## arg); 
#define INCIDENT_LED_DEV_INFO(format,arg...)printk(KERN_ALERT format, ## arg); 
#define INCIDENT_LED_DEV_ERR(format,arg...)printk(KERN_ALERT format, ## arg); 
#else
#define INCIDENT_LED_DEV_DUG(format,arg...)do { (void)(format); } while (0)
#define INCIDENT_LED_DEV_INFO(format,arg...)do { (void)(format); } while (0)
#define INCIDENT_LED_ERR(format,arg...)do { (void)(format); } while (0)
#endif

//#define LED_BASE_DEBUG

#define LED_MAX_LENGTH 8 /* maximum chars written to proc file */



typedef enum
{
    LED_BASE_TYPE_NONE,
    LED_BASE_TYPE_HDMI,
    LED_BASE_TYPE_CVBS,
    #ifdef CONFIG_HUAWEI_INCIDENT_LED
    LED_BASE_TYPE_INCIDENT_LED,
    #endif
} LED_BASE_TYPE;

#define BASE_LED_ON 1
#define BASE_LED_OFF    0

#define LED_GPIO_HDMI   148
#define LED_GPIO_CVBS   151
#ifdef CONFIG_HUAWEI_INCIDENT_LED
#define LED_GPIO_INCIDENT_LED   99
#endif

/* LED相关proc文件的目录 */
static struct proc_dir_entry *led_dir;

#define LED_HDMI_PROC_FILE_NAME "hdmi"
#define LED_CVBS_PROC_FILE_NAME "cvbs"

#ifdef CONFIG_HUAWEI_INCIDENT_LED
#define LED_INCIDENT_LED_PROC_FILE_NAME "fre_incident_led"
int fre_incident_level = 1;

static int my_atoi(const char *name)
{
    int val = 0;

    for (;; name++) {
	switch (*name) {
	    case '0' ... '9':
                val = 10*val+(*name-'0');
		break;
	    default:
		return val;
	}
    }
}
#endif


static void led_set_light(int type, int on)
{
    unsigned led_gpio_index = 0xff;
    unsigned led_gpio_config;
    
    if(LED_BASE_TYPE_HDMI == type)
    {
        led_gpio_index = LED_GPIO_HDMI;
    }
    else if(LED_BASE_TYPE_CVBS == type)
    {
        led_gpio_index = LED_GPIO_CVBS;
    }
    #ifdef CONFIG_HUAWEI_INCIDENT_LED
    else if(LED_BASE_TYPE_INCIDENT_LED == type)
    {
      led_gpio_index = LED_GPIO_INCIDENT_LED;
    }
    #endif
    else
    {
        return;
    }
    
    if(on)
    {
        led_gpio_config = GPIO_CFG(led_gpio_index, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA);
        gpio_tlmm_config(led_gpio_config, GPIO_ENABLE);
        gpio_set_value(led_gpio_index, 1);
    }
    else
    {
        led_gpio_config = GPIO_CFG(led_gpio_index, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA);
        gpio_tlmm_config(led_gpio_config, GPIO_ENABLE);
        gpio_set_value(led_gpio_index, 0);   
    }
}


static int led_hdmi_proc_read(char *page, char **start, off_t offset,
                              int count, int *eof, void *data)
{
    int status;
    
    *eof = 1;
    
    status = gpio_get_value(LED_GPIO_HDMI);
    
#ifdef LED_BASE_DEBUG
    printk(KERN_INFO "hdmi led status = %d\n", status);
#endif
    
    if( 1 == status)
    {
        return sprintf(page, "on\n");
    }
    else
    {
        return sprintf(page, "off\n");
    }
}

static int led_hdmi_proc_write(struct file *file, const char __user *buffer,
                               unsigned long count, void *ppos)
{
    char *buf = NULL;
    
    if (count > LED_MAX_LENGTH)
    {
        count = LED_MAX_LENGTH;
    }
    
    buf = kmalloc(sizeof(char) * (count + 1), GFP_KERNEL);
    
    if (!buf)
    {
        return -ENOMEM;
    }
    
    if (copy_from_user(buf, buffer, count)) 
    {
        kfree(buf);
        return -EFAULT;
    }
    
    buf[count] = '\0';
    
    /* work around \n when echo'ing into proc */
    if (buf[count - 1] == '\n')
        buf[count - 1] = '\0';
    
    if (!strcmp(buf, "on")) 
    {
#ifdef LED_BASE_DEBUG
        printk(KERN_INFO "cvbs on\n");
#endif
        led_set_light(LED_BASE_TYPE_HDMI, BASE_LED_ON);
    } 
    else
    {
#ifdef LED_BASE_DEBUG
        printk(KERN_INFO "cvbs off\n");
#endif
        led_set_light(LED_BASE_TYPE_HDMI, BASE_LED_OFF);
    }
    
    kfree(buf);
    
    return count;
}

static int led_cvbs_proc_read(char *page, char **start, off_t offset,
                              int count, int *eof, void *data)
{
    int status;
    
    *eof = 1;
    
    status = gpio_get_value(LED_GPIO_CVBS);
    
#ifdef LED_BASE_DEBUG
    printk(KERN_INFO "cvbs led status = %d\n", status);
#endif
    
    if( 1 == status)
    {
        return sprintf(page, "on\n");
    }
    else
    {
        return sprintf(page, "off\n");
    }
}

static int led_cvbs_proc_write(struct file *file, const char __user *buffer,
                               unsigned long count, void *ppos)
{
    char *buf = NULL;
    
    if (count > LED_MAX_LENGTH)
    {
        count = LED_MAX_LENGTH;
    }
    
    buf = kmalloc(sizeof(char) * (count + 1), GFP_KERNEL);
    if (!buf)
        return -ENOMEM;
    
    if (copy_from_user(buf, buffer, count)) 
    {
        kfree(buf);
        return -EFAULT;
    }
    
    buf[count] = '\0';
    
    /* work around \n when echo'ing into proc */
    if (buf[count - 1] == '\n')
        buf[count - 1] = '\0';
    
    if (!strcmp(buf, "on")) 
    {
#ifdef LED_BASE_DEBUG
        printk(KERN_INFO "cvbs on\n");
#endif
        led_set_light(LED_BASE_TYPE_CVBS, BASE_LED_ON);
    } 
    else
    {
#ifdef LED_BASE_DEBUG
        printk(KERN_INFO "cvbs off\n");
#endif
        led_set_light(LED_BASE_TYPE_CVBS, BASE_LED_OFF);
    }
    
    kfree(buf);
    
    return count;
}

#ifdef CONFIG_HUAWEI_INCIDENT_LED
static int led_incident_proc_read(char *page, char **start, off_t offset,
                              int count, int *eof, void *data)
{
    return sprintf(page,"level %d\n",fre_incident_level);
}

static int led_incident_proc_write(struct file *file, const char __user *buffer,
                               unsigned long count, void *ppos)
{
    char *buf = NULL;
    int i=0;
    // gpio config
    int rc = 0;

    if (count > LED_MAX_LENGTH)
    {
        count = LED_MAX_LENGTH;
    }
    
    buf = kmalloc(sizeof(char) * (count + 1), GFP_KERNEL);
    if (!buf)
        return -ENOMEM;
    
    if (copy_from_user(buf, buffer, count)) 
    {
        kfree(buf);
        return -EFAULT;
    }
    
    buf[count] = '\0';
    
    /* work around \n when echo'ing into proc */
    if (buf[count - 1] == '\n')
        buf[count - 1] = '\0';

    fre_incident_level = my_atoi(buf);
    return count;
}
#endif


static int led_create_hdmi_proc_file(void)
{
    int retval;
    struct proc_dir_entry *ent;
    
    /* Creating read/write "hdmi" entry */
    ent = create_proc_entry(LED_HDMI_PROC_FILE_NAME, 0666, led_dir);
    if (ent == NULL) 
    {
        printk(KERN_ERR "Unable to create /proc/%s/status entry", LED_HDMI_PROC_FILE_NAME);
        retval = -ENOMEM;
        goto fail;
    }
    ent->read_proc = led_hdmi_proc_read;
    ent->write_proc = led_hdmi_proc_write;
    return 0;
fail:
    remove_proc_entry(LED_HDMI_PROC_FILE_NAME, led_dir);
    return 0;
}

static int led_create_cvbs_proc_file(void)
{
    int retval;
    struct proc_dir_entry *ent;
    
    /* Creating read/write "cvbs" entry */
    ent = create_proc_entry(LED_CVBS_PROC_FILE_NAME, 0666, led_dir);
    if (ent == NULL) 
    {
        printk(KERN_ERR "Unable to create /proc/%s/status entry", LED_CVBS_PROC_FILE_NAME);
        retval = -ENOMEM;
        goto fail;
    }
    ent->read_proc = led_cvbs_proc_read;
    ent->write_proc = led_cvbs_proc_write;
    return 0;
fail:
    remove_proc_entry(LED_CVBS_PROC_FILE_NAME, led_dir);
    return 0;
}

#ifdef CONFIG_HUAWEI_INCIDENT_LED
static int led_create_incident_led_proc_file(void)
{
    int retval;
    struct proc_dir_entry *ent;
    
    /* Creating read/write "cvbs" entry */
    ent = create_proc_entry(LED_INCIDENT_LED_PROC_FILE_NAME, 0666, led_dir);
    if (ent == NULL) 
    {
        printk(KERN_ERR "Unable to create /proc/led/%s entry", LED_INCIDENT_LED_PROC_FILE_NAME);
        retval = -ENOMEM;
        goto fail;
    }
    ent->read_proc = led_incident_proc_read;
    ent->write_proc = led_incident_proc_write;
    return 0;
fail:
    remove_proc_entry(LED_INCIDENT_LED_PROC_FILE_NAME, led_dir);
    return 0;
}
#endif

#define LED_VERSION "1.1"

static int __init led_init(void)
{
    /* 创建led proc目录*/
    led_dir = proc_mkdir("led", NULL);
    if (led_dir == NULL) 
    {
        printk(KERN_ERR "Unable to create /proc/led directory");
        return -ENOMEM;
    }
    /* 创建HDMI led proc 文件*/
    led_create_hdmi_proc_file();
    
    /* 创建CVBS led proc 文件 */
    led_create_cvbs_proc_file();
    
    #ifdef CONFIG_HUAWEI_INCIDENT_LED
    led_create_incident_led_proc_file();
    #endif

    printk(KERN_INFO
        "led: version %s, Yunqing Huang \n",
        LED_VERSION);
    
    return 0;
}

static void __exit led_exit(void)
{
    remove_proc_entry(LED_HDMI_PROC_FILE_NAME, led_dir);
    remove_proc_entry(LED_CVBS_PROC_FILE_NAME, led_dir);
    #ifdef CONFIG_HUAWEI_INCIDENT_LED
    remove_proc_entry(LED_INCIDENT_LED_PROC_FILE_NAME, led_dir);
    #endif
}

module_init(led_init);
module_exit(led_exit);

MODULE_AUTHOR("Yunqing Huang <www.huawei.com>");
MODULE_DESCRIPTION("Provides control of the LEDs of HDMI and CVBS on bottom base.");
MODULE_LICENSE("GPL");
MODULE_VERSION(LED_VERSION);
