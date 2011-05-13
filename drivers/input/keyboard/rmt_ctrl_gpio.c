/* Driver for remote control device 

 *  Copyright 2010 HUAWEI Tech. Co., Ltd.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 *  IRDA remote control driver.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>

#include <asm/irq.h>

#include <mach/hardware.h>
#include <mach/io.h>
#include <mach/system.h>
#include <mach/gpio.h>
#include <mach/rmt_ctrl_gpio.h>
#include "rmt_ctrl_nec_protocol.h"

#define TASKLET_ENABLE_TIME 10

unsigned long g_pre_time = 0; //init g_pre_time as irda module loaded time
unsigned long g_cur_time = 0;

u32 g_protocol_status = REMOTER_IDLE;
u32 g_head_time = 0;
u32 g_databit_time = 0;
u32 g_remoter_bits = 0;
u32 g_remoter_bits_num = 0;
u32 g_remoter_press = 0;  //0: release; 1: press down

struct timer_list g_protocol_timer;
	
struct rmt_ctrl_gpio_driver_data* g_irda_gpio_driver_data = NULL;

struct rmt_ctrl_gpio_driver_data {
	u32                irq_gpio;
	struct input_dev  *rmt_ctrl_gpio_dev;
};

#define MSG_BUF_MAX 	128 

spinlock_t g_irq_irda_lock;


struct irda_key2_hidkey
{
	unsigned int irda_code;
	unsigned int hid_key;
};

static struct irda_key2_hidkey g_irda_code2_hid_key_map[] =
{
		{0x30,KEY_POWER},	//sleep
		{0x08,KEY_UP},	//up
		{0x38,KEY_UNKNOWN},	//hdmi/av

		{0x20,KEY_LEFT},	//left
		{0x10,BTN_MOUSE/*KEY_OK*//*KEY_PLAY*/},	//play/pause
		{0xe0,KEY_RIGHT},	//right
		
		{0xc0,KEY_PAGEDOWN},	//speed down
		{0xf8,KEY_DOWN},	//down
		{0x60,KEY_PAGEUP},	//speed up

		{0x40,KEY_HOME},	//home
		{0x78,KEY_MENU},	//menu
		{0xa0,KEY_BACK},	//back

		{0x80,KEY_VOLUMEDOWN},	//vol -
		{0x00,KEY_VOLUMEUP},	//vol +
		
};
static void irda_gpio_irq_tasklet (unsigned long arg);

DECLARE_TASKLET(irda_irq_tasklet, irda_gpio_irq_tasklet, 0);


static void get_sys_time_by_us(unsigned long* ptime_val) //us
{
	struct timeval tcurrent;
	do_gettimeofday(&tcurrent);
	*ptime_val = tcurrent.tv_sec * 1000000LLU + tcurrent.tv_usec;
}

static void translate_irdacode2hidkey(unsigned int irda_cmd, unsigned int* hid_key)
{
	int i = 0;
	int map_len = sizeof(g_irda_code2_hid_key_map)/sizeof(struct irda_key2_hidkey);
	*hid_key = KEY_UNKNOWN;
	
	for(i = 0; i < map_len; i++)
	{
		if(irda_cmd == g_irda_code2_hid_key_map[i].irda_code)
		{
			*hid_key = g_irda_code2_hid_key_map[i].hid_key;
		}
	}
}

#define IRDA_MOVE_STEP  400
static void irda_report_keyevent(unsigned int key_code,void *dev_id)
{
	struct rmt_ctrl_gpio_driver_data *ddate = dev_id;
	/*
	int rel_x = 0, rel_y = 0;
	
	switch(key_code)
	{
		case KEY_UP:
			rel_x = 0;
			rel_y = -IRDA_MOVE_STEP;
			input_report_rel(ddate->rmt_ctrl_gpio_dev, REL_X, rel_x);
			input_report_rel(ddate->rmt_ctrl_gpio_dev, REL_Y, rel_y);
			input_sync(ddate->rmt_ctrl_gpio_dev); 
			break;
		case KEY_DOWN:
			rel_x = 0;
			rel_y = IRDA_MOVE_STEP;
			input_report_rel(ddate->rmt_ctrl_gpio_dev, REL_X, rel_x);
			input_report_rel(ddate->rmt_ctrl_gpio_dev, REL_Y, rel_y);
			input_sync(ddate->rmt_ctrl_gpio_dev); 
			break;
		case KEY_LEFT:
			rel_x = -IRDA_MOVE_STEP;
			rel_y = 0;
			input_report_rel(ddate->rmt_ctrl_gpio_dev, REL_X, rel_x);
			input_report_rel(ddate->rmt_ctrl_gpio_dev, REL_Y, rel_y);
			input_sync(ddate->rmt_ctrl_gpio_dev); 
			break;
		case KEY_RIGHT:
			rel_x = IRDA_MOVE_STEP;
			rel_y = 0;
			input_report_rel(ddate->rmt_ctrl_gpio_dev, REL_X, rel_x);
			input_report_rel(ddate->rmt_ctrl_gpio_dev, REL_Y, rel_y);
			input_sync(ddate->rmt_ctrl_gpio_dev); 
			break;
		default:
			input_report_key(ddate->rmt_ctrl_gpio_dev,key_code , 1);
			input_report_key(ddate->rmt_ctrl_gpio_dev,key_code , 0);
			break;
	}	
	*/
/*	if(BTN_MOUSE == key_code)//double click
	{
		input_report_key(ddate->rmt_ctrl_gpio_dev,key_code , 1);
		input_report_key(ddate->rmt_ctrl_gpio_dev,key_code , 0);
	}*/

	input_report_key(ddate->rmt_ctrl_gpio_dev,key_code , 1);
	input_sync(ddate->rmt_ctrl_gpio_dev);
	input_report_key(ddate->rmt_ctrl_gpio_dev,key_code , 0);	
	input_sync(ddate->rmt_ctrl_gpio_dev);
}

static void irda_addbit(unsigned int current_bit,void *dev_id)
{    
    unsigned char irda_code = 0;
    unsigned char l_code;
    unsigned int key_code = KEY_UNKNOWN;
//	struct rmt_ctrl_gpio_driver_data *ddate = dev_id;
    g_remoter_bits =  g_remoter_bits << 1;
    g_remoter_bits |= current_bit;

    g_remoter_bits_num++;
     
    g_databit_time = 0;
    if ( g_remoter_bits_num != IRDA_DATA_BITS )  //32
    	return;
    
    g_remoter_bits_num = 0;

    irda_code =  ( g_remoter_bits & 0x0000ff00 ) >> 8;          
    l_code =   g_remoter_bits & 0x000000ff ;        
    l_code = ~l_code;


	RMT_CTRL_GPIO_DEG( "irda_addbit: remoter_user_code = 0x%08x, cmd = 0x%02x \n",g_remoter_bits & 0xffff0000,irda_code);
	printk(KERN_ALERT "irda_addbit: remoter_user_code = 0x%08x, cmd = 0x%02x \n",g_remoter_bits & 0xffff0000,irda_code); 
    if((g_remoter_bits & 0xffff0000) != REMOTER_USER_CODE ||(l_code != irda_code))
//	if(l_code != irda_code)
    {

        g_protocol_status = REMOTER_IDLE ;
        return;

    }
    
	translate_irdacode2hidkey(irda_code,&key_code);
	if(key_code != KEY_UNKNOWN)
	{
		irda_report_keyevent(key_code,dev_id);
	}
    g_remoter_press = 1;

    g_protocol_status = REMOTER_CATCH_DATA_DONE ;   
    g_head_time = 0;     

    return ;
}

#define IRDA_GPIO_IRQ_MAX_PRE_KEY    (80)  //every remote control press irq max count, conside every long press as a new special press
#define IRDA_IRQ_BUF_KEY_MAX		 (2)  //max key press process
struct irq_bit_info{
	int gpio_state;
	unsigned long pre_time;
	unsigned long cur_time;
};

struct irq_key_info{
	int bit_head;
	int bit_num;
	struct irq_bit_info bit_array[IRDA_GPIO_IRQ_MAX_PRE_KEY];
};

struct irq_key_array{
	int key_index;
	int key_num;
	struct irq_key_info key_array[IRDA_IRQ_BUF_KEY_MAX];
};

struct irq_key_array g_irq_key_array;  //irda key info buf of irq

static void init_irq_key_array(struct irq_key_array* pirq_key_struct)
{
	pirq_key_struct->key_index = 0;
	pirq_key_struct->key_num = 0;
//	memset(pirq_key_struct->key_array,0,IRDA_IRQ_BUF_KEY_MAX*sizeof(struct irq_key_info));
}

static int irq_key_addbit(struct irq_key_array* pirq_key_struct,struct irq_bit_info irq_bit)
{
	struct irq_key_info* pKey = NULL;
	struct irq_bit_info* pBit = NULL;
	int key_num = pirq_key_struct->key_num > 0 ? pirq_key_struct->key_num - 1 : 0;

	pKey = (struct irq_key_info*) (&(pirq_key_struct->key_array[(key_num + pirq_key_struct->key_index)% IRDA_IRQ_BUF_KEY_MAX]));

	if(pKey->bit_num < IRDA_GPIO_IRQ_MAX_PRE_KEY)
	{
		pBit = (struct irq_bit_info*)  (&(pKey->bit_array[(pKey->bit_head + pKey->bit_num)%IRDA_GPIO_IRQ_MAX_PRE_KEY]));
	//	memcpy(pBit,&irq_bit,sizeof(struct irq_bit_info));
		pBit->gpio_state = irq_bit.gpio_state;
		pBit->pre_time= irq_bit.pre_time;
		pBit->cur_time= irq_bit.cur_time;
		
		pKey->bit_num += 1;
	}
	return 0;
}

static int irq_key_addkey(struct irq_key_array* pirq_key_struct)
{
	struct irq_key_info* pKey = NULL;

	if(pirq_key_struct->key_num >= IRDA_IRQ_BUF_KEY_MAX)
	{
		return -1;
	}

	pKey = (struct irq_key_info*) (&(pirq_key_struct->key_array[(pirq_key_struct->key_index + pirq_key_struct->key_num)%IRDA_IRQ_BUF_KEY_MAX]));	
	//memset(pKey,0,sizeof(struct irq_key_info));
	pKey->bit_head = 0;
	pKey->bit_num = 0;
	memset(pKey->bit_array,0,IRDA_GPIO_IRQ_MAX_PRE_KEY*sizeof(struct irq_bit_info));
	
	pirq_key_struct->key_num += 1;
	return 0;
}

static int irq_key_removebit(struct irq_key_array* pirq_key_struct,struct irq_bit_info* pirq_bit_info)
{
	struct irq_key_info* pKey = NULL;
	struct irq_bit_info* pBit = NULL;

	pKey = (struct irq_key_info*) (&(pirq_key_struct->key_array[pirq_key_struct->key_index]));
	if(pKey->bit_num <= 0)
	{
		pKey->bit_head = 0;
		RMT_CTRL_GPIO_DEG( "irda_irq_key_array %d key bit array is empty! \n",pirq_key_struct->key_index );  
		return -1;
	}

	pBit = (struct irq_bit_info*) (&(pKey->bit_array[pKey->bit_head]));
//	RMT_CTRL_GPIO_DEG( "irq_key_removebit: remove %d key bit_num = %d, %d bits, gpio_state= %d, dalt_time= %lu\n",
//		pirq_key_struct->key_index,pKey->bit_num,pKey->bit_head,pBit->gpio_state,pBit->cur_time - pBit->pre_time); 

	pirq_bit_info->gpio_state = pBit->gpio_state;
	pirq_bit_info->pre_time = pBit->pre_time;
	pirq_bit_info->cur_time = pBit->cur_time;
	
	memset(pBit,0,sizeof(struct irq_bit_info));
	
	pKey->bit_num  = pKey->bit_num > 0 ? pKey->bit_num -1: 0;
	pKey->bit_head += 1;
	pKey->bit_head %= IRDA_GPIO_IRQ_MAX_PRE_KEY;
	return 0;
}

static int irq_key_removekey(struct irq_key_array* pirq_key_struct)
{
	struct irq_key_info* pKey = NULL;
	if(pirq_key_struct->key_num <= 0)
	{
		return -1;
	}
	pKey = (struct irq_key_info*) (&(pirq_key_struct->key_array[pirq_key_struct->key_index]));
	//memset(pKey,0,sizeof(struct irq_key_info));
	pKey->bit_head = 0;
	pKey->bit_num = 0;
	memset(pKey->bit_array,0,IRDA_GPIO_IRQ_MAX_PRE_KEY*sizeof(struct irq_bit_info));
	
	pirq_key_struct->key_index = (pirq_key_struct->key_index + 1)%IRDA_IRQ_BUF_KEY_MAX;
	pirq_key_struct->key_num =  pirq_key_struct->key_num > 0 ? pirq_key_struct->key_num - 1: 0;
	return 0;
}



#define GPIO_IRQ_TRIGGER_FALLING  	0
#define GPIO_IRQ_TRIGGER_RISING 	1

//NEC IRDA Remote Control Protocl decording
static void irda_gpio_irq_tasklet (unsigned long arg)
{
	int ret = 0;
	int irq_type = 0;
	struct irq_bit_info irq_bit;
	static u32 long_press_count = 0;
	int i = 0, j = 0;
	spin_lock_irq(&g_irq_irda_lock);  


	RMT_CTRL_GPIO_DEG( "irda_gpio_irq_tasklet: array len = %d, head = %d \n",g_irq_key_array.key_num,g_irq_key_array.key_index);

	#if 0 //printf irq buf data
	for(i = 0; i < g_irq_key_array.key_num; i++)
	{
		RMT_CTRL_GPIO_DEG( "printf %d key data: bit num = %d\n",(i+g_irq_key_array.key_index)%IRDA_IRQ_BUF_KEY_MAX,
			g_irq_key_array.key_array[(i+g_irq_key_array.key_index)%IRDA_IRQ_BUF_KEY_MAX].bit_num);
		for(j = 0; j < g_irq_key_array.key_array[(i+g_irq_key_array.key_index)%IRDA_IRQ_BUF_KEY_MAX].bit_num; j++)
		{
		/*	RMT_CTRL_GPIO_DEG("bit info %d bit: gpio state= %d, pre_time=%lu, cur_time=%lu, dalt_time = %lu \n",j,
				g_irq_key_array.key_array[(i+g_irq_key_array.key_index)%IRDA_IRQ_BUF_KEY_MAX].bit_array[j].gpio_state,
				g_irq_key_array.key_array[(i+g_irq_key_array.key_index)%IRDA_IRQ_BUF_KEY_MAX].bit_array[j].pre_time,
				g_irq_key_array.key_array[(i+g_irq_key_array.key_index)%IRDA_IRQ_BUF_KEY_MAX].bit_array[j].cur_time,
				g_irq_key_array.key_array[(i+g_irq_key_array.key_index)%IRDA_IRQ_BUF_KEY_MAX].bit_array[j].cur_time - 
				g_irq_key_array.key_array[(i+g_irq_key_array.key_index)%IRDA_IRQ_BUF_KEY_MAX].bit_array[j].pre_time); */
			RMT_CTRL_GPIO_DEG("bit info %d bit: gpio state= %d, dalt_time = %lu \n",j,
				g_irq_key_array.key_array[(i+g_irq_key_array.key_index)%IRDA_IRQ_BUF_KEY_MAX].bit_array[j].gpio_state,
				g_irq_key_array.key_array[(i+g_irq_key_array.key_index)%IRDA_IRQ_BUF_KEY_MAX].bit_array[j].cur_time - 
				g_irq_key_array.key_array[(i+g_irq_key_array.key_index)%IRDA_IRQ_BUF_KEY_MAX].bit_array[j].pre_time);
		}
	}
	#endif
	while(g_irq_key_array.key_num > 0)	
	{
		while(ret = irq_key_removebit(&g_irq_key_array,&irq_bit) == 0)
		{
			RMT_CTRL_GPIO_DEG("-------------irq_bit: gpio_state = %d, dalt_time = %lu \n",irq_bit.gpio_state,irq_bit.cur_time - irq_bit.pre_time);
			irq_type = irq_bit.gpio_state;
			// NEC protocol deal proc:
			#if(IRDA_GPIO_INVERTED)
			if(GPIO_IRQ_TRIGGER_FALLING== irq_type)
			#else
			if(GPIO_IRQ_TRIGGER_RISING == irq_type)
			#endif
			{
				#if(IRDA_GPIO_INVERTED)
				RMT_CTRL_GPIO_DEG( "irda_gpio_irq_tasklet: In FALLING case, g_protocol_status = %d \n",g_protocol_status); 
				#else
				RMT_CTRL_GPIO_DEG( "irda_gpio_irq_tasklet: In RISING case, g_protocol_status = %d \n",g_protocol_status); 
				#endif
				switch(g_protocol_status)
				{
					case REMOTER_IDLE:
						g_protocol_status = REMOTER_CATCH_START;
						long_press_count = 0;
						g_remoter_press = 0;
						g_head_time = 0;
					break;
					
					case REMOTER_CATCH_START:
						g_head_time += irq_bit.cur_time - irq_bit.pre_time;
						if((g_head_time <= REMOTER_START_CODE_TIME_MAX) && (g_head_time >= REMOTER_START_CODE_TIME_MIN))
						{
							g_protocol_status = REMOTER_CATCH_DATA;
							g_head_time = 0;
							g_databit_time = 0;
							break;
						}
					
						#if(IRDA_GPIO_INVERTED)
						RMT_CTRL_GPIO_DEG( "irda_gpio_irq_tasklet: In FALLING case, REMOTER_CATCH_START timeout  g_head_time = %d \n",g_head_time); 
						#else
						RMT_CTRL_GPIO_DEG( "irda_gpio_irq_tasklet: In RISING case, REMOTER_CATCH_START timeout  g_head_time = %d \n",g_head_time); 
						#endif
						g_head_time = 0;
						g_databit_time = 0;
					//	g_protocol_status = REMOTER_IDLE;
					break;
					
					case REMOTER_CATCH_DATA:
						g_databit_time += irq_bit.cur_time - irq_bit.pre_time;
						if((g_databit_time <= REMOTER_LOW_CODE_TIME_MAX) && (g_databit_time >= REMOTER_LOW_CODE_TIME_MIN)) // bit(0)
						{
							irda_addbit(0,g_irda_gpio_driver_data);
						}
						else if((g_databit_time <= REMOTER_HIGH_CODE_TIME_MAX) && (g_databit_time >= REMOTER_HIGH_CODE_TIME_MIN)) // bit(1)
						{
							irda_addbit(1,g_irda_gpio_driver_data);
						}
						else
						{
							g_protocol_status = REMOTER_IDLE;
							g_head_time = 0;
							g_databit_time = 0;
							RMT_CTRL_GPIO_DEG( "irda_gpio_irq_tasklet: invid bit  \n");
						}
					break;
					
					case REMOTER_CATCH_DATA_DONE: //data transmit finish either has one cycle high signal transmit
						g_protocol_status = REMOTER_CATCH_REPEAT_START;
					break;
					
					case REMOTER_CATCH_REPEAT_START:
						g_head_time += irq_bit.cur_time - irq_bit.pre_time;
						if((g_head_time <= REMOTER_REPEAT_CODE_TIME_MAX) && (g_head_time >= REMOTER_REPEAT_CODE_TIME_MIN))
						{
							g_protocol_status = REMOTER_CATCH_REPEAT_END;
							g_head_time = 0;
							g_databit_time = 0;
							break;
						}

						
						g_protocol_status = REMOTER_IDLE;
					break;
					
					case REMOTER_CATCH_REPEAT_END:
					break;
					
					default:
					break;
				}
			}
			#if(IRDA_GPIO_INVERTED)
			else if(GPIO_IRQ_TRIGGER_RISING== irq_type)
			#else
			else if(GPIO_IRQ_TRIGGER_FALLING == irq_type)
			#endif
			{
				#if(IRDA_GPIO_INVERTED)
				RMT_CTRL_GPIO_DEG( "irda_gpio_irq_tasklet: In RISING case, g_protocol_status = %d \n",g_protocol_status); 
				#else
				RMT_CTRL_GPIO_DEG( "irda_gpio_irq_tasklet: In FALLING case, g_protocol_status = %d \n",g_protocol_status); 
				#endif
				switch(g_protocol_status)
				{
					case REMOTER_IDLE:
					break;
					
					case REMOTER_CATCH_START:
						g_head_time += (irq_bit.cur_time - irq_bit.pre_time);
					break;
					
					case REMOTER_CATCH_DATA:
						g_databit_time += (irq_bit.cur_time - irq_bit.pre_time);
					break;
					
					case REMOTER_CATCH_DATA_DONE:
					break;
					
					case REMOTER_CATCH_REPEAT_START:
						g_head_time += (irq_bit.cur_time - irq_bit.pre_time);
					break;
					
					case REMOTER_CATCH_REPEAT_END:
						g_protocol_status = REMOTER_CATCH_REPEAT_START;
						g_head_time = 0;
						long_press_count = (long_press_count + 1)%REPEAT_LOADER_CODE_TIMES;
						if(long_press_count == 0)
						{
							//report a key long press msg
							RMT_CTRL_GPIO_DEG( "irda_gpio_irq_tasklet: In FALLING case, report key longpress \n"); 
						}
				
					break;
					
					default:
					break;
				}
			}
		} //while get bit 
		irq_key_removekey(&g_irq_key_array);
	}// while get key

	spin_unlock_irq(&g_irq_irda_lock);
}


void irda_irq_timer_fn(unsigned long arg)
{
	tasklet_schedule(&irda_irq_tasklet);
}


static irqreturn_t rmt_ctrl_gpio_interrupt(int irq, void *dev_id)
{
	struct rmt_ctrl_gpio_driver_data *ddate = dev_id;
	struct irq_bit_info irq_bit;
	unsigned long dalt_time = 0;
	
	u32 gpio_state = 0;

	//disable gpio interrupt
	disable_irq(ddate->irq_gpio);
	spin_lock_irq(&g_irq_irda_lock);  
	
	gpio_state = gpio_get_value(RMT_CTRL_GPIO_IRQ);
	irq_bit.gpio_state = gpio_state;

	get_sys_time_by_us(&g_cur_time);

	dalt_time = g_cur_time - g_pre_time;
	if(dalt_time > CATCH_TIMEOUT)  // it must be a new press of remote control
	{
	//	printk(KERN_ALERT "interrupt add key CATCH_TIMEOUT: dalt_time = %lu,g_protocol_status=%d\n",dalt_time,g_protocol_status); 
		g_protocol_status = REMOTER_CATCH_START;
		if(irq_key_addkey(&g_irq_key_array) != 0)
		{
			RMT_CTRL_GPIO_ERR("interrupt add key CATCH_TIMEOUT: irq_key_addkey failed!");
			g_protocol_status = REMOTER_IDLE;
			goto out_irq;
		}
		mod_timer(&g_protocol_timer,jiffies + TASKLET_ENABLE_TIME);
		g_pre_time = g_cur_time;
		goto out_irq;
	}
	if(dalt_time > CATCH_REPEAT_TIMEOUT) // catch repeat start time out , recatch as a new press irq
	{
	//	printk(KERN_ALERT "interrupt add key CATCH_REPEAT_TIMEOUT: dalt_time = %d,g_protocol_status=%d\n",dalt_time,g_protocol_status); 
		g_protocol_status = REMOTER_CATCH_START;
		if(irq_key_addkey(&g_irq_key_array) != 0)
		{
			RMT_CTRL_GPIO_ERR("interrupt add key CATCH_REPEAT_TIMEOUT: irq_key_addkey failed!");
			g_protocol_status = REMOTER_IDLE;
			goto out_irq;
		}
		mod_timer(&g_protocol_timer,jiffies + TASKLET_ENABLE_TIME);
		g_pre_time = g_cur_time;
		goto out_irq;
	}

	/*
	else if((dalt_time >= 20*1000) && (dalt_time <= CATCH_TIMEOUT)) //repeat packet
	{
		printk(KERN_ALERT "interrupt add key REMOTER_CATCH_REPEAT_START: dalt_time = %lu,g_protocol_status=%d\n",dalt_time,g_protocol_status); 
		g_protocol_status = REMOTER_CATCH_REPEAT_START;
		irq_key_addkey(&g_irq_key_array);
		mod_timer(&g_protocol_timer,jiffies + TASKLET_ENABLE_TIME);
		g_pre_time = g_cur_time;
		goto out_irq;
	}
	*/

	irq_bit.pre_time= g_pre_time;
	irq_bit.cur_time= g_cur_time;

	irq_key_addbit(&g_irq_key_array,irq_bit);
	g_pre_time = g_cur_time;

out_irq:
	spin_unlock_irq(&g_irq_irda_lock);
	enable_irq(ddate->irq_gpio);
	//call from time proc
//	tasklet_schedule(&irda_irq_tasklet);  //call freq is too continually
	return IRQ_HANDLED;
}

static int32_t __devinit rmt_ctrl_gpio_probe(struct platform_device *pdev)
{
	struct rmt_ctrl_gpio_platform_data* pdata = NULL; 
	struct rmt_ctrl_gpio_driver_data* ddata = NULL;
	int32_t key_count = 0, key_num = 0;
	int32_t rc = 0;


	pdata =(struct rmt_ctrl_gpio_platform_data*)(pdev->dev.platform_data);
	if(!pdata)
	{
		goto alloc_err;
	}
	ddata = kzalloc(sizeof(struct rmt_ctrl_gpio_driver_data), GFP_KERNEL);
	if (!ddata) 
	{
		rc = -ENOMEM;
		RMT_CTRL_GPIO_DEG( "remote control gpio:  kzalloc error \n");        
		goto alloc_err;
	}
	g_irda_gpio_driver_data = ddata;
	
	memset(ddata,0,sizeof(struct rmt_ctrl_gpio_driver_data));
	ddata->rmt_ctrl_gpio_dev  = input_allocate_device();
	if (!(ddata->rmt_ctrl_gpio_dev)) 
	{
		rc = -ENOMEM;
		RMT_CTRL_GPIO_DEG( "remote control gpio:  input_allocate_device error \n");        
		goto inputalloc_err;
	}
	

	if (pdata->gpio_setup == NULL)
		goto gpio_setup_err;
	rc = pdata->gpio_setup();
	if (rc)
		goto gpio_setup_err;

	input_set_drvdata(ddata->rmt_ctrl_gpio_dev, ddata);
	ddata->rmt_ctrl_gpio_dev->name       	= "irda_rmt_ctrl_gpio";
	ddata->rmt_ctrl_gpio_dev->phys       	= "/dev/irda_rmt_ctrl_gpio";
	ddata->rmt_ctrl_gpio_dev->id.bustype 	= BUS_HOST;
	ddata->rmt_ctrl_gpio_dev->id.product 	= 1;
	ddata->rmt_ctrl_gpio_dev->id.version 	= 1;
	__set_bit(EV_REL, ddata->rmt_ctrl_gpio_dev->evbit);
	__set_bit(REL_X, ddata->rmt_ctrl_gpio_dev->relbit);
	__set_bit(REL_Y, ddata->rmt_ctrl_gpio_dev->relbit);
	__set_bit(EV_KEY, ddata->rmt_ctrl_gpio_dev->evbit);
	__set_bit(BTN_MOUSE, ddata->rmt_ctrl_gpio_dev->evbit);

	key_num = sizeof(g_irda_code2_hid_key_map)/sizeof(struct irda_key2_hidkey);
	key_num = 14;
	for(key_count = 0; key_count < key_num; key_count++)
	{
		input_set_capability(ddata->rmt_ctrl_gpio_dev, EV_KEY, g_irda_code2_hid_key_map[key_count].hid_key);
	}
	
	
	platform_set_drvdata(pdev, ddata);
	
	ddata->rmt_ctrl_gpio_dev->name = (char*)pdev->name;
	
	init_irq_key_array(&g_irq_key_array);
	spin_lock_init(&g_irq_irda_lock);

	get_sys_time_by_us(&g_pre_time);
	
	ddata->irq_gpio = gpio_to_irq(pdata->irq_gpio_pin);
	
	//request gpio irq
	rc = request_irq(ddata->irq_gpio,
		       &rmt_ctrl_gpio_interrupt,
		       IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		       RMT_CTRL_GPIO_DEVICE_NAME,
		       ddata);
	if (rc) {
		goto gpio_irq_req_err;
	}
	
	rc = input_register_device(ddata->rmt_ctrl_gpio_dev);
	if (rc) 
	{
		RMT_CTRL_GPIO_DEG( "remote control gpio: Unable to register input device, " "error: %d\n", rc);
		goto register_device_err;
	}
	
	g_protocol_status = REMOTER_IDLE;

	g_protocol_timer.data = NULL;
	g_protocol_timer.function = irda_irq_timer_fn;
	g_protocol_timer.expires = jiffies + TASKLET_ENABLE_TIME;

	init_timer(&g_protocol_timer);
//	add_timer(&g_protocol_timer);
		
	return 0;
register_device_err:
gpio_irq_req_err:
	free_irq(ddata->irq_gpio, pdev);
	if (pdata->gpio_release)
		pdata->gpio_release();
	
gpio_setup_err:
	platform_set_drvdata(pdev, NULL);
	input_free_device(ddata->rmt_ctrl_gpio_dev);
inputalloc_err:
	kfree(ddata);
alloc_err:
	return rc;
	
}

static int rmt_ctrl_gpio_remove(struct platform_device *pdev)
{
	struct rmt_ctrl_gpio_platform_data* pdata =(struct rmt_ctrl_gpio_platform_data*)(pdev->dev.platform_data);
	struct rmt_ctrl_gpio_driver_data *ddata = input_get_drvdata(pdev);
	
    free_irq(ddata->irq_gpio, ddata);

	del_timer(&g_protocol_timer);
	if (pdata->gpio_release)
		pdata->gpio_release();
	input_free_device(ddata->rmt_ctrl_gpio_dev);
	platform_set_drvdata(pdev, NULL);
	kfree(ddata);

	g_irda_gpio_driver_data = NULL;
	return 0;
}


static int rmt_ctrl_gpio_suspend(struct platform_device* pdev, pm_message_t state)
{
    	return 0;
}

static int rmt_ctrl_gpio_resume(struct platform_device* pdev)
{
	return 0;
}

static struct platform_driver rmt_ctrl_gpio_platform_driver = {
	.probe		= rmt_ctrl_gpio_probe,
	.remove		= rmt_ctrl_gpio_remove,
	.suspend	= rmt_ctrl_gpio_suspend,
	.resume		= rmt_ctrl_gpio_resume,
	.driver		= {
		.name	= "rmt_ctrl_gpio",
		.owner	= THIS_MODULE,
	}
};

static int32_t __init rmt_ctrl_gpio_init(void)
{
	return platform_driver_register(&rmt_ctrl_gpio_platform_driver);
}

static void __exit rmt_ctrl_gpio_exit(void)
{
	platform_driver_unregister(&rmt_ctrl_gpio_platform_driver);
}


module_init(rmt_ctrl_gpio_init);
module_exit(rmt_ctrl_gpio_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.1");
MODULE_AUTHOR("HUAWEI Tech. Co., Ltd . wufan");
MODULE_DESCRIPTION(" Irda remote control receiver driver");
MODULE_ALIAS("platform:rmt-ctrl-hid");
