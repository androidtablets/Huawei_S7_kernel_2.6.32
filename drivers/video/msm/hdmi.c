/* Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "msm_fb.h"
#include <mach/gpio.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/workqueue.h>
#include <mach/vreg.h>
#include <linux/miscdevice.h>
#include <linux/ctype.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include "hdmi.h"
#include "i2c_master_sw.h"
#include "SIITPI.h"
#include "SIITPI_Access.h"
#include "SIITPI_Regs.h"
#include "SIIConstants.h"

#include <linux/timer.h>

#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/ioctl.h>

#define HDMI_IRQ_TRIGGER IRQF_TRIGGER_FALLING

#define HDMI_DEBUG_FLAG 0
#if (HDMI_DEBUG_FLAG)
 #define HDMI_DUG(format, arg...)  printk(KERN_INFO  "[%s] (line: %u) " format "\n", __FUNCTION__, __LINE__, ## arg);
 #define HDMI_INFO(format, arg...) printk(KERN_INFO  "[%s] (line: %u) " format "\n", __FUNCTION__, __LINE__, ## arg);
 #define HDMI_ERR(format, arg...)  printk(KERN_ALERT "[%s] (line: %u) " format "\n", __FUNCTION__, __LINE__, ## arg);
#else
 #define HDMI_DUG(format, arg...)  do { (void)(format); } while (0)
 #define HDMI_INFO(format, arg...) do { (void)(format); } while (0)
 #define HDMI_ERR(format, arg...) printk(KERN_ALERT "[%s] (line: %u) " format "\n", __FUNCTION__, __LINE__, ## arg);
#endif

// There are three parts in this file.

//***************************************************************************************************************************************/
//*************************1. The first part is i2c driver.It's used to detect is the hdmi is connected.*****************************************/
//**************************************************************************************************************************************/

static struct timer_list gHdmiTimer;
#define HDMI_POLLING_JIFFIES (2 * HZ)
#define HDMI_START_JIFFIES (50 * HZ)
static struct work_struct gHdmiWork;
static struct workqueue_struct *gpHdmiWorkqueue;
static int gFirstStart = 1;

struct i2c_client *gHdmi_client;

struct early_suspend early_susp_hdmi;
byte old_pow_reg_val;

// IOCTL Command
#define 	HDMI_IOC_MAGIC 'D'
#define     IOCTL_OPEN_VIDEO  _IO(HDMI_IOC_MAGIC, 0)
#define     IOCTL_CLOSE_VIDEO _IO(HDMI_IOC_MAGIC, 1)
#define     IOCTL_GET_STATUS  _IO(HDMI_IOC_MAGIC, 2)
#define     IOCTL_OPEN_AUDIO  _IO(HDMI_IOC_MAGIC, 3)
#define     IOCTL_CLOSE_AUDIO _IO(HDMI_IOC_MAGIC, 4)

#define  	HDMI_RESET_GPIO     159
enum
{
    EARLY_SUSPEND_LEVEL_HDMI = 200,
};


static int        hdmi_iic_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int __exit hdmi_iic_remove(struct i2c_client *client);
static char       hdmi_is_cable_connected(void);
int               hdmi_power(int on);

/*
the first bit indicate the calbe is connected,
the second bit indicate the audio mode is set to hdmi by user
thus, 0x00 ----> nothing has been done
      0x01 ---->the cable is connected
      0x02 ---->the audio mode is set to hdmi by user
      0x03 ----> both condition has been satisfied
 */
static int gHdmi_audio_status = 0;


/* the buf store the sink info, the user can get it in the status bar */
char gConnectState[20]={ 0 };
int  gParseState;
static int __init hdmi_probe(struct platform_device *pdev);
static int        usrfs_ioctl(struct inode* inode, struct file* file,
                              unsigned int cmd, unsigned long param);

/*************************************************************************
create three sys files.
audio state
video state
connect state
*************************************************************************/

// following are the sysfs callback functions

/* the  audio attribute & show function  */
static ssize_t hdmi_audio_attr_show(struct kobject *kobj, struct kobj_attribute *attr,
                                    char *buf)
{
	byte InterruptStatusImage;
    byte status;
	
		InterruptStatusImage = ReadByteTPI(TPI_INTERRUPT_STATUS_REG);    // Read Interrupt status register
		if (InterruptStatusImage < 0)
		{
		    HDMI_ERR("InterruptStatusImage error!\n");
		    return sprintf(buf, "%d", 0);
		}
		status = InterruptStatusImage & HOT_PLUG_STATE;
		if (hdmi_is_cable_connected())
		{
		    gHdmi_audio_status = 3;
		}
		else
		{
			gHdmi_audio_status = 0;
		}
    return sprintf(buf, "%d", gHdmi_audio_status);
}

static struct kobj_attribute hdmi_audio_attribute =
    __ATTR(audioState, 0666, hdmi_audio_attr_show, NULL);


/* the connect state attribute & show function  */
static ssize_t hdmi_connect_attr_show(struct kobject *kobj, struct kobj_attribute *attr,
                                      char *buf)
{
    return sprintf(buf, "%s", gConnectState);
}

static struct kobj_attribute hdmi_connect_attribute =
    __ATTR(connectState, 0666, hdmi_connect_attr_show, NULL);

static struct attribute* hdmi_attributes[] =
{
    &hdmi_audio_attribute.attr,
    &hdmi_connect_attribute.attr,
    NULL
};

static struct attribute_group hdmi_defattr_group =
{
    .attrs = hdmi_attributes,
};

static void hdmi_poll_thread(unsigned long data)
{
    queue_work(gpHdmiWorkqueue, &gHdmiWork);
}

/*****************************************************************************
 Prototype    : hdmi_poll_func
 Description  : The function poll the status of the chip and if hdmi cable
                is connected,init the TPI and start the connect the TV
 Output       : None
 Return Value :
 Calls        :
 Called By    :

  History        :
  1.Date         : 2010/12/7
    Author       : nielimin
    Modification : Created function

*****************************************************************************/
static void hdmi_poll_func(struct work_struct *pwork)
{
    byte InterruptStatusImage;
    static byte last_status = -1;
    byte status;
	int i;
	if (gFirstStart)
	{
		for(i=0; i<3; i++) 
		{
			hdmi_startup();
			if(gParseState)
				break;
		}
		gFirstStart = 0;
	}
		
    InterruptStatusImage = ReadByteTPI(TPI_INTERRUPT_STATUS_REG);    // Read Interrupt status register
    if (InterruptStatusImage < 0)
    {
        HDMI_ERR("InterruptStatusImage error!\n");
        return;
    }

    status = InterruptStatusImage & HOT_PLUG_STATE;

    // report the status message
    if ((-1 == last_status) || (status != last_status))
    {    	
        if (status)
        {		    	   
	        /* init the HDMI register  */	
			/*
		    hdmi_power(0);  // power on
		    msleep(3);
		    hdmi_power(1);  // power on	
		    */
		    for(i=0; i<3; i++) 
			{
				hdmi_startup();
				if(gParseState)
					break;
			}
        }
		else
		{
			memset(gConnectState,0,sizeof(gConnectState));
		}
    }

    last_status = status;
    mod_timer(&gHdmiTimer, jiffies + HDMI_POLLING_JIFFIES);
}

static int hdmi_iic_probe(struct i2c_client *         client,
                          const struct i2c_device_id *id)
{
    int rc = 0;
    struct kobject *kobj = NULL;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        HDMI_ERR("HDMI: i2c_check_functionality failed\n");
        goto probe_failure;
    }

    gHdmi_client = client;

    //1. create three system files
    kobj = kobject_create_and_add("hdmi", NULL);
    if (kobj == NULL)
    {
        HDMI_ERR("-------kobject_create_and_add error  kobj==NULL----------- \n" );
        rc = -1;
        goto err_create_kobject;
    }

    if (sysfs_create_group(kobj, &hdmi_defattr_group))
    {
        kobject_put(kobj);
        HDMI_ERR("-------sysfs_create_group---kobject_put(kobj)-------- \n" );
        rc = -1;
        goto err_sysfs_create_group;
    }

    // 2. create poll thread to detect the connect status
    gpHdmiWorkqueue = create_singlethread_workqueue("hdmi_wq");
    if (!gpHdmiWorkqueue)
    {
        HDMI_ERR("create hdmi_wq error\n");
        goto err_create_workqueue_fail;
    }

    // 3. init the poll function and timer
    INIT_WORK(&gHdmiWork, hdmi_poll_func);

    // init the timer
    init_timer(&gHdmiTimer);
    gHdmiTimer.expires = jiffies + HDMI_START_JIFFIES;
    gHdmiTimer.data = 0;
    gHdmiTimer.function = hdmi_poll_thread;
    add_timer(&gHdmiTimer);

    hdmi_power(0);  // power on
    msleep(3);
    hdmi_power(1);  // power on

    early_susp_hdmi.level = EARLY_SUSPEND_LEVEL_HDMI;
    early_susp_hdmi.suspend = hdmi_early_suspend;
    early_susp_hdmi.resume = hdmi_later_resume;
    register_early_suspend(&early_susp_hdmi);
    //hdmi_startup();
    return 0;
err_create_workqueue_fail:
err_sysfs_create_group:
err_create_kobject:
probe_failure:
    HDMI_ERR("HDMI: hdmi_iic_probe failed! rc = %d\n", rc);
    return rc;
}

static int __exit hdmi_iic_remove(struct i2c_client *client)
{
    gHdmi_client = NULL;
    return 0;
}

//************************************************************************************************************************************************/
//**********2. The second part is to provide an interface for the application to call the functions in the file.***************************************/
//************************************************************************************************************************************************/

static int usrfs_open(struct inode *inode, struct file *file)
{
    HDMI_DUG("call usrfs_open\n");
    return 0;
}

static int usrfs_close(struct inode *inode, struct file *file)
{
    HDMI_DUG("call usrfs_open\n");
    return 0;
}

static const struct file_operations usrfs_fops =
{
    //.owner = THIS_MODULE,
    .open    = usrfs_open,
    .ioctl   = usrfs_ioctl,
    .release = usrfs_close,
};

static struct miscdevice usrfs_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "hdmi",             //under /dev/usrfs_misc
    .fops  = &usrfs_fops,
};


	
static int usrfs_ioctl(struct inode* inode, struct file* file,
                       unsigned int cmd, unsigned long param)
{
    int ret = 0;

    switch (cmd)
    {
	    case IOCTL_OPEN_VIDEO:
	        break;
	    case IOCTL_CLOSE_VIDEO:
	        HDMI_DUG("------ioctl close video-------\n");
	        break;
	    case IOCTL_GET_STATUS:
	        ret = hdmi_is_cable_connected();
	        break;
	    case IOCTL_OPEN_AUDIO:
	        break;
	    case IOCTL_CLOSE_AUDIO:
	        break;
	default:
			break;
    }

    return ret;
}




//******************************************************************************************************************************/
//********************************3.The third part is the hdmi driver***************************************************************
//******************************************************************************************************************************/

static struct platform_driver this_driver =
{
    .probe    = hdmi_probe,
    .driver   =
    {
        .name = "hdmi",
    },
};

static int __init hdmi_probe(struct platform_device *pdev)
{
    HDMI_DUG("HDMI: hdmi_probe\n");
    return 0;
}

static unsigned int ulHdmiCableState = 0;
static char hdmi_is_cable_connected(void)
{
    char cablePlugStatus;

    cablePlugStatus = I2C_ReadByte(gHdmi_client->addr, 0x3d);
    if (cablePlugStatus & 0x08)  // cable is connected
    {
        //HDMI_DUG("hdmi_is_cable_connected return 1---\n");
        ulHdmiCableState = 1;
        return 1;
    }
    else  // cable is not connected
    {
        //HDMI_DUG("hdmi_is_cable_connected return 0---\n");
        ulHdmiCableState = 0;
        return 0;
    }
}

/*
    ·µ»ØÖµÓÐ:
        0   HDMI cableÎ´²åÈë
        1   HDMI cable²åÈë
 */
int hdmi_get_plug_status(void)
{
    return hdmi_is_cable_connected();
}

EXPORT_SYMBOL(hdmi_get_plug_status);

int hdmi_power(int on)
{
    struct vreg *vreg_mmc;
    struct vreg *vreg_msme2;
    int ret;
    /* 1.8 V */
    vreg_mmc = vreg_get(NULL, "mmc");
    if (IS_ERR(vreg_mmc))
    {
        printk(KERN_ERR "HDMI: %s: vreg_get gp1 failed (%ld)\n",
               __func__, PTR_ERR(vreg_mmc));
        return -EIO;
    }
    ret = vreg_set_level(vreg_mmc, 1800);
    if (ret)
    {
        printk(KERN_ERR "HDMI: %s: vreg gp1 set level failed (%d)\n",
               __func__, ret);
        return -EIO;
    }

    /* 1.2 V */
    vreg_msme2 = vreg_get(NULL, "msme2");
    if (IS_ERR(vreg_msme2))
    {
        printk(KERN_ERR "HDMI: %s: vreg_get gp1 failed (%ld)\n",
               __func__, PTR_ERR(vreg_msme2));
        return -EIO;
    }

    ret = vreg_set_level(vreg_msme2, 1500);
    if (ret)
    {
        printk(KERN_ERR "HDMI: %s: vreg gp1 set level failed (%d)\n",
               __func__, ret);
        return -EIO;
    }

    if (on)
    {        
        ret = vreg_enable(vreg_mmc);
        if (ret)
        {
            printk(KERN_ERR "HDMI: vreg enable failed");
            return -EIO;
        }	 
        ret = vreg_enable(vreg_msme2);
        if (ret)
        {
            printk(KERN_ERR "HDMI: vreg enable failed");
            return -EIO;
        }
    }
    else
    {
        HDMI_DUG("----------poweron(0), vreg_disable12 vreg_disable18------\n");
        vreg_disable(vreg_msme2);
        vreg_disable(vreg_mmc);  //TBC
    }
    return 0;
}

EXPORT_SYMBOL(hdmi_power);

/*****************************************************************************
 Prototype    : hdmi_startup
 Description  : The function reset the chip, initial the TPI and begin to
                poll the status of the chip
 Input        : void  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : usrfs_ioctl
 
  History        :
  1.Date         : 2010/12/7
    Author       : nielimin
    Modification : Created function
*****************************************************************************/
int hdmi_startup(void)
{
	
    
    HDMI_DUG("----------hdmi_startup------\n");
    
    /* reset HDMI IC */
    gpio_direction_output(HDMI_RESET_GPIO, 1);
    msleep(5);
    gpio_direction_output(HDMI_RESET_GPIO, 0);
    msleep(10);
    gpio_direction_output(HDMI_RESET_GPIO, 1);
    msleep(5);
    
    /* init the HDMI register  */
    TPI_Init();
    TPI_Poll();
    
    return 0;
}

EXPORT_SYMBOL(hdmi_startup);

/*****************************************************************************
 Prototype    : hdmi_shutdown
 Description  : shutdown the chip by power it off
 Input        : void  
 Output       : None
 Return Value : 
 Calls        : 
 Called By    : 
 
  History        :
  1.Date         : 2010/12/7
    Author       : nielimin
    Modification : Created function

*****************************************************************************/
int hdmi_shutdown(void)
{
    int ret;
    ret = hdmi_power(0);
    return ret;
}

#define HDMI_POWER_CONTRL_REG 0x1E

void hdmi_early_suspend(struct early_suspend *h)
{
    uint32 i2c_addr = gHdmi_client->addr;
    byte hdmi_reg_value = 0;

    if(0 == ulHdmiCableState)
    {   
        hdmi_reg_value = I2C_ReadByte(i2c_addr,HDMI_POWER_CONTRL_REG);
        hdmi_reg_value |= 0x04;
        I2C_WriteByte(i2c_addr, HDMI_POWER_CONTRL_REG, hdmi_reg_value);
       
        msleep(5);
        hdmi_reg_value = I2C_ReadByte(i2c_addr,HDMI_POWER_CONTRL_REG);
        hdmi_reg_value |= 0x03;
        I2C_WriteByte(i2c_addr, HDMI_POWER_CONTRL_REG, hdmi_reg_value);  
    }
}

static void Hdmi_resume_func(struct work_struct *work)
{
	hdmi_startup();
}

static DECLARE_DELAYED_WORK(hdmi_event, Hdmi_resume_func);

void hdmi_later_resume(struct early_suspend *h)
{
    if(0 == ulHdmiCableState)
    {
        schedule_delayed_work(&hdmi_event, msecs_to_jiffies(2000));
    }
}

EXPORT_SYMBOL(hdmi_shutdown);

static const struct i2c_device_id hdmi_id[] =
{
    { "hdmi_i2c", 0},
    { }
};

static struct i2c_driver hdmi_i2c_driver =
{
    .id_table = hdmi_id,
    .probe    = hdmi_iic_probe,
    .remove   = __exit_p(hdmi_iic_remove),
    .driver   =
    {
        .name = "hdmi_i2c",
    },
};

static int __init hdmi_init(void)
{
    int32_t rc = 0;

    rc = i2c_add_driver(&hdmi_i2c_driver);
    if (IS_ERR_VALUE(rc))
    {
        printk(KERN_ERR "HDMI: register i2c driver failed\n");
    }
    rc = misc_register(&usrfs_device);
    if (rc)
    {
        printk(KERN_ERR "HDMI: can't register misc device for minor %d\n", usrfs_device.minor);
        misc_deregister(&usrfs_device);
    }

    gpio_tlmm_config(GPIO_CFG(HDMI_RESET_GPIO, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);

    return platform_driver_register(&this_driver);
}

module_init(hdmi_init);
