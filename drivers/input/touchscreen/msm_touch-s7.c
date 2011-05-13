/* drivers/input/touchscreen/msm_touch.c
 *
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/io.h>

#include <mach/msm_touch.h>

#include <mach/board.h>

/* HW register map */
#define TSSC_CTL_REG      0x100
#define TSSC_SI_REG       0x108
#define TSSC_OPN_REG      0x104
#define TSSC_STATUS_REG   0x10C
#define TSSC_AVG12_REG    0x110

/* status bits */
#define TSSC_STS_OPN_SHIFT 0x6
#define TSSC_STS_OPN_BMSK  0x1C0
#define TSSC_STS_NUMSAMP_SHFT 0x1
#define TSSC_STS_NUMSAMP_BMSK 0x3E

/* CTL bits */
#define TSSC_CTL_EN		(0x1 << 0)
#define TSSC_CTL_SW_RESET	(0x1 << 2)
#define TSSC_CTL_MASTER_MODE	(0x3 << 3)
#define TSSC_CTL_AVG_EN		(0x1 << 5)
#define TSSC_CTL_DEB_EN		(0x1 << 6)
#define TSSC_CTL_DEB_12_MS	(0x2 << 7)	/* 1.2 ms */
#define TSSC_CTL_DEB_16_MS	(0x3 << 7)	/* 1.6 ms */
#define TSSC_CTL_DEB_2_MS	(0x4 << 7)	/* 2 ms */
#define TSSC_CTL_DEB_3_MS	(0x5 << 7)	/* 3 ms */
#define TSSC_CTL_DEB_4_MS	(0x6 << 7)	/* 4 ms */
#define TSSC_CTL_DEB_6_MS	(0x7 << 7)	/* 6 ms */
#define TSSC_CTL_INTR_FLAG1	(0x1 << 10)
#define TSSC_CTL_DATA		(0x1 << 11)
#define TSSC_CTL_SSBI_CTRL_EN	(0x1 << 13)

/* control reg's default state */
#define TSSC_CTL_STATE	  ( \
		TSSC_CTL_DEB_12_MS | \
		TSSC_CTL_DEB_EN | \
		TSSC_CTL_AVG_EN | \
		TSSC_CTL_MASTER_MODE | \
		TSSC_CTL_EN)

#define TSSC_NUMBER_OF_OPERATIONS 2
// Panjinling(20100515): TS_PENUP_TIMEOUT_MS from 20 -> 50 ms.
#define TS_PENUP_TIMEOUT_MS 50

#define X_OFFSET	
#define Y_OFFSET	

#define TS_DRIVER_NAME "msm_touchscreen"

#define X_MAX	1024
#define Y_MAX	1024
#define P_MAX	256

struct ts {
	struct input_dev *input;
	struct timer_list timer;
	int irq;
	unsigned int x_max;
	unsigned int y_max;
};

/* Added by yinxuehui 2009-11-02 begin */
struct orig_t {
    int x;
    int y;
};

/* upper left point */
static struct orig_t tsp_ul_50x50 = {
    89, 
    856
};

/* lower right point */
static struct orig_t tsp_dr_50x50 = {
    925,
    136
};

static struct orig_t lcd_size = {
    800, 
    480
};

static struct orig_t tsp_ul_50x50_convert, tsp_dr_50x50_convert;
static struct orig_t tsp_origin_convert;

/* end of Added by yinxuehui 2009-11-02*/

static void __iomem *virt;
#define TSSC_REG(reg) (virt + TSSC_##reg##_REG)

static void ts_update_pen_state(struct ts *ts, int x, int y, int pressure)
{
	if (pressure) {
		input_report_abs(ts->input, ABS_X, x);
		input_report_abs(ts->input, ABS_Y, y);
		input_report_abs(ts->input, ABS_PRESSURE, pressure);
		input_report_key(ts->input, BTN_TOUCH, !!pressure);
	} else {
		input_report_abs(ts->input, ABS_PRESSURE, 0);
		input_report_key(ts->input, BTN_TOUCH, 0);
	}

	input_sync(ts->input);
}

static void ts_timer(unsigned long arg)
{
	struct ts *ts = (struct ts *)arg;

	ts_update_pen_state(ts, 0, 0, 0);
}

#define TS_SAMPLE_POINT_NUM	3
#define TS_POINT_DISDENCE	30
#define DIST(x, y) ((x) * (x) + (y) * (y))

// return 1 for submit point, return 0 for no submit
static int ts_filter(int x, int y, int* xOut, int* yOut) {
	static int refPoint[2] = {-1, -1};
	static int sampleMetrix[TS_SAMPLE_POINT_NUM][2];
	static int pointIndex = 0; 	// TS_SAMPLE_POINT_NUM for finish
	static int omit = 0; 		// omit point number
	int i = 0;

	// arguments check
	if (x < 0 || y < 0) {
		return 0;
	}
	
	// Initial
	if (refPoint[0] == -1 || refPoint[1] == -1) {
		refPoint[0] = x;
		refPoint[1] = y;
	}

	if (omit >= 3) {
		// reset status
		omit = 0;
		refPoint[0] = -1;
		refPoint[1] = -1;
		pointIndex = 0;
		return 0;
	}

	if (DIST(x - refPoint[0], y - refPoint[1]) > 
		(omit + 1) * DIST(TS_POINT_DISDENCE, TS_POINT_DISDENCE)) {
		omit++;
		return 0;
	} else {
		// record this point
		sampleMetrix[pointIndex][0] = x;
		sampleMetrix[pointIndex][1] = y;
		pointIndex++;
		refPoint[0] = x;
		refPoint[1] = y;
		omit = 0;
	}

	if (pointIndex >= TS_SAMPLE_POINT_NUM) {
		if (NULL == xOut || NULL == yOut) {
			return 0;
		}

		*xOut = 0;
		for (i = 0; i < TS_SAMPLE_POINT_NUM; i++) {
			*xOut += sampleMetrix[i][0];
		}
		*xOut = *xOut / TS_SAMPLE_POINT_NUM;

		*yOut = 0;
		for (i = 0; i < TS_SAMPLE_POINT_NUM; i++) {
			*yOut += sampleMetrix[i][1];
		}
		*yOut = *yOut / TS_SAMPLE_POINT_NUM;
		pointIndex = 0;
		return 1;
	} else {
		return 0;
	}
}
static irqreturn_t ts_interrupt(int irq, void *dev_id)
{
	u32 avgs, x, y, lx, ly;
	u32 num_op, num_samp;
	u32 status;

#if !defined(CONFIG_ANDROID_TOUCHSCREEN_MSM_HACKS)
    int sx, sy;
#endif

	struct ts *ts = dev_id;

	status = readl(TSSC_REG(STATUS));
	avgs = readl(TSSC_REG(AVG12));

#if !HUAWEI_HWID(S70)
	x = avgs & 0xFFFF;
	y = avgs >> 16;
#else
#if HWVERID_HIGHER(S70, T1)
	x = avgs & 0xFFFF;
	y = avgs >> 16;
#else
	x = avgs >> 16;
	y = avgs & 0xFFFF;
#endif
#endif

	/* For pen down make sure that the data just read is still valid.
	 * The DATA bit will still be set if the ARM9 hasn't clobbered
	 * the TSSC. If it's not set, then it doesn't need to be cleared
	 * here, so just return.
	 */
	if (!(readl(TSSC_REG(CTL)) & TSSC_CTL_DATA))
		goto out;

	/* Data has been read, OK to clear the data flag */
	writel(TSSC_CTL_STATE, TSSC_REG(CTL));

	/* Valid samples are indicated by the sample number in the status
	 * register being the number of expected samples and the number of
	 * samples collected being zero (this check is due to ADC contention).
	 */
	num_op = (status & TSSC_STS_OPN_BMSK) >> TSSC_STS_OPN_SHIFT;
	num_samp = (status & TSSC_STS_NUMSAMP_BMSK) >> TSSC_STS_NUMSAMP_SHFT;

	if ((num_op == TSSC_NUMBER_OF_OPERATIONS) && (num_samp == 0)) {
		/* TSSC can do Z axis measurment, but driver doesn't support
		 * this yet.
		 */

		/*
		 * REMOVE THIS:
		 * These x, y co-ordinates adjustments will be removed once
		 * Android framework adds calibration framework.
		 */
		/* added by h00131430 */
#if !HUAWEI_HWID(S70)
#ifdef CONFIG_ANDROID_TOUCHSCREEN_MSM_HACKS
        lx = 1024 -(ts->x_max + 25 - x);
        ly = (ts->y_max + 25 - y);
#else
        lx = x;
        ly = y;
#endif
#else
#ifdef CONFIG_ANDROID_TOUCHSCREEN_MSM_HACKS
#if HWVERID_HIGHER(S70, T1)
		lx = 1024 - (ts->x_max + 25 - x);
		ly = ts->y_max + 25 - y;
#else
		lx = ts->x_max + 25 - x;
		ly = 1024 - (ts->y_max + 25 - y);
#endif
#else
#if HWVERID_HIGHER(S70, T1)
		/* coincide with LCD coordinator */
		lx = (tsp_ul_50x50.x > tsp_dr_50x50.x) ? (ts->x_max - x) : x;
		ly = (tsp_ul_50x50.y > tsp_dr_50x50.y) ? (ts->y_max - y) : y;

		/* added by h00131430 2009-11-02 begin */
		sx = (lx - tsp_origin_convert.x) 
			 * (lcd_size.x - 100)
			 * ts->x_max 
			 	/ ((tsp_dr_50x50_convert.x - tsp_ul_50x50_convert.x) * lcd_size.x);
		sy = (ly - tsp_origin_convert.y) 
			 * (lcd_size.y - 100)
			 * ts->y_max 
			 	/ ((tsp_dr_50x50_convert.y - tsp_ul_50x50_convert.y) * lcd_size.y);

        if (sx < 0) {
            lx = 0;
        } else if (sx > ts->x_max) {
            lx = ts->x_max;
        } else {
        	lx = sx;
        }

        if (sy < 0) {
            ly = 0;
        } else if (sy > ts->y_max) {
            ly = ts->y_max;
        } else {
        	ly = sy;
        }
		/* end of added by h00131430 2009-11-02 begin */
#else
        lx = x;
        ly = y;
#endif
#endif
#endif

#if 1
		

		if (1 == ts_filter(lx, ly, &lx, &ly)) {
			ts_update_pen_state(ts, lx, ly, 255);	
		} else {
		}
#else
		ts_update_pen_state(ts, lx, ly, 255);
#endif		
		/* kick pen up timer - to make sure it expires again(!) */
		mod_timer(&ts->timer,
			jiffies + msecs_to_jiffies(TS_PENUP_TIMEOUT_MS));

	} else
		printk(KERN_INFO "Ignored interrupt: {%3d, %3d},"
				" op = %3d samp = %3d\n",
				 x, y, num_op, num_samp);

out:
	return IRQ_HANDLED;
}

static int __devinit ts_probe(struct platform_device *pdev)
{
	int result;
	struct input_dev *input_dev;
	struct resource *res, *ioarea;
	struct ts *ts;
	unsigned int x_max, y_max, pressure_max;
	struct msm_ts_platform_data *pdata = pdev->dev.platform_data;

	/* The primary initialization of the TS Hardware
	 * is taken care of by the ADC code on the modem side
	 */
	ts = kzalloc(sizeof(struct ts), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!input_dev || !ts) {
		result = -ENOMEM;
		goto fail_alloc_mem;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Cannot get IORESOURCE_MEM\n");
		result = -ENOENT;
		goto fail_alloc_mem;
	}

	ts->irq = platform_get_irq(pdev, 0);
	if (!ts->irq) {
		dev_err(&pdev->dev, "Could not get IORESOURCE_IRQ\n");
		result = -ENODEV;
		goto fail_alloc_mem;
	}

	ioarea = request_mem_region(res->start, resource_size(res), pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "Could not allocate io region\n");
		result = -EBUSY;
		goto fail_alloc_mem;
	}

	virt = ioremap(res->start, resource_size(res));
	if (!virt) {
		dev_err(&pdev->dev, "Could not ioremap region\n");
		result = -ENOMEM;
		goto fail_ioremap;
	}

	input_dev->name = TS_DRIVER_NAME;
	input_dev->phys = "msm_touch/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0002;
	input_dev->id.version = 0x0100;
	input_dev->dev.parent = &pdev->dev;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
	input_dev->absbit[BIT_WORD(ABS_MISC)] = BIT_MASK(ABS_MISC);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	if (pdata) {
		x_max = pdata->x_max ? : X_MAX;
		y_max = pdata->y_max ? : Y_MAX;
		pressure_max = pdata->pressure_max ? : P_MAX;
	} else {
		x_max = X_MAX;
		y_max = Y_MAX;
		pressure_max = P_MAX;
	}

	ts->x_max = x_max;
	ts->y_max = y_max;

	input_set_abs_params(input_dev, ABS_X, 0, x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, pressure_max, 0, 0);

	result = input_register_device(input_dev);
	if (result)
		goto fail_ip_reg;

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
	printk(KERN_ERR "Upper-Left 50x50 point: (%d, %d)\n", 
           			tsp_ul_50x50_convert.x, tsp_ul_50x50_convert.y);
	printk(KERN_ERR "Lower-Right 50x50 point: (%d, %d)\n", 
           			tsp_dr_50x50_convert.x, tsp_dr_50x50_convert.y);
	printk(KERN_ERR "Upper-Lefte origin point: (%d, %d)\n", 
           			tsp_origin_convert.x, tsp_origin_convert.y);

	ts->input = input_dev;
	setup_timer(&ts->timer, ts_timer, (unsigned long)ts);
	platform_set_drvdata(pdev, ts);
	result = request_irq(ts->irq, ts_interrupt, IRQF_TRIGGER_RISING,
				 "touchscreen", ts);
	if (result)
		goto fail_req_irq;
	return 0;

fail_req_irq:
	input_unregister_device(input_dev);
	input_dev = NULL;
fail_ip_reg:
	iounmap(virt);
fail_ioremap:
	release_mem_region(res->start, resource_size(res));
fail_alloc_mem:
	input_free_device(input_dev);
	kfree(ts);
	return result;
}

static int __devexit ts_remove(struct platform_device *pdev)
{
	struct resource *res;
	struct ts *ts = platform_get_drvdata(pdev);

	free_irq(ts->irq, ts);
	del_timer_sync(&ts->timer);

	input_unregister_device(ts->input);
	iounmap(virt);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(res->start, resource_size(res));
	platform_set_drvdata(pdev, NULL);
	kfree(ts);

	return 0;
}

static struct platform_driver ts_driver = {
	.probe		= ts_probe,
	.remove		= __devexit_p(ts_remove),
	.driver		= {
		.name = TS_DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init ts_init(void)
{
	return platform_driver_register(&ts_driver);
}
module_init(ts_init);

static void __exit ts_exit(void)
{
	platform_driver_unregister(&ts_driver);
}
module_exit(ts_exit);

MODULE_DESCRIPTION("MSM Touch Screen driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:msm_touchscreen");
