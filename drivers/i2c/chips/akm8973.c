/* Add compass. Ported by suchangyu. 20100520. begin. */
/*BK4D00235, add  akm8973 of compass  code, dingxifeng KF14049, 2009-5-9 begin */


/* drivers/i2c/chips/akm8973.c - akm8973 compass driver
 *
 * Copyright (C) 2007-2008 HTC Corporation.
 * Author: Hou-Kun Chen <houkun.chen@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/gpio.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/akm8973.h>
#include <linux/earlysuspend.h>
#include <mach/akm8972_board.h>
#ifdef CONFIG_ANDROID_POWER
#include <linux/android_power.h>
#endif
#include <mach/board.h>

#include <linux/sysfs.h>
#include <linux/kobject.h>
#include "linux/hardware_self_adapt.h"
#include <mach/msm_rpcrouter.h>
#define DEBUG 0
#define MAX_FAILURE_COUNT 3

static int COMPASS_RST = 0; 
#define RXDATA_LENGTH	4

static struct i2c_client *this_client;


extern struct input_dev *sensor_dev;


struct akm8973_data {
	struct input_dev *input_dev;
	struct work_struct work;
	struct early_suspend early_suspend;
};
#ifdef CONFIG_HAS_EARLYSUSPEND
static void akm8973_early_suspend(struct early_suspend *h);
static void akm8973_early_resume(struct early_suspend *h);
#endif


/* Addresses to scan -- protected by sense_data_mutex */
static char sense_data[RBUFF_SIZE + 1];
static struct mutex sense_data_mutex;
#define AKM8973_RETRY_COUNT 10
static DECLARE_WAIT_QUEUE_HEAD(data_ready_wq);
static DECLARE_WAIT_QUEUE_HEAD(open_wq);

static atomic_t data_ready;

#ifdef CONFIG_UPDATE_COMPASS_FIRMWARE 
static char AKECS_ETST=0;
#define   AKECS_FACTORY_ETST       0xc7    
#define   AKECS_RECOMPOSE_ETST  0xCC    

static ssize_t set_e2prom_show(struct kobject *kobj, struct kobj_attribute *attr,char *buf);
static ssize_t  set_e2prom_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

static int akm8973_set_e2prom_file(void);

static struct kobj_attribute set_e2prom_attribute = {
	.attr = {.name = "set_e2prom", .mode = 0666},
	.show = set_e2prom_show,
	.store = set_e2prom_store,
};

#endif


static atomic_t open_count;
static atomic_t open_flag;
static atomic_t reserve_open_flag;

static atomic_t m_flag;
static atomic_t a_flag;
static atomic_t t_flag;
static atomic_t mv_flag;

static int failure_count = 0;

static short akmd_delay = 0;

#ifdef CONFIG_ANDROID_POWER
static atomic_t suspend_flag = ATOMIC_INIT(0);
#endif

static struct akm8973_platform_data *pdata;

static int AKI2C_RxData(char *rxData, int length)
{
	uint8_t loop_i;
    
#if HUAWEI_HWID_L2(S7, S7201)
    int tmp;
#endif

	struct i2c_msg msgs[] = {
		{
		 .addr = this_client->addr,
		 .flags = 0,
		 .len = 1,
		 .buf = rxData,
		 },
		{
		 .addr = this_client->addr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxData,
		 },
	};

	for (loop_i = 0; loop_i < AKM8973_RETRY_COUNT; loop_i++) {
		if (i2c_transfer(this_client->adapter, msgs, 2) > 0) {
			break;
		}
		mdelay(10);
	}

	if (loop_i >= AKM8973_RETRY_COUNT) {
		printk(KERN_ERR "%s retry over %d\n", __func__, AKM8973_RETRY_COUNT);
		return -EIO;
	}
    
	return 0;
}

static int AKI2C_TxData(char *txData, int length)
{
	uint8_t loop_i;
	struct i2c_msg msg[] = {
		{
		 .addr = this_client->addr,
		 .flags = 0,
		 .len = length,
		 .buf = txData,
		 },
	};

	for (loop_i = 0; loop_i < AKM8973_RETRY_COUNT; loop_i++) {
		if (i2c_transfer(this_client->adapter, msg, 1) > 0) {
			break;
		}
		mdelay(10);
	}

	if (loop_i >= AKM8973_RETRY_COUNT) {
		printk(KERN_ERR "%s retry over %d\n", __func__, AKM8973_RETRY_COUNT);
		return -EIO;
	}
	return 0;
}

static void AKECS_Reset(void)
{
  gpio_set_value(COMPASS_RST, 1);
  mdelay(10);
	gpio_set_value(pdata->reset, 0);
	udelay(120);
	gpio_set_value(pdata->reset, 1);

}

static int AKECS_StartMeasure(void)
{
	char buffer[2];

	atomic_set(&data_ready, 0);

	/* Set measure mode */
	buffer[0] = AKECS_REG_MS1;
	buffer[1] = AKECS_MODE_MEASURE;

	/* Set data */
	return AKI2C_TxData(buffer, 2);
}

static int AKECS_PowerDown(void)
{
	char buffer[2];
	int ret;

	/* Set powerdown mode */
	buffer[0] = AKECS_REG_MS1;
	buffer[1] = AKECS_MODE_POWERDOWN;
	/* Set data */
	ret = AKI2C_TxData(buffer, 2);
	if (ret < 0)
		return ret;

	/* Dummy read for clearing INT pin */
	buffer[0] = AKECS_REG_TMPS;
	/* Read data */
	ret = AKI2C_RxData(buffer, 1);
	if (ret < 0)
		return ret;
	return ret;
}

static int AKECS_StartE2PRead(void)
{
	char buffer[2];

	/* Set measure mode */
	buffer[0] = AKECS_REG_MS1;
	buffer[1] = AKECS_MODE_E2P_READ;
	/* Set data */
	return AKI2C_TxData(buffer, 2);
}

static int AKECS_GetData(void)
{
	char buffer[RBUFF_SIZE + 1];
	int ret;

	memset(buffer, 0, RBUFF_SIZE + 1);
	buffer[0] = AKECS_REG_ST;
	ret = AKI2C_RxData(buffer, RBUFF_SIZE+1);
	if (ret < 0)
		return ret;

	mutex_lock(&sense_data_mutex);
	memcpy(sense_data, buffer, sizeof(buffer));
	atomic_set(&data_ready, 1);
	wake_up(&data_ready_wq);
	mutex_unlock(&sense_data_mutex);

	return 0;
}

static int AKECS_SetMode(char mode)
{
	int ret;

	switch (mode) {
	case AKECS_MODE_MEASURE:
		ret = AKECS_StartMeasure();
		break;
	case AKECS_MODE_E2P_READ:
		ret = AKECS_StartE2PRead();
		break;
	case AKECS_MODE_POWERDOWN:
		ret = AKECS_PowerDown();
		break;
	default:
		return -EINVAL;
	}

	/* wait at least 300us after changing mode */
	mdelay(1);
	return ret;
}

static int AKECS_TransRBuff(char *rbuf, int size)
{
	wait_event_interruptible_timeout(data_ready_wq,
					 atomic_read(&data_ready), 1000);
	if (!atomic_read(&data_ready)) {
		/* Ignore data errors if there are no open handles */
		if (atomic_read(&open_count) > 0) {
			printk(KERN_ERR
				"AKM8973 AKECS_TransRBUFF: Data not ready\n");
			failure_count++;
			if (failure_count >= MAX_FAILURE_COUNT) {
				printk(KERN_ERR
				       "AKM8973 AKECS_TransRBUFF: successive %d failure.\n",
				       failure_count);
				atomic_set(&open_flag, -1);
				wake_up(&open_wq);
				failure_count = 0;
			}
		}
		return -1;
	}

	mutex_lock(&sense_data_mutex);
	memcpy(&rbuf[1], &sense_data[1], size);
	atomic_set(&data_ready, 0);
	mutex_unlock(&sense_data_mutex);

	failure_count = 0;
	return 0;
}


static void AKECS_Report_Value(short *rbuf)
{
	struct akm8973_data *data = i2c_get_clientdata(this_client);
#if DEBUG
	printk(KERN_INFO"AKECS_Report_Value: yaw = %d, pitch = %d, roll = %d\n", rbuf[0],
	       rbuf[1], rbuf[2]);
	printk(KERN_INFO"                    tmp = %d, m_stat= %d, g_stat=%d\n", rbuf[3],
	       rbuf[4], rbuf[5]);
	printk(KERN_INFO"          G_Sensor:   x = %d LSB, y = %d LSB, z = %d LSB\n",
	       rbuf[6], rbuf[7], rbuf[8]);
#endif
	/* Report magnetic sensor information */
	if (atomic_read(&m_flag)) {
		input_report_abs(data->input_dev, ABS_RX, rbuf[0]);
		input_report_abs(data->input_dev, ABS_RY, rbuf[1]);
		input_report_abs(data->input_dev, ABS_RZ, rbuf[2]);
		input_report_abs(data->input_dev, ABS_RUDDER, rbuf[4]);
	}
 #if 0
	/* Report acceleration sensor information */
	if (atomic_read(&a_flag)) {
	  #if DEBUG
	  printk("AKECS_Report_Value: Acceleration x = %d, y = %d, z = %d\n",
		 rbuf[6], rbuf[7], rbuf[8]);
	  #endif
		input_report_abs(data->input_dev, ABS_X, rbuf[6]);
		input_report_abs(data->input_dev, ABS_Y, rbuf[7]);
		input_report_abs(data->input_dev, ABS_Z, rbuf[8]);
		input_report_abs(data->input_dev, ABS_WHEEL, rbuf[5]);
	}
#endif
	/* Report temperature information */
	if (atomic_read(&t_flag)) {
		input_report_abs(data->input_dev, ABS_THROTTLE, rbuf[3]);
	}

	if (atomic_read(&mv_flag)) {
		input_report_abs(data->input_dev, ABS_HAT0X, rbuf[9]);
		input_report_abs(data->input_dev, ABS_HAT0Y, rbuf[10]);
		input_report_abs(data->input_dev, ABS_BRAKE, rbuf[11]);
	}

	input_sync(data->input_dev);
}

static int AKECS_GetOpenStatus(void)
{
	wait_event_interruptible(open_wq, (atomic_read(&open_flag) != 0));
	return atomic_read(&open_flag);

}

static int AKECS_GetCloseStatus(void)
{
	wait_event_interruptible(open_wq, (atomic_read(&open_flag) <= 0));
	return atomic_read(&open_flag);



}

static void AKECS_CloseDone(void)
{
	atomic_set(&m_flag, 1);
	atomic_set(&a_flag, 0); 
	atomic_set(&t_flag, 1);
	atomic_set(&mv_flag, 1);
}

static int akm_aot_open(struct inode *inode, struct file *file)
{
	int ret = -1;
	if (atomic_cmpxchg(&open_count, 0, 1) == 0) {
		if (atomic_cmpxchg(&open_flag, 0, 1) == 0) {
			atomic_set(&reserve_open_flag, 1);
			//enable_irq(this_client->irq); 
			wake_up(&open_wq);
			ret = 0;
		}
	}
	return ret;
}

static int akm_aot_release(struct inode *inode, struct file *file)
{
	atomic_set(&reserve_open_flag, 0);
	atomic_set(&open_flag, 0);
	atomic_set(&open_count, 0);
	wake_up(&open_wq);
	//disable_irq(this_client->irq); 
	return 0;
}

static int
akm_aot_ioctl(struct inode *inode, struct file *file,
	      unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	short flag;

	switch (cmd) {
	case ECS_IOCTL_APP_SET_MFLAG:
	case ECS_IOCTL_APP_SET_AFLAG:
	case ECS_IOCTL_APP_SET_TFLAG:
	case ECS_IOCTL_APP_SET_MVFLAG:
		if (copy_from_user(&flag, argp, sizeof(flag)))
			return -EFAULT;
		if (flag < 0 || flag > 1)
			return -EINVAL;
		break;
	case ECS_IOCTL_APP_SET_DELAY:
		if (copy_from_user(&flag, argp, sizeof(flag)))
			return -EFAULT;
		break;
	default:
		break;
	}

	switch (cmd) {
	case ECS_IOCTL_APP_SET_MFLAG: 
		atomic_set(&m_flag, flag);
		break;
	case ECS_IOCTL_APP_GET_MFLAG:
		flag = atomic_read(&m_flag);
		break;
	case ECS_IOCTL_APP_SET_AFLAG:
		atomic_set(&a_flag, flag);
		break;
	case ECS_IOCTL_APP_GET_AFLAG:
		flag = atomic_read(&a_flag);
		break;
	case ECS_IOCTL_APP_SET_TFLAG:
		atomic_set(&t_flag, flag);
		break;
	case ECS_IOCTL_APP_GET_TFLAG:
		flag = atomic_read(&t_flag);
		break;
	case ECS_IOCTL_APP_SET_MVFLAG:
		atomic_set(&mv_flag, flag);
		break;
	case ECS_IOCTL_APP_GET_MVFLAG:
		flag = atomic_read(&mv_flag);
		break;
	case ECS_IOCTL_APP_SET_DELAY:
		akmd_delay = flag;
		break;
	case ECS_IOCTL_APP_GET_DELAY:
		flag = akmd_delay;
		break;
	default:
		return -ENOTTY;
	}

	switch (cmd) {
	case ECS_IOCTL_APP_GET_MFLAG:
	case ECS_IOCTL_APP_GET_AFLAG:
	case ECS_IOCTL_APP_GET_TFLAG:
	case ECS_IOCTL_APP_GET_MVFLAG:
	case ECS_IOCTL_APP_GET_DELAY:

		if (copy_to_user(argp, &flag, sizeof(flag)))
			return -EFAULT;
		break;
	default:
		break;
	}

	return 0;
}

static int akmd_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int akmd_release(struct inode *inode, struct file *file)
{
	AKECS_CloseDone();
	return 0;
}

static int
akmd_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	   unsigned long arg)
{

	void __user *argp = (void __user *)arg;

	char msg[RBUFF_SIZE + 1], rwbuf[5];
	int ret = -1, status;
	short mode, value[12], delay;
	char project_name[64];
	short layouts[4][3][3];
	int i, j, k;


	switch (cmd) {
	case ECS_IOCTL_READ:
	case ECS_IOCTL_WRITE:
		if (copy_from_user(&rwbuf, argp, sizeof(rwbuf)))
			return -EFAULT;
		break;
	case ECS_IOCTL_SET_MODE:
		if (copy_from_user(&mode, argp, sizeof(mode)))
			return -EFAULT;
		break;
	case ECS_IOCTL_SET_YPR:
		if (copy_from_user(&value, argp, sizeof(value)))
			return -EFAULT;
		break;
	default:
		break;
	}

	switch (cmd) {
	case ECS_IOCTL_RESET:
		AKECS_Reset();
		break;
	case ECS_IOCTL_READ:
		if (rwbuf[0] < 1)
			return -EINVAL;
		ret = AKI2C_RxData(&rwbuf[1], rwbuf[0]);
		if (ret < 0)
			return ret;
		break;
	case ECS_IOCTL_WRITE:
		if (rwbuf[0] < 2)
			return -EINVAL;
		ret = AKI2C_TxData(&rwbuf[1], rwbuf[0]);
		if (ret < 0)
			return ret;
		break;
	case ECS_IOCTL_SET_MODE:
		ret = AKECS_SetMode((char)mode);
		if (ret < 0)
			return ret;
		break;
	case ECS_IOCTL_GETDATA:
		ret = AKECS_TransRBuff(msg, RBUFF_SIZE);
		if (ret < 0)
			return ret;
		break;
	case ECS_IOCTL_SET_YPR:
		AKECS_Report_Value(value);
		break;
	case ECS_IOCTL_GET_OPEN_STATUS:
		status = AKECS_GetOpenStatus();
		break;
	case ECS_IOCTL_GET_CLOSE_STATUS:
		status = AKECS_GetCloseStatus();
		break;
	case ECS_IOCTL_GET_DELAY:
		delay = akmd_delay;
		break;
	case ECS_IOCTL_GET_PROJECT_NAME:
		strncpy(project_name, pdata->project_name, 64);
		break;
	case ECS_IOCTL_GET_MATRIX:
		for (i = 0; i < 4; i++)
			for (j = 0; j < 3; j++)
				for (k = 0; k < 3; k++) {
				layouts[i][j][k] = pdata->layouts[i][j][k];
				}
		break;
	default:
		return -ENOTTY;
	}

	switch (cmd) {
	case ECS_IOCTL_READ:
		if (copy_to_user(argp, &rwbuf, sizeof(rwbuf)))
			return -EFAULT;
		break;
	case ECS_IOCTL_GETDATA:
		if (copy_to_user(argp, &msg, sizeof(msg)))
			return -EFAULT;
		break;
	case ECS_IOCTL_GET_OPEN_STATUS:
		if (copy_to_user(argp, &status, sizeof(status)))
			return -EFAULT;
	case ECS_IOCTL_GET_CLOSE_STATUS:
		if (copy_to_user(argp, &status, sizeof(status)))
			return -EFAULT;
		break;
	case ECS_IOCTL_GET_DELAY:
		if (copy_to_user(argp, &delay, sizeof(delay)))
			return -EFAULT;
		break;
	case ECS_IOCTL_GET_PROJECT_NAME:
		if (copy_to_user(argp, project_name, sizeof(project_name)))
			return -EFAULT;
		break;
	case ECS_IOCTL_GET_MATRIX:
		if (copy_to_user(argp, layouts, sizeof(layouts)))
			return -EFAULT;
		break;
	default:
		break;
	}

	return 0;
}

static void akm_work_func(struct work_struct *work)
{
	if (AKECS_GetData() < 0)
		printk(KERN_ERR "AKM8973 akm_work_func: Get data failed\n");
}

static int akm8973_gpio_setup(struct i2c_client *client)
{
  int ret = 0;
  struct compass_platform_data  *pd;
  pd = client->dev.platform_data;  

  ret = gpio_request(pd->reset, "Compass RESET Enable");
  if (ret) {
    printk("***suchangyu**AKECS_Reset* :gpio_request failed.\n");
    return -1;
  }

  ret = gpio_tlmm_config(GPIO_CFG(pd->reset, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_10MA), GPIO_ENABLE);
  if(ret < 0) {
    printk("***suchangyu**AKECS_Reset* : gpio_tlmm_config failed.\n");
    gpio_free(pd->reset);
    //return -1;
  }
  else {
    // successful
    COMPASS_RST = pd->reset;
  }
  return ret;
}

static void akm8973_gpio_release(struct i2c_client *client)
{
  struct compass_platform_data  *pd;
  pd = client->dev.platform_data;
  gpio_free(pd->reset);
}

static irqreturn_t akm8973_interrupt(int irq, void *dev_id)
{
	struct akm8973_data *data = dev_id;
	//disable_irq_nosync(this_client->irq); 
	schedule_work(&data->work);
	return IRQ_HANDLED;
}

static struct file_operations akmd_fops = {
	.owner = THIS_MODULE,
	.open = akmd_open,
	.release = akmd_release,
	.ioctl = akmd_ioctl,
};

static struct file_operations akm_aot_fops = {
	.owner = THIS_MODULE,
	.open = akm_aot_open,
	.release = akm_aot_release,
	.ioctl = akm_aot_ioctl,
};


static struct miscdevice akm_aot_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "akm8973_aot",
	.fops = &akm_aot_fops,
};


static struct miscdevice akmd_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "akm8973_dev",
	.fops = &akmd_fops,
};

static unsigned int gAkm8973_state = 0;
/* following are the sysfs callback functions */
static ssize_t akm8973_attr_show(struct kobject *kobj, struct kobj_attribute *attr,
        char *buf)
{
    return sprintf(buf, "%d", gAkm8973_state);
}

/* Define the device attributes */
 static struct kobj_attribute akm8973_attribute =
         __ATTR(state, 0666, akm8973_attr_show, NULL);

 static struct attribute* akm8973_attributes[] =
 {
         &akm8973_attribute.attr,
         NULL
 };

 static struct attribute_group akm8973_defattr_group =
 {
         .attrs = akm8973_attributes,
 };
#define HARDWARE_VERSION_RPC_PROG	0x30000089
#define HARDWARE_VERSION_RPC_VERS	0x00010001
#define HARDWARE_VERSION_READ_ONEPROC  22
#define HARDWARE_VERSION_RPC_TIMEOUT    1000	/* 10 sec 10000*/// 
#define DONT_OPERATE_65023_LDO2_HD_VER_NUM   10
static bool g_akm8973_valid = false;
static int g_hardware_version_num = DONT_OPERATE_65023_LDO2_HD_VER_NUM;

static int get_hardware_version_info(void)
{
	struct msm_rpc_endpoint *hardware_version_ep;
	struct rpc_request_hdr request_hardware_ver;
	struct rpc_rep_hardware_ver{
		struct rpc_reply_hdr hdr;
		u32 hardware_num;
	} reply_hardware_ver;

	int rc = 0;

	hardware_version_ep =
	    msm_rpc_connect_compatible(HARDWARE_VERSION_RPC_PROG, HARDWARE_VERSION_RPC_VERS, 0);

	if (hardware_version_ep == NULL)
	{
		printk(KERN_ERR "%s: rpc connect failed .\n", __func__);
		return -ENODEV;
	}
	else if (IS_ERR(hardware_version_ep)) 
	{
		rc = PTR_ERR(hardware_version_ep);
		printk(KERN_ERR
		       "%s: rpc connect failed ."
		       " rc = %d\n ", __func__, rc);
		hardware_version_ep = NULL;
		return rc;
	}
	
	rc = msm_rpc_call_reply(hardware_version_ep,
				HARDWARE_VERSION_READ_ONEPROC,
				&request_hardware_ver, sizeof(request_hardware_ver),
				&reply_hardware_ver, sizeof(reply_hardware_ver),
				msecs_to_jiffies(HARDWARE_VERSION_RPC_TIMEOUT));
	if (rc < 0) {
		printk(KERN_ERR "%s(): msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, HARDWARE_VERSION_READ_ONEPROC, rc);
		return rc;
	} 

	g_hardware_version_num = be32_to_cpu(reply_hardware_ver.hardware_num);  
	printk(KERN_ERR "%s(): msm_rpc_call_reply succeed! proc=%d, hardware_num=0x%x, g_hardware_version_num = %d\n",
		       __func__, HARDWARE_VERSION_READ_ONEPROC,reply_hardware_ver.hardware_num, g_hardware_version_num);
	
	return rc;
}


bool get_akm8973_allow_operate_65023_ldo2_flag(void)
{
	bool allow_flag = false;
	if((g_akm8973_valid) && (g_hardware_version_num == DONT_OPERATE_65023_LDO2_HD_VER_NUM))
	{
		allow_flag = false;
	}
	else
		allow_flag = true;

	printk(KERN_ERR "%s(): hardware_num=%d, akm8973_valid = %d, not_allow_hd_ver = %d, allow_flag = %d\n",
		       __func__, g_hardware_version_num,g_akm8973_valid,DONT_OPERATE_65023_LDO2_HD_VER_NUM,allow_flag);
	return allow_flag;
}

int akm8973_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct akm8973_data *akm;
  struct kobject *kobj = NULL;
  char buffer[2];
	int err = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

  kobj = kobject_create_and_add("akm8973", NULL);
  if (kobj == NULL) {
    return -1;
  }
  if (sysfs_create_group(kobj, &akm8973_defattr_group)) {
    kobject_put(kobj);
    return -1;
  }
    
  /* request gpio. */
  err = akm8973_gpio_setup(client);
  if (err < 0)
    goto exit_check_functionality_failed;
	akm = kzalloc(sizeof(struct akm8973_data), GFP_KERNEL);
	if (!akm) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	//INIT_WORK(&akm->work, akm_work_func); // [Sean@2011-3-7]: change to polling mode
	i2c_set_clientdata(client, akm);

	pdata = client->dev.platform_data;
	if (pdata == NULL) {
		printk(KERN_ERR"AKM8973 akm8973_probe: platform data is NULL\n");
		goto exit_platform_data_null;
	}
	this_client = client;
  AKECS_Reset();
  buffer[0] = AKECS_REG_TMPS;
  /* Read data to check device existence. */
  err = AKI2C_RxData(buffer, 1);
  if (err < 0)
  {
    printk(KERN_ERR "akm8973_probe fail!\n");
    goto exit_platform_data_null;
  }

  gAkm8973_state = 1;
	akm->input_dev = sensor_dev;
	if ((akm->input_dev == NULL)||((akm->input_dev->id.vendor != GS_ADIX345)&&(akm->input_dev->id.vendor != GS_ST35DE))) {   //NOTICE(suchangyu): we only accept already known input device
	  err = -ENOMEM;
	  printk(KERN_ERR "akm8973_probe: Failed to allocate input device\n");
	  goto exit_input_dev_alloc_failed;
	}


	err = AKECS_PowerDown();
	if (err < 0) {
		printk(KERN_ERR"AKM8973 akm8973_probe: set power down mode error\n");
		goto exit_set_mode_failed;
	}

	/*
	// [Sean@2011-3-7]: change to polling mode
	err = request_irq(client->irq, akm8973_interrupt, IRQF_TRIGGER_HIGH,
			  "akm8973", akm);
	disable_irq(this_client->irq);

	if (err < 0) {
		printk(KERN_ERR"AKM8973 akm8973_probe: request irq failed\n");
		goto exit_irq_request_failed;
	}
	*/
	
	set_bit(EV_ABS, akm->input_dev->evbit);
	/* yaw */
	input_set_abs_params(akm->input_dev, ABS_RX, 0, 23040, 0 ,0); //0, 360, 0, 0); //[Sean@2011-03-07] why ?
	/* pitch */
	input_set_abs_params(akm->input_dev, ABS_RY, -11520, 11520, 0, 0);//-180, 180, 0, 0); //[Sean@2011-03-07] why ?
	/* roll */
	input_set_abs_params(akm->input_dev, ABS_RZ, -5760, 5760, 0, 0);//-90, 90, 0, 0); //[Sean@2011-03-07] why ?
	/* x-axis acceleration */
	input_set_abs_params(akm->input_dev, ABS_X, -5760, 5760, 0, 0);//-1872, 1872, 0, 0);
	/* y-axis acceleration */
	input_set_abs_params(akm->input_dev, ABS_Y, -5760, 5760, 0, 0);//-1872, 1872, 0, 0);
	/* z-axis acceleration */
	input_set_abs_params(akm->input_dev, ABS_Z, -5760, 5760, 0, 0);//-1872, 1872, 0, 0);
	/* temparature */
	input_set_abs_params(akm->input_dev, ABS_THROTTLE, -30, 85, 0, 0);
	/* status of magnetic sensor */
	input_set_abs_params(akm->input_dev, ABS_RUDDER, 0, 3, 0, 0);//-32768, 3, 0, 0);
	/* status of acceleration sensor */
	input_set_abs_params(akm->input_dev, ABS_WHEEL, 0, 3, 0, 0);//-32768, 3, 0, 0);
	/* step count */
	//input_set_abs_params(akm->input_dev, ABS_GAS, 0, 65535, 0, 0);   //[Sean@2011-03-07] why ?
	/* x-axis of raw magnetic vector */
	input_set_abs_params(akm->input_dev, ABS_HAT0X, -2048, 2032, 0, 0);
	/* y-axis of raw magnetic vector */
	input_set_abs_params(akm->input_dev, ABS_HAT0Y, -2048, 2032, 0, 0);
	/* z-axis of raw magnetic vector */
	input_set_abs_params(akm->input_dev, ABS_BRAKE, -2048, 2032, 0, 0);

    // delete register compass input dev, and let err=0
/*    
      akm->input_dev->name = "compass";
      
      err = input_register_device(akm->input_dev);
*/
	err = 0;
	if (err) {
		printk(KERN_ERR
		       "AKM8973 akm8973_probe: Unable to register input device: %s\n",
		       akm->input_dev->name);
		goto exit_input_register_device_failed;
	}

	err = misc_register(&akmd_device);
	if (err) {
		printk(KERN_ERR "AKM8973 akm8973_probe: akmd_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	err = misc_register(&akm_aot_device);
	if (err) {
		printk(KERN_ERR
		       "AKM8973 akm8973_probe: akm_aot_device register failed\n");
		goto exit_misc_device_register_failed;
	}

	mutex_init(&sense_data_mutex);

	init_waitqueue_head(&data_ready_wq);
	init_waitqueue_head(&open_wq);

	/* As default, report all information */
	atomic_set(&m_flag, 1);
	atomic_set(&a_flag, 1);
	atomic_set(&t_flag, 1);
	atomic_set(&mv_flag, 1);
#ifdef CONFIG_HAS_EARLYSUSPEND
	akm->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	akm->early_suspend.suspend = akm8973_early_suspend;
	akm->early_suspend.resume = akm8973_early_resume;
	register_early_suspend(&akm->early_suspend);
#endif

#ifdef CONFIG_UPDATE_COMPASS_FIRMWARE
	akm8973_set_e2prom_file();
#endif
	/* debug message. suchangyu. 20100521. begin. */
#if DEBUG
        printk("***suchangyu*** : akm8973 probe success\n");
#endif
	/* debug message. suchangyu. 20100521. end. */
	AKECS_Reset();

	err = get_hardware_version_info();
	if(err)
		printk(KERN_ERR "%s : get hardware version err = %d\n",__func__,err);
	g_akm8973_valid = true;
	return 0;

exit_misc_device_register_failed:
exit_input_register_device_failed:
	input_free_device(akm->input_dev);
exit_input_dev_alloc_failed:
	free_irq(client->irq, akm);
exit_irq_request_failed:
exit_set_mode_failed:
exit_platform_data_null:
	kfree(akm);
exit_alloc_data_failed:
    akm8973_gpio_release(client);
exit_check_functionality_failed:
	return err;
}

static int akm8973_detect(struct i2c_client *client, int kind,
			  struct i2c_board_info *info)
{
#if DEBUG
	printk(KERN_ERR "%s\n", __FUNCTION__);
#endif
	strlcpy(info->type, "akm8973", I2C_NAME_SIZE);
	return 0;
}

static int akm8973_remove(struct i2c_client *client)
{
	struct akm8973_data *akm = i2c_get_clientdata(client);
	akm8973_gpio_release(client);
	free_irq(client->irq, akm);
	input_unregister_device(akm->input_dev);
	kfree(akm);
	return 0;
}
static int akm8973_suspend(struct i2c_client *client, pm_message_t mesg)
{ 

	int ret;
#if DEBUG
	printk(KERN_ERR "AK8973 compass driver: akm8973_suspend\n");
#endif
	//disable_irq(client->irq); // [Sean@2011-3-7]: change to polling mode
	ret = AKECS_SetMode(AKECS_MODE_POWERDOWN);
	if (ret < 0)
			printk(KERN_ERR "akm8973_suspend power off failed\n");

	atomic_set(&reserve_open_flag, atomic_read(&open_flag));
	atomic_set(&open_flag, 0);
	wake_up(&open_wq);
	
	return 0;
}

static int akm8973_resume(struct i2c_client *client)
{

	int ret;
#if DEBUG
	printk(KERN_ERR "AK8973 compass driver: akm8973_resume\n");
#endif
	ret = AKECS_SetMode(AKECS_MODE_MEASURE);
	if (ret < 0)
			printk(KERN_ERR "akm8973_resume power measure failed\n");
	//enable_irq(client->irq); // [Sean@2011-3-7]: Change to polling mode
	atomic_set(&open_flag, atomic_read(&reserve_open_flag));
	wake_up(&open_wq);

	return 0;
}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void akm8973_early_suspend(struct early_suspend *h)
{
      struct akm8973_data *akm ;
	akm = container_of(h, struct akm8973_data, early_suspend);
	
	akm8973_suspend(this_client, PMSG_SUSPEND);
}

static void akm8973_early_resume(struct early_suspend *h)
{	
	struct akm8973_data *akm ;
	akm = container_of(h, struct akm8973_data, early_suspend);
	
	akm8973_resume(this_client);
}
#endif
 
#ifdef CONFIG_UPDATE_COMPASS_FIRMWARE 

static int AKECS_StartE2PWrite(void)
{
	char buffer[2];
       buffer[0] = AKECS_REG_MS1;
	buffer[1] = 0xAA;    /*write e2prom */
	return AKI2C_TxData(buffer, 2);
}

static  int AKECS_WriteE2PReg( void )
{
	int ret;
	char tx_buffer[3];
	
	tx_buffer[0] = AKECS_REG_ETS;
	tx_buffer[1] =0x26;   /*akm factory support value*/
	ret= AKI2C_TxData(tx_buffer, 2);
	if(ret < 0) {
		printk(KERN_ERR "AKECS_WriteE2PReg: transfer error\n");
		return -EIO;
	}
	msleep(10);
	tx_buffer[0] = AKECS_REG_EVIR;
	tx_buffer[1] =0x77;   /*akm factory support value*/
	ret= AKI2C_TxData(tx_buffer, 2);
	if(ret < 0) {
		printk(KERN_ERR "AKECS_WriteE2PReg: transfer error\n");
		return -EIO;
	}
	msleep(10);
	tx_buffer[0] = AKECS_REG_EIHE;
	tx_buffer[1] =0x66;   /*akm factory support value*/
	ret= AKI2C_TxData(tx_buffer, 2);
	if(ret < 0) {
		printk(KERN_ERR "AKECS_WriteE2PReg: transfer error\n");
		return -EIO;
	}
	msleep(10);
	tx_buffer[0] = AKECS_REG_EHXGA;
	tx_buffer[1] =0x06;   /*akm factory support value*/
	ret= AKI2C_TxData(tx_buffer, 2);
	if(ret < 0) {
		printk(KERN_ERR "AKECS_WriteE2PReg: transfer error\n");
		return -EIO;
	}
	msleep(10);
	tx_buffer[0] = AKECS_REG_EHYGA;
	tx_buffer[1] =0x06;   /*akm factory support value*/
	ret= AKI2C_TxData(tx_buffer, 2);
	if(ret < 0) {
		printk(KERN_ERR "AKECS_WriteE2PReg: transfer error\n");
		return -EIO;
	}
	msleep(10);
	tx_buffer[0] = AKECS_REG_EHZGA;
	tx_buffer[1] =0x07;   /*akm factory support value*/
	ret= AKI2C_TxData(tx_buffer, 2);
	if(ret < 0) {
		printk(KERN_ERR "AKECS_WriteE2PReg: transfer error\n");
		return -EIO;
	}
	msleep(10);
	return ret;
}

static  int AKECS_ReadE2PReg( void )
{
	int ret;
	char rx_buffer[6];
	rx_buffer[0] = AKECS_REG_ETS;
	ret= AKI2C_RxData(rx_buffer, 5);
	if(ret < 0) {
		printk(KERN_ERR "AKECS_ReadE2PReg: transfer error\n");
		return -EIO;
	}
	rx_buffer[0] = AKECS_REG_EHXGA;
	ret= AKI2C_RxData(rx_buffer, 4);
	if(ret < 0) {
		printk(KERN_ERR "AKECS_ReadE2PReg: transfer error\n");
		return -EIO;
	}
			
	if(( rx_buffer[0] ==0x06)&&( rx_buffer[1] ==0x06)&& rx_buffer[2] ==0x07)
		return 0;
	else 
		return -EIO;
	
}

static int akm8973_set_e2prom_file(void)
{
	int ret;
	struct kobject *kobject_akm;
	kobject_akm = kobject_create_and_add("compass", NULL);
	if (!kobject_akm) {
		printk(KERN_ERR "create kobjetct error!\n");
		return -EIO;
	}
	ret = sysfs_create_file(kobject_akm, &set_e2prom_attribute.attr);
	if (ret) {
		kobject_put(kobject_akm);
		printk(KERN_ERR "create file error\n");
		return -EIO;
	}
	return 0;	
}

static ssize_t set_e2prom_show(struct kobject *kobj, struct kobj_attribute *attr,char *buf)
{
	AKECS_StartE2PRead();
	msleep(1);  /* start-upwait at least 300us */
	AKECS_ETST=i2c_smbus_read_byte_data(this_client,AKECS_REG_ETST); 
	return sprintf(buf, "%x\n", AKECS_ETST);
}

static ssize_t  set_e2prom_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	int ret,i=10;
	if(buf[0] != '1')
	{   
		return -1;
	}
	AKECS_StartE2PRead();
	msleep(1);  /* start-upwait at least 300us */
	AKECS_ETST=i2c_smbus_read_byte_data(this_client,AKECS_REG_ETST); 
	if((AKECS_ETST != AKECS_FACTORY_ETST)&&(AKECS_ETST != AKECS_RECOMPOSE_ETST)){
		while(i--){
			ret = AKECS_StartE2PWrite();
			if(ret<0)
				return ret;
			msleep(1);
			ret = AKECS_WriteE2PReg();
			if(ret<0)
				return ret;
			ret = AKECS_StartE2PRead();
			if(ret<0)
				return ret;
			msleep(1);
			if(!AKECS_ReadE2PReg())
				break;
		}
	}
	ret = AKECS_StartE2PWrite();
	if(ret<0)
		return ret;
	msleep(1);
	ret = i2c_smbus_write_byte_data(this_client,AKECS_REG_ETST,AKECS_RECOMPOSE_ETST);
	if(ret<0)
		return ret;
	msleep(10);
	ret = AKECS_SetMode(AKECS_MODE_POWERDOWN);
	if(ret<0)
		return ret;
	AKECS_ETST = AKECS_RECOMPOSE_ETST;
	return count;
}
#endif


static const struct i2c_device_id akm8973_id[] = {
	{ "akm8973", 0 },
	{ }
};

static struct i2c_driver akm8973_driver = {
	.class = I2C_CLASS_HWMON,
	.probe 	= akm8973_probe,
	.remove 	= akm8973_remove,

#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = akm8973_suspend,
	.resume = akm8973_resume,
#endif
	.id_table	= akm8973_id,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "akm8973",
		   },
	.detect = akm8973_detect,
	
};

static int __init akm8973_init(void)
{
	printk(KERN_INFO "AKM8973 compass driver: init\n");
	return i2c_add_driver(&akm8973_driver);
}

static void __exit akm8973_exit(void)
{
	i2c_del_driver(&akm8973_driver);
}

__define_initcall("7s",akm8973_init,7s);
// module_init(akm8973_init);
module_exit(akm8973_exit);

MODULE_AUTHOR("viral wang <viral_wang@htc.com>");
MODULE_DESCRIPTION("AKM8973 compass driver");
MODULE_LICENSE("GPL");
