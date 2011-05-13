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
#include "ov2650.h"

/* Sensor Core Registers */
#define  REG_OV2650_MODEL_ID 0x300A
#define  OV2650_MODEL_ID     0x26

/*  SOC Registers Page 1  */
#define  REG_OV2650_SENSOR_RESET     0x3012

#define NUM_REGS_CONTRAST 16
#define NUM_REGS_EFFECT 3
#define NUM_REGS_BRIHTNESS 3 

#define OV2650_I2C_RETRY_TIMES        3

unsigned int gCameraBackNums_ov2650 = 0;

struct ov2650_work_t {
	struct work_struct work;
};

static struct  ov2650_work_t *ov2650_sensorw;
static struct  i2c_client *ov2650_client;

struct ov2650_ctrl_t {
	const struct msm_camera_sensor_info *sensordata;
};

static long camsensor_ov2650_config_snapshot(void);
static long camsensor_ov2650_config_preview(void);
static long ov2650_set_sensor_mode(int mode);
static int ov2650_set_effect(int mode,int8_t effect);
static long ov2650_set_whitebalance(int mode,int8_t wb);
static int ov2650_sensor_init_done(const struct msm_camera_sensor_info *data);

static struct ov2650_ctrl_t *ov2650_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(ov2650_wait_queue);
DECLARE_MUTEX(ov2650_sem);

static struct camsensor_ov2650_reg_type ov2650_init_reg_config[] =
{
#if 0
	{0x308c,0x80},
	{0x308d,0x0e},
	{0x360b,0x00},
	{0x30b0,0xff},
	{0x30b1,0xff},
	{0x30b2,0x24},

	{0x300e,0x34},
	{0x300f,0xa6},
	{0x3010,0x81},
	{0x3082,0x01},
	{0x30f4,0x01},
	{0x3090,0x33},
	{0x3091,0xc0},
	{0x30ac,0x42},

	{0x30d1,0x08},
	{0x30a8,0x56},
	{0x3015,0x03},//16->8 gain
	{0x3093,0x00},
	{0x307e,0xe5},
	{0x3079,0x00},
	{0x30aa,0x42},
	{0x3017,0x40},
	{0x30f3,0x82},
	{0x306a,0x0c},
	{0x306d,0x00},
	{0x336a,0x3c},
	{0x3076,0x6a},
	{0x30d9,0x95},
	{0x3016,0x82},
	{0x3601,0x30},
	{0x304e,0x88},
	{0x30f1,0x82},
	{0x306f,0x14},
	{0x302a,0x02},
	{0x302b,0x6a},

	{0x3012,0x10},
	{0x3011,0x00},
	{0x3013,0xf7},
	{0x301c,0x13},
	{0x301d,0x17},
	{0x3070,0x5d},
	{0x3072,0x4d},

	//D5060
	{0x30af,0x00},
	{0x3048,0x1f},
	{0x3049,0x4e}, 
	{0x304a,0x20}, 
	{0x304f,0x20}, 
	{0x304b,0x02},
	{0x304c,0x00}, 
	{0x304d,0x02}, 
	{0x304f,0x20}, 
	{0x30a3,0x10}, 
	{0x3013,0xf7},
	{0x3014,0x44}, 
	{0x3071,0x00},
	{0x3070,0x5d},
	{0x3073,0x00},
	{0x3072,0x4d},
	{0x301c,0x05},
	{0x301d,0x06},
	{0x304d,0x42},    
	{0x304a,0x40}, 
	{0x304f,0x40}, 
	{0x3095,0x07}, 
	{0x3096,0x16},
	{0x3097,0x1d},
	       
	//Window Setup
	{0x300e,0x34},
	{0x3020,0x01},
	{0x3021,0x18},
	{0x3022,0x00},
	{0x3023,0x06},
	{0x3024,0x06},
	{0x3025,0x58},
	{0x3026,0x02},
	{0x3027,0x5e},
	{0x3088,0x03},
	{0x3089,0x20},
	{0x308a,0x02},
	{0x308b,0x58},
	{0x3316,0x64},
	{0x3317,0x25},
	{0x3318,0x80},
	{0x3319,0x08},
	{0x331a,0x64},
	{0x331b,0x4b},
	{0x331c,0x00},
	{0x331d,0x38},
	{0x3100,0x00},
	  

	//Gamma
	{0x3340,0x0e},
	{0x3341,0x1a},
	{0x3342,0x31},
	{0x3343,0x45},
	{0x3344,0x5a},
	{0x3345,0x69},
	{0x3346,0x75},
	{0x3347,0x7e},
	{0x3348,0x88},
	{0x3349,0x96},
	{0x334a,0xa3},
	{0x334b,0xaf},
	{0x334c,0xc4},
	{0x334d,0xd7},
	{0x334e,0xe8},
	{0x334f,0x20},

	//Lens correction
	{ 0x3300, 0xfc},
	{ 0x3350, 0x32},
	{ 0x3352, 0x80},
	{ 0x3351, 0x25},
	{ 0x3352, 0x80},
	{ 0x3353, 0x64},
	{ 0x3355, 0xf6},
	{ 0x3354, 0x0 },
	{ 0x3355, 0xf6},
	{ 0x3356, 0x32},
	{ 0x3358, 0x80},
	{ 0x3357, 0x25},
	{ 0x3358, 0x80},
	{ 0x3359, 0x5c},
	{ 0x335b, 0xf6},
	{ 0x335a, 0x00},
	{ 0x335b, 0xf6},
	{ 0x335c, 0x32},
	{ 0x335e, 0x80},
	{ 0x335d, 0x25},
	{ 0x335e, 0x80},
	{ 0x335f, 0x60},
	{ 0x3361, 0xf6},
	{ 0x3360, 0x0 },
	{ 0x3361, 0xf6},      
	{ 0x3363, 0x70},
	{ 0x3364, 0x7f},
	{ 0x3365, 0x00},
	{ 0x3366, 0x00},
	 
	//UVadjust
	{0x3301,0xff},
	{0x338B,0x11},
	{0x338c,0x10},
	{0x338d,0x40},

	//Sharpness/De-noise
	{0x3370,0xd0},
	{0x3371,0x00},
	{0x3372,0x00},
	{0x3373,0x40},
	{0x3374,0x10},
	{0x3375,0x10},
	{0x3376,0x04},
	{0x3377,0x00},
	{0x3378,0x04},
	{0x3379,0x80},

	//BLC
	{0x3069,0x84},
	{0x307c,0x10},
	{0x3087,0x02},

	//Other functions
	{0x3300,0xfc},
	{0x3302,0x11},
	{0x3400,0x01},
	{0x3606,0x20},
	{0x3601,0x30},
	{0x300e,0x34},
	{0x30f3,0x83},
	{0x304e,0x88},

	{0x3086,0x0f},
	{0x3086,0x00},

	//color matrix
	{0x3380,0x28},  
	{0x3381,0x48}, 
	{0x3382,0x10}, 
	{0x3383,0x3a},   
	{0x3384,0xad},   
	{0x3385,0xe6},   
	{0x3386,0xc0}, 
	{0x3387,0xb6}, 
	{0x3388,0x0a},
	{0x3389,0x98},  
	{0x338a,0x01},  
	//awb
	{0x3320,0x9a},
	{0x3321,0x11},
	{0x3322,0x92},
	{0x3323,0x01},
	{0x3324,0x96},
	{0x3325,0x02},
	{0x3326,0xff},
	{0x3327,0x0c},
	{0x3328,0x10},
	{0x3329,0x10},
	{0x332a,0x5e},
	{0x332b,0x5d},
	{0x332c,0xbe},
	{0x332d,0xce},
	{0x332e,0x3c},
	{0x332f,0x32},
	{0x3330,0x46},
	{0x3331,0x40},
	{0x3332,0xff},
	{0x3333,0x00},
	{0x3334,0xf0},
	{0x3335,0xf0},
	{0x3336,0xf0},
	{0x3337,0x40},
	{0x3338,0x40},
	{0x3339,0x40},
	{0x333a,0x00},
	{0x333b,0x00},

	{0x3391,0x02},
	{0x3394,0x45},
	{0x3395,0x45},
	{0x3306,0x04},
	{0x3370,0x03},
	{0x3376,0x06},

	{0x302b,0x98},
	{0x3070,0x5f},
	{0x3072,0x4f},
	{0x3600,0x01},
#endif
#if 1
{0x308c, 0x80},
{0x308d, 0x0e},
{0x360b, 0x00},
{0x30b0, 0xff},
{0x30b1, 0xff},
{0x30b2, 0x25},//0x27

{0x300e, 0x34},
{0x300f, 0xa6},
{0x3010, 0x81},
{0x3082, 0x01},
{0x30f4, 0x01},
{0x3090, 0x33},
{0x3091, 0xc0},
{0x30ac, 0x42},

{0x30d1, 0x08},
{0x30a8, 0x56},
{0x3015, 0x02},
{0x3093, 0x00},
{0x307e, 0xe5},
{0x3079, 0x00},
{0x30aa, 0x42},
{0x3017, 0x40},
{0x30f3, 0x82},
{0x306a, 0x0c},
{0x306d, 0x00},
{0x336a, 0x3c},
{0x3076, 0x6a},
{0x30d9, 0x8c},
{0x3016, 0x82},
{0x3601, 0x30},
{0x304e, 0x88},
{0x30f1, 0x82},
{0x306f, 0x14},

{0x3012, 0x10},
{0x3011, 0x00},
{0x302A, 0x03},
{0x302B, 0x9f},
{0x3028, 0x07},
{0x3029, 0x93},

//;saturation
{0x3391, 0x06},
{0x3394, 0x38},
{0x3395, 0x38},

{0x3015, 0x32}, 
{0x302d, 0x00},
{0x302e, 0x00},

//;AEC/AGC
{0x3013, 0xf7},
{0x3018, 0x78}, 
{0x3019, 0x68},
{0x301a, 0xc4},

//;D5060
{0x30af, 0x00},
{0x3048, 0x1f}, 
{0x3049, 0x4e},  
{0x304a, 0x20},  
{0x304f, 0x20},  
{0x304b, 0x02}, 
{0x304c, 0x00},  
{0x304d, 0x02},  
{0x304f, 0x20},  
{0x30a3, 0x10},  
{0x3013, 0xf7}, 
{0x3014, 0x8c},  
{0x3071, 0x00},
{0x3070, 0xb9},
{0x3073, 0x00},
{0x3072, 0xb9},
{0x301c, 0x04},
{0x301d, 0x04},
{0x304d, 0x42},     
{0x304a, 0x40},  
{0x304f, 0x40},  
{0x3095, 0x07},  
{0x3096, 0x16}, 
{0x3097, 0x1d}, 

//;Window Setup
{0x3020, 0x01},
{0x3021, 0x18},
{0x3022, 0x00},
{0x3023, 0x06},
{0x3024, 0x06},
{0x3025, 0x58},
{0x3026, 0x02},
{0x3027, 0x5e},
{0x3088, 0x03},
{0x3089, 0x20},
{0x308a, 0x02},
{0x308b, 0x58},
{0x3316, 0x64},
{0x3317, 0x25},
{0x3318, 0x80},
{0x3319, 0x08},
{0x331a, 0x64},
{0x331b, 0x4b},
{0x331c, 0x00},
{0x331d, 0x38},
{0x3100, 0x00},

//;AWB
{0x3320, 0xfa},
{0x3321, 0x11},
{0x3322, 0x92},
{0x3323, 0x01},
{0x3324, 0x97},
{0x3325, 0x02},
{0x3326, 0xff},
{0x3327, 0x0c},
{0x3328, 0x10}, 
{0x3329, 0x10},
{0x332a, 0x54},
{0x332b, 0x52},
{0x332c, 0xbe},
{0x332d, 0xe1},
{0x332e, 0x3a},
{0x332f, 0x36},
{0x3330, 0x4d},
{0x3331, 0x44},
{0x3332, 0xf8},
{0x3333, 0x0a},
{0x3334, 0xf0},
{0x3335, 0xf0},
{0x3336, 0xf0},
{0x3337, 0x40},
{0x3338, 0x40},
{0x3339, 0x40},
{0x333a, 0x00},
{0x333b, 0x00}, 

//;Color Matrix
{0x3380, 0x28}, 
{0x3381, 0x48}, 
{0x3382, 0x10}, 
{0x3383, 0x22}, 
{0x3384, 0xc0}, 
{0x3385, 0xe2}, 
{0x3386, 0xe2}, 
{0x3387, 0xf2}, 
{0x3388, 0x10}, 
{0x3389, 0x98}, 
{0x338a, 0x00}, 

//;Gamma
{0x3340, 0x04},
{0x3341, 0x07},
{0x3342, 0x19},
{0x3343, 0x34},
{0x3344, 0x4a},
{0x3345, 0x5a},
{0x3346, 0x67},
{0x3347, 0x71},
{0x3348, 0x7c},
{0x3349, 0x8c},
{0x334a, 0x9b},
{0x334b, 0xa9},
{0x334c, 0xc0},
{0x334d, 0xd5},
{0x334e, 0xe8},
{0x334f, 0x20},

//Lens correction
{ 0x3300, 0xfc},
{ 0x3350, 0x32},
{ 0x3352, 0x80},
{ 0x3351, 0x25},
{ 0x3352, 0x80},
{ 0x3353, 0x64},
{ 0x3355, 0xf6},
{ 0x3354, 0x0 },
{ 0x3355, 0xf6},
{ 0x3356, 0x32},
{ 0x3358, 0x80},
{ 0x3357, 0x25},
{ 0x3358, 0x80},
{ 0x3359, 0x5c},
{ 0x335b, 0xf6},
{ 0x335a, 0x00},
{ 0x335b, 0xf6},
{ 0x335c, 0x32},
{ 0x335e, 0x80},
{ 0x335d, 0x25},
{ 0x335e, 0x80},
{ 0x335f, 0x60},
{ 0x3361, 0xf6},
{ 0x3360, 0x0 },
{ 0x3361, 0xf6},      
{ 0x3363, 0x70},
{ 0x3364, 0x7f},
{ 0x3365, 0x00},
{ 0x3366, 0x00},

//;UVadjust
{0x3301, 0xff},
{0x338B, 0x14},
{0x338c, 0x10},
{0x338d, 0x40},

//;Sharpness/De-noise
{0x3370, 0xd0},
{0x3371, 0x00},
{0x3372, 0x00},
{0x3373, 0x50},
{0x3374, 0x10},
{0x3375, 0x10},
{0x3376, 0x06},
{0x3377, 0x00},
{0x3378, 0x04},
{0x3379, 0x80},

//;BLC
{0x3069, 0x84},
{0x307c, 0x10},
{0x3087, 0x02},

//;Other functions
{0x3300, 0xfc},
{0x3302, 0x11},
{0x3400, 0x01},
{0x3606, 0x20},
{0x3601, 0x30},
{0x30f3, 0x83},
{0x304e, 0x88},

{0x3600,0x01},

{0x30aa, 0x72},
{0x30a3, 0x80},
{0x30a1, 0x41},

{0x3086, 0x0f},
{0x3086, 0x00},
	
#endif
};

static struct camsensor_ov2650_reg_type camsensor_ov2650_effect_off_reg_tbl[NUM_REGS_EFFECT] = 
{
	{0x3391, 0x06}
};

static struct camsensor_ov2650_reg_type camsensor_ov2650_effect_mono_reg_tbl[NUM_REGS_EFFECT] = 
{
	{0x3391, 0x26}
};

static struct camsensor_ov2650_reg_type camsensor_ov2650_effect_negative_reg_tbl[NUM_REGS_EFFECT] =
{
	{0x3391, 0x46}
};

static struct camsensor_ov2650_reg_type camsensor_ov2650_effect_sepia_reg_tbl[NUM_REGS_EFFECT] = 
{
	{0x3391,0x1e},       
	{0x3396,0x40},       
	{0x3397,0xa6},  
};

static struct camsensor_ov2650_reg_type camsensor_ov2650_preview_reg_tbl[] = 
{
{0x3013, 0xf7},
{0x3014, 0x8c},
//{0x3015, 0x32},

{0x3011, 0x00},
{0x3012, 0x10},
{0x302a, 0x03},
{0x302b, 0x9f},
{0x306f, 0x14}, 
{0x3362, 0x90},

{0x3070, 0xb9},
{0x3072, 0xb9},
{0x301c, 0x04},
{0x301d, 0x04},

{0x3020, 0x01},
{0x3021, 0x18},
{0x3022, 0x00},
{0x3023, 0x06},
{0x3024, 0x06},
{0x3025, 0x58},
{0x3026, 0x02},
{0x3027, 0x5E},
{0x3088, 0x03},
{0x3089, 0x20},
{0x308A, 0x02},
{0x308B, 0x58},
{0x3316, 0x64},
{0x3317, 0x25},
{0x3318, 0x80},
{0x3319, 0x08},
{0x331A, 0x64},
{0x331B, 0x4B},
{0x331C, 0x00},
{0x331D, 0x38},
{0x3302, 0x11},

};

static struct camsensor_ov2650_reg_type camsensor_ov2650_snapshot_reg_tbl[] = 
{
{0x3013, 0xf2},//3002,3003
{0x3014, 0x84},//302d,302e
//{0x3015, 0x02},

{0x3011, 0x01},
{0x3012, 0x00},
{0x302a, 0x07},
{0x302b, 0x3e},
{0x306f, 0x54}, 
{0x3362, 0x80},

{0x3070, 0x5c},//b9
{0x3072, 0x5c},//b9
{0x301c, 0x0f},
{0x301d, 0x0f},

{0x3020, 0x01},
{0x3021, 0x18},
{0x3022, 0x00},
{0x3023, 0x0A},
{0x3024, 0x06},
{0x3025, 0x58},
{0x3026, 0x04},
{0x3027, 0xbc},
{0x3088, 0x06},
{0x3089, 0x40},
{0x308A, 0x04},
{0x308B, 0xB0},
{0x3316, 0x64},
{0x3317, 0x4B},
{0x3318, 0x00},
{0x3319, 0x6C},
{0x331A, 0x64},
{0x331B, 0x4B},
{0x331C, 0x00},
{0x331D, 0x6C},
{0x3302, 0x01},

};

/*=============================================================*/
static int ov2650_i2c_rxdata(unsigned short saddr,
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
		.addr  = saddr,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

	if (i2c_transfer(ov2650_client->adapter, msgs, 2) < 0) {
		CDBG("ov2650_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t ov2650_i2c_read_w(unsigned short saddr,
	unsigned short raddr, unsigned char *rdata)
{
	int32_t rc = 0;
	int32_t i = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00)>>8;
	buf[1] = (raddr & 0x00FF);
    
	for(i = 0; i < OV2650_I2C_RETRY_TIMES; i++)
	{
 	  rc = ov2650_i2c_rxdata(saddr, buf, 2);
	  if (rc >= 0)
	  {
        *rdata = buf[0] ;
        return rc;
	  }
	}
    
    CDBG("ov2650_i2c_read failed, raddr = 0x%x\n", raddr);
    
    return -EIO;

}

static int32_t ov2650_i2c_txdata(unsigned short saddr,
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

	if (i2c_transfer(ov2650_client->adapter, msg, 1) < 0) {
		CDBG("ov2650_i2c_txdata faild\n");
		return -EIO;
	}

	return 0;
}


static int32_t ov2650_i2c_write_w(unsigned short saddr,
	unsigned short waddr, unsigned char wdata)
{
	long rc = -EFAULT;
	//unsigned char buf[4];
	unsigned char buf[3];
	
	int32_t i = 0;

	memset(buf, 0, sizeof(buf));
	buf[0] = (waddr & 0xFF00)>>8;
	buf[1] = (waddr & 0x00FF);
	//buf[2] = (wdata & 0xFF00)>>8;
	//buf[3] = (wdata & 0x00FF);
	buf[2] = (wdata & 0xFF);

	for(i = 0; i < OV2650_I2C_RETRY_TIMES; i++)
	{
 	  rc = ov2650_i2c_txdata(saddr, buf, 3);
	  if(0 <= rc)
	    return 0;
	}
    
    CDBG("i2c_write_w failed, addr = 0x%x, val = 0x%x!\n", waddr, wdata);
    return -EIO;


}

static int32_t ov2650_i2c_write_w_table(
	struct camsensor_ov2650_reg_type const *reg_conf_tbl,
	int num_of_items_in_table)
{
	int i;
	int32_t rc = -EFAULT;

	for (i = 0; i < num_of_items_in_table; i++) {
		rc = ov2650_i2c_write_w(ov2650_client->addr,
			reg_conf_tbl->addr, reg_conf_tbl->val);
		if (rc < 0)
			break;
		reg_conf_tbl++;
	}

	return rc;
}
 static long ov2650_set_sensor_mode(int mode)
{
     	long rc = 0;


	switch (mode) {
	case SENSOR_PREVIEW_MODE:
	       rc = camsensor_ov2650_config_preview();
	       if (rc < 0) {
		CDBG("ov2650_sensor_mode failed!\n");
		goto set_sensor_mode_fail;
	       }
          	mdelay(5);
		break;

	case SENSOR_SNAPSHOT_MODE:
	       rc = camsensor_ov2650_config_snapshot();
	       if (rc < 0) {
		CDBG("ov2650_sensor_mode failed!\n");
		goto set_sensor_mode_fail;
	       }
		mdelay(5);
		break;

	default:
		return -EFAULT;
	}
	
set_sensor_mode_fail:
	return rc;
}

 static int ov2650_set_effect(int mode,int8_t effect)
{

	int rc = 0;
	
	switch (effect) {
	case CAMERA_EFFECT_OFF: 
	       rc = ov2650_i2c_write_w_table(camsensor_ov2650_effect_off_reg_tbl,
                    sizeof(camsensor_ov2650_effect_off_reg_tbl)/sizeof(camsensor_ov2650_effect_off_reg_tbl[0]));
	       if (rc < 0) {
			CDBG("ov2650_effect_off failed!\n");
			goto set_effect_fail;
		    }
		   
		break;
		
	case CAMERA_EFFECT_MONO: 
		rc = ov2650_i2c_write_w_table(camsensor_ov2650_effect_mono_reg_tbl,
	             sizeof(camsensor_ov2650_effect_mono_reg_tbl)/sizeof(camsensor_ov2650_effect_mono_reg_tbl[0]));
		if (rc < 0) {
			CDBG("ov2650_effect_mono failed!\n");
			goto set_effect_fail;
			}
	
		break;

	case CAMERA_EFFECT_NEGATIVE: 
		rc = ov2650_i2c_write_w_table(camsensor_ov2650_effect_negative_reg_tbl,
	              sizeof(camsensor_ov2650_effect_negative_reg_tbl)/sizeof(camsensor_ov2650_effect_negative_reg_tbl[0]));
		if (rc < 0) {
			CDBG("ov2650_effect_negative failed!\n");
			goto set_effect_fail;
			}
		
		break;

	case CAMERA_EFFECT_SOLARIZE: {

	}
		break;

	case CAMERA_EFFECT_SEPIA: 
	       rc = ov2650_i2c_write_w_table(camsensor_ov2650_effect_sepia_reg_tbl,
                    sizeof(camsensor_ov2650_effect_sepia_reg_tbl)/sizeof(camsensor_ov2650_effect_sepia_reg_tbl[0]));
		if (rc < 0) {
			CDBG("ov2650_effect_sepia failed!\n");
			goto set_effect_fail;
			}

		break;

	default: {
	             rc = -EFAULT;
			return rc;

	}
	}


set_effect_fail:
	return rc;
}

static long ov2650_set_whitebalance(int mode,int8_t wb)
{

	long rc = 0;

	CDBG("ov2650_set_whitebalance  wb=%d\n",wb);

	switch ((enum config3a_wb_t)wb) {
	case CAMERA_WB_AUTO: {
	       rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3306, 0x00);
	       mdelay(50);
	}
		break;

	case CAMERA_WB_INCANDESCENT: {
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3306, 0x02);
	
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3337, 0x5E);
	
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3338, 0x40);
	
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3339, 0x46);
	
	       mdelay(50);
	}
		break;

 	case CAMERA_WB_CLOUDY_DAYLIGHT: {
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3306, 0x02);
	
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3337, 0x68);
	
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3338, 0x40);
	
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3339, 0x4E);
	
	       mdelay(50);
	}
		break;

	case CAMERA_WB_DAYLIGHT: {
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3306, 0x02);
	
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3337, 0x52);
	
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3338, 0x40);
	
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3339, 0x58);
	
		mdelay(50);
	}
		break;
        case CAMERA_WB_FLUORESCENT: {
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3306, 0x02);
	
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3337, 0x65);
	
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3338, 0x40);
	
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3339, 0x41);
	
	       mdelay(50);
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

static long ov2650_set_mirror(	int mode,int8_t mirror)
{

	long rc = 0;

	switch ((enum mirror_t)mirror) {
	case CAMERA_MIRROR_NORMAL: {
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x307c, 0x00);
	
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3090, 0x08);

	       mdelay(10);
	}
		break;

	case CAMERA_MIRROR_ABNORMAL: {
		rc = ov2650_i2c_write_w(ov2650_client->addr,	0x3090, 0x00);

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

 static long camsensor_ov2650_config_snapshot(void)
{
	long rc = 0;
	rc = ov2650_i2c_write_w_table(camsensor_ov2650_snapshot_reg_tbl,
	        sizeof(camsensor_ov2650_snapshot_reg_tbl)/sizeof(camsensor_ov2650_snapshot_reg_tbl[0]));
	if (rc < 0) {
	CDBG("ov2650_effect_off failed!\n");
	goto config_snapshot_fail;
	}
	 mdelay(200);

config_snapshot_fail:
    return rc;
}

static long camsensor_ov2650_config_preview(void)
{       
       int32_t rc = 0;	   
	rc = ov2650_i2c_write_w_table(camsensor_ov2650_preview_reg_tbl,
	        sizeof(camsensor_ov2650_preview_reg_tbl)/sizeof(camsensor_ov2650_preview_reg_tbl[0]));

	if (rc < 0) {
	CDBG("ov2650_config_preview failed!\n");
	goto config_preview_fail;
	}
	 mdelay(600);

config_preview_fail:
    return rc;
}

static int ov2650_sensor_init_done(const struct msm_camera_sensor_info *data)
{
	gpio_direction_output(data->sensor_reset, 0);
	gpio_free(data->sensor_reset);

	gpio_direction_output(data->sensor_pwd, 1);
	gpio_free(data->sensor_pwd);

	if (data->vreg_enable_func){
		data->vreg_enable_func(0);
		}
	return 0;
              
}

static int ov2650_sensor_init_probe(const  struct msm_camera_sensor_info *data)
{
       int rc;
	unsigned char chipid=0;	

	rc = gpio_request(data->sensor_pwd, "ov2650");
	if (!rc || rc == -EBUSY)
		gpio_direction_output(data->sensor_pwd, 0);
	else 
	       goto init_probe_fail;

	rc = gpio_request(data->sensor_reset, "ov2650");
	if (!rc) 
		rc = gpio_direction_output(data->sensor_reset, 0);
	else
		goto init_probe_fail;
       mdelay(10);  
	   
	if (data->vreg_enable_func){
              data->vreg_enable_func(1);
    	}
	mdelay(10);
	
       rc = gpio_direction_output(data->sensor_reset, 1);
	
       mdelay(100);

        rc = ov2650_i2c_write_w(ov2650_client->addr,
		REG_OV2650_SENSOR_RESET, 0x80);
	if (rc < 0)
		goto init_probe_fail;
       mdelay(10);

	/* Read sensor Model ID: */
	rc = ov2650_i2c_read_w(ov2650_client->addr,
		REG_OV2650_MODEL_ID, &chipid);
	if (rc < 0)
		goto init_probe_fail;

	/* Compare sensor ID to OV2650 ID: */
	if (chipid != OV2650_MODEL_ID) {
		rc = -ENODEV;
		goto init_probe_fail;
	}
    goto init_probe_done;

init_probe_fail:
    ov2650_sensor_init_done(data);
init_probe_done:
	return rc;
       
}

int ov2650_sensor_init(const struct msm_camera_sensor_info *data)
{
       
	int rc = 0;
	ov2650_ctrl = kzalloc(sizeof(struct ov2650_ctrl_t), GFP_KERNEL);
	if (!ov2650_ctrl) {
		CDBG("ov2650_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		ov2650_ctrl->sensordata = data;

	msm_camio_clk_rate_set(24000000);
	mdelay(5);

	msm_camio_camif_pad_reg_reset();

	rc = ov2650_sensor_init_probe(data);

       rc = ov2650_i2c_write_w_table(ov2650_init_reg_config,
                            sizeof(ov2650_init_reg_config) / sizeof(ov2650_init_reg_config[0]));
	if (rc < 0) {
		CDBG("ov2650_sensor_init failed!\n");
		goto init_fail;
	}

init_done:
	return rc;

init_fail:
	kfree(ov2650_ctrl);
	return rc;
}


static int ov2650_init_client(struct i2c_client *client)
{

	init_waitqueue_head(&ov2650_wait_queue);
	return 0;
}

int ov2650_sensor_config(void __user *argp)
{
       
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	if (copy_from_user(
				&cfg_data,
				(void *)argp,
				sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	 //down(&ov2650_sem); 

	CDBG("ov2650_ioctl, cfgtype = %d, mode = %d\n",cfg_data.cfgtype, cfg_data.mode);

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = ov2650_set_sensor_mode(
					cfg_data.mode);
		break;

	case CFG_SET_EFFECT:
		rc = ov2650_set_effect(
					cfg_data.mode,
					cfg_data.cfg.effect);
		break;
		
	case CFG_SET_WB:
		rc=ov2650_set_whitebalance(
					cfg_data.mode,
					cfg_data.cfg.whitebalance);
		break;
	case CFG_SET_MIRROR:
		rc=ov2650_set_mirror(
					cfg_data.mode,
					cfg_data.cfg.mirror);
		break;

	default:
		rc = -EFAULT;
		break;
	}
	//up(&ov2650_sem); 

	return rc;
}

static int ov2650_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	ov2650_sensorw =
		kzalloc(sizeof(struct ov2650_work_t), GFP_KERNEL);

	if (!ov2650_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, ov2650_sensorw);
	ov2650_init_client(client);
	ov2650_client = client;

	CDBG("ov2650_probe successed!\n");

	return 0;

probe_failure:
	kfree(ov2650_sensorw);
	ov2650_sensorw = NULL;
	CDBG("ov2650_probe failed!\n");
	return rc;
}

int ov2650_sensor_release(void)
{
	int rc = 0;

	/* down(&mt9d113_sem); */
	ov2650_sensor_init_done(ov2650_ctrl->sensordata);
	kfree(ov2650_ctrl);
	/* up(&mt9d113_sem); */

	return rc;
}

static const struct i2c_device_id ov2650_i2c_id[] = {
	{ "ov2650", 0},
	{ },
};

static struct i2c_driver ov2650_i2c_driver = {
	.id_table = ov2650_i2c_id,
	.probe  = ov2650_probe,
	.remove = __exit_p(ov2650_i2c_remove),
	.driver = {
		.name = "ov2650",
	},
};

static int ov2650_sensor_probe(const struct msm_camera_sensor_info *info,
									struct msm_sensor_ctrl *s)
{
	int32_t rc = 0;
	CDBG("-----ov2650_probe_init------begin \n");
	rc = i2c_add_driver(&ov2650_i2c_driver);
	if (rc < 0 || ov2650_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_done;
	}

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);
	mdelay(5);

	rc = ov2650_sensor_init_probe(info);
	if (rc < 0)
		goto probe_done;
	
	gCameraBackNums_ov2650 = 1;

	s->s_init = ov2650_sensor_init;
	s->s_release = ov2650_sensor_release;
	s->s_config  = ov2650_sensor_config;
        ov2650_sensor_init_done(info);
probe_done:
	CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;

}

static int __ov2650_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, ov2650_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __ov2650_probe,
	.driver = {
		.name = "msm_camera_ov2650",
		.owner = THIS_MODULE,
	},
};

static int __init ov2650_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(ov2650_init);
EXPORT_SYMBOL(gCameraBackNums_ov2650);


