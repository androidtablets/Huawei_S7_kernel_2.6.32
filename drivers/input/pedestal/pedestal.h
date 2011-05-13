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
 
#define PEDESTAL_DECT_GPIO             			(106)
#define PEDESTAL_INT_GPIO             			(40)
#define PEDESTAL_DRIVER_NAME                    "pedestal"
#define PEDESTAL_DEVICE_NAME                    "pedestal"
#define PEDESTAL_NAME                    "pedestal"
#define PEDESTAL_DEVICE                "/dev/pedestal"

#define PEDESTAL_I2C_ADDR 						(0x50)

//remote control dug msg control
#define _PEDESTAL_DEBUG_ 1
#if(_PEDESTAL_DEBUG_)
#define PEDESTAL_DEG(format,arg...)   printk(KERN_ALERT format, ## arg); 
#else
#define PEDESTAL_DEG(format,arg...)   do { (void)(format); } while (0)
#endif

#define PEDESTAL_INFO(format,arg...)   printk(KERN_ALERT format, ## arg); 
#define PEDESTAL_ERR(format,arg...)   printk(KERN_ALERT format, ## arg); 



struct pedestal_platform_data {
	int dect_irq;
	int (*gpio_setup)(void);
	void (*gpio_release)(void);
};

struct pedestal_func {
	int32_t (*driver_init)(struct platform_device *pdev);
	void (*driver_exit)(struct platform_device *pdev);
	irqreturn_t (*driver_interrupt)(int irq, void *dev_id);
	void (*driver_work_f)(struct work_struct *work);
	#ifdef CONFIG_PM
	int32_t (*driver_suspend)(struct platform_device* pdev, pm_message_t state);
	int32_t (*driver_resume)(struct platform_device* pdev);
	#endif
};

struct pedestal_driver_data {
	u32                pedestal_irq;
	struct input_dev  *pedestal_dev;
	struct i2c_client *pedestal_client;
	struct work_struct pedestal_work;
	struct pedestal_func *pedestal_driver_func;
};

#endif