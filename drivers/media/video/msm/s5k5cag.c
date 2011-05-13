/*=============================================================*/
/*add sensor s5k5cag
/*=============================================================*/
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include "s5k5cag.h"
#include <linux/sysfs.h>
#include <linux/kobject.h>

#define  S5K5CAG_MODEL_ID     0x05ca

#define S5K5CAG_I2C_RETRY_TIMES        3

struct s5k5cag_work {
	struct work_struct work;
};

static struct  s5k5cag_work *s5k5cag_sensorw;
static struct  i2c_client *s5k5cag_client;


struct s5k5cag_ctrl {
	const struct msm_camera_sensor_info *sensordata;
};


static struct s5k5cag_ctrl *s5k5cag_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(s5k5cag_wait_queue);
DECLARE_MUTEX(s5k5cag_sem);

/*=============================================================*/

static int s5k5cag_reset(const struct msm_camera_sensor_info *dev)
{
	int rc = 0;
	CDBG("s5k5cag_reset-------------------------------begin\n");
	rc = gpio_request(dev->sensor_pwd, "s5k5cag");
	if (!rc || rc == -EBUSY)
		gpio_direction_output(dev->sensor_pwd, 1);
	else 
		return rc;
	mdelay(10);
       
	if (dev->vreg_enable_func)
		dev->vreg_enable_func(1);
	mdelay(20);	
      
	gpio_direction_output(dev->sensor_pwd, 0);
	mdelay(10);	

	rc = gpio_request(dev->sensor_reset, "s5k5cag");
	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 0);
	}
	else
		return rc;
	mdelay(10);

	rc = gpio_direction_output(dev->sensor_reset, 1);
	mdelay(5);
	   mdelay(10);

	CDBG("s5k5cag_reset-------------------------------end\n");
	return rc;
}


static int s5k5cag_sensor_init_done(const struct msm_camera_sensor_info *data)
{

	CDBG("s5k5cag_sensor_init_done-------------------------------begin\n");
    //gpio_direction_output(data->sensor_reset, 0);
    //gpio_free(data->sensor_reset);
	
	gpio_direction_output(data->sensor_pwd, 1);
	gpio_free(data->sensor_pwd);
	CDBG("s5k5cag_sensor_init_done-----entry standby mode delay 100ms--------------\n");
	mdelay(200);
       #if 0  //entry standby mode
	if (data->vreg_enable_func)
	{
	data->vreg_enable_func(0);
	}
       #endif
	CDBG("s5k5cag_sensor_init_done-------------------------------end\n");
	return 0;
}

static int32_t s5k5cag_i2c_txdata(unsigned short saddr,
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

	if (i2c_transfer(s5k5cag_client->adapter, msg, 1) < 0) {
		CDBG("s5k5cag_i2c_txdata faild\n");
		return -EIO;
	}

	return 0;
}

static int32_t s5k5cag_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata)
{
	int32_t rc = -EIO;
	unsigned char buf[4];
	
	memset(buf, 0, sizeof(buf));

	buf[0] = (waddr & 0xFF00)>>8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = (wdata & 0xFF00)>>8;
	buf[3] = (wdata & 0x00FF);		

	rc = s5k5cag_i2c_txdata(saddr, buf, 4);
	if (rc < 0)
		CDBG("i2c_write failed, addr = 0x%x, val = 0x%x!\n",waddr, wdata);
	return rc;
}



static int s5k5cag_i2c_rxdata(unsigned short saddr,
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

	if (i2c_transfer(s5k5cag_client->adapter, msgs, 2) < 0) {
		CDBG("s5k5cag_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t s5k5cag_i2c_read(unsigned short   saddr,
	unsigned short raddr, unsigned short *rdata)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00)>>8;
	buf[1] = (raddr & 0x00FF);

	rc = s5k5cag_i2c_rxdata(saddr, buf, 2);
	if(rc >= 0)
	{
	*rdata = buf[0] << 8 | buf[1];
	return 0;
	}
       CDBG("s5k5cag_i2c_read failed, raddr = 0x%x\n", raddr);
	if (rc < 0)
		CDBG("s5k5cag_i2c_read failed!\n");

	return rc;
}


static long s5k5cag_reg_init(void)
{
	int32_t array_length;
	int32_t i;
	struct register_address_value_pair const * reg_init;
	long rc;
	array_length=s5k5cag_regs.prev_snap_reg_init_size;
	reg_init=&(s5k5cag_regs.prev_snap_reg_init[0]);

	for (i = 0; i < array_length; i++) {

		if(0==reg_init[i].register_address)
			{
			mdelay(reg_init[i].register_value);
			}
		else
			{
			rc=s5k5cag_i2c_write(s5k5cag_client->addr,
		  		reg_init[i].register_address,reg_init[i].register_value);
			if (rc < 0)
				return rc;
			}
	}
	
	return 0;
}

 
static long s5k5cag_set_sensor_mode(int mode)
{

	long rc = 0;
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		printk("--------s5k5cag_set_sensor_mode start mode=%d----SENSOR_PREVIEW_MODE--------\n",mode);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0028, 0x7000);
		if (rc < 0)
			break;
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x002a, 0x023c);
		//rc = s5k5cag_i2c_write(s5k5cag_client->addr,
		//	0x0f12, 0x0000);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0f12, 0x0001);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x002a, 0x0240);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0f12, 0x0001);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x002a, 0x0230);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0f12, 0x0001);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x002a, 0x023e);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0f12, 0x0001);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x002a, 0x0220);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0f12, 0x0001);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0f12, 0x0001);

		mdelay(100);

		break;

	case SENSOR_SNAPSHOT_MODE:
		/* Switch to lower fps for Snapshot */
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0028, 0x7000);
		if (rc < 0)
			break;
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x002a, 0x0244);
		//rc = s5k5cag_i2c_write(s5k5cag_client->addr,
		//	0x0f12, 0x0001);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0f12, 0x0000);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x002a, 0x0230);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0f12, 0x0001);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x002a, 0x0246);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0f12, 0x0001);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x002a, 0x0224);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0f12, 0x0001);
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0f12, 0x0001);

		mdelay(100);

		break;

	default:
		return -EFAULT;
	}

	return 0;
}


static long s5k5cag_set_effect(int mode,int effect)
{

	long rc = 0;
	CDBG("s5k5cag_set_effect  effect=%d\n",effect);

	switch (effect) {
	case CAMERA_EFFECT_OFF: {
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	
			0x0028, 0x7000);		
		if (rc < 0)			
			break;		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,			
			0x002a, 0x021e);		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,			
			0x0f12, 0x0000);
	}
		break;

	case CAMERA_EFFECT_MONO: {
		 rc = s5k5cag_i2c_write(s5k5cag_client->addr,
		 	0x0028, 0x7000);		
		 if (rc < 0)			
		 	break;		
		 rc = s5k5cag_i2c_write(s5k5cag_client->addr,			
		 	0x002a, 0x021e);		 
		 rc = s5k5cag_i2c_write(s5k5cag_client->addr,			
		 	0x0f12, 0x0001);
	}
		break;

	case CAMERA_EFFECT_NEGATIVE: {
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0028, 0x7000);		
		if (rc < 0)			
			break;		 
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,			
			0x002a, 0x021e);		 
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,			
			0x0f12, 0x0003);
	}
		break;

	case CAMERA_EFFECT_SOLARIZE: {

	}
		break;

	case CAMERA_EFFECT_SEPIA: {
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,
			0x0028, 0x7000);		
		if (rc < 0)			
			break;		 
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,			
			0x002a, 0x021e);		 
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,			
			0x0f12, 0x0004);
	}
		break;

	default: {
		break;

		//return -EINVAL;
	}
	}

	return rc;
}


static long s5k5cag_set_whitebalance(	int mode,int8_t wb)
{

	long rc = 0;


	switch ((enum config3a_wb_t)wb) {
	case CAMERA_WB_AUTO: {
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0028, 0x7000);
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x002A, 0X04D2);
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x067F);

		mdelay(50);
	}
		break;

	case CAMERA_WB_INCANDESCENT: 	{
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0028, 0x7000);
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x002A, 0x04D2);
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0677); // AWB Off    
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x002A, 0x04A0); //#REG_SF_USER_Rgain   
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0380);  
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0001);  
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0400); //REG_SF_USER_Ggain  
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0001);
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x09C0); // #REG_SF_USER_Bgain 
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0001);                                                      

              mdelay(50);
	}
		break;

 	case CAMERA_WB_CLOUDY_DAYLIGHT: {

		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0028, 0x7000);  
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x002A, 0x04D2);    
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0677); // AWB Off     
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x002A, 0x04A0); //#REG_SF_USER_Rgain  
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0540);        
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0001); //#REG_SF_USER_Rgainchanged  
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0400); //REG_SF_USER_Ggain      
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0001); //REG_SF_USER_Ggainchanged  
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0500); // #REG_SF_USER_Bgain  
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0001); // #REG_SF_USER_Bgainchanged  

		mdelay(50);
	}
		break;

	case CAMERA_WB_DAYLIGHT: {
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0028, 0x7000);
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x002A, 0x04D2);  
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0677); // AWB Off 
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x002A, 0x04A0); //#REG_SF_USER_Rgain 
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x05A0); 
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0001); 
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0400); //REG_SF_USER_Ggain 
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0001);
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x05F0); // #REG_SF_USER_Bgain 
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0001);                         

		mdelay(50);

	}
		break;
        case CAMERA_WB_FLUORESCENT: {
			
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0028, 0x7000); 
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x002A, 0x04D2);     
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0677); // AWB Off      
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x002A, 0x04A0); //#REG_SF_USER_Rgain  
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0400);   
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0001);  
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0400); //REG_SF_USER_Ggain  
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0001);  
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x083C); // #REG_SF_USER_Bgain 
		
		rc = s5k5cag_i2c_write(s5k5cag_client->addr,	0x0F12, 0x0001);                            

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

	CDBG("s5k5cag_set_whitebalance  wb=%d----------------end\n",wb);

	return rc;
}

static long s5k5cag_set_mirror(	int mode,int8_t mirror)
{

	long rc = 0;

	CDBG(KERN_ERR "s5k5cag_set_mirror  mirror=%d\n",mirror);

	return rc;
}

static int s5k5cag_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	uint16_t model_id = 0;
	int rc = 0;

	CDBG("init entry \n");
	rc = s5k5cag_reset(data);
	if (rc < 0) {
		CDBG("reset failed!\n");
		goto init_probe_fail;
	}
	
	/* Read the Model ID of the sensor */
	rc = s5k5cag_i2c_write(s5k5cag_client->addr,
		0x002c, 0x0000);
	if (rc < 0)
		goto init_probe_fail;
	rc = s5k5cag_i2c_write(s5k5cag_client->addr,
		0x002e, 0x0040);
	if (rc < 0)
		goto init_probe_fail;
	rc = s5k5cag_i2c_read(s5k5cag_client->addr,
		0x0f12, &model_id);
	if (rc < 0)
		goto init_probe_fail;

	CDBG("s5k5cag model_id = 0x%x\n", model_id);
	
	if (model_id != S5K5CAG_MODEL_ID) {
		rc = -EFAULT;
		goto init_probe_fail;
	}

	rc = s5k5cag_reg_init();
	if (rc < 0)
		goto init_probe_fail;

        mdelay(10);
        

	return rc;


init_probe_fail:
	CDBG("s5k5cag init probe fail \n");
	s5k5cag_sensor_init_done(data);
	return rc;
}

static int s5k5cag_sensor_init_probe_init(const struct msm_camera_sensor_info *data)
{
	uint16_t model_id = 0;
	int rc = 0;
	/*rc = s5k5cag_reset(data);
	if (rc < 0) {
		CDBG("reset failed!\n");
		goto init_probe_fail;
	}
	*/
	rc = gpio_request(data->sensor_pwd, "s5k5cag");
	if (!rc || rc == -EBUSY)
		gpio_direction_output(data->sensor_pwd, 0);
	else 
		return rc;
	mdelay(10);
	
	/* Read the Model ID of the sensor */
	rc = s5k5cag_i2c_write(s5k5cag_client->addr,
		0x002c, 0x0000);
	if (rc < 0)
		goto init_probe_fail;
	rc = s5k5cag_i2c_write(s5k5cag_client->addr,
		0x002e, 0x0040);
	if (rc < 0)
		goto init_probe_fail;
	rc = s5k5cag_i2c_read(s5k5cag_client->addr,
		0x0f12, &model_id);
	if (rc < 0)
		goto init_probe_fail;

	CDBG("s5k5cag model_id = 0x%x\n", model_id);
	
	if (model_id != S5K5CAG_MODEL_ID) {
		rc = -EFAULT;
		goto init_probe_fail;
	}	
	//rc = s5k5cag_reg_init();
	//if (rc < 0)
	//	goto init_probe_fail;
    mdelay(10);        

	return rc;

init_probe_fail:
	CDBG("s5k5cag init probe fail \n");
	s5k5cag_sensor_init_done(data);
	return rc;
}

int s5k5cag_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	s5k5cag_ctrl = kzalloc(sizeof(struct s5k5cag_ctrl), GFP_KERNEL);
	if (!s5k5cag_ctrl) {
		CDBG("s5k5cag_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		s5k5cag_ctrl->sensordata = data;

	/* Input MCLK = 24MHz */

	msm_camio_clk_rate_set(24000000);
	mdelay(5);

	msm_camio_camif_pad_reg_reset();
	//s5k5cag_sensor_init_probe(data);
	s5k5cag_sensor_init_probe_init(data);

	if (rc < 0) {
		goto init_fail;
	}

init_done:
	return rc;

init_fail:
	kfree(s5k5cag_ctrl);
	return rc;
}

static int s5k5cag_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&s5k5cag_wait_queue);
	return 0;
}

int s5k5cag_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;

	if (copy_from_user(
				&cfg_data,
				(void *)argp,
				sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	/* down(&s5k5cag_sem); */

	CDBG("s5k5cag_ioctl, cfgtype = %d, mode = %d\n",
		cfg_data.cfgtype, cfg_data.mode);

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = s5k5cag_set_sensor_mode(
					cfg_data.mode);
		break;

	case CFG_SET_EFFECT:
		rc = s5k5cag_set_effect(
					cfg_data.mode,
					cfg_data.cfg.effect);
		break;
	case CFG_SET_WB:
		rc=s5k5cag_set_whitebalance(
					cfg_data.mode,
					cfg_data.cfg.whitebalance);
		break;
	case CFG_SET_MIRROR:
		rc=s5k5cag_set_mirror(
					cfg_data.mode,
					cfg_data.cfg.mirror);
		break;
	default:
		rc = -EINVAL;
		break;
	}

	/* up(&s5k5cag_sem); */

	return rc;
}

int s5k5cag_sensor_release(void)
{
	int rc = 0;

	/* down(&s5k5cag_sem); */
	s5k5cag_sensor_init_done(s5k5cag_ctrl->sensordata);
	kfree(s5k5cag_ctrl);
	/* up(&s5k5cag_sem); */

	return rc;
}

static int s5k5cag_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	s5k5cag_sensorw =
		kzalloc(sizeof(struct s5k5cag_work), GFP_KERNEL);

	if (!s5k5cag_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, s5k5cag_sensorw);
	s5k5cag_init_client(client);
	s5k5cag_client = client;

	CDBG("s5k5cag_probe successed!\n");

	return 0;

probe_failure:
	kfree(s5k5cag_sensorw);
	s5k5cag_sensorw = NULL;
	CDBG("s5k5cag_probe failed!\n");
	return rc;
}



static const struct i2c_device_id s5k5cag_i2c_id[] = {
	{ "s5k5cag", 0},
	{ },
};

static struct i2c_driver s5k5cag_i2c_driver = {
	.id_table = s5k5cag_i2c_id,
	.probe  = s5k5cag_i2c_probe,
	.remove = __exit_p(s5k5cag_i2c_remove),
	.driver = {
		.name = "s5k5cag",
	},
};


#ifdef CONFIG_MT9V114
extern unsigned int gCameraFrontNums_mt9v114;
#else
unsigned int gCameraFrontNums_mt9v114 = 0;
#endif

static unsigned int gCameraBackNums_s5k5cag = 0;

static ssize_t camera_attr_show(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf)
{
    ssize_t iLen = 0;

    if(1 == gCameraFrontNums_mt9v114)
    {
        iLen += sprintf(buf, "%s", "INNER_CAMERA = true\n");
    }
    if(1 == gCameraBackNums_s5k5cag)
    {
        iLen += sprintf(buf+iLen, "%s", "OUTER_CAMERA = true\n");
    }

    return iLen;
}

 static struct kobj_attribute camera_attribute =
         __ATTR(sensor, 0666, camera_attr_show, NULL);

 static struct attribute* camera_attributes[] =
 {
         &camera_attribute.attr,
         NULL
 };

 static struct attribute_group camera_defattr_group =
 {
         .attrs = camera_attributes,
 };

static int s5k5cag_sensor_probe(const struct msm_camera_sensor_info *info,
									struct msm_sensor_ctrl *s)
{
	int32_t rc = 0;

	struct kobject *kobj = NULL;

	rc = i2c_add_driver(&s5k5cag_i2c_driver);
	if (rc < 0 || s5k5cag_client == NULL) {
		rc = -ENOTSUPP;
		goto probe_done;
	}

	/* Input MCLK = 24MHz */

	msm_camio_clk_rate_set(24000000);//
	
	mdelay(5);
#if 0
	kobj = kobject_create_and_add("camera", NULL);
	if (kobj == NULL) {
		return -1;
	}
	if (sysfs_create_group(kobj, &camera_defattr_group)) {
		kobject_put(kobj);
		return -1;
	}
#endif
	rc = s5k5cag_sensor_init_probe(info);    
	if (rc < 0)
		goto probe_done;
	kobj = kobject_create_and_add("camera", NULL);
	if (kobj == NULL) {
		return -1;
	}
	if (sysfs_create_group(kobj, &camera_defattr_group)) {
		kobject_put(kobj);
		return -1;
	}
	gCameraBackNums_s5k5cag = 1;
  
	s->s_init = s5k5cag_sensor_init;
	s->s_release = s5k5cag_sensor_release;
	s->s_config  = s5k5cag_sensor_config;
        s5k5cag_sensor_init_done(info);
probe_done:
	return rc;

}

static int __s5k5cag_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, s5k5cag_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __s5k5cag_probe,
	.driver = {
		.name = "msm_camera_s5k5cag", 
		.owner = THIS_MODULE,
	},
};

static int __init s5k5cag_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

late_initcall(s5k5cag_init);

