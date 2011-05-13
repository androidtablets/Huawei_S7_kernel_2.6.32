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
 * Sound Box Pedestal Driver
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
#include "sound_box.h"

irqreturn_t pedestal_soundbox_interrupt(int irq, void *dev_id);
void pedestal_soundbox_work_f(struct work_struct *work);

struct work_struct pedestal_soundbox_work;
static unsigned char g_sound_box_status = 0;

void get_soundbox_status(struct soundbox_staus_info* pstatusinfo )
{
	pstatusinfo->soundbox_plugflag = (g_sound_box_status&0x08)>>3;
	pstatusinfo->soundbox_status = g_sound_box_status & 0x07;
	PEDESTAL_INFO( "---------get_soundbox_status plugflag = 0x%02x, status = 0x%02x \n",pstatusinfo->soundbox_plugflag,pstatusinfo->soundbox_status); 
}

#define PEDESTAL_TIME_YEAR			0x80
#define PEDESTAL_TIME_MONTH			0x81
#define PEDESTAL_TIME_DATE			0x82
#define PEDESTAL_TIME_HOUR			0x83
#define PEDESTAL_TIME_MIN			0x84
#define PEDESTAL_TIME_SEC			0x85
int set_soundbox_time(struct soundbox_time_info timeinfo)
{
	int rc = 0;
//	mdelay(500);
	rc = pedestal_i2c_write_byte_data(g_pdriverdata->pedestal_client,PEDESTAL_TIME_YEAR,timeinfo.year);
	if(rc < 0)
		return rc;
	rc = pedestal_i2c_write_byte_data(g_pdriverdata->pedestal_client,PEDESTAL_TIME_MONTH,timeinfo.month);
	if(rc < 0)
		return rc;
	rc = pedestal_i2c_write_byte_data(g_pdriverdata->pedestal_client,PEDESTAL_TIME_DATE,timeinfo.day);
	if(rc < 0)
		return rc;
	rc = pedestal_i2c_write_byte_data(g_pdriverdata->pedestal_client,PEDESTAL_TIME_HOUR,timeinfo.hour);
	if(rc < 0)
		return rc;
	rc = pedestal_i2c_write_byte_data(g_pdriverdata->pedestal_client,PEDESTAL_TIME_MIN,timeinfo.min);
	if(rc < 0)
		return rc;
	rc = pedestal_i2c_write_byte_data(g_pdriverdata->pedestal_client,PEDESTAL_TIME_SEC,timeinfo.sec);
	if(rc < 0)
		return rc;

	PEDESTAL_INFO( "---------set_soundbox_time succeed! year = 0%d, month = 0%d, day = 0%d, hour = 0%d, min = 0%d, sec = 0%d \n", \
		timeinfo.year,timeinfo.month,timeinfo.day,timeinfo.hour,timeinfo.min,timeinfo.sec); 
	return 0;
}
static struct pedestalkey_2_hidkey g_soundBoxKey2hidKey_map[] =
{
	//begin sound box pedestal remote control key map
	{0x0a,KEY_UNKNOWN},	// sound box power
	{0x0b,KEY_UNKNOWN},	// hid 
	{0x0d,KEY_UNKNOWN},	// Radio/Line in

	{0x0c,KEY_POWER},	//sleep
	{0x10,KEY_UP},	//up
	{0x1c,KEY_UNKNOWN},	//hdmi/av

	{0x04,KEY_LEFT},	//left
	{0x08,BTN_MOUSE/*KEY_OK*//*KEY_PLAY*/},	//play/pause
	{0x07,KEY_RIGHT},	//right
	
	{0x03,KEY_PREVIOUSSONG},	//previous song
	{0x1f,KEY_DOWN},	//down
	{0x06,KEY_NEXTSONG},	//next song

	{0x02,KEY_HOME},	//home
	{0x1e,KEY_MENU},	//menu
	{0x05,KEY_BACK},	//back

	{0x01,KEY_VOLUMEDOWN},	//vol -
	//{0x09,KEY_UNKNOWN},		//unknow
	{0x00,KEY_VOLUMEUP},	//vol +

	//{0x0e,KEY_UNKNOWN},	//unknow
	//{0x1d,KEY_UNKNOWN}, //unknow
	//{0x0f,KEY_UNKNOWN},	//unknow
	//end sound box pedestal remote control key map
};

static int translate_soundboxkey2hidkey(unsigned int soundbox_key, unsigned int* hid_key)
{
	int rc = -1;
	int i = 0;
	int map_len = sizeof(g_soundBoxKey2hidKey_map)/sizeof(struct pedestalkey_2_hidkey);
	*hid_key = KEY_UNKNOWN;

	PEDESTAL_INFO( "---------translate_soundboxkey2hidkey irda cmd = 0x%x, map_len=%d \n",soundbox_key,map_len); 
	for(i = 0; i < map_len; i++)
	{
		if(soundbox_key == g_soundBoxKey2hidKey_map[i].pedestal_key)
		{
			*hid_key = g_soundBoxKey2hidKey_map[i].hid_key;
			break;
		}
	}

	if(i < map_len)
		rc = 0;

	return rc;
}


void soundbox_report_keyevent(unsigned int soundbox_key)
{
	unsigned int hid_key = 0;
	PEDESTAL_INFO( "---------------------enter irda_rmtctrl_report_keyevent cmd = 0x%02x \n",soundbox_key); 

	if(g_pdriverdata->pedestal_dev)
	{
		if(translate_soundboxkey2hidkey(soundbox_key,&hid_key) == 0)
		{
			PEDESTAL_INFO( "---------------------report key = %d \n",hid_key); 
			input_report_key(g_pdriverdata->pedestal_dev,hid_key , 1);
			input_report_key(g_pdriverdata->pedestal_dev,hid_key , 0);	
			input_sync(g_pdriverdata->pedestal_dev);
		}
		else
			PEDESTAL_ERR("----------------translate sound box key to hid key err!");
	}
}

int pedestal_soundbox_init(void *dev_id)
{
	struct pedestal_driver_data *dd = dev_id;
	int rc = 0, key_count = 0;
	int key_num = sizeof(g_soundBoxKey2hidKey_map)/sizeof(struct pedestalkey_2_hidkey);
	PEDESTAL_INFO( "---------------------enter pedestal_soundbox_init \n"); 

	rc = pedestal_gpio_setup(PEDESTAL_TYPE_SOUNDBOX);
	if(rc)
	{
		PEDESTAL_ERR("pedestal_interface_init: pedestal interface request gpio failed. err code = %d\n",rc);
		goto soundbox_init_exit;
	}

	PEDESTAL_INFO( "---------------------enter pedestal_soundbox_init init work\n"); 

	INIT_WORK(&pedestal_soundbox_work, pedestal_soundbox_work_f);
	
	PEDESTAL_INFO( "---------------------enter pedestal_soundbox_init request gpio\n"); 
	rc = pedestal_irq_request(PEDESTAL_TYPE_SOUNDBOX,dd->pedestal_gpio_irq,
		       &pedestal_soundbox_interrupt,
		       IRQF_TRIGGER_FALLING,
		       PEDESTAL_SOUNDBOX_NAME,
		       dd);
	if (rc) 
	{
		PEDESTAL_ERR("pedestal: irda rmtctrl init request irq failed. err code = %d\n",rc);
		goto soundbox_gpio_err;
	}
		PEDESTAL_INFO( "---------------------enter pedestal_soundbox_init request irq\n"); 

	for(key_count = 0; key_count < key_num; key_count++)
	{
		input_set_capability(dd->pedestal_dev, EV_KEY, g_soundBoxKey2hidKey_map[key_count].hid_key);
	}

	
	PEDESTAL_INFO( "---------------------enter pedestal_soundbox_init succeed!\n"); 
	return 0;
soundbox_gpio_err:
	pedestal_gpio_release(PEDESTAL_TYPE_SOUNDBOX);
soundbox_init_exit:
	PEDESTAL_INFO( "---------------------enter pedestal_soundbox_init failed, err code = %d!\n",rc); 
	return rc;
	
}

void pedestal_soundbox_exit(void *dev_id)
{
	struct pedestal_driver_data *dd = dev_id;
	PEDESTAL_INFO( "---------------------enter pedestal_soundbox_exit \n"); 

	cancel_work_sync(&pedestal_soundbox_work);
	pedestal_irq_free(PEDESTAL_TYPE_SOUNDBOX,dd->pedestal_gpio_irq, dd);
	pedestal_gpio_release(PEDESTAL_TYPE_SOUNDBOX);
}

void pedestal_soundbox_work_f(struct work_struct *work)
{
	int pedestal_status = 0, soundbox_key = 0;
	
	pedestal_status = pedestal_i2c_read_byte_data(g_pdriverdata->pedestal_client, PEDESTAL_STATUS_REG); 
	soundbox_key = pedestal_i2c_read_byte_data(g_pdriverdata->pedestal_client, PEDESTAL_KEYVALUE_REG); 

	if((pedestal_status >= 0) && (soundbox_key >= 0))
	{
		g_sound_box_status = (pedestal_status & 0x000000f0)>>4 ;
		soundbox_report_keyevent(soundbox_key);

    	{
			struct soundbox_staus_info statusinfo = {0};
			get_soundbox_status(&statusinfo);
    	}
		{
			int temprc = 0;
			struct soundbox_time_info temp_time;
			temp_time.year = 0x10;
			temp_time.month= 0x03;
			temp_time.day= 0x17;
			temp_time.hour= 0x13;
			temp_time.min= 0x32;
			temp_time.sec= 0x45;

			temprc = set_soundbox_time(temp_time);
			if(temprc < 0)
				PEDESTAL_ERR( "---------------------set_soundbox_time failed %d!\n",temprc);
		}

		
	}
	else
	{
		g_sound_box_status = 0;
		PEDESTAL_ERR( "---------------------pedestal_soundbox_exit read i2c reg err!\n");
	}
	
}

irqreturn_t pedestal_soundbox_interrupt(int irq, void *dev_id)
{
	irqreturn_t rc = 0;
	struct pedestal_driver_data *dd = dev_id;
	disable_irq(dd->pedestal_gpio_irq);
	schedule_work(&pedestal_soundbox_work);
	enable_irq(dd->pedestal_gpio_irq);
	return rc;
}



#ifdef CONFIG_PM
int pedestal_soundbox_suspend(void *dev_id, pm_message_t state)
{
	int rc = 0;
	struct pedestal_driver_data *dd = dev_id;
	disable_irq(dd->pedestal_gpio_irq);
	cancel_work_sync(&pedestal_soundbox_work);
	return rc;
}

int pedestal_soundbox_resume(void *dev_id)
{
	int rc = 0;
	struct pedestal_driver_data *dd = dev_id;
	enable_irq(dd->pedestal_gpio_irq);
	return rc;
}
#endif
