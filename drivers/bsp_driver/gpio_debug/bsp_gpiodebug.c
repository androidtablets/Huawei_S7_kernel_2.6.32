/*****************************************************************************
 *               Copyright (C) 2006, Huawei Tech. Co., Ltd.
 *                           ALL RIGHTS RESERVED
 * FileName: bsp_gpiodebug.c
 * Version: 1.0
 * Description:  ctrl gpio driver.
 *
 * History:
 ******************************************************************************/

/*****************************************************************************/
/*****************************************************************************/
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/param.h>
#include <linux/uaccess.h>
#include <mach/gpio.h>
#include <linux/uaccess.h>

#include "../include/bsp_interface.h"

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif/* __cpluscplus */
#endif /* __cpluscplus */


#define   BSP_GPIO_MAX_NUMS  164

static int BSP_GpioIoctl(struct inode *inode, struct file *file,
                         unsigned int cmd, unsigned long arg);

static int BSP_GpioOpen(struct inode *inode, struct file *file);

static int BSP_GpioClose(struct inode *inode, struct file *file);

static DECLARE_MUTEX(mGpioSem);

struct file_operations gpio_fops =
{
    .ioctl   = BSP_GpioIoctl,
    .open    = BSP_GpioOpen,
    .release = BSP_GpioClose
};

static struct miscdevice gpio_dev =
{
    MISC_DYNAMIC_MINOR,
    BSP_GPIO_DEVICE_NAME,
    &gpio_fops,
};

static int __init BSP_GpioInit(void)
{
    int ret = 0;

    ret = misc_register(&gpio_dev);
    if (ret < 0)
    {
        printk("%s : misc_register error .\n", __func__);
        return ret;
    }

    printk("gpio debug driver init.\n");
    return 0;
}

static void __exit BSP_GpioFree(void)
{
    misc_deregister(&gpio_dev);
    return;
}

static int BSP_GpioOpen(struct inode *inode, struct file *file)
{
    down(&mGpioSem);

    return 0;
}

static int BSP_GpioClose(struct inode *inode, struct file *file)
{
    up(&mGpioSem);

    return 0;
}

static int BSP_GpioIoctl(struct inode *inode, struct file *file,
                         unsigned int cmd, unsigned long arg)
{
    unsigned int ulRet = 0, ret = 0;
    BSP_GPIO_DEVICE_S_PARA stTestPara;

    switch (cmd)
    {
    case BSP_GPIO_DEBUG_CRTL:
        ulRet = copy_from_user((void *)&stTestPara, (void __user *)arg, sizeof(stTestPara));
        if (0 != ulRet)
        {
            return -1;
        }
        else
        {
            if(stTestPara.ucgpio_num > BSP_GPIO_MAX_NUMS)
            {
                printk(KERN_ERR "Gpio num error\n");
                return -1;
            }
            
            switch (stTestPara.usOpType)
            {
            case BSP_GPIO_SET_VALUE:

                ulRet = gpio_request(stTestPara.ucgpio_num, " INCIDENT_LED_GPIO RESET Enable");
                if (ulRet)
                {
                    if (-EBUSY != ulRet)
                    {
                        printk("gpio_request failed.\n");
                        return -1;
                    }
                }

                ret = gpio_tlmm_config(GPIO_CFG(stTestPara.ucgpio_num, 0, GPIO_OUTPUT, stTestPara.ucgpio_cfg,
                                                GPIO_2MA), GPIO_ENABLE);
                if (ret)
                {
                    printk("gpio_tlmm_config failed.\n");
                }

                gpio_set_value(stTestPara.ucgpio_num, stTestPara.ucval);

                if (-EBUSY != ulRet)
                {
                    gpio_free(stTestPara.ucgpio_num);
                }

                break;
            case BSP_GPIO_GET_VALUE:

                ulRet = gpio_request(stTestPara.ucgpio_num, " INCIDENT_LED_GPIO RESET Enable");
                if (ulRet)
                {
                    if (-EBUSY != ulRet)
                    {
                        printk("gpio_request failed.\n");
                        return -1;
                    }
                }

                ret = gpio_tlmm_config(GPIO_CFG(stTestPara.ucgpio_num, 0, GPIO_INPUT, stTestPara.ucgpio_cfg,
                                                GPIO_2MA), GPIO_ENABLE);
                if (ret)
                {
                    printk("gpio_tlmm_config failed.\n");
                }

                stTestPara.ucval = gpio_get_value(stTestPara.ucgpio_num);

                if (-EBUSY != ulRet)
                {
                    gpio_free(stTestPara.ucgpio_num);
                }

                ulRet = copy_to_user((void __user *)arg, (void *)&stTestPara, sizeof(stTestPara));
                if (0 != ulRet)
                {
                    return -1;
                }

                break;
            default:
                printk("gpio ctrl para error.\n");
                return -1;
            }
        }

        break;
    default:
        printk("Error Ioctl operation.\r\n");
        return -1;
    }

    return 0;
}

module_init(BSP_GpioInit);
module_exit(BSP_GpioFree);

MODULE_AUTHOR("HUAWEI Ltd");
MODULE_DESCRIPTION("HUAWEI HID GPIO Debug Driver");
MODULE_LICENSE("GPL");

#ifdef __cplusplus
 #if __cplusplus
}
 #endif/* __cpluscplus */
#endif /* __cpluscplus */
