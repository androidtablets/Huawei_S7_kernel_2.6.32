/* Driver for IRDA Remote Control device 

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
 * IRDA Remote Control Driver
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
#include <linux/i2c.h>
#include <linux/platform_device.h>

#include <asm/irq.h>

#include <mach/hardware.h>
#include <mach/io.h>
#include <mach/system.h>
#include <mach/gpio.h>

#include <mach/pedestal.h>
#include "irda_rmtctrl_protocol.h"
#include "irda_rmtctrl.h"

static unsigned long g_pre_time = 0; //init g_pre_time as irda module loaded time
static unsigned int g_protocol_status = REMOTER_IDLE;
static unsigned int g_remoter_bits = 0;
static unsigned int g_remoter_bits_num = 0;
static unsigned int g_remoter_code =0;   
static unsigned int g_remoter_key_flag = 0;  
static unsigned int g_invalid_keycount = 0; 


static struct pedestalkey_2_hidkey g_irdaCmd2hidKey_map[] =
{
		{0x30,KEY_POWER},	//sleep
		{0x08,KEY_UP},	//up
		{0x38,KEY_UNKNOWN},	//hdmi/av

		{0x20,KEY_LEFT},	//left
		{0x10,BTN_MOUSE/*KEY_OK*//*KEY_PLAY*/},	//play/pause
		{0xe0,KEY_RIGHT},	//right
		
		{0xc0,KEY_PREVIOUSSONG},	//previous song
		{0xf8,KEY_DOWN},	//down
		{0x60,KEY_NEXTSONG},	//next song

		{0x40,KEY_HOME},	//home
		{0x78,KEY_MENU},	//menu
		{0xa0,KEY_BACK},	//back

		{0x80,KEY_VOLUMEDOWN},	//vol -
		{0x00,KEY_VOLUMEUP},	//vol +
		
};

static void get_sys_time_by_us(unsigned long* ptime_val) //us
{
	struct timeval tcurrent;
	do_gettimeofday(&tcurrent);
	*ptime_val = tcurrent.tv_sec * 1000000LLU + tcurrent.tv_usec;
}

static int translate_irdacode2hidkey(unsigned int irda_cmd, unsigned int* hid_key)
{
	int rc = -1;
	int i = 0;
	int map_len = sizeof(g_irdaCmd2hidKey_map)/sizeof(struct pedestalkey_2_hidkey);
	*hid_key = KEY_UNKNOWN;

	for(i = 0; i < map_len; i++)
	{
		if(irda_cmd == g_irdaCmd2hidKey_map[i].pedestal_key)
		{
			*hid_key = g_irdaCmd2hidKey_map[i].hid_key;
			break;
		}
	}

	if(i < map_len)
		rc = 0;

	return rc;
}

void irda_rmtctrl_report_keyevent(unsigned int irda_code)
{
	unsigned char l_code = 0;  
	unsigned char irda_cmd = 0;  
	unsigned int hid_key = 0;

	if(!g_pdriverdata)
		return;

	if(g_pdriverdata->pedestal_dev)
	{
		irda_cmd =  ( irda_code & 0x0000ff00 ) >> 8;  
		g_remoter_code = irda_cmd;
		l_code =   irda_code & 0x000000ff ;                
		l_code = ~l_code;
		if(((irda_code & 0xffff0000) != REMOTER_USER_CODE) ||(l_code != irda_cmd))       
		{            
			g_protocol_status = REMOTER_IDLE ;             
			return;        
		} 
		
		if(translate_irdacode2hidkey(irda_cmd,&hid_key) == 0)
		{
			input_report_key(g_pdriverdata->pedestal_dev,hid_key , 1);
			input_report_key(g_pdriverdata->pedestal_dev,hid_key , 0);	
			input_sync(g_pdriverdata->pedestal_dev);
		}
	}
}

/************************************************************************* 
函数名称	：IRDA_AddBit* 
功能描述	：红外数据组合，将一位位的0 1组合成数据* 
输入参数	：略* 
输出参数	：无* 
返 回 值	：无* 
影    响	：* 
其它说明	：* 
作    者	：* 
************************************************************************/
static void irda_addbit(unsigned int current_bit)
{        
	  
	unsigned int temp_bits = 0;

//	PEDESTAL_INFO( "---------------------enter irda_addbit \n"); 
	
	g_remoter_bits =  g_remoter_bits << 1;    
	g_remoter_bits |= current_bit;    
	g_remoter_bits_num ++;    
	if ( g_remoter_bits_num == IRDA_DATA_BITS )  //32    
	{        
		temp_bits = g_remoter_bits;
		g_remoter_bits_num = 0;   
		//g_remoter_bits = 0;

       	//PEDESTAL_DEG( "---------------------add key,irda cmd is 0x%08x \n",temp_bits); 
		
		irda_rmtctrl_report_keyevent(temp_bits);
		
		g_remoter_key_flag = 1;
		
		g_protocol_status = REMOTER_CATCH_REPEAT_START ;             
	          
		return;   
	}   
	return ;
}

irqreturn_t pedestal_interface_interrupt(int irq, void *dev_id)
{
	struct pedestal_driver_data *dd = dev_id;
	unsigned long  dalt_time;         
	static unsigned int iCount=0;    
	unsigned long tcurrent;

	disable_irq(dd->pedestal_dect_irq);
	spin_lock_irq(&dd->pedestal_lock); 

	
	get_sys_time_by_us(&tcurrent);
	dalt_time = tcurrent - g_pre_time;
	g_pre_time = tcurrent;

	if(dalt_time < MIN_INTEVAL)
	{
		g_protocol_status = REMOTER_IDLE;

		g_invalid_keycount++;                          
		if(g_invalid_keycount > 65000)            
		{                
			g_invalid_keycount = 1;	           
		} 
		goto irda_irq_exit;
	}

	switch (g_protocol_status)
	{
		case REMOTER_IDLE:
			g_remoter_bits = 0;            
			g_remoter_bits_num = 0;
			g_protocol_status = REMOTER_CATCH_START;
			break;
		case REMOTER_CATCH_START:
			if((dalt_time > REMOTER_START_CODE_TIME_MIN) && (dalt_time <= REMOTER_START_CODE_TIME_MAX))
			{
				g_protocol_status = REMOTER_CATCH_DATA;
				break;
			}

			if(g_remoter_key_flag == 1)
			{
				irda_rmtctrl_report_keyevent(g_remoter_bits);
				g_remoter_key_flag = 0;
			}

			g_remoter_bits = 0;            
			g_remoter_bits_num = 0; 
			g_invalid_keycount++;                 //防止溢出            
			if(g_invalid_keycount > 65000)            
			{                
				g_invalid_keycount = 1;	           
			} 
			g_protocol_status = REMOTER_IDLE;
			break;
		case REMOTER_CATCH_DATA:
			iCount = 0;
			
			if((dalt_time > REMOTER_LOW_CODE_TIME_MIN) && (dalt_time <= REMOTER_LOW_CODE_TIME_MAX))
			{
				irda_addbit(0);
				break;
			}
			if((dalt_time > REMOTER_HIGH_CODE_TIME_MIN) && (dalt_time <= REMOTER_HIGH_CODE_TIME_MAX))
			{
				irda_addbit(1);
				break;
			}

			if(g_remoter_key_flag == 1)
			{
				/*上报该键码按下*/		
				irda_rmtctrl_report_keyevent(g_remoter_bits);
				g_remoter_key_flag = 0;
			}

			g_remoter_bits = 0;            
			g_remoter_bits_num = 0;            
			g_invalid_keycount++;                              
			if(g_invalid_keycount > 65000)                           
				g_invalid_keycount = 1;	    
			
			g_protocol_status = REMOTER_IDLE;
			break;

		case REMOTER_CATCH_REPEAT_START:
			g_protocol_status = REMOTER_CATCH_REPEAT_END ;
			break;
		case REMOTER_CATCH_REPEAT_END:
			if((dalt_time > REMOTER_REPEAT_CODE_TIME_MIN) && (dalt_time <= REMOTER_REPEAT_CODE_TIME_MAX))
			{
				iCount++;                //防止溢出                
				if(iCount > 65000)                
				{                    
					iCount = REPEAT_LOADER_CODE_TIMES + 1;                
				}                                

				if (iCount % REPEAT_LOADER_CODE_TIMES == 0)                
				{                                             
					irda_rmtctrl_report_keyevent(g_remoter_bits);					                                   
				}                
				g_protocol_status = REMOTER_CATCH_REPEAT_START ;                
				break;
			}

			if((dalt_time > REMOTER_START_CODE_TIME_MIN) &&(dalt_time  < REMOTER_START_CODE_TIME_MAX))            
			{                 
				g_remoter_bits = 0;                
				g_remoter_bits_num = 0;                
				g_protocol_status = REMOTER_CATCH_DATA ;                                               
				break;            
			}        

			if(g_remoter_key_flag == 1)            
			{                
				irda_rmtctrl_report_keyevent(g_remoter_bits);
				g_remoter_key_flag = 0;
			}      
			
			g_invalid_keycount++;  //防止溢出           
			if(g_invalid_keycount > 65000 )          
			{               
				g_invalid_keycount = 1;	      
			}                       
			g_protocol_status = REMOTER_IDLE ;
			
			break;
		default:
			break;
	}
	
irda_irq_exit:
	spin_unlock_irq(&dd->pedestal_lock);
	enable_irq(dd->pedestal_dect_irq);
	return 0;
}


int pedestal_interface_init(void *dev_id)
{
	struct pedestal_driver_data *dd = dev_id;
	int rc = 0, key_count = 0;
	int key_num = sizeof(g_irdaCmd2hidKey_map)/sizeof(struct pedestalkey_2_hidkey);
	unsigned long irq_type = 0;
	PEDESTAL_INFO( "---------------------enter pedestal_interface_init \n"); 

	rc = pedestal_gpio_setup(PEDESTAL_TYPE_INTERFACE);
	if(rc)
	{
		PEDESTAL_ERR("pedestal_interface_init: pedestal interface request gpio failed. err code = %d\n",rc);
		goto interface_init_exit;
	}
	
	//request gpio irq
	#if(IRDA_GPIO_INVERTED == 1)
	irq_type = IRQF_TRIGGER_FALLING;
	#else
	irq_type = IRQF_TRIGGER_RISING;
	#endif

	rc = pedestal_irq_request(PEDESTAL_TYPE_INTERFACE,dd->pedestal_gpio_irq,
		       &pedestal_interface_interrupt,
		       irq_type,
		       PEDESTAL_IRDA_NAME,
		       dd);
	if (rc) 
	{
		PEDESTAL_ERR("pedestal: irda rmtctrl init request irq failed. err code = %d\n",rc);
		goto interface_gpio_err;
	}

	for(key_count = 0; key_count < key_num; key_count++)
	{
		input_set_capability(dd->pedestal_dev, EV_KEY, g_irdaCmd2hidKey_map[key_count].hid_key);
	}

	get_sys_time_by_us(&g_pre_time);
	g_protocol_status = REMOTER_IDLE;

	PEDESTAL_INFO( "---------------------enter pedestal_interface_init succeed!\n"); 
	return 0;
interface_gpio_err:
	pedestal_gpio_release(PEDESTAL_TYPE_INTERFACE);
interface_init_exit:
	PEDESTAL_INFO( "---------------------enter pedestal_interface_init failed, err code = %d!\n",rc); 
	return rc;
}

void pedestal_interface_exit(void *dev_id)
{
	struct pedestal_driver_data *dd = dev_id;
	PEDESTAL_INFO( "---------------------enter pedestal_interface_exit \n"); 

	pedestal_irq_free(PEDESTAL_TYPE_INTERFACE,dd->pedestal_gpio_irq, dd);
	pedestal_gpio_release(PEDESTAL_TYPE_INTERFACE);
}


#ifdef CONFIG_PM
int pedestal_interface_suspend(void *dev_id, pm_message_t state)
{
	int rc = 0;
//	struct pedestal_driver_data *dd = dev_id;
//	PEDESTAL_INFO( "---------------------enter pedestal_interface_suspend \n"); 
	return rc;
}

int pedestal_interface_resume(void *dev_id)
{
	int rc = 0;
//	struct pedestal_driver_data *dd = dev_id;
//	PEDESTAL_INFO( "---------------------enter pedestal_interface_resume \n"); 
	return rc;
}
#endif	
