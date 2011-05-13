/*
 * Copyright 2010 HUAWEI Tech. Co., Ltd.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * Pedestal Driver
 *
 */
#ifndef _PEDESTAL_H_
#define _PEDESTAL_H_

#include <linux/pm.h>			/* pm_message_t */

#define PEDESTAL_DECT_GPIO             			(106)
#define PEDESTAL_INT_GPIO             			(40)
#define PEDESTAL_DRIVER_NAME                    "pedestal"
#define PEDESTAL_DEVICE_NAME                    "pedestal"
#define PEDESTAL_NAME                    "pedestal"
#define PEDESTAL_DEVICE                "/dev/pedestal"
#define PEDESTAL_DECT_NAME                    "pedestal_detect"

#define PEDESTAL_I2C_ADDR						(0x50 >> 1)
//remote control dug msg control
#define _PEDESTAL_DEBUG_ 1
#if(_PEDESTAL_DEBUG_)
#define PEDESTAL_DEG(format,arg...)   printk(KERN_ALERT format, ## arg); 
#define PEDESTAL_INFO(format,arg...)   printk(KERN_ALERT format, ## arg); 
#else
#define PEDESTAL_DEG(format,arg...)   do { (void)(format); } while (0)
#define PEDESTAL_INFO(format,arg...)   do { (void)(format); } while (0)
#endif
#define PEDESTAL_ERR(format,arg...)   printk(KERN_ALERT format, ## arg); 

#define PEDESTAL_PRODUCT_ID_REG     0x00
#define PEDESTAL_STATUS_REG     	0x01
#define PEDESTAL_KEYVALUE_REG    	0x02
#define PEDESTAL_VLOUME				0x03

#define PEDESTAL_TIME_YEAR			0x80
#define PEDESTAL_TIME_MONTH			0x81
#define PEDESTAL_TIME_DATE			0x82
#define PEDESTAL_TIME_HOUR			0x83
#define PEDESTAL_TIME_MIN			0x84
#define PEDESTAL_TIME_SEC			0x85
enum PEDESTAL_TYPE {
	PEDESTAL_TYPE_INTERFACE = 0, 
	PEDESTAL_TYPE_SOUNDBOX , 
	PEDESTAL_TYPE_UNDEF
};

struct pedestalkey_2_hidkey
{
	unsigned int pedestal_key;
	unsigned int hid_key;
};


struct pedestal_platform_data {
	int dect_gpio;
	int interrupt_gpio;
	int (*gpio_setup)(void);
	void (*gpio_release)(void);
};

struct pedestal_func {
	int (*driver_init)(void *dev_id);
	void (*driver_exit)(void *dev_id);
	irqreturn_t (*driver_interrupt)(int irq, void *dev_id);
#ifdef CONFIG_PM
	int (*driver_suspend)(void *dev_id, pm_message_t state);
	int (*driver_resume)(void *dev_id);
#endif
};

struct pedestal_driver_data {
	u32				   pedestal_dect_irq;
	u32                pedestal_gpio_irq;
	struct input_dev  *pedestal_dev;
	struct i2c_client *pedestal_client;
	struct work_struct pedestal_plugin_work;
	struct work_struct pedestal_plugout_work;
	struct pedestal_func pedestal_driver_func;
	spinlock_t pedestal_lock;
};

struct pedestal_gpio_state{
	u32 	gpio_dect_req_state;
	u32		irq_dect_req_state;
	u32		pedestal_gpio_req_state[PEDESTAL_TYPE_UNDEF];
	u32		pedestal_irq_req_state[PEDESTAL_TYPE_UNDEF];
};

extern struct pedestal_gpio_state g_gpio_irq_state;
extern struct pedestal_driver_data* g_pdriverdata;
extern unsigned int g_pedestal_type;
int pedestal_gpio_setup(unsigned int pedestal_type);
void pedestal_gpio_release(unsigned int pedestal_type);
int pedestal_irq_request(unsigned int pedestal_type, unsigned int irq, irq_handler_t handler,
		unsigned long irqflags, const char *devname, void *dev_id);
void pedestal_irq_free(unsigned int pedestal_type, unsigned int irq, void *dev_id);

int pedestal_i2c_read_byte_data(struct i2c_client *client, unsigned char command);
int pedestal_i2c_write_byte_data(struct i2c_client *client, unsigned char command, unsigned char value);
#endif



