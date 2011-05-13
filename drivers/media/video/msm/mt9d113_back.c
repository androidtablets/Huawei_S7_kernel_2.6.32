/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include "mt9d113_back.h"

/* Micron MT9D113 Registers and their values */
/* Sensor Core Registers */
#define  REG_MT9D113_BACK_MODEL_ID 0x0000
#define  MT9D113_BACK_MODEL_ID     0x2580

/*  SOC Registers Page 1  */
#define  REG_MT9D113_BACK_SENSOR_RESET     0x0014
#define  REG_MT9D113__BACK_STANDBY_CONTROL  0x0018
#define  REG_MT9D113__BACK_MCU_BOOT         0x001a

unsigned int gCameraBackNums_mt9d113 = 0;

#define MT9D113_BACK_I2C_RETRY_TIMES        3

struct mt9d113_back_work {
	struct work_struct work;
};

static struct  mt9d113_back_work *mt9d113_back_sensorw;
static struct  i2c_client *mt9d113_back_client;

struct mt9d113_back_ctrl {
	const struct msm_camera_sensor_info *sensordata;
};


static struct mt9d113_back_ctrl *mt9d113_back_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(mt9d113_back_wait_queue);
DECLARE_MUTEX(mt9d113_back_sem);

/*=============================================================*/

static int mt9d113_back_reset(const struct msm_camera_sensor_info *dev)
{
	int rc = 0;
	
	rc = gpio_request(dev->sensor_pwd, "mt9d113_back");
	if (!rc || rc == -EBUSY)
		gpio_direction_output(dev->sensor_pwd,1);
	else 
		return rc;    	
	rc = gpio_request(dev->sensor_reset, "mt9d113_back");
	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 0);
	}
	else
		return rc;
	 mdelay(5);
	 /*power down*/
      	dev->vreg_enable_func(0);
	mdelay(5);	
	if (dev->vreg_enable_func)
		dev->vreg_enable_func(1);
	mdelay(20);
	
	rc=gpio_direction_output(dev->sensor_pwd, 0);

	mdelay(20);
    
      	
    	rc = gpio_direction_output(dev->sensor_reset, 0);
	mdelay(20);    		
	rc = gpio_direction_output(dev->sensor_reset, 1);	
	mdelay(1);

	return rc;
}

#ifndef CONFIG_MSM_CAMERA_POWER_OFF_WHEN_LP
static int mt9d113_back_reset_init(const struct msm_camera_sensor_info *dev)
{	
       int rc = 0;
	gpio_free(dev->sensor_pwd);
	rc=gpio_request(dev->sensor_pwd, "mt9d113_back");
	
	if (!rc || rc == -EBUSY)
		gpio_direction_output(dev->sensor_pwd, 0);
	else 
		return rc;
        /*
         if (dev->vreg_enable_func)
         dev->vreg_enable_func(1);
         mdelay(20);
	*/
	
       gpio_free(dev->sensor_reset);
	rc=gpio_request(dev->sensor_reset, "mt9d113_back");
	
	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 0);
	}
	else
		return rc;
       mdelay(5);    

       rc = gpio_direction_output(dev->sensor_reset, 1);
	mdelay(5);
	return rc;
}
#endif

static int mt9d113_back_sensor_init_done(const struct msm_camera_sensor_info *data)
{


#ifdef CONFIG_MSM_CAMERA_POWER_OFF_WHEN_LP
       CDBG("CONFIG_MSM_CAMERA_POWER_OFF_WHEN_LP  defined\n");
       gpio_direction_output(data->sensor_reset, 0);
	gpio_free(data->sensor_reset);

	gpio_direction_output(data->sensor_pwd, 1);
	gpio_free(data->sensor_pwd);
    
#if 0
	if (data->vreg_enable_func)
	{
	data->vreg_enable_func(0);
	}		
#endif    

#else
       CDBG("CONFIG_MSM_CAMERA_POWER_OFF_WHEN_LP  not defined\n");

	gpio_direction_output(data->sensor_reset, 1);
	//	gpio_free(data->sensor_reset);

	gpio_direction_output(data->sensor_pwd, 1);
	//	gpio_free(data->sensor_pwd);


	/* if (data->vreg_enable_func)
		{
		data->vreg_enable_func(0);
		}
	*/
#endif

	return 0;
}

static int32_t mt9d113_back_i2c_txdata(unsigned short saddr,
	unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};

	if (i2c_transfer(mt9d113_back_client->adapter, msg, 1) < 0) {
		CDBG("mt9d113_back_i2c_txdata faild\n");
		return -EIO;
	}

	return 0;
}

static int32_t mt9d113_back_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum mt9d113_back_width_t width)
{
	int32_t rc = -EIO;
	unsigned char buf[4];
	
	int32_t i = 0;

	memset(buf, 0, sizeof(buf));
	switch (width) {
	case WORD_LEN: {
		buf[0] = (waddr & 0xFF00)>>8;
		buf[1] = (waddr & 0x00FF);
		buf[2] = (wdata & 0xFF00)>>8;
		buf[3] = (wdata & 0x00FF);
	for(i = 0; i < MT9D113_BACK_I2C_RETRY_TIMES; i++)
	{
	  rc = mt9d113_back_i2c_txdata(saddr, buf, 4);
	  if(0 <= rc)
	    return 0;
	}

    CDBG("i2c_write_w failed, addr = 0x%x, val = 0x%x!\n", waddr, wdata);
    return -EIO;


	}
		break;

	case BYTE_LEN: {
		buf[0] = waddr;
		buf[1] = wdata;
		rc = mt9d113_back_i2c_txdata(saddr, buf, 2);
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		CDBG(
		"i2c_write failed, addr = 0x%x, val = 0x%x!\n",
		waddr, wdata);

	return rc;
}



static int mt9d113_back_i2c_rxdata(unsigned short saddr,
	unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
	{
		.addr   = saddr,
		.flags = 0,
		.len   = 2,
		.buf   = rxdata,
	},
	{
		.addr   = saddr,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

	if (i2c_transfer(mt9d113_back_client->adapter, msgs, 2) < 0) {
		CDBG("mt9d113_back_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t mt9d113_back_i2c_read(unsigned short   saddr,
	unsigned short raddr, unsigned short *rdata, enum mt9d113_back_width_t width)
{
	int32_t rc = 0;
	int32_t i = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	switch (width) {
	case WORD_LEN: {
		buf[0] = (raddr & 0xFF00)>>8;
		buf[1] = (raddr & 0x00FF);
    	for(i = 0; i < MT9D113_BACK_I2C_RETRY_TIMES; i++)
    	{
    	  rc = mt9d113_back_i2c_rxdata(saddr, buf, 2);
    	  if(rc >= 0)
    	  {
            *rdata = buf[0] << 8 | buf[1];
    	    return 0;
    	  }
    	}

        CDBG("mt9d113_back_i2c_read failed, raddr = 0x%x\n", raddr);
        return -EIO;
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		CDBG("mt9d113_back_i2c_read failed!\n");

	return rc;
}


static long mt9d113_back_reg_init(void)
{
	int32_t array_length;
	int32_t i;
	struct register_address_value_pair const * reg_init;
	long rc;

	array_length=mt9d113_back_regs.prev_snap_reg_init_size;
	reg_init=&(mt9d113_back_regs.prev_snap_reg_init[0]);

	for (i = 0; i < array_length; i++) {

		if(0==reg_init[i].register_address)
			{
			mdelay(reg_init[i].register_value);
			}
		else
			{
			rc=mt9d113_back_i2c_write(mt9d113_back_client->addr,
		  		reg_init[i].register_address,reg_init[i].register_value,WORD_LEN);
			if (rc < 0)
				return rc;
			}
	}

	return 0;
}

static long mt9d113_back_set_awb(void)
{
	int32_t array_length;
	int32_t i;
	struct register_address_value_pair const * reg_init;
	long rc;

	array_length=mt9d113_back_regs.byd_sunny_awb_reg_settings_size;
	        reg_init=&(mt9d113_back_regs.byd_sunny_awb_reg_settings[0]); 
	for (i = 0; i < array_length; i++) {

		rc=mt9d113_back_i2c_write(mt9d113_back_client->addr,
		  	reg_init[i].register_address,reg_init[i].register_value,WORD_LEN);
		if (rc < 0)
			return rc;
	}

	return 0;
}


static long mt9d113_back_set_lens_shading(void)
{
	int32_t array_length;
	int32_t i;
	struct register_address_value_pair const * reg_init;
	long rc;

       CDBG("mt9d113_back_set_lens_shading\n");
       array_length=mt9d113_back_regs.sunny_lens_reg_settings_size;
       reg_init=&(mt9d113_back_regs.sunny_lens_reg_settings[0]); 

        
	for (i = 0; i < array_length; i++) {

		rc=mt9d113_back_i2c_write(mt9d113_back_client->addr,
	  		reg_init[i].register_address,reg_init[i].register_value,WORD_LEN);
		if (rc < 0)
			return rc;

	}

	return 0;
}


static long mt9d113_back_set_sensor_mode(int mode)
{

	long rc = 0;
	uint16_t camera_mode;
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x098c, 0xa115,WORD_LEN);
		if (rc < 0)
			break;
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x0990, 0x0000,WORD_LEN);
		if (rc < 0)
			break;
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x098c, 0xa103,WORD_LEN);
		if (rc < 0)
			break;
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x0990, 0x0001,WORD_LEN);
		if (rc < 0)
			break;
		mdelay(200);
		break;

	case SENSOR_SNAPSHOT_MODE:
		/* Switch to lower fps for Snapshot */
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x098c, 0xa115,WORD_LEN);
		if (rc < 0)
			break;
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x0990, 0x0002,WORD_LEN);
		if (rc < 0)
			break;
		msleep(40);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x098c, 0xa103,WORD_LEN);
		if (rc < 0)
			break;
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x0990, 0x0002,WORD_LEN);
		if (rc < 0)
			break;
              mdelay(200);

	       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x098C, 0xA104 ,WORD_LEN);
		if (rc < 0)
			break;
              rc = mt9d113_back_i2c_read(mt9d113_back_client->addr,
		       0x0990, &camera_mode, WORD_LEN);
	       if (rc < 0)
			break;
	       if(camera_mode!=7){
		   	
		   	CDBG("seq_state SNAPSHOT not ready, mdelay 50 ms and send seq_cmd again-----  \n");
                     mdelay(50);  			  
		       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			       0x098c, 0xa115,WORD_LEN);
		       if (rc < 0)
			break;
		       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			       0x0990, 0x0002,WORD_LEN);
		       if (rc < 0)
			       break;
			rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			       0x098c, 0xa103,WORD_LEN);
		       if (rc < 0)
			       break;
		       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			       0x0990, 0x0002,WORD_LEN);
		       if (rc < 0)
			       break;
		       mdelay(100);
			 rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x098C, 0xA104 ,WORD_LEN);
		if (rc < 0)
			break;
              rc = mt9d113_back_i2c_read(mt9d113_back_client->addr,
		       0x0990, &camera_mode, WORD_LEN);
	       CDBG("second seq_cmd camera_mode=%d",camera_mode);
		
		 }
   
		break;

	default:
		return -EFAULT;
	}

	return 0;
}

static long mt9d113_back_set_effect(int mode,int effect)
{

	long rc = 0;

	CDBG("mt9d113_back_set_effect  effect=%d\n",effect);

	switch (effect) {
	case CAMERA_EFFECT_OFF: {
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0x2759, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x6440, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0x275b, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x6440, WORD_LEN);

		if (rc < 0)
			return rc;
	}
			break;

	case CAMERA_EFFECT_MONO: {

		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0x2759, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x6441, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0x275b, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x6441, WORD_LEN);


		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_NEGATIVE: {
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0x2759, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x6443, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0x275b, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x6443, WORD_LEN);
	

		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_SOLARIZE: {
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0x2759, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x6445, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0x275b, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x6445, WORD_LEN);


		if (rc < 0)
			return rc;
	}
		break;

	case CAMERA_EFFECT_SEPIA: {
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0x2759, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x6442, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0x275b, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x6442, WORD_LEN);


		if (rc < 0)
			return rc;
	}
		break;

	default: {
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0x2759, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x6440, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0x275b, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x6440, WORD_LEN);

		if (rc < 0)
			return rc;
		break;

		return -EINVAL;
	}
	}

  /* Refresh Sequencer */
	rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
		0x98C, 0xa103, WORD_LEN);
	rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
		0x990, 0x0006, WORD_LEN);

	return rc;
}

static long mt9d113_back_set_whitebalance(	int mode,int8_t wb)
{

	long rc = 0;

	CDBG("mt9d113_back_set_whitebalance  wb=%d\n",wb);



	switch ((enum config3a_wb_t)wb) {
	case CAMERA_WB_AUTO: {
     		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa11f, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0001, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa103, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0006, WORD_LEN);
                mdelay(500);


	}
			break;

	case CAMERA_WB_INCANDESCENT: 
		{

            rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa11f, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0000, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa34e, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0090, WORD_LEN);
              rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa34f, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0080, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa353, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0070, WORD_LEN);

        	rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa103, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0006, WORD_LEN);
                mdelay(500);
	}
		break;

 	case CAMERA_WB_CLOUDY_DAYLIGHT: {

             rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa11f, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0000, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa34e, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0088, WORD_LEN);
              rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa34f, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0080, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa350, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0080, WORD_LEN);
     		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa353, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0040, WORD_LEN);

        	rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa103, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0006, WORD_LEN);
                mdelay(500);
	}
		break;

	case CAMERA_WB_DAYLIGHT: {
		 rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa11f, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0001, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa103, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0006, WORD_LEN);
                mdelay(500);



	}
		break;
        case CAMERA_WB_FLUORESCENT: {
             rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa11f, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0000, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa34e, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x00bd, WORD_LEN);
              rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa34f, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0080, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa350, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0069, WORD_LEN);
     		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa353, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x007f, WORD_LEN);

        	rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x98C, 0xa103, WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x990, 0x0006, WORD_LEN);
                mdelay(500);

	}
		break;



	case CAMERA_WB_TWILIGHT: {

	}
		break;


	case CAMERA_WB_SHADE:
	default: {

		break;

		return -EINVAL;
	}
	}



	return rc;
}
//lijuan add end

static long mt9d113_back_set_mirror(	int mode,int8_t mirror)
{

	long rc = 0;

	//printk(KERN_ERR "mt9d113_back_set_mirror  mirror=%d\n",mirror);
    
	switch ((enum mirror_t)mirror) {
	case CAMERA_MIRROR_NORMAL: {

		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x098c, 0x2717,WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x0990, 0x46D,WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x098c, 0x272D,WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x0990, 0x0025,WORD_LEN);
              mdelay(10);

	}
		break;

	case CAMERA_MIRROR_ABNORMAL: {

		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x098c, 0x2717,WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x0990, 0x46C,WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x098c, 0x272D,WORD_LEN);
		rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,
			0x0990, 0x0024,WORD_LEN);
              mdelay(10);
	}
		break;

	default: {

		break;

		return -EINVAL;
	}
	}
	
	return rc;
}
static int mt9d113_back_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	uint16_t model_id = 0;
	int rc = 0;

	CDBG("init entry \n");
	rc = mt9d113_back_reset(data);
	if (rc < 0) {
		CDBG("reset failed!\n");
		goto init_probe_fail;
	}
	

	/* Micron suggested Power up block End */
	/* Read the Model ID of the sensor */
	rc = mt9d113_back_i2c_read(mt9d113_back_client->addr,
		REG_MT9D113_BACK_MODEL_ID, &model_id, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;

	CDBG("mt9d113_back model_id = 0x%x\n", model_id);
	/* Check if it matches it with the value in Datasheet */
	if (model_id != MT9D113_BACK_MODEL_ID) {
		rc = -EINVAL;
		goto init_probe_fail;
	}

	 
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x001A, 0x0050, WORD_LEN);
	mdelay(10);
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0014, 0x21F9, WORD_LEN);

       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0010, 0x011c, WORD_LEN);

	rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0012, 0x21FB, WORD_LEN);
	
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0014, 0x21FB, WORD_LEN);
	
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0014, 0x20FB, WORD_LEN);

	mdelay(10);
	
       //rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0014, 0x20FA, WORD_LEN);
	
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0018, 0x402D, WORD_LEN);

       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0018, 0x402C, WORD_LEN);
	
	mdelay(10);
		


       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x001e, 0x700, WORD_LEN);
	mdelay(1);
	
	rc = mt9d113_back_reg_init();
	if (rc < 0)
		goto init_probe_fail;

	rc = mt9d113_back_set_lens_shading();
	if (rc < 0)
		goto init_probe_fail;

    	rc = mt9d113_back_set_awb();
	if (rc < 0)
		goto init_probe_fail;

        rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0018, 0x0028, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;
       mdelay(10);
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x098c, 0xa103, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0990, 0x0006, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;
        mdelay(10);
        

	return rc;


init_probe_fail:
	CDBG("mt9d113_back init probe fail \n");
	mt9d113_back_sensor_init_done(data);
	return rc;
}

#ifdef CONFIG_MSM_CAMERA_POWER_OFF_WHEN_LP
static int mt9d113_back_sensor_init_probe_init_power_off(const struct msm_camera_sensor_info *data)
{
	uint16_t model_id = 0;
	int rc = 0;

	CDBG("mt9d113_back_sensor_init_probe_init_power_off---------init entry \n");
	rc = mt9d113_back_reset(data);
	if (rc < 0) {
		CDBG("reset failed!\n");
		goto init_probe_fail;
	}

	rc = mt9d113_back_i2c_read(mt9d113_back_client->addr,
		REG_MT9D113_BACK_MODEL_ID, &model_id, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;

	CDBG("mt9d113_back model_id = 0x%x\n", model_id);
	/* Check if it matches it with the value in Datasheet */
	if (model_id != MT9D113_BACK_MODEL_ID) {
		rc = -EINVAL;
		goto init_probe_fail;
	}

	rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x001A, 0x0050, WORD_LEN);
	mdelay(10);
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0014, 0x21F9, WORD_LEN);

       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0010, 0x011c, WORD_LEN);

	rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0012, 0x21FB, WORD_LEN);
	
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0014, 0x21FB, WORD_LEN);
	
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0014, 0x20FB, WORD_LEN);

	mdelay(10);
	
       //rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0014, 0x20FA, WORD_LEN);

       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0018, 0x402D, WORD_LEN);

       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0018, 0x402C, WORD_LEN);
	
	mdelay(10);
		
	
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x001e, 0x700, WORD_LEN);
	mdelay(1);


	rc = mt9d113_back_reg_init();
	if (rc < 0)
		goto init_probe_fail;

	rc = mt9d113_back_set_lens_shading();
	if (rc < 0)
		goto init_probe_fail;

    	rc = mt9d113_back_set_awb();
	if (rc < 0)
		goto init_probe_fail;

        rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0018, 0x0028, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;
       mdelay(10);
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x098c, 0xa103, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0990, 0x0006, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;
        mdelay(10);
        

	return rc;


init_probe_fail:
	CDBG("mt9d113_back init probe fail \n");
	mt9d113_back_sensor_init_done(data);
	return rc;
}
#endif


#ifndef CONFIG_MSM_CAMERA_POWER_OFF_WHEN_LP
static int mt9d113_back_sensor_init_probe_init_power_on(const struct msm_camera_sensor_info *data)
{
	uint16_t model_id = 0;
	int rc = 0;

	rc = mt9d113_back_reset_init(data);
	if (rc < 0) {
		CDBG("reset failed!\n");
		goto init_probe_fail;
	}
	
	/* Micron suggested Power up block End */
	/* Read the Model ID of the sensor */
	rc = mt9d113_back_i2c_read(mt9d113_back_client->addr,
		REG_MT9D113_BACK_MODEL_ID, &model_id, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;

	/* Check if it matches it with the value in Datasheet */
	if (model_id != MT9D113_BACK_MODEL_ID) {
		rc = -EINVAL;
		goto init_probe_fail;
	}
    

	rc = mt9d113_back_reg_init();
	if (rc < 0)
		goto init_probe_fail;

 /*      
       mt9d113_back_i2c_read(mt9d113_back_client->addr,
		0x0024, &manufact_id, WORD_LEN);      
	CDBG("mt9d113_back manufact_id = 0x%x\n", manufact_id);
*/

	rc = mt9d113_back_set_lens_shading();
	if (rc < 0)
		goto init_probe_fail;

    	rc = mt9d113_back_set_awb();
	if (rc < 0)
		goto init_probe_fail;

        rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0018, 0x0028, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;
       mdelay(10);
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x098c, 0xa103, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;
       rc = mt9d113_back_i2c_write(mt9d113_back_client->addr,0x0990, 0x0006, WORD_LEN);
	if (rc < 0)
		goto init_probe_fail;
        mdelay(10);
        

	return rc;


init_probe_fail:
	CDBG("mt9d113_back init probe fail \n");
	mt9d113_back_sensor_init_done(data);
	return rc;
}
#endif

int mt9d113_back_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	mt9d113_back_ctrl = kzalloc(sizeof(struct mt9d113_back_ctrl), GFP_KERNEL);
	if (!mt9d113_back_ctrl) {
		CDBG("mt9d113_back_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		mt9d113_back_ctrl->sensordata = data;

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);//
	// 20101002 xionglei end @end: qiuhanning merge to android2.2
	mdelay(5);

	msm_camio_camif_pad_reg_reset();
	
#ifdef CONFIG_MSM_CAMERA_POWER_OFF_WHEN_LP
	rc = mt9d113_back_sensor_init_probe_init_power_off(data);
#else
	rc = mt9d113_back_sensor_init_probe_init_power_on(data);
#endif

	if (rc < 0) {
		CDBG("mt9d113_back_sensor_init failed!\n");
		goto init_fail;
	}

init_done:
	return rc;

init_fail:
	kfree(mt9d113_back_ctrl);
	return rc;
}

static int mt9d113_back_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&mt9d113_back_wait_queue);
	return 0;
}

int mt9d113_back_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	if (copy_from_user(
				&cfg_data,
				(void *)argp,
				sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	/* down(&mt9d113_back_sem); */

	CDBG("mt9d113_back_ioctl, cfgtype = %d, mode = %d\n",
		cfg_data.cfgtype, cfg_data.mode);

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = mt9d113_back_set_sensor_mode(
					cfg_data.mode);
		break;

	case CFG_SET_EFFECT:
		rc = mt9d113_back_set_effect(
					cfg_data.mode,
					cfg_data.cfg.effect);
		break;
	case CFG_SET_WB:
		rc=mt9d113_back_set_whitebalance(
					cfg_data.mode,
					cfg_data.cfg.whitebalance);
		break;
	case CFG_SET_MIRROR:
		rc=mt9d113_back_set_mirror(
					cfg_data.mode,
					cfg_data.cfg.mirror);
		break;
	default:
		rc = -EINVAL;
		break;
	}

	/* up(&mt9d113_back_sem); */

	return rc;
}

int mt9d113_back_sensor_release(void)
{
	int rc = 0;

	/* down(&mt9d113_back_sem); */
	mt9d113_back_sensor_init_done(mt9d113_back_ctrl->sensordata);
	kfree(mt9d113_back_ctrl);
	/* up(&mt9d113_back_sem); */

	return rc;
}

static int mt9d113_back_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	mt9d113_back_sensorw =
		kzalloc(sizeof(struct mt9d113_back_work), GFP_KERNEL);

	if (!mt9d113_back_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, mt9d113_back_sensorw);
	mt9d113_back_init_client(client);
	mt9d113_back_client = client;
	mt9d113_back_client->addr = mt9d113_back_client->addr >>1;

	CDBG("mt9d113_back_probe successed!\n");

	return 0;

probe_failure:
	kfree(mt9d113_back_sensorw);
	mt9d113_back_sensorw = NULL;
	CDBG("mt9d113_back_probe failed!\n");
	return rc;
}



static const struct i2c_device_id mt9d113_back_i2c_id[] = {
	{ "mt9d113_back", 0},
	{ },
};

static struct i2c_driver mt9d113_back_i2c_driver = {
	.id_table = mt9d113_back_i2c_id,
	.probe  = mt9d113_back_i2c_probe,
	.remove = __exit_p(mt9d113_back_i2c_remove),
	.driver = {
		.name = "mt9d113_back",
	},
};

static int mt9d113_back_sensor_probe(const struct msm_camera_sensor_info *info,
									struct msm_sensor_ctrl *s)
{
	int32_t rc = 0;
	rc = i2c_add_driver(&mt9d113_back_i2c_driver);
	if (rc < 0 || mt9d113_back_client == NULL) {
		if(rc==0)
			CDBG("rc==0, i2c_add_driver  OK,but mt9d113_back_client == NULL\n");
		else
			CDBG("rc<0 i2c_add_driver   error");
		rc = -ENOTSUPP;
		goto probe_done;
	}


	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);//

	mdelay(5);

	rc = mt9d113_back_sensor_init_probe(info);
	if (rc < 0)
		goto probe_done;

    gCameraBackNums_mt9d113 = 1;

	s->s_init = mt9d113_back_sensor_init;
	s->s_release = mt9d113_back_sensor_release;
	s->s_config  = mt9d113_back_sensor_config;
        mt9d113_back_sensor_init_done(info);
probe_done:
	return rc;

}

static int __mt9d113_back_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, mt9d113_back_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __mt9d113_back_probe,
	.driver = {
		.name = "msm_camera_mt9d113_back",
		.owner = THIS_MODULE,
	},
};

static int __init mt9d113_back_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(mt9d113_back_init);
EXPORT_SYMBOL(gCameraBackNums_mt9d113);
