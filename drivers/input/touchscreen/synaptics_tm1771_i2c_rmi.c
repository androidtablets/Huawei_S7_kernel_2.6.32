/* drivers/input/keyboard/synaptics_tm1771_i2c_rmi.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (C) 2008 Texas Instrument Inc.
 * Copyright (C) 2009 Synaptics, Inc.
 *
 * provides device files /dev/input/event#
 * for named device files, use udev
 * 2D sensors report ABS_X_FINGER(0), ABS_Y_FINGER(0) through ABS_X_FINGER(7), ABS_Y_FINGER(7)
 * NOTE: requires updated input.h, which should be included with this driver
 * 1D/Buttons report BTN_0 through BTN_0 + button_count
 * TODO: report REL_X, REL_Y for flick, BTN_TOUCH for tap (on 1D/0D; done for 2D)
 * TODO: check ioctl (EVIOCGABS) to query 2D max X & Y, 1D button count
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/synaptics_tm1771_i2c_rmi.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>

#define BTN_F19 BTN_0
#define BTN_F30 BTN_0
#define SCROLL_ORIENTATION REL_Y

static struct workqueue_struct *synaptics_wq;

/* Register: EGR_0 */
#define EGR_PINCH_REG		0
#define EGR_PINCH 		(1 << 6)
#define EGR_PRESS_REG 		0
#define EGR_PRESS 		(1 << 5)
#define EGR_FLICK_REG 		0
#define EGR_FLICK 		(1 << 4)
#define EGR_EARLY_TAP_REG	0
#define EGR_EARLY_TAP		(1 << 3)
#define EGR_DOUBLE_TAP_REG	0
#define EGR_DOUBLE_TAP		(1 << 2)
#define EGR_TAP_AND_HOLD_REG	0
#define EGR_TAP_AND_HOLD	(1 << 1)
#define EGR_SINGLE_TAP_REG	0
#define EGR_SINGLE_TAP		(1 << 0)
/* Register: EGR_1 */
#define EGR_PALM_DETECT_REG	1
#define EGR_PALM_DETECT		(1 << 0)

struct synaptics_function_descriptor {
	__u8 queryBase;
	__u8 commandBase;
	__u8 controlBase;
	__u8 dataBase;
	__u8 intSrc;
#define FUNCTION_VERSION(x) ((x >> 5) & 3)
#define INTERRUPT_SOURCE_COUNT(x) (x & 7)

	__u8 functionNumber;
};
#define FD_ADDR_MAX 0xE9
#define FD_ADDR_MIN 0x05
#define FD_BYTE_COUNT 6

#define MIN_ACTIVE_SPEED 5

#define TS_PENUP_TIMEOUT_MS 20
#define TS_KEYUP_TIMEOUT_MS 30
/* end: added by z00168965 for multi-touch */

#define TOUCH_LCD_X_MAX	3128
#define TOUCH_LCD_Y_MAX	1758

/*virtual key boundarys*/
#define SYNAPTICS_TM1771_HOME_X		3340
#define SYNAPTICS_TM1771_HOME_Y		382
#define SYNAPTICS_TM1771_MENU_X		3340
#define SYNAPTICS_TM1771_MENU_Y		915
#define SYNAPTICS_TM1771_BACK_X		3340
#define SYNAPTICS_TM1771_BACK_Y		1434
#define KEYPAD_DIAMETER				100
#define KEYPAD_AREA(x, y, KEYNAME) 	((x >= SYNAPTICS_TM1771_##KEYNAME##_X - KEYPAD_DIAMETER / 2) \
									 && (x <= SYNAPTICS_TM1771_##KEYNAME##_X + KEYPAD_DIAMETER / 2) \
								     && (y >= SYNAPTICS_TM1771_##KEYNAME##_Y - KEYPAD_DIAMETER / 2) \
        	                         && (y <= SYNAPTICS_TM1771_##KEYNAME##_Y + KEYPAD_DIAMETER / 2))

#define TOUCH_HOME		(1 << 0)
#define TOUCH_MENU		(1 << 1)
#define TOUCH_BACK		(1 << 2)
#define TOUCH_PEN		(1 << 3)

static int touch_state = 0;
static unsigned char g_tm1771_dect_flag = 0;
/* define in platform/board file(s) */
#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_rmi4_early_suspend(struct early_suspend *h);
static void synaptics_rmi4_late_resume(struct early_suspend *h);
#endif
static int synaptics_rmi4_attn_clear(struct synaptics_rmi4 *ts);
static ssize_t cap_touchscreen_attr_show(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf)
 {
 	  return sprintf(buf, "%d", g_tm1771_dect_flag);		  
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

static void ts_update_pen_state(struct synaptics_rmi4 *ts, int x, int y, int pressure, int wx, int wy)
{

	if (pressure) {
#ifdef CONFIG_SYNA_MOUSE
		input_report_abs(ts->input_dev, ABS_X, x);
		input_report_abs(ts->input_dev, ABS_Y, y);
		input_report_abs(ts->input_dev, ABS_PRESSURE, pressure);
		input_report_key(ts->input_dev, BTN_TOUCH, !!pressure);
#endif

#ifdef CONFIG_SYNA_MT
		input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
        input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, max(wx, wy));
        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MINOR, min(wx, wy));
        input_report_abs(ts->input_dev, ABS_MT_ORIENTATION, (wx > wy ? 1 : 0));
        input_mt_sync(ts->input_dev);
#endif
	} else {
#ifdef CONFIG_SYNA_MOUSE		
		input_report_abs(ts->input_dev, ABS_PRESSURE, 0);
		input_report_key(ts->input_dev, BTN_TOUCH, 0);
#endif

#ifdef CONFIG_SYNA_MT
        input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
		input_mt_sync(ts->input_dev);
#endif

	}
	/* end: added by z00168965 for multi-touch */
}

static void ts_timer(unsigned long arg)
{
	struct synaptics_rmi4 *ts = (struct synaptics_rmi4 *)arg;

    if (touch_state & TOUCH_PEN) {
        ts_update_pen_state(ts, 0, 0, 0, 0, 0);
        touch_state &= ~TOUCH_PEN;
	} 
}

static void tskey_timer(unsigned long arg)
{
	struct synaptics_rmi4 *ts = (struct synaptics_rmi4 *)arg;
       
    if (touch_state & TOUCH_HOME) {
        input_report_key(ts->input_dev, KEY_HOME, 0);
        touch_state &= ~TOUCH_HOME;
	} 

    if (touch_state & TOUCH_MENU) {
        input_report_key(ts->input_dev, KEY_MENU, 0);
        touch_state &= ~TOUCH_MENU;
	}

    if (touch_state & TOUCH_BACK) {
        input_report_key(ts->input_dev, KEY_BACK, 0);
        touch_state &= ~TOUCH_BACK;
	}
}


static int synaptics_rmi4_read_pdt(struct synaptics_rmi4 *ts)
{
	int ret = 0;
	int nFd = 0;
	int interruptCount = 0;
	__u8 data_length;

	struct i2c_msg fd_i2c_msg[2];
	__u8 fd_reg;
	struct synaptics_function_descriptor fd;

	struct i2c_msg query_i2c_msg[2];
	__u8 query[14];
	__u8 *egr;

	fd_i2c_msg[0].addr = ts->client->addr;
	fd_i2c_msg[0].flags = 0;
	fd_i2c_msg[0].buf = &fd_reg;
	fd_i2c_msg[0].len = 1;

	fd_i2c_msg[1].addr = ts->client->addr;
	fd_i2c_msg[1].flags = I2C_M_RD;
	fd_i2c_msg[1].buf = (__u8 *)(&fd);
	fd_i2c_msg[1].len = FD_BYTE_COUNT;

	query_i2c_msg[0].addr = ts->client->addr;
	query_i2c_msg[0].flags = 0;
	query_i2c_msg[0].buf = &fd.queryBase;
	query_i2c_msg[0].len = 1;

	query_i2c_msg[1].addr = ts->client->addr;
	query_i2c_msg[1].flags = I2C_M_RD;
	query_i2c_msg[1].buf = query;
	query_i2c_msg[1].len = sizeof(query);


	ts->hasF11 = false;
	ts->hasF19 = false;
	ts->hasF30 = false;
	ts->data_reg = 0xff;
	ts->data_length = 0;

	for (fd_reg = FD_ADDR_MAX; fd_reg >= FD_ADDR_MIN; fd_reg -= FD_BYTE_COUNT) {
		ret = i2c_transfer(ts->client->adapter, fd_i2c_msg, 2);
		if (ret < 0) {
			printk(KERN_ERR "I2C read failed querying RMI4 $%02X capabilities\n", ts->client->addr);
			return ret;
		}

		if (!fd.functionNumber) {
			/* End of PDT */
			ret = nFd;
			printk("Read %d functions from PDT\n", nFd);
			break;
		}

		++nFd;

		switch (fd.functionNumber) {
			case 0x01: /* Interrupt */
				ts->f01.data_offset = fd.dataBase;
				/*
				 * Can't determine data_length
				 * until whole PDT has been read to count interrupt sources
				 * and calculate number of interrupt status registers.
				 * Setting to 0 safely "ignores" for now.
				 */
				data_length = 0;
				break;
			case 0x11: /* 2D */
				ts->hasF11 = true;

				ts->f11.data_offset = fd.dataBase;
				ts->f11.interrupt_offset = interruptCount / 8;
				ts->f11.interrupt_mask = ((1 << INTERRUPT_SOURCE_COUNT(fd.intSrc)) - 1) << (interruptCount % 8);

				ret = i2c_transfer(ts->client->adapter, query_i2c_msg, 2);
				if (ret < 0)
					printk(KERN_ERR "Error reading F11 query registers\n");

				ts->f11.points_supported = (query[1] & 7) + 1;
				if (ts->f11.points_supported == 6)
					ts->f11.points_supported = 10;

				ts->f11_fingers = kcalloc(ts->f11.points_supported,
				                          sizeof(*ts->f11_fingers), 0);

				ts->f11_has_gestures = (query[1] >> 5) & 1;
				ts->f11_has_relative = (query[1] >> 3) & 1;

				egr = &query[7];

#define EGR_DEBUG
#ifdef EGR_DEBUG
#define EGR_INFO printk
#else
#define EGR_INFO
#endif
				EGR_INFO("EGR features:\n");
				ts->hasEgrPinch = egr[EGR_PINCH_REG] & EGR_PINCH;
				EGR_INFO("\tpinch: %u\n", ts->hasEgrPinch);
				ts->hasEgrPress = egr[EGR_PRESS_REG] & EGR_PRESS;
				EGR_INFO("\tpress: %u\n", ts->hasEgrPress);
				ts->hasEgrFlick = egr[EGR_FLICK_REG] & EGR_FLICK;
				EGR_INFO("\tflick: %u\n", ts->hasEgrFlick);
				ts->hasEgrEarlyTap = egr[EGR_EARLY_TAP_REG] & EGR_EARLY_TAP;
				EGR_INFO("\tearly tap: %u\n", ts->hasEgrEarlyTap);
				ts->hasEgrDoubleTap = egr[EGR_DOUBLE_TAP_REG] & EGR_DOUBLE_TAP;
				EGR_INFO("\tdouble tap: %u\n", ts->hasEgrDoubleTap);
				ts->hasEgrTapAndHold = egr[EGR_TAP_AND_HOLD_REG] & EGR_TAP_AND_HOLD;
				EGR_INFO("\ttap and hold: %u\n", ts->hasEgrTapAndHold);
				ts->hasEgrSingleTap = egr[EGR_SINGLE_TAP_REG] & EGR_SINGLE_TAP;
				EGR_INFO("\tsingle tap: %u\n", ts->hasEgrSingleTap);
				ts->hasEgrPalmDetect = egr[EGR_PALM_DETECT_REG] & EGR_PALM_DETECT;
				EGR_INFO("\tpalm detect: %u\n", ts->hasEgrPalmDetect);


				query_i2c_msg[0].buf = &fd.controlBase;
				ret = i2c_transfer(ts->client->adapter, query_i2c_msg, 2);
				if (ret < 0)
					printk(KERN_ERR "Error reading F11 control registers\n");

				query_i2c_msg[0].buf = &fd.queryBase;

				ts->f11_max_x = ((query[7] & 0x0f) * 0x100) | query[6];
				ts->f11_max_y = ((query[9] & 0x0f) * 0x100) | query[8];

				printk("max X: %d; max Y: %d\n", ts->f11_max_x, ts->f11_max_y);

				ts->f11.data_length = data_length =
					/* finger status, four fingers per register */
					((ts->f11.points_supported + 3) / 4)
					/* absolute data, 5 per finger */
					+ 5 * ts->f11.points_supported
					/* two relative registers */
					+ (ts->f11_has_relative ? 2 : 0)
					/* F11_2D_Data8 is only present if the egr_0 register is non-zero. */
					+ (egr[0] ? 1 : 0)
					/* F11_2D_Data9 is only present if either egr_0 or egr_1 registers are non-zero. */
					+ ((egr[0] || egr[1]) ? 1 : 0)
					/* F11_2D_Data10 is only present if EGR_PINCH or EGR_FLICK of egr_0 reports as 1. */
					+ ((ts->hasEgrPinch || ts->hasEgrFlick) ? 1 : 0)
					/* F11_2D_Data11 and F11_2D_Data12 are only present if EGR_FLICK of egr_0 reports as 1. */
					+ (ts->hasEgrFlick ? 2 : 0)
					;

				break;
			case 0x19: /* Cap Buttons */
				ts->hasF19 = true;

				ts->f19.data_offset = fd.dataBase;
				ts->f19.interrupt_offset = interruptCount / 8;
				ts->f19.interrupt_mask = ((1 < INTERRUPT_SOURCE_COUNT(fd.intSrc)) - 1) << (interruptCount % 8);

				ret = i2c_transfer(ts->client->adapter, query_i2c_msg, 2);
				if (ret < 0)
					printk(KERN_ERR "Error reading F19 query registers\n");


				ts->f19.points_supported = query[1] & 0x1F;
				ts->f19.data_length = data_length = (ts->f19.points_supported + 7) / 8;

				printk(KERN_NOTICE "$%02X F19 has %d buttons\n", ts->client->addr, ts->f19.points_supported);

				break;
			case 0x30: /* GPIO */
				ts->hasF30 = true;

				ts->f30.data_offset = fd.dataBase;
				ts->f30.interrupt_offset = interruptCount / 8;
				ts->f30.interrupt_mask = ((1 < INTERRUPT_SOURCE_COUNT(fd.intSrc)) - 1) << (interruptCount % 8);

				ret = i2c_transfer(ts->client->adapter, query_i2c_msg, 2);
				if (ret < 0)
					printk(KERN_ERR "Error reading F30 query registers\n");


				ts->f30.points_supported = query[1] & 0x1F;
				ts->f30.data_length = data_length = (ts->f30.points_supported + 7) / 8;

				break;
			default:
				goto pdt_next_iter;
		}

		// Change to end address for comparison
		// NOTE: make sure final value of ts->data_reg is subtracted
		data_length += fd.dataBase;
		if (data_length > ts->data_length) {
			ts->data_length = data_length;
		}

		if (fd.dataBase < ts->data_reg) {
			ts->data_reg = fd.dataBase;
		}

pdt_next_iter:
		interruptCount += INTERRUPT_SOURCE_COUNT(fd.intSrc);
	}

	// Now that PDT has been read, interrupt count determined, F01 data length can be determined.
	ts->f01.data_length = data_length = 1 + ((interruptCount + 7) / 8);
	// Change to end address for comparison
	// NOTE: make sure final value of ts->data_reg is subtracted
	data_length += ts->f01.data_offset;
	if (data_length > ts->data_length) {
		ts->data_length = data_length;
	}

	// Change data_length back from end address to length
	// NOTE: make sure this was an address
	ts->data_length -= ts->data_reg;

	// Change all data offsets to be relative to first register read
	// TODO: add __u8 *data (= &ts->data[ts->f##.data_offset]) to struct rmi_function_info?
	ts->f01.data_offset -= ts->data_reg;
	ts->f11.data_offset -= ts->data_reg;
	ts->f19.data_offset -= ts->data_reg;
	ts->f30.data_offset -= ts->data_reg;

	ts->data = kcalloc(ts->data_length, sizeof(*ts->data), 0);
	if (ts->data == NULL) {
		printk(KERN_ERR "Not enough memory to allocate space for RMI4 data\n");
		ret = -ENOMEM;
	}

	ts->data_i2c_msg[0].addr = ts->client->addr;
	ts->data_i2c_msg[0].flags = 0;
	ts->data_i2c_msg[0].len = 1;
	ts->data_i2c_msg[0].buf = &ts->data_reg;

	ts->data_i2c_msg[1].addr = ts->client->addr;
	ts->data_i2c_msg[1].flags = I2C_M_RD;
	ts->data_i2c_msg[1].len = ts->data_length;
	ts->data_i2c_msg[1].buf = ts->data;

	printk(KERN_ERR "RMI4 $%02X data read: $%02X + %d\n",
		ts->client->addr, ts->data_reg, ts->data_length);

	return ret;
}

#ifdef CONFIG_SYNA_BUTTONS_SCROLL
static int first_button(__u8 *button_data, __u8 points_supported)
{
	int b, reg;

	for (reg = 0; reg < ((points_supported + 7) / 8); reg++)
		for (b = 0; b < 8; b++)
			if ((button_data[reg] >> b) & 1)
				return reg * 8 + b;

	return -1;
}

static void synaptics_report_scroll(struct input_dev *dev,
                                    __u8 *button_data,
                                    __u8 points_supported,
                                    int ev_code)
{
	int scroll = 0;
	static int last_button = -1, current_button = -1;

	// This method is slightly problematic
	// It makes no check to find if lift/touch is more likely than slide
	current_button = first_button(button_data, points_supported);

	if (current_button >= 0 && last_button >= 0) {
		scroll = current_button - last_button;
		// This filter mostly works to isolate slide motions from lift/touch
		if (abs(scroll) == 1) {
			input_report_rel(dev, ev_code, scroll);
		}
	}

	last_button = current_button;
}
#endif

static void synaptics_report_buttons(struct input_dev *dev,
                                     __u8 *data,
                                     __u8 points_supported,
                                     int base)
{
	int b;

	for (b = 0; b < points_supported; ++b) {
		int button = (data[b / 8] >> (b % 8)) & 1;
		input_report_key(dev, base + b, button);
	}
}

static void synaptics_rmi4_work_func(struct work_struct *work)
{
	int ret;
	u32 z = 0;
	u32 wx = 0, wy = 0;
	u32 x = 0, y = 0;

	struct synaptics_rmi4 *ts = container_of(work,
											 struct synaptics_rmi4, work);
	ret = i2c_transfer(ts->client->adapter, ts->data_i2c_msg, 2);

	if (ret < 0) {
		printk(KERN_ERR "%s: i2c_transfer failed\n", __func__);
	} else {
		__u8 *interrupt = &ts->data[ts->f01.data_offset + 1];
		if (ts->hasF11 && interrupt[ts->f11.interrupt_offset] & ts->f11.interrupt_mask) {
			__u8 *f11_data = &ts->data[ts->f11.data_offset];
			int f;
			__u8 finger_status_reg = 0;
			__u8 fsr_len = (ts->f11.points_supported + 3) / 4;
           

			for (f = 0; f < ts->f11.points_supported; ++f) {
				__u8 finger_status;

				if (!(f % 4))
					finger_status_reg = f11_data[f / 4];

				finger_status = (finger_status_reg >> ((f % 4) * 2)) & 3;
               
				if (finger_status == 1 || finger_status == 2) {
					__u8 reg = fsr_len + 5 * f;
					__u8 *finger_reg = &f11_data[reg];

					x = (finger_reg[0] * 0x10) | (finger_reg[2] % 0x10);
					y = (finger_reg[1] * 0x10) | (finger_reg[2] / 0x10);
					wx = finger_reg[3] % 0x10;
					wy = finger_reg[3] / 0x10;
					z = finger_reg[4];

	                if (KEYPAD_AREA(x, y, HOME)) {
		                input_report_key(ts->input_dev, KEY_HOME, 1);           
		                touch_state |= TOUCH_HOME;
                        mod_timer(&ts->timerlistkey, jiffies + msecs_to_jiffies(TS_KEYUP_TIMEOUT_MS));
	                } else if (KEYPAD_AREA(x, y, MENU)) {
						input_report_key(ts->input_dev, KEY_MENU, 1);           
		                touch_state |= TOUCH_MENU;
                        mod_timer(&ts->timerlistkey, jiffies + msecs_to_jiffies(TS_KEYUP_TIMEOUT_MS));
					} else if (KEYPAD_AREA(x, y, BACK)) {
						input_report_key(ts->input_dev, KEY_BACK, 1);           
		                touch_state |= TOUCH_BACK;
                        mod_timer(&ts->timerlistkey, jiffies + msecs_to_jiffies(TS_KEYUP_TIMEOUT_MS));						
					} else {
                        ts_update_pen_state(ts, x, y, z, wx, wy);
                        mod_timer(&ts->timerlist, jiffies + msecs_to_jiffies(TS_PENUP_TIMEOUT_MS));
						touch_state |= TOUCH_PEN;

#ifdef CONFIG_SYNA_MULTIFINGER
						/* Report multiple fingers for software prior to 2.6.31 - not standard - uses special input.h */
						input_report_abs(ts->input_dev, ABS_X_FINGER(f), x);
						input_report_abs(ts->input_dev, ABS_Y_FINGER(f), y);
						input_report_abs(ts->input_dev, ABS_Z_FINGER(f), z);
#endif

						ts->f11_fingers[f].status = finger_status;
					}
            	}
            }
                
			/* f == ts->f11.points_supported */
			/* set f to offset after all absolute data */
			f = (f + 3) / 4 + f * 5;
			if (ts->f11_has_relative) {
				/* NOTE: not reporting relative data, even if available */
				/* just skipping over relative data registers */
				f += 2;
			}

            if (ts->hasEgrPalmDetect) {
                         	input_report_key(ts->input_dev,
				                 BTN_DEAD,
				                 f11_data[f + EGR_PALM_DETECT_REG] & EGR_PALM_DETECT);
			}

            if (ts->hasEgrFlick) {
                         	if (f11_data[f + EGR_FLICK_REG] & EGR_FLICK) {
					input_report_rel(ts->input_dev, REL_X, f11_data[f + 2]);
					input_report_rel(ts->input_dev, REL_Y, f11_data[f + 3]);
				}
			}

            if (ts->hasEgrSingleTap) {
				input_report_key(ts->input_dev,
				                 BTN_TOUCH,
				                 f11_data[f + EGR_SINGLE_TAP_REG] & EGR_SINGLE_TAP);
			}

            if (ts->hasEgrDoubleTap) {
				input_report_key(ts->input_dev,
				                 BTN_TOOL_DOUBLETAP,
				                 f11_data[f + EGR_DOUBLE_TAP_REG] & EGR_DOUBLE_TAP);
			}
		}

		if (ts->hasF19 && interrupt[ts->f19.interrupt_offset] & ts->f19.interrupt_mask) {
			int reg;
			int touch = 0;
			for (reg = 0; reg < ((ts->f19.points_supported + 7) / 8); reg++) {
				if (ts->data[ts->f19.data_offset + reg]) {
					touch = 1;
					break;
				}
            }
			input_report_key(ts->input_dev, BTN_DEAD, touch);

#ifdef  CONFIG_SYNA_BUTTONS
			synaptics_report_buttons(ts->input_dev,
			                         &ts->data[ts->f19.data_offset],
                                                 ts->f19.points_supported, BTN_F19);
#endif

#ifdef  CONFIG_SYNA_BUTTONS_SCROLL
			synaptics_report_scroll(ts->input_dev,
			                        &ts->data[ts->f19.data_offset],
			                        ts->f19.points_supported,
			                        SCROLL_ORIENTATION);
#endif
		}

		if (ts->hasF30 && interrupt[ts->f30.interrupt_offset] & ts->f30.interrupt_mask) {
			synaptics_report_buttons(ts->input_dev,
			                         &ts->data[ts->f30.data_offset],
		                                 ts->f30.points_supported, BTN_F30);
		}
		input_sync(ts->input_dev);
	}

	if (ts->use_irq){
		enable_irq(ts->client->irq);
		synaptics_rmi4_attn_clear(ts);
    }
}

static enum hrtimer_restart synaptics_rmi4_timer_func(struct hrtimer *timer)
{
	struct synaptics_rmi4 *ts = container_of(timer, \
					struct synaptics_rmi4, timer);

	queue_work(synaptics_wq, &ts->work);

	hrtimer_start(&ts->timer, ktime_set(0, 12 * NSEC_PER_MSEC), HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}

irqreturn_t synaptics_rmi4_irq_handler(int irq, void *dev_id)
{
	struct synaptics_rmi4 *ts = dev_id;

	disable_irq_nosync(ts->client->irq);
	queue_work(synaptics_wq, &ts->work);

	return IRQ_HANDLED;
}

static void synaptics_rmi4_enable(struct synaptics_rmi4 *ts)
{
	if (ts->use_irq)
		enable_irq(ts->client->irq);
	else
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

	ts->enable = 1;
}

static void synaptics_rmi4_disable(struct synaptics_rmi4 *ts)
{
	if (ts->use_irq)
		disable_irq_nosync(ts->client->irq);
	else
		hrtimer_cancel(&ts->timer);

	cancel_work_sync(&ts->work);

	ts->enable = 0;
}

static ssize_t synaptics_rmi4_enable_show(struct device *dev,
                                         struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4 *ts = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", ts->enable);
}

static ssize_t synaptics_rmi4_enable_store(struct device *dev,
                                          struct device_attribute *attr,
                                          const char *buf, size_t count)
{
	struct synaptics_rmi4 *ts = dev_get_drvdata(dev);
	unsigned long val;
	int error;

	error = strict_strtoul(buf, 10, &val);

	if (error)
		return error;

	val = !!val;

	if (val != ts->enable) {
		if (val)
			synaptics_rmi4_enable(ts);
		else
			synaptics_rmi4_disable(ts);
	}

	return count;
}

static int synaptics_rmi4_gpio_request(void)
{
	int rc = 0;

	struct msm_gpio synaptics_dect_gpio = { GPIO_CFG(SYNAPTICS_TM1771_INT_GPIO, 0, GPIO_INPUT, 
	        GPIO_PULL_UP, GPIO_2MA),"synaptics_dect_irq" };
	rc = msm_gpios_request_enable(&synaptics_dect_gpio,1);
    	if (rc) 
	{
		msm_gpios_disable_free(&synaptics_dect_gpio,1);
        	gpio_free(SYNAPTICS_TM1771_INT_GPIO);
	       printk(KERN_ERR "request interrupt GPIO failed,err code %d\n",rc);
		return rc;
	}

	rc = gpio_request(SYNAPTICS_TM1771_RESET_GPIO, " synaptics_reset_en");
	if (rc) 
	{
	    msm_gpios_disable_free(&synaptics_dect_gpio,1);
	    printk(KERN_ERR "reset gpio request failed,err code %d\n",rc);
	    return rc;
	}

	rc = gpio_tlmm_config(GPIO_CFG(SYNAPTICS_TM1771_RESET_GPIO, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA), GPIO_ENABLE);
	if(rc < 0)
	{
	    printk(KERN_ERR "reset gpio config failed,err code %d\n",rc);
	    gpio_free(SYNAPTICS_TM1771_RESET_GPIO);
	    msm_gpios_disable_free(&synaptics_dect_gpio,1);
	    return rc;
	}
	return rc;
}

static void synaptics_rmi4_gpio_free(void)
{
	struct msm_gpio synaptics_dect_gpio = { GPIO_CFG(SYNAPTICS_TM1771_INT_GPIO, 0, GPIO_INPUT, 
			GPIO_PULL_UP, GPIO_2MA),"synaptics_dect_irq_test" };
	msm_gpios_disable_free(&synaptics_dect_gpio,1);
	gpio_free(SYNAPTICS_TM1771_RESET_GPIO);
}

static int synaptics_rmi4_attn_clear(struct synaptics_rmi4 *ts)
{
	int ret = 0;
    	__u8 attn_reg = 0x14;
       __u8 data = 0x00;
       struct i2c_msg fd_i2c_msg[2];

	fd_i2c_msg[0].addr = ts->client->addr;
	fd_i2c_msg[0].flags = 0;
	fd_i2c_msg[0].buf = &attn_reg;
	fd_i2c_msg[0].len = 1;

	fd_i2c_msg[1].addr = ts->client->addr;
	fd_i2c_msg[1].flags = I2C_M_RD;
	fd_i2c_msg[1].buf = (__u8 *)(&data);
	fd_i2c_msg[1].len = 1;

	ret = i2c_transfer(ts->client->adapter, fd_i2c_msg, 2);
	if (ret < 0) {
	    printk(KERN_ERR "synaptics_rmi4_attn_clear: init attn fail\n");
	    return 0;
	}
	return 1;
}

static int synaptics_ts_gpio_reset(void)
{
	gpio_set_value(SYNAPTICS_TM1771_RESET_GPIO, 1);
    mdelay(5);
	gpio_set_value(SYNAPTICS_TM1771_RESET_GPIO, 0);
	mdelay(5);
	gpio_set_value(SYNAPTICS_TM1771_RESET_GPIO, 1);
   	mdelay(5);     
	return 0;
}

static struct device_attribute synaptics_rmi4_dev_attr_enable = {
	   .attr = {.name = "synaptics_rmi4_dev_attr", .mode = 0664},
       .show = synaptics_rmi4_enable_show,
       .store = synaptics_rmi4_enable_store
};

static int synaptics_rmi4_probe(struct i2c_client *client, 
    							const struct i2c_device_id *id)
{
	int i;
	int ret = 0;

	struct synaptics_rmi4 *ts;
    struct kobject *kobj = NULL;
	printk(KERN_ERR "Synaptics RMI4 device %s at $%02X...\n", client->name, client->addr);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "%s: need I2C_FUNC_I2C\n", __func__);
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = (struct synaptics_rmi4 *)client->dev.platform_data;
	INIT_WORK(&ts->work, synaptics_rmi4_work_func);
	ts->client = client;
	i2c_set_clientdata(client, ts);

	ret = synaptics_rmi4_gpio_request();
	if (ret) {
		printk(KERN_ERR "synaptics request gpio fail\n");
       	goto err_gpio_request_failed;
    } else 
    	printk(KERN_INFO "synaptics request gpio SUCCESS!\n");

	synaptics_ts_gpio_reset();
    mdelay(50); 

	ret = synaptics_rmi4_attn_clear(ts);
	if (!ret) 
		goto err_pdt_read_failed;

	ret = synaptics_rmi4_read_pdt(ts);
	if (ret <= 0) {
		if (ret == 0)
			printk(KERN_ERR "Empty PDT\n");

		printk(KERN_ERR "Error identifying device (%d)\n", ret);
		ret = -ENODEV;
		goto err_pdt_read_failed;
	}
	ts->input_dev = input_allocate_device();
	if (!ts->input_dev) {
		printk(KERN_ERR "failed to allocate input device.\n");
		ret = -EBUSY;
		goto err_alloc_dev_failed;
	}

    /* convert touchscreen coordinator to coincide with LCD coordinator */
    /* get tsp at (0,0) */
	ts->input_dev->name = "Synaptics RMI4";
	ts->input_dev->phys = client->name;

	set_bit(EV_ABS, ts->input_dev->evbit);
//	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);

	set_bit(ABS_X, ts->input_dev->absbit);
    set_bit(ABS_Y, ts->input_dev->absbit);
    set_bit(ABS_PRESSURE, ts->input_dev->absbit);

   	set_bit(KEY_HOME, ts->input_dev->keybit);
	set_bit(KEY_MENU, ts->input_dev->keybit);
   	set_bit(KEY_BACK, ts->input_dev->keybit);

	ts->x_max = TOUCH_LCD_X_MAX;
	ts->y_max = TOUCH_LCD_Y_MAX;
	if (ts->hasF11) {
		for (i = 0; i < ts->f11.points_supported; ++i) {
#ifdef CONFIG_SYNA_MOUSE
			/* old standard touchscreen for single-finger software */
			input_set_abs_params(ts->input_dev, ABS_X, 0, ts->x_max, 0, 0);
			input_set_abs_params(ts->input_dev, ABS_Y, 0, ts->y_max, 0, 0);
			input_set_abs_params(ts->input_dev, ABS_PRESSURE, 0, 0xFF, 0, 0);
			input_set_abs_params(ts->input_dev, ABS_TOOL_WIDTH, 0, 0xF, 0, 0);
			set_bit(BTN_TOUCH, ts->input_dev->keybit);
#endif

#ifdef CONFIG_SYNA_MT
			/* Linux 2.6.31 multi-touch */
			input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 1,
								 ts->f11.points_supported, 0, 0);

			/* begin: added by z00168965 for multi-touch */
			input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->x_max, 0, 0);
			input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->y_max, 0, 0);
			/* end: added by z00168965 for multi-touch */

            input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 0xF, 0, 0);
			input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MINOR, 0, 0xF, 0, 0);
			input_set_abs_params(ts->input_dev, ABS_MT_ORIENTATION, 0, 1, 0, 0);
#endif

#ifdef CONFIG_SYNA_MULTIFINGER
			/*  multiple fingers for software built prior to 2.6.31 - uses non-standard input.h file. */
			input_set_abs_params(ts->input_dev, ABS_X_FINGER(i), 0, ts->f11_max_x, 0, 0);
			input_set_abs_params(ts->input_dev, ABS_Y_FINGER(i), 0, ts->f11_max_y, 0, 0);
			input_set_abs_params(ts->input_dev, ABS_Z_FINGER(i), 0, 0xFF, 0, 0);
#endif
		}
        
		if (ts->hasEgrPalmDetect)
			set_bit(BTN_DEAD, ts->input_dev->keybit);
		if (ts->hasEgrFlick) {
			set_bit(REL_X, ts->input_dev->keybit);
			set_bit(REL_Y, ts->input_dev->keybit);
		}
        
		if (ts->hasEgrSingleTap)
			set_bit(BTN_TOUCH, ts->input_dev->keybit);
		if (ts->hasEgrDoubleTap)
			set_bit(BTN_TOOL_DOUBLETAP, ts->input_dev->keybit);
	}
    
	if (ts->hasF19) {
		set_bit(BTN_DEAD, ts->input_dev->keybit);
#ifdef CONFIG_SYNA_BUTTONS
		/* F19 does not (currently) report ABS_X but setting maximum X is a convenient way to indicate number of buttons */
		input_set_abs_params(ts->input_dev, ABS_X, 0, ts->f19.points_supported, 0, 0);
		for (i = 0; i < ts->f19.points_supported; ++i) {
			set_bit(BTN_F19 + i, ts->input_dev->keybit);
		}
#endif

#ifdef CONFIG_SYNA_BUTTONS_SCROLL
		set_bit(EV_REL, ts->input_dev->evbit);
		set_bit(SCROLL_ORIENTATION, ts->input_dev->relbit);
#endif
	}
    
	if (ts->hasF30) {
		for (i = 0; i < ts->f30.points_supported; ++i) {
			set_bit(BTN_F30 + i, ts->input_dev->keybit);
		}
	}

	if (client->irq) {
		gpio_request(client->irq, client->name);
		gpio_direction_input(client->irq);

		printk(KERN_INFO "Requesting IRQ...\n");
		ret = request_irq(client->irq, synaptics_rmi4_irq_handler,
				IRQF_TRIGGER_LOW,  client->name, ts);

		if(ret) {
			printk(KERN_ERR "Failed to request IRQ!ret = %d\n", ret);          		
		}else {
			printk(KERN_INFO "Set IRQ Success!\n");
            		ts->use_irq = 1;
		}

	}

	if (!ts->use_irq) {
		printk(KERN_ERR "Synaptics RMI4 device %s in polling mode\n", client->name);
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = synaptics_rmi4_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}

	/*
	 * Device will be /dev/input/event#
	 * For named device files, use udev
	 */
	ret = input_register_device(ts->input_dev);
	if (ret) {
		printk(KERN_ERR "synaptics_rmi4_probe: Unable to register %s"
				"input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	} else {
		printk("synaptics input device registered\n");
	}

	setup_timer(&ts->timerlist, ts_timer, (unsigned long)ts);
    setup_timer(&ts->timerlistkey, tskey_timer, (unsigned long)ts);

	ts->enable = 1;
	dev_set_drvdata(&ts->input_dev->dev, ts);

	if (sysfs_create_file(&ts->input_dev->dev.kobj, &synaptics_rmi4_dev_attr_enable.attr) < 0)
		printk("failed to create sysfs file for input device\n");

	#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = synaptics_rmi4_early_suspend;
	ts->early_suspend.resume = synaptics_rmi4_late_resume;
	#endif
    kobj = kobject_create_and_add("cap_touchscreen", NULL);
  	if (kobj == NULL) {	
		printk(KERN_ERR "kobject_create_and_add error\n" );
		goto err_input_register_device_failed;
	}
  	if (sysfs_create_group(kobj, &cap_touchscreen_defattr_group)) {
		kobject_put(kobj);
		printk(KERN_ERR "sysfs_create_group error\n" );
		goto err_input_register_device_failed;
	}

    g_tm1771_dect_flag = 1;

	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_alloc_dev_failed:
err_pdt_read_failed:
err_gpio_request_failed:
err_check_functionality_failed:
	return ret;
}


static int synaptics_rmi4_remove(struct i2c_client *client)
{
struct synaptics_rmi4 *ts = i2c_get_clientdata(client);
	unregister_early_suspend(&ts->early_suspend);
	if (ts->use_irq)
		free_irq(client->irq, ts);
	else
		hrtimer_cancel(&ts->timer);
	input_unregister_device(ts->input_dev);
	kfree(ts);
    	synaptics_rmi4_gpio_free();
	return 0;
}

static int synaptics_rmi4_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct synaptics_rmi4 *ts = i2c_get_clientdata(client);

	if (!ts->use_irq)
		hrtimer_cancel(&ts->timer);

	cancel_work_sync(&ts->work);

	ts->enable = 0;

	return 0;
}

static int synaptics_rmi4_resume(struct i2c_client *client)
{
	struct synaptics_rmi4 *ts = i2c_get_clientdata(client);

	synaptics_rmi4_enable(ts);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_rmi4_early_suspend(struct early_suspend *h)
{
	struct synaptics_rmi4 *ts;
	ts = container_of(h, struct synaptics_rmi4, early_suspend);
	synaptics_rmi4_suspend(ts->client, PMSG_SUSPEND);
}

static void synaptics_rmi4_late_resume(struct early_suspend *h)
{
	struct synaptics_rmi4 *ts;
	ts = container_of(h, struct synaptics_rmi4, early_suspend);
	synaptics_rmi4_resume(ts->client);
}
#endif

static struct i2c_device_id synaptics_rmi4_id[]={
	{"synaptics-rmi4",0},
	{},	
};

static struct i2c_driver synaptics_rmi4_driver = {
	.probe		= synaptics_rmi4_probe,
	.remove		= synaptics_rmi4_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= synaptics_rmi4_suspend,
	.resume		= synaptics_rmi4_resume,
#endif
	.id_table	= synaptics_rmi4_id,
	.driver = {
		.name	= "synaptics-rmi4",
	},
};

static int __devinit synaptics_rmi4_init(void)
{
	synaptics_wq = create_singlethread_workqueue("synaptics_wq");
	if (!synaptics_wq) {
		printk(KERN_ERR "Could not create work queue synaptics_wq: no memory");
		return -ENOMEM;
	}

	return i2c_add_driver(&synaptics_rmi4_driver);
}

static void __exit synaptics_rmi4_exit(void)
{
	i2c_del_driver(&synaptics_rmi4_driver);

	if (synaptics_wq)
		destroy_workqueue(synaptics_wq);
}

module_init(synaptics_rmi4_init);
module_exit(synaptics_rmi4_exit);

MODULE_DESCRIPTION("Synaptics RMI4 Driver");
MODULE_LICENSE("GPL");
