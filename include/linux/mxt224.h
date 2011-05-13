/*
 * drivers/input/touchscreen/mxt422_generic.c
 *
 * Copyright (c) 2011 Huawei Device Co., Ltd.
 *	Li Yaobing <liyaobing@huawei.com>
 *
 * Using code from:
 *  The code is originated from Atmel Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef __LINUX_I2C_MXT224_H
#define __LINUX_I2C_MXT224_H

#include <linux/i2c.h>
#include <linux/earlysuspend.h>

#define MAX_FINGERS		4

struct ts_event {
	uint8_t touch_number_id;//liyaobing214
    uint8_t tchstatus; 
	u16	x;
	u16	y;
    uint8_t tcharea;
    uint8_t tchamp;
	uint8_t tchvector;
};

struct mxt224 {
	struct input_dev	*input;
	char phys[32];
	struct hrtimer hr_timer;
   	struct timer_list timer;
    struct early_suspend early_suspend;
	struct ts_event tc;
	uint32_t x_max;
	uint32_t y_max;
	struct i2c_client *client;
	spinlock_t lock;
	int irq; 
};

extern int mxt224_generic_probe(struct mxt224 *tsc);
extern void mxt224_generic_remove(struct mxt224 *tsc);
extern void mxt224_get_message(struct mxt224 *tsc);
extern void mxt224_update_pen_state(void *tsc);
extern uint8_t config_disable_mxt244(void);
extern uint8_t config_enable_mxt244(void);

struct mxt224_platform_data {
	u16	model;				/* 2007. */
	u16	x_plate_ohms;

	int	(*get_pendown_state)(void);
	void	(*clear_penirq)(void);		/* If needed, clear 2nd level
						   interrupt source */
	int	(*init_platform_hw)(void);
	void	(*exit_platform_hw)(void);
};

#endif
