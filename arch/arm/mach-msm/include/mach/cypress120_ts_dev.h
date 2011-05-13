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
 * Cypress 120 i2c touch panle driver
 *
 */
 
#ifndef _CYPRESS120_TS_DEV_H_
#define _CYPRESS120_TS_DEV_H_
#include <linux/kernel.h>
#include <linux/earlysuspend.h>

#define CYPRESS120_I2C_TS_NAME    "cypress-120-ts"


struct cypress120_ts_i2c_platform_data {
	int dect_irq;
    int reset_gpio;
    int power_gpio;
    
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct  work;
	struct early_suspend early_suspend;

    void (*gpio_free) (void);
};

struct cypress120_ts_read_info {
	unsigned char crl_status;
	unsigned char intl_flag;
	unsigned char x1_pos_h;
	unsigned char x1_pos_l;
	unsigned char y1_pos_h;
	unsigned char y1_pos_l;

	unsigned char x2_pos_h;
	unsigned char x2_pos_l;
	unsigned char y2_pos_h;
	unsigned char y2_pos_l;

	unsigned char finger;
	unsigned char gesture;
	
	unsigned short x1_pos;
	unsigned short y1_pos;
	unsigned short x2_pos;
	unsigned short y2_pos;
};

#endif
