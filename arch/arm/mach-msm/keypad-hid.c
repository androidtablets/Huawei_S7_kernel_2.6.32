/*
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 * Author: Brian Swetland <swetland@google.com>
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

#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_event.h>

#include <asm/mach-types.h>

#include <mach/board.h>

#if HUAWEI_HWID(S70)
#include "keypad-hid.h"
#endif

#if !HUAWEI_HWID(S70)
static struct gpio_event_direct_entry hid_keypad_map[] = {
#ifdef CONFIG_HID_GPIO_KEYPAD
	{ HID_VOLUMEUP_GPIO,     KEY_VOLUMEUP },
	{ HID_VOLUMEDOWN_GPIO,   KEY_VOLUMEDOWN },
	{ HID_HOME_GPIO,   KEY_HOME},
	{ HID_MENU_GPIO,   KEY_MENU },
	{ HID_BACK_GPIO,   KEY_BACK },
	{ HID_SEND_GPIO,   KEY_SEND},
	{ HID_END_GPIO,   KEY_END },
	/*here we can add more gpio button here*/
#else
	{ HID_VOLUMEUP_GPIO,     158 },
	{ HID_VOLUMEDOWN_GPIO,   229 },
#endif
};

static const unsigned short keypad_virtual_keys[] = {
    KEY_END,
    KEY_POWER,
    KEY_MEDIA
};

static int keypad_gpio_event_input_func(struct input_dev *input_dev,struct gpio_event_info *info, void **data, int func);

static struct gpio_event_input_info hid_keypad_standard_info =
{
	.info.func = gpio_event_input_func,
	.flags = GPIOEDF_PRINT_KEY_DEBOUNCE,
	.type = EV_KEY,
	.debounce_time.tv.nsec = 20 * NSEC_PER_MSEC,
	.keymap = hid_keypad_map,
	.keymap_size = ARRAY_SIZE(hid_keypad_map)
	
};

struct gpio_event_info *hid_keypad_info[] = {
	&hid_keypad_standard_info.info
};

static struct gpio_event_platform_data hid_keypad_data = {
	.name = "s7_keypad",
	.info = hid_keypad_info,
	.info_count = ARRAY_SIZE(hid_keypad_info)
};

struct platform_device hid_keypad_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev		= {
		.platform_data	= &hid_keypad_data,
	},
};

static struct input_dev *keypad_dev;

static int keypad_gpio_event_input_func(struct input_dev *input_dev,
			struct gpio_event_info *info, void **data, int func)
{
	int err;
	int i;

	err = gpio_event_input_func(input_dev, info, data, func);

	if (func == GPIO_EVENT_FUNC_INIT && !err) {
		keypad_dev = input_dev;
		for (i = 0; i < ARRAY_SIZE(keypad_virtual_keys); i++)
			set_bit(keypad_virtual_keys[i] & KEY_MAX,
				input_dev->keybit);
	} else if (func == GPIO_EVENT_FUNC_UNINIT) {
		keypad_dev = NULL;
	}
	else
      {
             keypad_dev = NULL;
      }
	return err;
}


struct input_dev *msm_keypad_get_input_dev(void)
{
	return keypad_dev;
}

#else

/*
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008 QUALCOMM USA, INC.
 * Author: Brian Swetland <swetland@google.com>
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

#include <asm/mach-types.h>
#include <linux/platform_device.h>
#include <linux/gpio_event.h>

/* don't turn this on without updating the ffa support */
#define SCAN_FUNCTION_KEYS 0

#if HUAWEI_HWID_L2(S7, S7201)
static unsigned int keypad_row_gpios[] = { 39 };
static unsigned int keypad_col_gpios[] = { 36, 40 };
#define KEYMAP_INDEX(row, col) ((col)*ARRAY_SIZE(keypad_row_gpios) + (row))
static const unsigned short keypad_keymap[ARRAY_SIZE(keypad_col_gpios) *
					      ARRAY_SIZE(keypad_row_gpios)] = {
    [KEYMAP_INDEX(0, 0)] = KEY_VOLUMEUP,
    [KEYMAP_INDEX(0, 1)] = KEY_VOLUMEDOWN,
};

#else

static unsigned int keypad_row_gpios[] = { 36, 31, 39 };
static unsigned int keypad_col_gpios[] = { 37, 32, 41 };

#define KEYMAP_INDEX(row, col) ((col)*ARRAY_SIZE(keypad_row_gpios) + (row))

static const unsigned short keypad_keymap[ARRAY_SIZE(keypad_col_gpios) *
					      ARRAY_SIZE(keypad_row_gpios)] = {
    [KEYMAP_INDEX(0, 0)] = KEY_MUTE,   /* DEBUG 1 */ 

	#ifdef CONFIG_HUAWEI_WIFI_ONLY
    [KEYMAP_INDEX(0, 1)] = KEY_SEARCH,   /* DEBUG 2 */ 
    [KEYMAP_INDEX(0, 2)] = KEY_SEARCH,   
    [KEYMAP_INDEX(1, 0)] = KEY_BACK, 
    [KEYMAP_INDEX(1, 1)] = KEY_VOLUMEDOWN,  
    [KEYMAP_INDEX(1, 2)] = KEY_HOME, 
    [KEYMAP_INDEX(2, 0)] = KEY_NOTIFICATION, 
    #else
    [KEYMAP_INDEX(0, 1)] = KEY_BACK,   /* DEBUG 2 */ 
    [KEYMAP_INDEX(0, 2)] = KEY_BACK,   
    [KEYMAP_INDEX(1, 0)] = KEY_SEND,  
    [KEYMAP_INDEX(1, 1)] = KEY_VOLUMEDOWN,  
    [KEYMAP_INDEX(1, 2)] = KEY_HOME, 
	[KEYMAP_INDEX(2, 0)] = KEY_END,  
    #endif

    [KEYMAP_INDEX(2, 1)] = KEY_VOLUMEUP,  
    [KEYMAP_INDEX(2, 2)] = KEY_MENU,
};
#endif

static const unsigned short keypad_virtual_keys[] = {
	#ifdef CONFIG_HUAWEI_WIFI_ONLY
	KEY_NOTIFICATION,
	#else    
    KEY_END,
    #endif
	KEY_POWER,
	KEY_MEDIA
};

#if 0
static int keypad_gpio_event_matrix_func(struct input_dev *input_dev,
					  struct gpio_event_info *info,
					  void **data, int func);
#endif

static struct gpio_event_matrix_info keypad_matrix_info = {
	.info.func	= gpio_event_matrix_func,
	.keymap		= keypad_keymap,
	.output_gpios	= keypad_row_gpios,
	.input_gpios	= keypad_col_gpios,
	.noutputs	= ARRAY_SIZE(keypad_row_gpios),
	.ninputs	= ARRAY_SIZE(keypad_col_gpios),
	.settle_time.tv.nsec = 0,
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS,
};

struct gpio_event_info *keypad_info[] = {
	&keypad_matrix_info.info
};

static struct gpio_event_platform_data keypad_data = {
	.name		= "s7_keypad",
	.info		= keypad_info,
	.info_count	= ARRAY_SIZE(keypad_info)
};

struct platform_device hid_keypad_device = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &keypad_data,
	},
};

static struct input_dev *keypad_dev;

#if 0
static int keypad_gpio_event_matrix_func(struct input_dev *input_dev,
					  struct gpio_event_info *info,
					  void **data, int func)
{
	int err;
	int i;

	err = gpio_event_matrix_func(input_dev, info, data, func);

	if (func == GPIO_EVENT_FUNC_INIT && !err) {
		keypad_dev = input_dev;
		for (i = 0; i < ARRAY_SIZE(keypad_virtual_keys); i++)
			set_bit(keypad_virtual_keys[i] & KEY_MAX,
				input_dev->keybit);
	} else if (func == GPIO_EVENT_FUNC_UNINIT) {
		keypad_dev = NULL;
	}

	return err;
}
#endif

struct input_dev *msm_keypad_get_input_dev(void)
{
	return keypad_dev;
}
#endif


