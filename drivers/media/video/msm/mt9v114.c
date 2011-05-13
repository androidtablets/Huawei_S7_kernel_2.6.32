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
#include "mt9v114.h"

/* Sensor Core Registers */
#define  REG_MT9V114_MODEL_ID 0x0000
#define  MT9V114_MODEL_ID     0x2283


#define NUM_REGS_EFFECT 4

unsigned int gCameraFrontNums_mt9v114 = 0;

struct mt9v114_work_t {
	struct work_struct work;
};

static struct  mt9v114_work_t *mt9v114_sensorw;
static struct  i2c_client *mt9v114_client;

struct mt9v114_ctrl_t {
	const struct msm_camera_sensor_info *sensordata;
};

static long mt9v114_set_sensor_mode(int mode);
static int mt9v114_set_effect(int mode,int8_t effect);
static long mt9v114_set_whitebalance(int mode,int8_t wb);
static int mt9v114_sensor_init_done(const struct msm_camera_sensor_info *data);

static struct mt9v114_ctrl_t *mt9v114_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(mt9v114_wait_queue);
DECLARE_MUTEX(mt9v114_sem);

static struct camsensor_mt9v114_reg_type mt9v114_init_reg_config[] =
{
//[SOC390 Recommend initial settings]
//{0x001A, 0x0106}, // RESET_AND_MISC_CONTROL
//{0,5},
//{0x001A, 0x0124}, // RESET_AND_MISC_CONTROL
//[Step2-PLL_Timing] //PLL, Timing & Flicker
//PLL_settings
{0x0010, 0x0116}, //110 // PLL_DIVIDERS
{0x0012, 0x0500}, // PLL_P_DIVIDERS
{0x001E, 0x0777}, // PAD_SLEW
//{0x001E, 0x0700}, // PAD_SLEW
{0x0018, 0x0006}, // STANDBY_CONTROL_AND_STATUS
{0,5},
// Required patch, must be done at this point.
// patch driver table
{0x098A, 0x0000}, // PHYSICAL_ADDRESS_ACCESS
{0x8082, 0x0194},
{0x8084, 0x0163},
{0x8086, 0x0107},
{0x8088, 0x01C7},
{0x808A, 0x01A1},
{0x808C, 0x022A},
{0x098E, 0x0000}, // LOGICAL_ADDRESS_ACCESS
//patch SISR
// patch version 2, delta dark sensor fix and flicker detect fix
{0x0982, 0x0000},
{0x098A, 0x0000},
{0x8098, 0x3C3C},
{0x809A, 0x1300},
{0x809C, 0x0147},
{0x809E, 0xCC31},
{0x80A0, 0x8230},
{0x80A2, 0xED02},
{0x80A4, 0xCC00},
{0x80A6, 0x90ED},
{0x80A8, 0x00C6},
{0x80AA, 0x04BD},
{0x80AC, 0xDDBD},
{0x80AE, 0x5FD7},
{0x80B0, 0x8E86},
{0x80B2, 0x90BD},
{0x80B4, 0x0330},
{0x80B6, 0xDD90},
{0x80B8, 0xCC92},
{0x80BA, 0x02BD},
{0x80BC, 0x0330},
{0x80BE, 0xDD92},
{0x80C0, 0xCC94},
{0x80C2, 0x04BD},
{0x80C4, 0x0330},
{0x80C6, 0xDD94},
{0x80C8, 0xCC96},
{0x80CA, 0x00BD},
{0x80CC, 0x0330},
{0x80CE, 0xDD96},
{0x80D0, 0xCC07},
{0x80D2, 0xFFDD},
{0x80D4, 0x8ECC},
{0x80D6, 0x3180},
{0x80D8, 0x30ED},
{0x80DA, 0x02CC},
{0x80DC, 0x008E},
{0x80DE, 0xED00},
{0x80E0, 0xC605},
{0x80E2, 0xBDDE},
{0x80E4, 0x13BD},
{0x80E6, 0x03FA},
{0x80E8, 0x3838},
{0x80EA, 0x3913},
{0x80EC, 0x0001},
{0x80EE, 0x0109},
{0x80F0, 0xBC01},
{0x80F2, 0x9D26},
{0x80F4, 0x0813},
{0x80F6, 0x0004},
{0x80F8, 0x0108},
{0x80FA, 0xFF02},
{0x80FC, 0xEF7E},
{0x80FE, 0xC278},
{0x8330, 0x364F},
{0x8332, 0x36CE},
{0x8334, 0x02F3},
{0x8336, 0x3AEC},
{0x8338, 0x00CE},
{0x833A, 0x0018},
{0x833C, 0xBDE4},
{0x833E, 0x5197},
{0x8340, 0x8F38},
{0x8342, 0xEC00},
{0x8344, 0x938E},
{0x8346, 0x2C02},
{0x8348, 0x4F5F},
{0x834A, 0x3900},
{0x83E4, 0x3C13},
{0x83E6, 0x0001},
{0x83E8, 0x0CCC},
{0x83EA, 0x3180},
{0x83EC, 0x30ED},
{0x83EE, 0x00CC},
{0x83F0, 0x87FF},
{0x83F2, 0xBDDD},
{0x83F4, 0xF5BD},
{0x83F6, 0xC2A9},
{0x83F8, 0x3839},
{0x83FA, 0xFE02},
{0x83FC, 0xEF7E},
{0x83FE, 0x00EB},
{0x098E, 0x0000}, // LOGICAL_ADDRESS_ACCESS
//SISR patch enable
{0x098A, 0x0000}, // PHYSICAL_ADDRESS_ACCESS
{0x83E0, 0x0098},
{0x83E2, 0x03E4},
{0x098E, 0x0000},
//Timing_settings
{0x098E, 0x1000},
{0x300A, 0x01F9}, //frame_length_lines = 505
{0x300C, 0x02D6}, //line_length_pck = 726
{0x3010, 0x0012}, //fine_correction = 18
{0x098E, 0x9803}, // LOGICAL_ADDRESS_ACCESS
{0x9803, 0x0730}, //stat_fd_zone_height = 7
{0xA06E, 0x0098}, //cam_fd_config_fdperiod_50hz = 152
{0xA070, 0x007E}, //cam_fd_config_fdperiod_60hz = 126
{0xA072, 0x1113}, //cam_fd_config_search_f1_50 = 17
{0xA074, 0x1416}, //cam_fd_config_search_f1_60 = 20
{0xA076, 0x0006}, //cam_fd_config_max_fdzone_50hz = 3
{0xA078, 0x0008}, //cam_fd_config_max_fdzone_60hz = 4
{0xA01A, 0x0003}, //cam_ae_config_target_fdzone = 3
{0x8c00, 0x0201},
//[Step3-Recommended] //Char settings
{0x3E22, 0x3307}, // SAMP_BOOST_ROW
{0x3ECE, 0x4311}, // DAC_LD_2_3
{0x3ED0, 0x16AF}, // DAC_LD_4_5
//[Step4-PGA] //PGA
//LSC
{0x3640, 0x00F0},
{0x3642, 0x2667},
{0x3644, 0x0510},
{0x3646, 0x3CEC},
{0x3648, 0x830F},
{0x364A, 0x00F0},
{0x364C, 0x9625},
{0x364E, 0x696F},
{0x3650, 0x04CB},
{0x3652, 0xB36E},
{0x3654, 0x00B0},
{0x3656, 0x5B26},
{0x3658, 0x7BAF},
{0x365A, 0x1888},
{0x365C, 0x9D4F},
{0x365E, 0x0210},
{0x3660, 0x9729},
{0x3662, 0x0630},
{0x3664, 0x48CC},
{0x3666, 0x88CF},
{0x3680, 0x1F6A},
{0x3682, 0xCDE9},
{0x3684, 0x14EE},
{0x3686, 0x4CA8},
{0x3688, 0x95CE},
{0x368A, 0x168C},
{0x368C, 0x8ECC},
{0x368E, 0x5DEC},
{0x3690, 0x076D},
{0x3692, 0xCB29},
{0x3694, 0x62AA},
{0x3696, 0x83AC},
{0x3698, 0x2A8D},
{0x369A, 0x256D},
{0x369C, 0x754C},
{0x369E, 0x3808},
{0x36A0, 0x8B6B},
{0x36A2, 0x348E},
{0x36A4, 0x66EC},
{0x36A6, 0xB5EE},
{0x36C0, 0x5D0F},
{0x36C2, 0xB3CD},
{0x36C4, 0xE850},
{0x36C6, 0x304E},
{0x36C8, 0x2DF0},
{0x36CA, 0x776F},
{0x36CC, 0x0C6F},
{0x36CE, 0x9030},
{0x36D0, 0xA131},
{0x36D2, 0xAF30},
{0x36D4, 0x7EAF},
{0x36D6, 0x0D2F},
{0x36D8, 0x87F1},
{0x36DA, 0x9071},
{0x36DC, 0x6DAF},
{0x36DE, 0x5A2F},
{0x36E0, 0xC5EC},
{0x36E2, 0xE4F0},
{0x36E4, 0xEB6C},
{0x36E6, 0x582F},
{0x3700, 0x110E},
{0x3702, 0xA10F},
{0x3704, 0xDA70},
{0x3706, 0x2A91},
{0x3708, 0x3F92},
{0x370A, 0xB9EC},
{0x370C, 0x028E},
{0x370E, 0x058D},
{0x3710, 0xBBCC},
{0x3712, 0x0A30},
{0x3714, 0x044B},
{0x3716, 0x294E},
{0x3718, 0x31CE},
{0x371A, 0xBEEF},
{0x371C, 0x9E11},
{0x371E, 0x0EEE},
{0x3720, 0xEECE},
{0x3722, 0x8271},
{0x3724, 0x01F1},
{0x3726, 0x4952},
{0x3740, 0xEF4E},
{0x3742, 0x5DAF},
{0x3744, 0x69D1},
{0x3746, 0xC7D2},
{0x3748, 0xAA33},
{0x374A, 0x9CF0},
{0x374C, 0x96B1},
{0x374E, 0x534F},
{0x3750, 0x16B3},
{0x3752, 0x6172},
{0x3754, 0xC7B0},
{0x3756, 0xA151},
{0x3758, 0x3951},
{0x375A, 0x1F73},
{0x375C, 0x1FD2},
{0x375E, 0xC76E},
{0x3760, 0x534E},
{0x3762, 0x3451},
{0x3764, 0xCD51},
{0x3766, 0xBDB2},
{0x3782, 0x00CC},
{0x3784, 0x0164},
{0x3210, 0x00B8},
#if 0//old AWB_CCM
//[Step5-AWB_CCM] //AWB & CCM
//CCM-Optimized for noise reduce
{0x098E, 0x202F}, // LOGICAL_ADDRESS_ACCESS [CAM_AWB_CONFIG_CCM_L_0]
{0xA02F, 0x01D5}, // CAM_AWB_CONFIG_CCM_L_0
{0xA031, 0xFFCB}, // CAM_AWB_CONFIG_CCM_L_1
{0xA033, 0xFF60}, // CAM_AWB_CONFIG_CCM_L_2
{0xA035, 0xFFA0}, // CAM_AWB_CONFIG_CCM_L_3
{0xA037, 0x01F9}, // CAM_AWB_CONFIG_CCM_L_4
{0xA039, 0xFF66}, // CAM_AWB_CONFIG_CCM_L_5
{0xA03B, 0xFF2C}, // CAM_AWB_CONFIG_CCM_L_6
{0xA03D, 0xFEF7}, // CAM_AWB_CONFIG_CCM_L_7
{0xA03F, 0x02DD}, // CAM_AWB_CONFIG_CCM_L_8
{0xA041, 0x0021}, // CAM_AWB_CONFIG_CCM_L_9
{0xA043, 0x004A}, // CAM_AWB_CONFIG_CCM_L_10
{0xA045, 0xFFF0}, // CAM_AWB_CONFIG_CCM_RL_0
{0xA047, 0xFF8B}, // CAM_AWB_CONFIG_CCM_RL_1
{0xA049, 0x0085}, // CAM_AWB_CONFIG_CCM_RL_2
{0xA04B, 0xFFFD}, // CAM_AWB_CONFIG_CCM_RL_3
{0xA04D, 0x0027}, // CAM_AWB_CONFIG_CCM_RL_4
{0xA04F, 0xFFDC}, // CAM_AWB_CONFIG_CCM_RL_5
{0xA051, 0x009D}, // CAM_AWB_CONFIG_CCM_RL_6
{0xA053, 0x0067}, // CAM_AWB_CONFIG_CCM_RL_7
{0xA055, 0xFEFC}, // CAM_AWB_CONFIG_CCM_RL_8
{0xA057, 0x0010}, // CAM_AWB_CONFIG_CCM_RL_9
{0xA059, 0xFFDD}, // CAM_AWB_CONFIG_CCM_RL_10
{0x940A, 0x0000}, // AWB_X_START
{0x940C, 0x0000}, // AWB_Y_START
{0x940E, 0x027F}, // AWB_X_END
{0x9410, 0x01DF}, // AWB_Y_END
#else
//[Step5-AWB_CCM] //AWB & CCM
//CCM-Optimized for noise reduce
{ 0x098E, 0x202F}, // LOGICAL_ADDRESS_ACCESS [CAM_AWB_CONFIG_CCM_L_0]
{ 0xA02F, 0x0259}, // CAM_AWB_CONFIG_CCM_L_0
{ 0xA031, 0xFF65}, // CAM_AWB_CONFIG_CCM_L_1
{ 0xA033, 0xFF5F}, // CAM_AWB_CONFIG_CCM_L_2
{ 0xA035, 0xFFD8}, // CAM_AWB_CONFIG_CCM_L_3
{ 0xA037, 0x00E1}, // CAM_AWB_CONFIG_CCM_L_4
{ 0xA039, 0x0064}, // CAM_AWB_CONFIG_CCM_L_5
{ 0xA03B, 0xFF5B}, // CAM_AWB_CONFIG_CCM_L_6
{ 0xA03D, 0xFE72}, // CAM_AWB_CONFIG_CCM_L_7
{ 0xA03F, 0x0351}, // CAM_AWB_CONFIG_CCM_L_8
{ 0xA041, 0x0021}, // CAM_AWB_CONFIG_CCM_L_9
{ 0xA043, 0x004A}, // CAM_AWB_CONFIG_CCM_L_10
{ 0xA045, 0x0000}, // CAM_AWB_CONFIG_CCM_RL_0
{ 0xA047, 0xFFAB}, // CAM_AWB_CONFIG_CCM_RL_1
{ 0xA049, 0x0055}, // CAM_AWB_CONFIG_CCM_RL_2
{ 0xA04B, 0x000C}, // CAM_AWB_CONFIG_CCM_RL_3
{ 0xA04D, 0x001F}, // CAM_AWB_CONFIG_CCM_RL_4
{ 0xA04F, 0xFFD5}, // CAM_AWB_CONFIG_CCM_RL_5
{ 0xA051, 0x0097}, // CAM_AWB_CONFIG_CCM_RL_6
{ 0xA053, 0x00AF}, // CAM_AWB_CONFIG_CCM_RL_7
{ 0xA055, 0xFEB9}, // CAM_AWB_CONFIG_CCM_RL_8
{ 0xA057, 0x0010}, // CAM_AWB_CONFIG_CCM_RL_9
{ 0xA059, 0xFFDD}, // CAM_AWB_CONFIG_CCM_RL_10
{ 0x940A, 0x0000}, // AWB_X_START
{ 0x940C, 0x0000}, // AWB_Y_START
{ 0x940E, 0x027F}, // AWB_X_END
{ 0x9410, 0x01DF}, // AWB_Y_END
#endif


//AWB
{0x098E, 0x2061}, // LOGICAL_ADDRESS_ACCESS [CAM_AWB_CONFIG_X_SHIFT_PRE_ADJ]
{0xA061, 0x002A}, // CAM_AWB_CONFIG_X_SHIFT_PRE_ADJ
{0xA063, 0x0038}, // CAM_AWB_CONFIG_Y_SHIFT_PRE_ADJ
{0xA065, 0x0402}, // CAM_AWB_CONFIG_X_SCALE
{0x9408, 0x10F0}, // AWB_LUMA_THRESH_HIGH
{0x9416, 0x2D8C}, // AWB_R_SCENE_RATIO_LOWER
{0x9418, 0x1678}, // AWB_B_SCENE_RATIO_LOWER
{0x2112, 0x0000},
{0x2114, 0x0000},
{0x2116, 0x0000},
{0x2118, 0x0F80},
{0x211A, 0x2A80},
{0x211C, 0x0B40},
{0x211E, 0x01AC},
{0x2120, 0x0038},
//[Step6-CPIPE_Calibration] //Color Pipe Calibration settings, if any
//[Step7-CPIPE_Preference] //Color Pipe preference settings, if any
//NR
{0x326E, 0x0006}, // LOW_PASS_YUV_FILTER
{0x33F4, 0x000B}, // KERNEL_CONFIG
//GAMMA
{0x098E, 0xA087},
{0xA087, 0x0007},
{0xA089, 0x1630},
{0xA08B, 0x526d},
{0xA08D, 0x869b},
{0xA08F, 0xAbb9},
{0xA091, 0xC5cf},
{0xA093, 0xD8e0},
{0xA095, 0xE7ee},
{0xA097, 0xF4fa},
{0xA0AD, 0x0001},
{0xA0AF, 0x0338},
////AE Preferred
{0x098E, 0xA0B1}, // LOGICAL_ADDRESS_ACCESS [CAM_LL_CONFIG_NR_RED_START]
{0xA0B1, 0x1e2D}, //, //1, //E CAM_LL_CONFIG_NR_RED_START
{0xA0B3, 0x1e2D}, // CAM_LL_CONFIG_NR_GREEN_START
{0xA0B5, 0x1e2D}, // CAM_LL_CONFIG_NR_BLUE_START
{0xA0B7, 0x1e2D}, // CAM_LL_CONFIG_NR_MIN_MAX_START
{0xA0B9, 0x0040}, // CAM_LL_CONFIG_START_GAIN_METRIC
{0xA0BB, 0x00C8}, // CAM_LL_CONFIG_STOP_GAIN_METRIC
{0xA07A, 0x040F}, // CAM_LL_CONFIG_AP_THRESH_START
{0xA07C, 0x0300}, // CAM_LL_CONFIG_AP_GAIN_START
{0xA07E, 0x0078}, // CAM_LL_CONFIG_CDC_THRESHOLD_BM
{0xA080, 0x0528}, // CAM_LL_CONFIG_CDC_GATE_PERCENTAGE
{0xA081, 0x2832},
{0xA083, 0x000F}, // CAM_LL_CONFIG_FTB_AVG_YSUM_START
{0xA085, 0x0000}, // CAM_LL_CONFIG_FTB_AVG_YSUM_STOP
{0xA01F, 0x8040}, // CAM_AE_CONFIG_BASE_TARGET
{0xA027, 0x0050}, // CAM_AE_CONFIG_MIN_VIRT_AGAIN
{0xA029, 0x0140}, // CAM_AE_CONFIG_MAX_VIRT_AGAIN
{0xA025, 0x0080}, // CAM_AE_CONFIG_MAX_VIRT_DGAIN
{0xA01C, 0x00C8}, // CAM_AE_CONFIG_TARGET_AGAIN
{0xA01E, 0x0080}, // CAM_AE_CONFIG_TARGET_DGAIN
{0xA01A, 0x000B}, // CAM_AE_CONFIG_TARGET_FDZONE
{0xA05F, 0xD428}, // CAM_AWB_CONFIG_START_SATURATION
{0xA05B, 0x0005}, // CAM_AWB_CONFIG_START_BRIGHTNESS_BM
{0xA05D, 0x0023}, // CAM_AWB_CONFIG_STOP_BRIGHTNESS_BM
{0x9801, 0x000F}, // STAT_CLIP_MAX
{0x9C01, 0x3201}, // LL_GAMMA_SELECT
{0x9001, 0x0564}, // AE_TRACK_MODE
{0x9007, 0x0509}, // AE_TRACK_TARGET_GATE
{0x9003, 0x2D02}, // AE_TRACK_BLACK_LEVEL_MAX
{0x9005, 0x2D09}, // AE_TRACK_BLACK_CLIP_PERCENT
{0x8C03, 0x0103}, // FD_STAT_MIN
{0x8C04, 0x0305},
//[Step8-Features] //Ports, special features, etc., if any
{0x098E, 0xA05F}, // LOGICAL_ADDRESS_ACCESS [CAM_AWB_CONFIG_START_SATURATION]
{0xA05F, 0x601F}, // CAM_AWB_CONFIG_START_SATURATION
{0xA029, 0x00f8}, // d0 modify @20100706for lowlight #20 light CAM_AE_CONFIG_MAX_VIRT_AGAIN

{0x3040, 0x4041},
//EXIT Standby
{0x0018, 0x0002}, // STANDBY_CONTROL_AND_STATUS
};

static struct camsensor_mt9v114_reg_type camsensor_mt9v114_effect_off_reg_tbl[NUM_REGS_EFFECT] = 
{
{0x098E, 0x8400},
{0xA010, 0x0041},
{0,5},
};

static struct camsensor_mt9v114_reg_type camsensor_mt9v114_effect_mono_reg_tbl[NUM_REGS_EFFECT] = 
{
{0x098E, 0x8400},
{0xA010, 0x0141},
{0,5},
};

static struct camsensor_mt9v114_reg_type camsensor_mt9v114_effect_negative_reg_tbl[NUM_REGS_EFFECT] =
{
{0x098E, 0x8400},
{0xA010, 0x0341},
{0,5},
};

static struct camsensor_mt9v114_reg_type camsensor_mt9v114_effect_sepia_reg_tbl[NUM_REGS_EFFECT] = 
{
{0x098E, 0x8400},
{0xA010, 0x0241},
{0xA012, 0x1ED8},
 {0,5},
};


/*=============================================================*/

static int32_t mt9v114_i2c_txdata(unsigned short saddr,
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

	if (i2c_transfer(mt9v114_client->adapter, msg, 1) < 0) {
		CDBG("mt9v114_i2c_txdata failed\n");
		return -EIO;
	}

	return 0;
}

static int32_t mt9v114_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata)
{
	int32_t rc = -EIO;
	unsigned char buf[4];
  //CDBG("mt9v114_i2c_write , addr = 0x%x, val = 0x%x!\n", waddr, wdata);
	memset(buf, 0, sizeof(buf));

	buf[0] = (waddr & 0xFF00)>>8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = (wdata & 0xFF00)>>8;
	buf[3] = (wdata & 0x00FF);

	rc = mt9v114_i2c_txdata(saddr, buf, 4);

	if (rc < 0)
		CDBG("i2c_write failed, addr = 0x%x, val = 0x%x!\n",waddr, wdata);
	return rc;
}


static int mt9v114_i2c_rxdata(unsigned short saddr,
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

	if (i2c_transfer(mt9v114_client->adapter, msgs, 2) < 0) {
		CDBG("mt9v114_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t mt9v114_i2c_read(unsigned short   saddr,
	unsigned short raddr, unsigned short *rdata)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00)>>8;
	buf[1] = (raddr & 0x00FF);

	rc = mt9v114_i2c_rxdata(saddr, buf, 2);
	if(rc >= 0)
	{
	*rdata = buf[0] << 8 | buf[1];
	return 0;
	}
       CDBG("mt9v114_i2c_read failed, raddr = 0x%x\n", raddr);
	if (rc < 0)
		CDBG("mt9v114_i2c_read failed!\n");

	return rc;
}

 
static int32_t mt9v114_i2c_write_w_table(
	struct camsensor_mt9v114_reg_type const *reg_conf_tbl,
	int num_of_items_in_table)
{
	int i;
	int32_t rc = -EFAULT;
	for (i = 0; i < num_of_items_in_table; i++) {
		if(0==reg_conf_tbl->addr)
			{
			mdelay(reg_conf_tbl->val);
			}
		else
			{
			rc = mt9v114_i2c_write(mt9v114_client->addr,
				reg_conf_tbl->addr, reg_conf_tbl->val);
			if (rc < 0)
			break;
			}
		reg_conf_tbl++;
	}
	return rc;
}
 static long mt9v114_set_sensor_mode(int mode)
{
     	long rc = 0;

	return rc;
}

 static int mt9v114_set_effect(int mode,int8_t effect)
{

	int rc = 0;
	
	switch (effect) {
	case CAMERA_EFFECT_OFF: 
	       rc = mt9v114_i2c_write_w_table(camsensor_mt9v114_effect_off_reg_tbl,
                    sizeof(camsensor_mt9v114_effect_off_reg_tbl)/sizeof(camsensor_mt9v114_effect_off_reg_tbl[0]));
	       if (rc < 0) {
			CDBG("mt9v114_effect_off failed!\n");
			goto set_effect_fail;
		    }
		   
		break;
		
	case CAMERA_EFFECT_MONO: 
		rc = mt9v114_i2c_write_w_table(camsensor_mt9v114_effect_mono_reg_tbl,
	             sizeof(camsensor_mt9v114_effect_mono_reg_tbl)/sizeof(camsensor_mt9v114_effect_mono_reg_tbl[0]));
		if (rc < 0) {
			CDBG("mt9v114_effect_mono failed!\n");
			goto set_effect_fail;
			}
	
		break;

	case CAMERA_EFFECT_NEGATIVE: 
		rc = mt9v114_i2c_write_w_table(camsensor_mt9v114_effect_negative_reg_tbl,
	              sizeof(camsensor_mt9v114_effect_negative_reg_tbl)/sizeof(camsensor_mt9v114_effect_negative_reg_tbl[0]));
		if (rc < 0) {
			CDBG("mt9v114_effect_negative failed!\n");
			goto set_effect_fail;
			}
		
		break;

	case CAMERA_EFFECT_SOLARIZE: {

	}
		break;

	case CAMERA_EFFECT_SEPIA: 
	       rc = mt9v114_i2c_write_w_table(camsensor_mt9v114_effect_sepia_reg_tbl,
                    sizeof(camsensor_mt9v114_effect_sepia_reg_tbl)/sizeof(camsensor_mt9v114_effect_sepia_reg_tbl[0]));
		if (rc < 0) {
			CDBG("mt9v114_effect_sepia failed!\n");
			goto set_effect_fail;
			}

		break;

	default: {
	             rc = -EFAULT;
			return rc;

	}
	}
return rc;

set_effect_fail:
	return rc;
}

static long mt9v114_set_whitebalance(int mode,int8_t wb)
{

	long rc = 0;

	CDBG("mt9v114_set_whitebalance  wb=%d\n",wb);

	switch ((enum config3a_wb_t)wb) {
	case CAMERA_WB_AUTO: {
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,	0x098E, 0x9401);
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,	0x9401, 0x0D00);
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,	0x9406, 0x007F);
		
	       mdelay(50);
	}
		break;

	case CAMERA_WB_INCANDESCENT: {
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,	0x098E, 0x9401);
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,	0x9401, 0x0C00);
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,	0x9436, 0x4C31);
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,0x9406, 0x3A3A);

	       mdelay(50);
	}
		break;

 	case CAMERA_WB_CLOUDY_DAYLIGHT: {

		rc = mt9v114_i2c_write(mt9v114_client->addr,0x098E, 0x9401);
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,0x9401, 0x0C00);
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,0x9436, 0x3D47);
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,0x9406, 0x7F7F);

	       mdelay(50);
	}
		break;

	case CAMERA_WB_DAYLIGHT: {

		rc = mt9v114_i2c_write(mt9v114_client->addr,0x098E, 0x9401);
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,0x9401, 0x0C00);
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,0x9436, 0x4144);
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,0x9406, 0x7474);

		mdelay(50);
	}
		break;
        case CAMERA_WB_FLUORESCENT: {
			
		rc = mt9v114_i2c_write(mt9v114_client->addr,0x098E, 0x9401);
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,0x9401, 0x0C00);
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,0x9436, 0x4F33);
		
		rc = mt9v114_i2c_write(mt9v114_client->addr,0x9406, 0x4444);

	
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

static long mt9v114_set_mirror(	int mode,int8_t mirror)
{

	long rc = 0;
	return rc;
}


static int mt9v114_sensor_init_done(const struct msm_camera_sensor_info *data)
{
	gpio_direction_output(data->sensor_reset, 0);
	gpio_free(data->sensor_reset);

	gpio_direction_output(data->sensor_pwd, 1);
	gpio_free(data->sensor_pwd);
#if 0  /*only enter standby mode*/
	if (data->vreg_enable_func){
		data->vreg_enable_func(0);
		}
#endif
	return 0;
              
}

static int mt9v114_sensor_init_probe(const  struct msm_camera_sensor_info *data)
{
       int rc;
	unsigned short chipid=0;	

	rc = gpio_request(data->sensor_pwd, "mt9v114");
	if (!rc || rc == -EBUSY)
		gpio_direction_output(data->sensor_pwd, 0);
	else 
	       goto init_probe_fail;

	rc = gpio_request(data->sensor_reset, "mt9v114");
	if (!rc) 
		rc = gpio_direction_output(data->sensor_reset, 0);
	else
		goto init_probe_fail;
       mdelay(10);  
	   
	if (data->vreg_enable_func){
              data->vreg_enable_func(1);
    	}
	mdelay(20);

	rc = mt9v114_i2c_read(mt9v114_client->addr,
	REG_MT9V114_MODEL_ID, &chipid);

       rc = mt9v114_i2c_write_w_table(mt9v114_init_reg_config,
                sizeof(mt9v114_init_reg_config) / sizeof(mt9v114_init_reg_config[0]));
	if (rc < 0)
		goto init_probe_fail;
    goto init_probe_done;

init_probe_fail:
    mt9v114_sensor_init_done(data);
init_probe_done:
	return rc;
       
}

int mt9v114_sensor_init(const struct msm_camera_sensor_info *data)
{
       
	int rc = 0;
	mt9v114_ctrl = kzalloc(sizeof(struct mt9v114_ctrl_t), GFP_KERNEL);
	if (!mt9v114_ctrl) {
		CDBG("mt9v114_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		mt9v114_ctrl->sensordata = data;

	msm_camio_clk_rate_set(24000000);
	mdelay(5);

	msm_camio_camif_pad_reg_reset();

	rc = mt9v114_sensor_init_probe(data);

       rc = mt9v114_i2c_write_w_table(mt9v114_init_reg_config,
                            sizeof(mt9v114_init_reg_config) / sizeof(mt9v114_init_reg_config[0]));
	if (rc < 0) {
		CDBG("mt9v114_sensor_init failed!\n");
		goto init_fail;
	}

init_done:
	return rc;

init_fail:
	kfree(mt9v114_ctrl);
	return rc;
}


static int mt9v114_init_client(struct i2c_client *client)
{

	init_waitqueue_head(&mt9v114_wait_queue);
	return 0;
}

int mt9v114_sensor_config(void __user *argp)
{
       
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	if (copy_from_user(
				&cfg_data,
				(void *)argp,
				sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	 //down(&mt9v114_sem); 

	CDBG("mt9v114_ioctl, cfgtype = %d, mode = %d\n",cfg_data.cfgtype, cfg_data.mode);

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = mt9v114_set_sensor_mode(
					cfg_data.mode);
		break;

	case CFG_SET_EFFECT:
		rc = mt9v114_set_effect(
					cfg_data.mode,
					cfg_data.cfg.effect);
		break;
		
	case CFG_SET_WB:
		rc=mt9v114_set_whitebalance(
					cfg_data.mode,
					cfg_data.cfg.whitebalance);
		break;
	case CFG_SET_MIRROR:
		rc=mt9v114_set_mirror(
					cfg_data.mode,
					cfg_data.cfg.mirror);
		break;

	default:
		rc = -EFAULT;
		break;
	}
	//up(&mt9v114_sem); 

	return rc;
}

static int mt9v114_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	mt9v114_sensorw =
		kzalloc(sizeof(struct mt9v114_work_t), GFP_KERNEL);

	if (!mt9v114_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, mt9v114_sensorw);
	mt9v114_init_client(client);
	mt9v114_client = client;

	CDBG("mt9v114_probe successed!\n");

	return 0;

probe_failure:
	kfree(mt9v114_sensorw);
	mt9v114_sensorw = NULL;
	CDBG("mt9v114_probe failed!\n");
	return rc;
}

int mt9v114_sensor_release(void)
{
	int rc = 0;

	/* down(&mt9v114_sem); */
	mt9v114_sensor_init_done(mt9v114_ctrl->sensordata);
	kfree(mt9v114_ctrl);
	/* up(&mt9v114_sem); */

	return rc;
}

static const struct i2c_device_id mt9v114_i2c_id[] = {
	{ "mt9v114", 0},
	{ },
};

static struct i2c_driver mt9v114_i2c_driver = {
	.id_table = mt9v114_i2c_id,
	.probe  = mt9v114_probe,
	.remove = __exit_p(mt9v114_i2c_remove),
	.driver = {
		.name = "mt9v114",
	},
};

static int mt9v114_sensor_probe(const struct msm_camera_sensor_info *info,
									struct msm_sensor_ctrl *s)
{
	int32_t rc = 0;
	CDBG("-----mt9v114_probe_init------begin \n");
	rc = i2c_add_driver(&mt9v114_i2c_driver);
	if (rc < 0 || mt9v114_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_done;
	}

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);
	mdelay(1);

	rc = mt9v114_sensor_init_probe(info);
	if (rc < 0)
		goto probe_done;
      //mdelay(100000);
	
	gCameraFrontNums_mt9v114 = 1;

	s->s_init = mt9v114_sensor_init;
	s->s_release = mt9v114_sensor_release;
	s->s_config  = mt9v114_sensor_config;
        mt9v114_sensor_init_done(info);
probe_done:
	CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;

}

static int __mt9v114_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, mt9v114_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __mt9v114_probe,
	.driver = {
		.name = "msm_camera_mt9v114",
		.owner = THIS_MODULE,
	},
};

static int __init mt9v114_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(mt9v114_init);
EXPORT_SYMBOL(gCameraFrontNums_mt9v114);


