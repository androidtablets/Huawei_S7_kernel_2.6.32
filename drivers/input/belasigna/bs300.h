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
 * Belasigna BS300 (MIC noise reducer) driver
 *
 */
#ifndef __BS300_AUDIO_H
#define __BS300_AUDIO_H
#include <mach/gpio.h>

#define GPIO_BS300_SDA	37			/*I2C data*/
#define GPIO_BS300_SCK	34			/*I2C clock*/

#define BS300_SDA_HIGH()	gpio_set_value(GPIO_BS300_SDA,1)
#define BS300_SDA_LOW()	gpio_set_value(GPIO_BS300_SDA,0)
#define BS300_SCK_HIGH()	gpio_set_value(GPIO_BS300_SCK,1)
#define BS300_SCK_LOW()		gpio_set_value(GPIO_BS300_SCK,0)

//#define BS300_DEBUG_FLAG   1
#define BS300_DEBUG_FLAG   0

#if(BS300_DEBUG_FLAG == 1)
#define BS300_DBG(format,arg...)     do { printk(KERN_ALERT format, ## arg);  } while (0)
#define BS300_ERR(format,arg...)	  do { printk(KERN_ERR format, ## arg);  } while (0)
#define BS300_FAT(format,arg...)     do { printk(KERN_CRIT format, ## arg); } while (0)
#else
#define BS300_DBG(format,arg...)     do { (void)(format); } while (0)
#define BS300_ERR(format,arg...)	  do { (void)(format); } while (0)
#define BS300_FAT(format,arg...)	  do { (void)(format); } while (0)
#endif

#define BS300_DELAY_TIME_USEC	 5 	/*100k Hz I2C clock frequency*/
#define BS300_I2C_ACK			1
#define BS300_I2C_NACK			0
#define BS300_SUCCESS			1
#define BS300_FAIL				0
#endif
