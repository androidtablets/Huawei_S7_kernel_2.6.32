/*
 * drivers/input/touchscreen/mxt422.c
 *
 * Copyright (c) 2011 Huawei Device Co., Ltd.
 *
 * Using code from:
 *  - ads7846.c
 *	Copyright (c) 2005 David Brownell
 *	Copyright (c) 2006 Nokia Corporation
 *  - corgi_ts.c
 *	Copyright (C) 2004-2005 Richard Purdie
 *  - omap_ts.[hc], ads7846.h, ts_osk.c
 *	Copyright (C) 2002 MontaVista Software
 *	Copyright (C) 2004 Texas Instruments
 *	Copyright (C) 2005 Dirk Behme
 *  - mxt224.c
 *	Copyright (c) 2010 Huang Zhikui
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/module.h>  
#include <linux/hrtimer.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/mxt224.h>
#include <linux/jiffies.h>
#include <mach/mpp.h>
#include <linux/delay.h>

#if 0
#define DBG(fmt, args...) printk(KERN_INFO "[%s,%d] "fmt"\n", __FUNCTION__, __LINE__, ##args)
#else
#define DBG(fmt, args...) do {} while (0)
#endif

#define TS_POLL_DELAY		(10 * 1000)	/* ns delay before the first sample */
#define TS_PENUP_TIMEOUT_MS 20
#define	MAX_8BIT			((1 << 8) - 1)
#define USE_WQ

#define CTP_HOME_X		845
#define CTP_HOME_Y		95
#define CTP_MENU_X		845
#define CTP_MENU_Y		238
#define CTP_BACK_X		845
#define CTP_BACK_Y		382
#define KEYPAD_DIAMETER				100
#define KEYPAD_AREA(x, y, KEYNAME) 	((x >= CTP_##KEYNAME##_X - KEYPAD_DIAMETER / 2) \
									 && (x <= CTP_##KEYNAME##_X + KEYPAD_DIAMETER / 2) \
								     && (y >= CTP_##KEYNAME##_Y - KEYPAD_DIAMETER / 2) \
        	                         && (y <= CTP_##KEYNAME##_Y + KEYPAD_DIAMETER / 2))

#define	MXT_MSG_T9_STATUS				0x01
/* Status bit field */
#define		MXT_MSGB_T9_SUPPRESS		0x02
#define		MXT_MSGB_T9_AMP			0x04
#define		MXT_MSGB_T9_VECTOR		0x08
#define		MXT_MSGB_T9_MOVE		0x10
#define		MXT_MSGB_T9_RELEASE		0x20
#define		MXT_MSGB_T9_PRESS		0x40
#define		MXT_MSGB_T9_DETECT		0x80
    
#define	MXT_MSG_T9_XPOSMSB				0x02
#define	MXT_MSG_T9_YPOSMSB				0x03
#define	MXT_MSG_T9_XYPOSLSB				0x04
#define	MXT_MSG_T9_TCHAREA				0x05
#define	MXT_MSG_T9_TCHAMPLITUDE				0x06
#define	MXT_MSG_T9_TCHVECTOR				0x07

#ifdef USE_WQ
#include <linux/workqueue.h>
static void mxt224_timer(struct work_struct *work);
DECLARE_WORK(mxt224_work, mxt224_timer);
#endif

static struct mxt224 *mxt224_tsc;

struct orig_t {
    int x;
    int y;
};
 
/* upper left point */
static struct orig_t tsp_ul_50x50 = {
    50, 
    50
};

/* lower right point */
static struct orig_t tsp_dr_50x50 = {
    750,
    430
};

static struct orig_t lcd_size = {
    800, 
    480
};

static struct orig_t tsp_ul_50x50_convert, tsp_dr_50x50_convert;
static struct orig_t tsp_origin_convert;

static void mxt224_penup_timeout(unsigned long arg)
{
 	return ;
}

static inline void restart_scan(struct mxt224 *ts, unsigned long delay)
{
#if defined(USE_WQ)
    schedule_work(&mxt224_work);
#elif defined(USE_NORMAL_TIMER)
    mod_timer(&ts->timer,
        jiffies + usecs_to_jiffies(500000));  // delay / 1000
#else
    hrtimer_start(&ts->hr_timer, ktime_set(0, delay),
                HRTIMER_MODE_REL);
#endif
}

struct finger {
	int x;
	int y;
    int z;    
	int size; 
    int pressure; 
};
static struct finger fingers[MAX_FINGERS] = {};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxt224_early_suspend(struct early_suspend *h);
static void mxt224_late_resume(struct early_suspend *h);
#endif

void mxt224_update_pen_state(void *tsc)
{
	struct mxt224 *ts = tsc;
	struct input_dev *input = ts->input; 

	u8  status;
	u16 xpos = 0xFFFF;
	u16 ypos = 0xFFFF;
	u8  touch_size = 255;
	u8  touch_number;
	u8  amplitude;
    static int nPrevID= -1;
	int bChangeUpDn= 0;
    u8  i;
    
	status = ts->tc.tchstatus;
    
	if (status & MXT_MSGB_T9_SUPPRESS) {
		/* Touch has been suppressed by grip/face */
		/* detection                              */
	} else { 	
		xpos = ts->tc.x;
	    ypos = ts->tc.y;
        touch_number = ts->tc.touch_number_id-2;

		if (status & MXT_MSGB_T9_DETECT) {
          	/* judge key pressed*/
	        if (KEYPAD_AREA(xpos, ypos, HOME)) {
	           	input_report_key(input, KEY_HOME, 1);  
	            input_sync(input);                           
	        } else if (KEYPAD_AREA(xpos, ypos, MENU)) {
	            input_report_key(input, KEY_MENU, 1); 
	            input_sync(input);  
	        } else if (KEYPAD_AREA(xpos, ypos, BACK)) { 
	            input_report_key(input, KEY_BACK, 1);
	            input_sync(input);  
	        }else{
			/*
			 * TODO: more precise touch size calculation?
			 * mXT224 reports the number of touched nodes,
			 * so the exact value for touch ellipse major
			 * axis length would be 2*sqrt(touch_size/pi)
			 * (assuming round touch shape).
			 */
			DBG( " ----process_T9_message --MXT_MSGB_T9_DETECT--- \n");
			touch_size = ts->tc.tcharea;
			touch_size = touch_size >> 2;
			if (!touch_size)
				touch_size = 1;
            
            if (status & MXT_MSGB_T9_PRESS)
            	bChangeUpDn= 1;
            
			if (status & MXT_MSGB_T9_AMP)
				/* Amplitude of touch has changed */
				amplitude = ts->tc.tchamp;
            
            if ((status & MXT_MSGB_T9_PRESS)
			    || (status & MXT_MSGB_T9_MOVE)
			    || (status & MXT_MSGB_T9_AMP)){	
	            fingers[touch_number].x = xpos;
				fingers[touch_number].y = ypos;
				fingers[touch_number].size = touch_size;
	            fingers[touch_number].pressure= ts->tc.tchamp;         
            }
            
          }
		} else if (status & MXT_MSGB_T9_RELEASE) {
			/* The previously reported touch has been removed.*/
			DBG( " ----process_T9_message --MXT_MSGB_T9_RELEASE--- \n");
            bChangeUpDn= 1;
            
            input_report_key(input, BTN_TOUCH, 0);
			input_report_abs(input, ABS_PRESSURE, 0);
			input_report_key(input, KEY_HOME, 0);  
			input_report_key(input, KEY_MENU, 0); 
			input_report_key(input, KEY_BACK, 0); 

            fingers[touch_number].pressure= 0;
		}
        
		//report_sync(ts);
		if( nPrevID >= touch_number || bChangeUpDn )
	    {
			for (i = 0; i < MAX_FINGERS; i++) {
               
                if(fingers[i].pressure == -1 )
                {
                  continue;
                }
                
				DBG("report_sync touch_number=%d, x=%d, y=%d, width=%d\n",
						i, fingers[i].x, fingers[i].y, fingers[i].size);
				input_report_abs(ts->input, ABS_MT_POSITION_X, fingers[i].x);
				input_report_abs(ts->input, ABS_MT_POSITION_Y, fingers[i].y);
				input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, fingers[i].pressure);
				input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR,  (touch_number <<8)|fingers[i].size);
				input_mt_sync(ts->input);

                if(fingers[i].pressure == 0 )
                {
                    fingers[i].pressure= -1;
                }
			}

			input_sync(ts->input);
			DBG("report_sync() OUT\n");
	    }
    	nPrevID= touch_number;
	}
        
	return;
}

static int mxt224_read_values(struct mxt224 *tsc)
{
    mxt224_get_message(tsc);
	return 0;
}

#if defined(USE_WQ)
static void mxt224_timer(struct work_struct *work)
{
	struct mxt224 *ts = mxt224_tsc;
#elif defined(USE_NORMAL_TIMER)
static void mxt224_timer(unsigned long tsc)
{
    struct mxt224 *ts = (struct mxt224 *)tsc;
#else
static enum hrtimer_restart mxt224_timer(struct hrtimer *handle)
{
	struct mxt224 *ts = container_of(handle, struct mxt224, hr_timer);
#endif
	ts = mxt224_tsc;
	mxt224_read_values(ts);
	mxt224_update_pen_state(ts);

    /* kick pen up timer - to make sure it expires again(!) */
    mod_timer(&ts->timer, jiffies + msecs_to_jiffies(TS_PENUP_TIMEOUT_MS));
    enable_irq(ts->irq);

#if defined(USE_WQ)
	return;
#elif defined(USE_NORMAL_TIMER)
	return;
#else
	return HRTIMER_NORESTART;
#endif
}

static irqreturn_t mxt224_irq(int irq, void *handle)
{
	struct mxt224 *ts = handle;
	unsigned long flags;

	spin_lock_irqsave(&ts->lock, flags);
	disable_irq_nosync(ts->irq);
	restart_scan(ts, TS_POLL_DELAY);
	spin_unlock_irqrestore(&ts->lock, flags);

	return IRQ_HANDLED;
}

static int mxt224_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct mxt224 *ts;
	struct mxt224_platform_data *pdata = client->dev.platform_data;
	struct input_dev *input_dev;
	int err;

	DBG("Begin Probe MXT224 driver");
	if (!pdata) {
		dev_err(&client->dev, "platform data is required!\n");
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_READ_WORD_DATA))
		return -EIO;

	ts = kzalloc(sizeof(struct mxt224), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!ts || !input_dev) {
		err = -ENOMEM;
		goto err_kfree;
	}

    mxt224_tsc = ts;
	ts->x_max = 800; // MAX_12BIT;
	ts->y_max = 480; // MAX_12BIT;
	
	ts->client = client;
	ts->input = input_dev;
	ts->irq = client->irq;
	i2c_set_clientdata(client, ts);    

	/* convert touchscreen coordinator to coincide with LCD coordinator */
	/* get tsp at (0,0) */
	tsp_ul_50x50_convert.x = (tsp_ul_50x50.x < tsp_dr_50x50.x) 
	                        ? tsp_ul_50x50.x : (ts->x_max - tsp_ul_50x50.x);
	tsp_ul_50x50_convert.y = (tsp_ul_50x50.y < tsp_dr_50x50.y) 
	                        ? tsp_ul_50x50.y : (ts->y_max - tsp_ul_50x50.y);
	tsp_dr_50x50_convert.x = (tsp_ul_50x50.x < tsp_dr_50x50.x) 
	                        ? tsp_dr_50x50.x : (ts->x_max - tsp_dr_50x50.x);
	tsp_dr_50x50_convert.y = (tsp_ul_50x50.y < tsp_dr_50x50.y) 
	                        ? tsp_dr_50x50.y : (ts->y_max - tsp_dr_50x50.y);
	tsp_origin_convert.x = tsp_ul_50x50_convert.x 
	                            - (((tsp_dr_50x50_convert.x - tsp_ul_50x50_convert.x) 
	                               * 50) / (lcd_size.x - 100));
	tsp_origin_convert.y = tsp_ul_50x50_convert.y 
	                            - (((tsp_dr_50x50_convert.y - tsp_ul_50x50_convert.y) 
	                               * 50) / (lcd_size.y - 100));
#if defined(USE_WQ)
	setup_timer(&ts->timer, mxt224_penup_timeout, (unsigned long)ts);
#elif defined(USE_NORMAL_TIMER)
	setup_timer(&ts->timer, mxt224_timer, (unsigned long)ts);
#else
	ts->hr_timer.function = mxt224_timer;
	hrtimer_init(&ts->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
#endif

	spin_lock_init(&ts->lock);

	if (pdata->init_platform_hw) {
		if ((err = pdata->init_platform_hw()) < 0)
	        goto err_hrtimer;
    }

	if ((err = mxt224_generic_probe(ts)) < 0)
        goto err_platform_hw;
        
	snprintf(ts->phys, sizeof(ts->phys),
		 "%s/input0", dev_name(&client->dev));

	input_dev->name = "mxt224_touchscreen";
	input_dev->phys = ts->phys;
	input_dev->id.bustype = BUS_I2C;

    set_bit(EV_ABS, input_dev->evbit);
    set_bit(EV_KEY, input_dev->evbit);
    set_bit(ABS_X, input_dev->absbit);
    set_bit(ABS_Y, input_dev->absbit);
    set_bit(ABS_PRESSURE, input_dev->absbit);
    set_bit(KEY_HOME, input_dev->keybit);
    set_bit(KEY_MENU, input_dev->keybit);
    set_bit(KEY_BACK, input_dev->keybit);
    set_bit(BTN_TOUCH, input_dev->keybit);

	input_set_abs_params(input_dev, ABS_X, 0, ts->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, ts->y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, MAX_8BIT, 0, 0);
    
   //input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 1,4, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ts->x_max, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ts->y_max, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 0xF, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 0xF, 0, 0);

	if ((err = input_register_device(input_dev)) < 0)
		goto err_mxt224_generic_remove;

	mdelay(1000);
    	err = request_irq(ts->irq, mxt224_irq, IRQF_TRIGGER_FALLING, /* IRQF_TRIGGER_LOW IRQF_TRIGGER_HIGH */
            client->dev.driver->name, ts);
	if (err < 0) {
		dev_err(&client->dev, "irq %d busy?\n", ts->irq);
		goto err_input_unregister_device;
	}
	dev_info(&client->dev, "registered with irq (%d)\n", ts->irq);
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = mxt224_early_suspend;
	ts->early_suspend.resume = mxt224_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif
	DBG("Probe MXT224 driver SUCCESS");
	return 0;


err_input_unregister_device:
    	input_unregister_device(ts->input);
err_mxt224_generic_remove:
    	mxt224_generic_remove(ts);
err_platform_hw:
    if (pdata->exit_platform_hw)
		pdata->exit_platform_hw();
err_hrtimer:
#if !defined(USE_WQ) && !defined(USE_NORMAL_TIMER)
	hrtimer_cancel(&ts->hr_timer);
#endif
	input_free_device(input_dev);
err_kfree:
	kfree(ts);

	DBG("Probe MXT224 driver FAILED");
	return err;
}

static void mxt224_suspend(struct i2c_client *client, 
			pm_message_t mesg)
{
	/* clear msg when being sleep */
    mxt224_read_values(mxt224_tsc);
    /* disable mxt224 interrupte when being sleep */
    config_disable_mxt244();
    return ;
}

static void mxt224_resume(struct i2c_client *client)
{
    /* enable mxt224 interrupte when being sleep */
    config_enable_mxt244();
    /* clear msg when being resume */
    mxt224_read_values(mxt224_tsc);
	
    return ;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void mxt224_early_suspend(struct early_suspend *h)
{
	struct mxt224 *ts;
	ts = container_of(h, struct mxt224, early_suspend);
	mxt224_suspend(ts->client, PMSG_SUSPEND);
}

static void mxt224_late_resume(struct early_suspend *h)
{
	struct mxt224 *ts;
	ts = container_of(h, struct mxt224, early_suspend);
	mxt224_resume(ts->client);
}
#endif

static int mxt224_remove(struct i2c_client *client)
{
	struct mxt224 *ts = i2c_get_clientdata(client);
	struct mxt224_platform_data *pdata;

	DBG("mxt224 remove");
	pdata = client->dev.platform_data;

	input_unregister_device(ts->input);
	mxt224_generic_remove(ts);
	if (pdata->exit_platform_hw)
	    pdata->exit_platform_hw();
#if !defined(USE_WQ) && !defined(USE_NORMAL_TIMER)
	hrtimer_cancel(&ts->hr_timer);
#endif
	input_free_device(ts->input);
	kfree(ts);

	free_irq(ts->irq, ts);

	return 0;
}



static struct i2c_device_id mxt224_idtable[] = {
	{ "mxt224", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, mxt224_idtable);

static struct i2c_driver mxt224_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "mxt224"
	},
	.id_table	= mxt224_idtable,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend 	= mxt224_suspend,
	.resume 	= mxt224_resume,
#endif
	.probe		= mxt224_probe,
	.remove		= mxt224_remove,
};

static int __init mxt224_init(void)
{
	DBG("Begin INIT MXT224 Module!");
	return i2c_add_driver(&mxt224_driver);
}

static void __exit mxt224_exit(void)
{
	i2c_del_driver(&mxt224_driver);
}

module_init(mxt224_init);
module_exit(mxt224_exit);

MODULE_AUTHOR("Li Yaobing <liyaobing@huawei.com>");
MODULE_DESCRIPTION("MXT224 TouchScreen Driver");
MODULE_LICENSE("GPL v2");
