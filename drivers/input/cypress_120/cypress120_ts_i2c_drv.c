/* Driver for Cypress 120 touch panel device 

 *  Copyright 2010 HUAWEI Tech. Co., Ltd.

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
/*
 *  Cypress 120 i2c touch panle driver.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <mach/io.h>
#include <mach/system.h>
#include <mach/gpio.h>
#include <linux/pm.h>			/* pm_message_t */
#include <mach/cypress120_ts_dev.h>

//reg define
#define   REG_FW_VERSION     		0x03
#define   REG_CON_STATUS	 		0x04
#define   REG_DEEPSLEEP_TIMEOUT	 	0x06
#define   REG_C_FLAG				0x07

#define   REG_POS_X1_H	 	 		0x09
#define   REG_POS_X1_L	 	 		0x0a
#define   REG_POS_Y1_H	 	 		0x0b
#define   REG_POS_Y1_L	 	 		0x0c

#define   REG_POS_X2_H	 	 		0x0d
#define   REG_POS_X2_L	 	 		0x0e
#define   REG_POS_Y2_H	 	 		0x0f
#define   REG_POS_Y2_L	 	 		0x10

#define   REG_FINGER	 	 		0x11
#define   REG_GESTURE	 	 		0x12

#define   TOUCH_SCREEN_WIDTH		800
#define   TOUCH_SCREEN_HIGHT		480

#define BSP_X_SIZE  800
#define BSP_Y_SIZE  480
 
#define BSP_X_COMPENSATE  4
#define BSP_Y_COMPENSATE  16

#define CYPRESS_PENUP_TIMEOUT_MS 60
static int gfig_num = 0; //touch finger num
static struct cypress120_ts_read_info gcypress_read_info;
static struct timer_list gCypress_Timer;

static void Cypress_TimerHandler(unsigned long data);

struct  i2c_client* cypress120_i2c_client;
static unsigned char g_cypress_dect_flag = 0;

// following are the sysfs callback functions 
static ssize_t cap_touchscreen_attr_show(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf)
 {
     	  return sprintf(buf, "%d", g_cypress_dect_flag);		  
 }

// Define the device attributes 
 static struct kobj_attribute cap_touchscreen_attribute =
         __ATTR(state, 0666, cap_touchscreen_attr_show, NULL);

 static struct attribute* cap_touchscreen_attributes[] =
 {
         &cap_touchscreen_attribute.attr,
         NULL
 };

 static struct attribute_group cap_touchscreen_defattr_group =
 {
         .attrs = cap_touchscreen_attributes,
 };

static struct workqueue_struct *cypress120_wq;
static int cypress120_ts_suspend(struct i2c_client *client, pm_message_t mesg);
static int cypress120_ts_resume(struct i2c_client *client);
static void cypress120_ts_early_suspend(struct early_suspend *h);
static void cypress120_ts_early_resume(struct early_suspend *h);
static int cypress120_ts_gpio_reset(struct i2c_client *client);

static int cypress120_i2c_write(struct i2c_client *client, unsigned char reg, unsigned char data)
{
	int rc;
	
	rc = i2c_smbus_write_byte_data(client, reg, data);
	return rc;
}

static int cypress120_i2c_read(struct i2c_client *client, unsigned char reg)
{
	int rc;	

	rc = i2c_smbus_read_byte_data(client, reg);
	return rc;
}

static int cypress120_get_touchinfo(struct cypress120_ts_read_info *pts_info)
{
	int rc = 0;
    
	rc = i2c_smbus_read_word_data(cypress120_i2c_client,REG_POS_X1_H);
	if(rc < 0)
		goto read_ts_err;
	else
		pts_info->x1_pos = ( (unsigned short)(rc & 0xff )<<8 | (unsigned short)(rc & 0xff00)>>8 );
    
	rc = i2c_smbus_read_word_data(cypress120_i2c_client,REG_POS_Y1_H);
	if(rc < 0)
		goto read_ts_err;
	else
		pts_info->y1_pos = ( (unsigned short)(rc & 0xff )<<8 | (unsigned short)(rc & 0xff00)>>8 );
    
	rc = i2c_smbus_read_word_data(cypress120_i2c_client,REG_POS_X2_H);
	if(rc < 0)
		goto read_ts_err;
	else
		pts_info->x2_pos = ( (unsigned short)(rc & 0xff )<<8 | (unsigned short)(rc & 0xff00)>>8 );
    
	rc = i2c_smbus_read_word_data(cypress120_i2c_client,REG_POS_Y2_H);
	if(rc < 0)
		goto read_ts_err;
	else
		pts_info->y2_pos = ( (unsigned short)(rc & 0xff )<<8 | (unsigned short)(rc & 0xff00)>>8 );
    
	rc = cypress120_i2c_read(cypress120_i2c_client,REG_FINGER);
	if(rc < 0)
		goto read_ts_err;
	else
		pts_info->finger = (unsigned char)(rc & 0xff);
    
	rc = cypress120_i2c_read(cypress120_i2c_client,REG_GESTURE);
	if(rc < 0)
		goto read_ts_err;
	else
		pts_info->gesture = (unsigned char)(rc & 0xff);
    
	return rc;
    
read_ts_err:
	printk(KERN_ERR "cypress120_get_touchinfo: read data err! errcode=%d\n",rc);
	memset(pts_info,0,sizeof(struct cypress120_ts_read_info));
	return rc;
}

#define  MUTI_TOUCH_SUPPORT 1
static void cypress120_report_event(struct cypress120_ts_i2c_platform_data *ts, 
	struct cypress120_ts_read_info* pTSEventInfo)
{

	static int x1_last = 0, y1_last = 0, x2_last = 0, y2_last = 0; 
	unsigned short position[2][2]={{0,0},{0,0}}; 

#if (MUTI_TOUCH_SUPPORT)
    //one finger touch 
	if(pTSEventInfo->finger == 1) {
	    position[0][0] = pTSEventInfo->x1_pos + (BSP_X_SIZE - 2*(pTSEventInfo->x1_pos))*BSP_X_COMPENSATE/BSP_X_SIZE;
        position[0][1] = pTSEventInfo->y1_pos + (BSP_Y_SIZE - 2*(pTSEventInfo->y1_pos))*BSP_Y_COMPENSATE/BSP_Y_SIZE;
        
        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 255);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_X, position[0][0]);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, position[0][1]);
		input_mt_sync(ts->input_dev);
		input_sync(ts->input_dev); 

        //first scan
		if(gfig_num == 0) {  
			gfig_num = 1;
			x1_last = pTSEventInfo->x1_pos;
			y1_last = pTSEventInfo->y1_pos;
			return;
		}
	}
    //two finger touch
	else if(pTSEventInfo->finger == 2) {
		position[0][0] = pTSEventInfo->x1_pos;
		position[0][1] = pTSEventInfo->y1_pos;
		position[1][0] = pTSEventInfo->x2_pos;
		position[1][1] = pTSEventInfo->y2_pos;

        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 255);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_X, position[0][0]);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, position[0][1]);
		input_mt_sync(ts->input_dev);
		
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 255);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_X, position[1][0]);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, position[1][1]);
		input_mt_sync(ts->input_dev);
		input_sync(ts->input_dev);

        //first scan
		if(gfig_num == 0) {
			gfig_num = 2;
			x1_last = pTSEventInfo->x1_pos;
			y1_last = pTSEventInfo->y1_pos;
			x2_last = pTSEventInfo->x2_pos;
			y2_last = pTSEventInfo->y2_pos;

			return;
		}
	}
#else
    //one finger touch 
	else if ((pTSEventInfo->finger == 1) || (pTSEventInfo->finger == 2)) {
		position[0][0] = pTSEventInfo->x1_pos;
		position[0][1] = pTSEventInfo->y1_pos;
		position[1][0] = pTSEventInfo->x2_pos;
		position[1][1] = pTSEventInfo->y2_pos;

        //first scan
		if(gfig_num == 0) {
			gfig_num = pTSEventInfo->finger;
			input_report_abs(ts->input_dev, ABS_PRESSURE, 255);
			input_report_abs(ts->input_dev, ABS_X, position[0][0]);
			input_report_abs(ts->input_dev, ABS_Y, position[0][1]);					 	 
			input_report_abs(ts->input_dev, ABS_TOOL_WIDTH, 1);
			input_report_key(ts->input_dev, BTN_TOUCH, 1);
			input_sync(ts->input_dev);
			x1_last = position[0][0];
			y1_last = position[0][1];
		} else if (((position[0][0]-x1_last) >= 5) || ((x1_last-position[0][0]) >= 5) \
				|| ((position[0][1]-y1_last) >= 5) || ((y1_last-position[0][1]) >= 5)) //valid touch motion
			{
					input_report_abs(ts->input_dev, ABS_PRESSURE, 255);
					input_report_abs(ts->input_dev, ABS_X, position[0][0]);
					input_report_abs(ts->input_dev, ABS_Y, position[0][1]);					 	 
					input_report_abs(ts->input_dev, ABS_TOOL_WIDTH, 1);
					input_report_key(ts->input_dev, BTN_TOUCH, 1);
					input_sync(ts->input_dev);
					x1_last = position[0][0];
					y1_last = position[0][1];
			}
		}
	}
#endif
	if(pTSEventInfo->gesture > GESTURE_NO_GESTURE) {
		input_report_gesture(ts->input_dev, pTSEventInfo->gesture ,0);
		input_sync(ts->input_dev);
	}
	//out_cypress_report_event:
	x1_last = position[0][0];
	y1_last = position[0][1];
	x2_last = position[1][0];
	y2_last = position[1][1];
}

static void Cypress_TimerHandler(unsigned long data)
{
    struct cypress120_ts_i2c_platform_data *ts = (struct cypress120_ts_i2c_platform_data *)data;
    
	if((gcypress_read_info.finger == 0) && (gfig_num > 0)) {
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,255 );
		input_sync(ts->input_dev);
		gfig_num = 0;
	}
}

static void cypress120_ts_work_func(struct work_struct *work)
{
	struct cypress120_ts_i2c_platform_data *ts = container_of(work, struct cypress120_ts_i2c_platform_data, work);
	int rc = 0;

	rc = cypress120_get_touchinfo(&gcypress_read_info);
	if(rc < 0)
		printk(KERN_ERR "cypress120_ts_work_func:cypress120_get_touchinfo return err code %d!\n",rc);

	//clear reg07 c_flag
	rc = cypress120_i2c_write(cypress120_i2c_client,REG_C_FLAG,0x01);

	//enable the interrupt 
	enable_irq(ts->client->irq);
	
	cypress120_report_event(ts,&gcypress_read_info);	

    mod_timer(&gCypress_Timer, jiffies + msecs_to_jiffies(CYPRESS_PENUP_TIMEOUT_MS));
	
}

static irqreturn_t cypress120_ts_irq_handler(int irq, void *dev_id)
{
	struct cypress120_ts_i2c_platform_data *ts = dev_id;
    
    disable_irq_nosync(ts->client->irq);

    queue_work(cypress120_wq, &ts->work);

	return IRQ_HANDLED;
}


static int cypress120_ts_dev_open(struct input_dev *dev)
{
	struct cypress120_ts_i2c_platform_data *ts = input_get_drvdata(dev);
	int rc;
    
	if (!ts->client) {
		printk(KERN_ERR "cypress120_ts_dev_open: no i2c adapter present\n");
		rc = -ENODEV;
		return rc;
	}

    cypress120_ts_gpio_reset(ts->client);

	return 0;
}

static void cypress120_ts_dev_close(struct input_dev *dev)
{
	struct cypress120_ts_i2c_platform_data *ts = input_get_drvdata(dev);

	if (ts->dect_irq) {
		free_irq(ts->dect_irq, ts);
		ts->dect_irq = 0;
	}
	cancel_work_sync(&ts->work);
}


static int cypress120_ts_gpio_reset(struct i2c_client *client)
{
    struct cypress120_ts_i2c_platform_data *ts = client->dev.platform_data;
    
    gpio_set_value(ts->reset_gpio, 0);
    mdelay(5);
    gpio_set_value(ts->reset_gpio, 1);
    mdelay(5);
    gpio_set_value(ts->reset_gpio, 0);
    mdelay(5);
    
    return 0;
}

static int cypress120_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cypress120_ts_i2c_platform_data *ts = NULL;
	int ret = 0;
	int icount = 0;
	unsigned char ts_status = 0;
	struct kobject *kobj = NULL;    
       
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "cypress120_ts_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}
    
	// 1. create kobject and  store the type of touchscreen
	kobj = kobject_create_and_add("cap_touchscreen", NULL);
  	if (kobj == NULL) {	
		return -1;
	}
  	if (sysfs_create_group(kobj, &cap_touchscreen_defattr_group)) {
		kobject_put(kobj);
		return -1;
	}

	// 2. reset and wakeup the touchscreen
	cypress120_ts_gpio_reset(client);
	cypress120_i2c_client = client;	
	ts_status = (1<<3);
	ret = cypress120_i2c_write(cypress120_i2c_client,REG_CON_STATUS,ts_status);
	cypress120_ts_gpio_reset(client);
    

	// 3. check if it's cap_touchscreen and reprot the higher lever  start  start
	if(!g_cypress_dect_flag) {
		unsigned char hw_ver = 0;
		ret = cypress120_i2c_read(cypress120_i2c_client,REG_FW_VERSION);
		if(ret < 0)
			goto err_read_vendor;
		else
			hw_ver = (unsigned char)(ret & 0xff);
		g_cypress_dect_flag = 1;
	}
	
	// 4. create single thread workqueue
	cypress120_wq = create_singlethread_workqueue("cypress120_wq");
	if (!cypress120_wq) {
		printk(KERN_ERR "create cypress120_wq error\n");
		goto err_creat_workqueue_fail;
	}
    
	// 5. alloc struct of cypress120
	ts = kzalloc(sizeof(struct cypress120_ts_i2c_platform_data), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	ts->client = client;
	i2c_set_clientdata(client, ts);
	INIT_WORK(&ts->work, cypress120_ts_work_func);

	// 6. alloc and register a input dev and fill the property and operation, set the bits
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		printk(KERN_ERR "cypress120_ts_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}

	input_set_drvdata(ts->input_dev, ts);
	ts->input_dev->open       	= cypress120_ts_dev_open;
	ts->input_dev->close      	= cypress120_ts_dev_close;
	ts->input_dev->name       	= "cypress120-i2c-touchscreen";
	ts->input_dev->phys       	= "dev/cypress120_ts";
	ts->input_dev->id.bustype 	= BUS_I2C;
	ts->input_dev->id.product 	= 1;
	ts->input_dev->id.version 	= 1;

	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);	
	set_bit(EV_ABS, ts->input_dev->evbit);	
	
	input_set_abs_params(ts->input_dev, ABS_X, 0, TOUCH_SCREEN_WIDTH, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_Y, 0, TOUCH_SCREEN_HIGHT, 0, 0); 
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 64, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_TOOL_WIDTH, 0, 15, 0, 0);

	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, TOUCH_SCREEN_WIDTH, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, TOUCH_SCREEN_HIGHT, 0, 0);
	
	set_bit(EV_GESTURE, ts->input_dev->evbit);
	for (icount = GESTURE_NO_GESTURE; icount < GESTURE_MAX; icount++)
		set_bit(icount, ts->input_dev->gesturebit);	
	
	// 7. register the input device
	ret = input_register_device(ts->input_dev);
	if (ret) {
		printk(KERN_ERR "cypress120_ts_probe: Unable to register %s input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	}  

#ifdef CONFIG_HAS_EARLYSUSPEND   
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB - 1;
	ts->early_suspend.suspend = cypress120_ts_early_suspend;
	ts->early_suspend.resume = cypress120_ts_early_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	ret = request_irq(ts->client->irq,
		       &cypress120_ts_irq_handler,
		       IRQF_TRIGGER_FALLING,
		       "Cypress120 dect int ",
		       ts);
	if (ret) {
		printk(KERN_ERR "cypress120_ts_dev_open: FAILED: request_irq ret=%d\n", ret);
		return ret;
	}

    memset(&gcypress_read_info,0,sizeof(struct cypress120_ts_read_info));
    setup_timer(&gCypress_Timer, Cypress_TimerHandler, (unsigned long)ts);
    
	return 0;

err_input_register_device_failed:                //step 7
	input_free_device(ts->input_dev);
err_input_dev_alloc_failed:                      //step 6
	kfree(ts);
err_alloc_data_failed:                           //step 5
	destroy_workqueue(cypress120_wq);
err_creat_workqueue_fail:                            //step 4
err_read_vendor:                                     //step 3
err_check_functionality_failed:
	return ret;
}

static int cypress120_ts_remove(struct i2c_client *client)
{
	struct cypress120_ts_i2c_platform_data *ts = i2c_get_clientdata(client);

	g_cypress_dect_flag = 0;
	if (ts->dect_irq) {
		free_irq(ts->dect_irq, ts);
		ts->dect_irq = 0;
	}

    del_timer_sync(&gCypress_Timer);
    
	ts->gpio_free();
	input_unregister_device(ts->input_dev);
	input_free_device(ts->input_dev);
	kfree(ts);
    
	return 0;
}

static int cypress120_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int rc = 0;
	unsigned char ts_status = 0;

	struct cypress120_ts_i2c_platform_data *ts = i2c_get_clientdata(cypress120_i2c_client);
	
	disable_irq_nosync( cypress120_i2c_client->irq );

	cancel_work_sync(&ts->work);

    // deep sleep sequence flow as 1,2,3,4
    //1.Issue HW Reset
 	rc = cypress120_ts_gpio_reset(client);
 	
	if(rc < 0) {
		printk(KERN_ERR "cypress120_ts_suspend: rst gpio reset failed! err is %d! \n",rc );
		return rc;
	}
	mdelay(5);
    
	//2.Configure Low Power Timeout as "0x00"
	rc = cypress120_i2c_write(cypress120_i2c_client,REG_DEEPSLEEP_TIMEOUT,0x00);
	if(rc < 0) {
		printk(KERN_ERR "cypress120_ts_suspend: cypress120_i2c_write failed! err = %d! \n",rc);
		return rc;		
	}
	rc = cypress120_i2c_read(cypress120_i2c_client,REG_CON_STATUS);
	if(rc < 0)
		return rc;
	else
		ts_status = rc | (1<<4) | (1<<3);

	rc = cypress120_i2c_write(cypress120_i2c_client,REG_CON_STATUS,ts_status);
	if(rc < 0)
		return rc;

	return 0;
}

static int cypress120_ts_resume(struct i2c_client *client)
{
	/*  Wakeup sequence
	1.	Issue HW Reset
	2.	PSoC is now waking up
	*/
	int rc = 0;
	rc = cypress120_ts_gpio_reset(client);
	if(rc < 0) {
		return rc;
	}
	enable_irq( cypress120_i2c_client->irq );
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cypress120_ts_early_suspend(struct early_suspend *h)
{
	struct cypress120_ts_i2c_platform_data *ts;
	ts = container_of(h, struct cypress120_ts_i2c_platform_data, early_suspend);
	cypress120_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void cypress120_ts_early_resume(struct early_suspend *h)
{
    struct cypress120_ts_i2c_platform_data *ts;
	ts = container_of(h, struct cypress120_ts_i2c_platform_data, early_suspend);
	cypress120_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id  cypress120_ts_id[] = {
	{ CYPRESS120_I2C_TS_NAME, 0 },
	{ }
};

static struct i2c_driver cypress120_ts_driver = {
	.probe		= cypress120_ts_probe,
	.remove		= cypress120_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= cypress120_ts_suspend,
	.resume		= cypress120_ts_remove,
#endif
	.id_table	= cypress120_ts_id,
	.driver = {
		.name	= CYPRESS120_I2C_TS_NAME,
	},
};

static int __devinit cypress120_ts_init(void)
{
	return i2c_add_driver(&cypress120_ts_driver);
}

static void __exit cypress120_ts_exit(void)
{  
	i2c_del_driver(&cypress120_ts_driver);
	if (cypress120_wq) {
    	destroy_workqueue(cypress120_wq);
    }
}

module_init(cypress120_ts_init);
module_exit(cypress120_ts_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.1");
MODULE_AUTHOR("HUAWEI Tech. Co., Ltd . wufan");
MODULE_DESCRIPTION("Cypress120 I2c Touchscreen Driver");
MODULE_ALIAS("platform:cypress120-ts");
