/*
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

#ifndef _KEYPAD_HID_H
#define _KEYPAD_HID_H

#include <linux/input.h>

#ifdef CONFIG_HID_GPIO_KEYPAD
#define HID_VOLUMEUP_GPIO 36
#define HID_VOLUMEDOWN_GPIO 31
#define HID_HOME_GPIO 16
#define HID_MENU_GPIO 17
#define HID_BACK_GPIO  15
#define HID_SEND_GPIO 107
#define HID_END_GPIO 8 
#else
#define HID_VOLUMEUP_GPIO 41
#define HID_VOLUMEDOWN_GPIO 141
#endif
#if 0
#if defined(CONFIG_HID_GPIO_KEYPAD)
struct input_dev *msm_keypad_get_input_dev(void);
#else
struct input_dev *msm_keypad_get_input_dev(void)
{
	return NULL;
}
#endif
#endif

#endif
