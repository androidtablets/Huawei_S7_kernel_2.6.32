/*****************************************************************************
 *               Copyright (C) 2010, Huawei Tech. Co., Ltd.               
 *                           ALL RIGHTS RESERVED                               
 * FileName: s70_ledctl.c                                                          
 * Version: 1.0                                                                
 * Description:  Led ctrl driver.     
 *                                                                             
 * History:                                                                    
 * 1. 2010-04-02,z00101756 Create this file.                                                  
******************************************************************************/


    
/*****************************************************************************/
/*  本文件需要包含的其他头文件依赖                                           */
/*****************************************************************************/
/*标准头文件*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <mach/gpio.h>
#include <linux/uaccess.h>

/*自定义头文件*/
#include "incident_led.h"

#define LED_GPIO_INCIDENT_LED   99

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

static int DRV_LedDevIoctl(struct inode *inode, struct file *file,
                           unsigned int cmd, unsigned long arg);

static int DRV_LedDevOpen(struct inode *inode, struct file *file);

static int DRV_LedDevClose(struct inode *inode, struct file *file);


static void LED_TimerHandler(unsigned long data);

static DEFINE_TIMER(gLED_Timer, LED_TimerHandler, 0, 0);


static DECLARE_MUTEX(mLedSem);

static int m_status = 0;

static BSP_LED_S_CTRL gsLedParam;

static ssize_t Incident_led_attr_store(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf)
 {
    if(0 == strncmp(buf,"0",1))
    {
        gpio_set_value(LED_GPIO_INCIDENT_LED, 0);
    }
    else
    {
        gpio_set_value(LED_GPIO_INCIDENT_LED, 1);
    }
    
    return 0;
 }

 static struct kobj_attribute Incident_led_attribute =
         __ATTR(light, 0666, NULL, Incident_led_attr_store);

 static struct attribute* Incident_led_attributes[] =
 {
         &Incident_led_attribute.attr,
         NULL
 };

 static struct attribute_group Incident_led_defattr_group =
 {
         .attrs = Incident_led_attributes,
 };
static void Led_Ctrl_func(struct work_struct *work)
{    
    if (m_status)
    {
        gpio_set_value(LED_GPIO_INCIDENT_LED, 1);
        mod_timer(&gLED_Timer, jiffies + gsLedParam.usOnTime * HZ / 1000);
    }
    else
    {
        gpio_set_value(LED_GPIO_INCIDENT_LED, 0);
        mod_timer(&gLED_Timer, jiffies + gsLedParam.usOffTime * HZ / 1000);
    }

    m_status = ~m_status;    
}

static DECLARE_DELAYED_WORK(led_event, Led_Ctrl_func);
static void LED_TimerHandler(unsigned long data)
{
    schedule_delayed_work(&led_event, msecs_to_jiffies(1));    
    return;
}


struct file_operations led_fops = 
{
    .ioctl   = DRV_LedDevIoctl,
    .open    = DRV_LedDevOpen,
    .release = DRV_LedDevClose
};

static struct miscdevice led_dev = {
	MISC_DYNAMIC_MINOR,
	BSP_LED_DEVICE_NAME,
	&led_fops,
};

/************************************************************************
 * 函数名称：DRV_LedDevInit
 * 功能描述：装载LED驱动模块
 * 输入参数：无
 * 输出参数：无
 * 返 回 值：无
 * 影    响：
 * 其它说明：
 * 作    者：
 * 修改日期    版本号     修改人     修改内容
 * -----------------------------------------------
 * 200X/XX/XX VX.X    XXX       XXXX
 ************************************************************************/
static int __init DRV_LedDevInit(void)
{
    int ret = 0;    
    struct kobject *kobj = NULL;

    ret = gpio_request(LED_GPIO_INCIDENT_LED, " INCIDENT_LED_GPIO RESET Enable");
    if (ret) 
    {
        printk("gpio_request failed.\n");
        return -1;
    }
    
    ret = gpio_tlmm_config(GPIO_CFG(LED_GPIO_INCIDENT_LED, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA), GPIO_ENABLE);
    if(ret < 0) 
    {
        printk("gpio_tlmm_config failed.\n");
        gpio_free(LED_GPIO_INCIDENT_LED);
        return -1;
    }

    gpio_set_value(LED_GPIO_INCIDENT_LED, 0);
    
    ret = misc_register(&led_dev);
    if (ret)
    {
        printk("register device led failed.\n");
        return ret;
    }    

    kobj = kobject_create_and_add("incident_led", NULL);
    if (kobj == NULL) {
        return -1;
    }
    if (sysfs_create_group(kobj, &Incident_led_defattr_group)) {
        kobject_put(kobj);
        return -1;
    }

    printk("S70 led driver init.\n");
    return 0;
}


/************************************************************************
 * 函数名称：DRV_LedDevFree
 * 功能描述：卸载LED驱动模块
 * 输入参数：无
 * 输出参数：无
 * 返 回 值：无
 * 影    响：
 * 其它说明：
 * 作    者：
 * 修改日期    版本号     修改人     修改内容
 * -----------------------------------------------
 * 200X/XX/XX VX.X    XXX       XXXX
 ************************************************************************/
static void __exit DRV_LedDevFree(void)
{
    gpio_set_value(LED_GPIO_INCIDENT_LED, 0);
    gpio_free(LED_GPIO_INCIDENT_LED);
    del_timer(&gLED_Timer);
    misc_deregister(&led_dev);
    return;
}


/************************************************************************
 * 函数名称：DRV_LedDevOpen
 * 功能描述：打开LED设备
 * 输入参数：略
 * 输出参数：无
 * 返 回 值：成功返回0，失败返回-EBUSY,表示设备忙
 * 影    响：
 * 其它说明：
 * 作    者：
 * 修改日期    版本号     修改人     修改内容
 * -----------------------------------------------
 * 200X/XX/XX VX.X    XXX       XXXX
 ************************************************************************/
static int DRV_LedDevOpen(struct inode *inode, struct file *file)
{

    down(&mLedSem);
    return 0;
}

/************************************************************************
 * 函数名称：DRV_LedDevClose
 * 功能描述：关闭LED设备
 * 输入参数：略
 * 输出参数：无
 * 返 回 值：返回0
 * 影    响：
 * 其它说明：
 * 作    者：
 * 修改日期    版本号     修改人     修改内容
 * -----------------------------------------------
 * 200X/XX/XX VX.X    XXX       XXXX
 ************************************************************************/
static int DRV_LedDevClose(struct inode *inode, struct file *file)
{
    up(&mLedSem);
    return 0;
}

/************************************************************************
 * 函数名称：DRV_LedDevIoctl
 * 功能描述：控制LED设备
 * 输入参数：cmd:操作命令，包括亮、灭、闪烁
             arg：命令参数，忽略
 * 输出参数：无
 * 返 回 值：返回0
 * 影    响：
 * 其它说明：
 * 作    者：
 * 修改日期    版本号     修改人     修改内容
 * -----------------------------------------------
 * 200X/XX/XX VX.X    XXX       XXXX
************************************************************************/
static int DRV_LedDevIoctl(struct inode *inode, struct file *file,
                           unsigned int cmd, unsigned long arg)
{
    //BSP_LED_S_CTRL sPara;
    unsigned int ulRet;
    
    switch(cmd)
    {
        case BSP_LED_E_ON:            
            del_timer_sync(&gLED_Timer);
            gpio_set_value(LED_GPIO_INCIDENT_LED, 1);
            break;
        case BSP_LED_E_OFF:
            del_timer(&gLED_Timer);
            gpio_set_value(LED_GPIO_INCIDENT_LED, 0);
            break;
        case BSP_LED_E_FLASH:
            ulRet = copy_from_user((void *)&gsLedParam, (void __user *)arg, sizeof(gsLedParam));
            if(0 != ulRet)
            {
                return -1;
            }
            mod_timer(&gLED_Timer, jiffies + 2);
            break;
        default:
            printk("Error Ioctl operation.\r\n");
            return -1;
    }
    return 0;
}

module_init(DRV_LedDevInit);
module_exit(DRV_LedDevFree);

MODULE_AUTHOR("HUAWEI Ltd");
MODULE_DESCRIPTION("HUAWEI S70 Led Driver");
MODULE_LICENSE("GPL");


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */




