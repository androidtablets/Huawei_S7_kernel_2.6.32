/*
 * BQ275x0 battery driver
 *
 * Copyright (C) 2008 Rodolfo Giometti <giometti@linux.it>
 * Copyright (C) 2008 Eurotech S.p.A. <info@eurotech.it>
 *
 * Based on a previous work by Copyright (C) 2008 Texas Instruments, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/idr.h>
#include <linux/i2c.h>
#include <asm/unaligned.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <linux/module.h> 
#include <linux/i2c/bq275x0_battery.h>
#include <linux/io.h>
#include <linux/uaccess.h> 
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/ctype.h>

#define DRIVER_VERSION			"1.0.0"

#define BQ275x0_REG_TEMP	0x06
#define BQ275x0_REG_VOLT	0x08
#define BQ275x0_REG_TTE		0x16		/*Time to Empty*/
#define BQ275x0_REG_TTF		0x18		/*Time to Full*/
#define BQ275x0_REG_SOC		0x2C		/* State-of-Charge */
#define BQ275x0_REG_AI		0x14		/*Average Current*/

#define CONST_NUM_10		10
#define CONST_NUM_2730		2730

#define BSP_UPGRADE_FIRMWARE_BQFS_CMD       "upgradebqfs"
#define BSP_UPGRADE_FIRMWARE_DFFS_CMD       "upgradedffs"
#define BSP_UPGRADE_FIRMWARE_BQFS_NAME      "/system/etc/coulometer/bq27510.bqfs"
#define BSP_UPGRADE_FIRMWARE_DFFS_NAME      "/system/etc/coulometer/bq27510.dffs"
#define BSP_ROM_MODE_I2C_ADDR               0x0B
#define BSP_NORMAL_MODE_I2C_ADDR            0x55
#define BSP_FIRMWARE_FILE_SIZE              (400*1024)
#define BSP_I2C_MAX_TRANSFER_LEN            128
#define BSP_MAX_ASC_PER_LINE                400
#define BSP_ENTER_ROM_MODE_CMD              0x00
#define BSP_ENTER_ROM_MODE_DATA             0x0F00
#define BSP_FIRMWARE_DOWNLOAD_MODE          0xDDDDDDDD
#define BSP_NORMAL_MODE                     0x00

//#define BQ275x0_DEBUG_FLAG
#if defined(BQ275x0_DEBUG_FLAG)
#define BQ275x0_DBG(format,arg...)     do { printk(KERN_ALERT format, ## arg);  } while (0)
#define BQ275x0_ERR(format,arg...)	  do { printk(KERN_ERR format, ## arg);  } while (0)
#define BQ275x0_FAT(format,arg...)     do { printk(KERN_CRIT format, ## arg); } while (0)
#else
#define BQ275x0_DBG(format,arg...)     do { (void)(format); } while (0)
#define BQ275x0_ERR(format,arg...)	  do { (void)(format); } while (0)
#define BQ275x0_FAT(format,arg...)	  do { (void)(format); } while (0)
#endif

static DEFINE_IDR(bq275x0_battery_id);
static DEFINE_MUTEX(bq275x0_battery_mutex);

static enum power_supply_property bq275x0_battery_props[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW,
	POWER_SUPPLY_PROP_TIME_TO_FULL_NOW,
};
struct i2c_client* g_battery_measure_by_bq275x0_i2c_client = NULL;

static struct i2c_driver bq275x0_battery_driver;
/*在固件下载器件不允许Bq275x0其他类型的I2C操作*/
static unsigned int gBq275x0DownloadFirmwareFlag = BSP_NORMAL_MODE;

/*
 * Common code for bq275x0 devices
 */
static int bq275x0_i2c_read_word(struct bq275x0_device_info *di,u8 reg)
{
	int err;

    if(BSP_FIRMWARE_DOWNLOAD_MODE == gBq275x0DownloadFirmwareFlag)
    {
        return -1;
    }

	mutex_lock(&bq275x0_battery_mutex);
	err = i2c_smbus_read_word_data(di->client,reg);
	if (err < 0) 
    {
		BQ275x0_ERR("[%s,%d] i2c_smbus_read_byte_data failed\n",__FUNCTION__,__LINE__);
       }
	mutex_unlock(&bq275x0_battery_mutex);

	return err;
}

static int bq275x0_i2c_word_write(struct i2c_client *client, u8 reg, u16 value)
{
	int err;

	mutex_lock(&bq275x0_battery_mutex);
    err = i2c_smbus_write_word_data(client, reg, value);
	if (err < 0) 
    {
		BQ275x0_ERR("[%s,%d] i2c_smbus_write_word_data failed\n",__FUNCTION__,__LINE__);
    }
	mutex_unlock(&bq275x0_battery_mutex);

	return err;
}

static int bq275x0_i2c_bytes_write(struct i2c_client *client, u8 reg, u8 *pBuf, u16 len)
{
	int i2c_ret, i,j;
    u8 *p;

    p = pBuf;

    mutex_lock(&bq275x0_battery_mutex);
    for(i=0; i<len; i+=I2C_SMBUS_BLOCK_MAX)
    {
        j = ((len - i) > I2C_SMBUS_BLOCK_MAX) ? I2C_SMBUS_BLOCK_MAX : (len - i);
        i2c_ret = i2c_smbus_write_i2c_block_data(client, reg+i, j, p+i);
        if (i2c_ret < 0) 
        {
    		BQ275x0_ERR("[%s,%d] i2c_transfer failed\n",__FUNCTION__,__LINE__);
    	}
    }
    mutex_unlock(&bq275x0_battery_mutex);

	return i2c_ret;
}


static int bq275x0_i2c_bytes_read(struct i2c_client *client, u8 reg, u8 *pBuf, u16 len)
{
	int i2c_ret = 0, i = 0, j = 0;
    u8 *p;

    p = pBuf;

    mutex_lock(&bq275x0_battery_mutex);
    for(i=0; i<len; i+=I2C_SMBUS_BLOCK_MAX)
    {
        j = ((len - i) > I2C_SMBUS_BLOCK_MAX) ? I2C_SMBUS_BLOCK_MAX : (len - i);
        i2c_ret = i2c_smbus_read_i2c_block_data(client, reg+i, j, p+i);
        if (i2c_ret < 0) 
        {
    		BQ275x0_ERR("[%s,%d] i2c_transfer failed\n",__FUNCTION__,__LINE__);
    	}
    }
    mutex_unlock(&bq275x0_battery_mutex);

	return i2c_ret;
    
}


static int bq275x0_i2c_bytes_read_and_compare(struct i2c_client *client, u8 reg, u8 *pSrcBuf, u8 *pDstBuf, u16 len)
{
    int i2c_ret;

    i2c_ret = bq275x0_i2c_bytes_read(client, reg, pSrcBuf, len);
    if(i2c_ret < 0)
    {
        BQ275x0_ERR("[%s,%d] bq275x0_i2c_bytes_read failed\n",__FUNCTION__,__LINE__);
        return i2c_ret;
    }

    i2c_ret = strncmp(pDstBuf, pSrcBuf, len);

    return i2c_ret;
}

/*
 * Return the battery temperature in Celcius degrees
 * Or < 0 if something fails.
 */
int bq275x0_battery_temperature(struct bq275x0_device_info *di)
{
	int data=0;

	data = bq275x0_i2c_read_word(di,BQ275x0_REG_TEMP);
	data = (data-CONST_NUM_2730)/CONST_NUM_10;
	BQ275x0_DBG("read temperature result = %d Celsius\n",data);
	return data;
}

/*
 * Return the battery Voltage in milivolts
 * Or < 0 if something fails.
 */
int bq275x0_battery_voltage(struct bq275x0_device_info *di)
{
	int data=0;

	data = bq275x0_i2c_read_word(di,BQ275x0_REG_VOLT);
	BQ275x0_DBG("read voltage result = %d volts\n",data);
	return data;
}

/*
 * Return the battery average current
 * Note that current can be negative signed as well
 * Or 0 if something fails.
 */
 short bq275x0_battery_current(struct bq275x0_device_info *di)
{
	int data=0;
	short nCurr=0;

	data = bq275x0_i2c_read_word(di,BQ275x0_REG_AI);
	
	nCurr = (signed short )data;
	BQ275x0_DBG("read current result = %d mA\n",nCurr);
	return nCurr;
}

/*
 * Return the battery Relative State-of-Charge
 * Or < 0 if something fails.
 */
int bq275x0_battery_capacity(struct bq275x0_device_info *di)
{
	int data=0;

	data = bq275x0_i2c_read_word(di,BQ275x0_REG_SOC);

	BQ275x0_DBG("read soc result = %d Hundred Percents\n",data);
	return data;
}

/*
 * Return the battery Time to Empty
 * Or < 0 if something fails
 */
int bq275x0_battery_tte(struct bq275x0_device_info *di)
{
	int data=0;

	data = bq275x0_i2c_read_word(di,BQ275x0_REG_TTE);
	BQ275x0_DBG("read tte result = %d minutes\n",data);
	return data;
}

/*
 * Return the battery Time to Full
 * Or < 0 if something fails
 */
int bq275x0_battery_ttf(struct bq275x0_device_info *di)
{
	int data=0;

	data = bq275x0_i2c_read_word(di,BQ275x0_REG_TTF);
	BQ275x0_DBG("read ttf result = %d minutes\n",data);
	return data;
}

/*
 * Return the battery Control
 * Or < 0 if something fails
 */
#define to_bq275x0_device_info(x) container_of((x), \
				struct bq275x0_device_info, bat);

static int bq275x0_battery_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	struct bq275x0_device_info *di = to_bq275x0_device_info(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = bq275x0_battery_voltage(di);
		if (psp == POWER_SUPPLY_PROP_PRESENT)
			val->intval = val->intval <= 0 ? 0 : 1;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = bq275x0_battery_current(di);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = bq275x0_battery_capacity(di);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = bq275x0_battery_temperature(di);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_EMPTY_NOW:
		val->intval = bq275x0_battery_tte(di);
		break;
	case POWER_SUPPLY_PROP_TIME_TO_FULL_NOW:
		val->intval = bq275x0_battery_ttf(di);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static void bq275x0_powersupply_init(struct bq275x0_device_info *di)
{
	di->bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bat.properties = bq275x0_battery_props;
	di->bat.num_properties = ARRAY_SIZE(bq275x0_battery_props);
	di->bat.get_property = bq275x0_battery_get_property;
	di->bat.external_power_changed = NULL;
}


int bq275x0_register_power_supply(struct power_supply ** ppBq275x0_power_supply)
{
	int rc = 0;
	struct bq275x0_device_info *di = NULL;
	
	if(g_battery_measure_by_bq275x0_i2c_client) {
		di = i2c_get_clientdata(g_battery_measure_by_bq275x0_i2c_client);
		rc = power_supply_register(&g_battery_measure_by_bq275x0_i2c_client->dev,&di->bat);
		if (rc) {
			BQ275x0_ERR("failed to register bq275x0 battery\n");
			return rc;
		}
		*ppBq275x0_power_supply = &di->bat;
	}
	else
		rc = -ENODEV;

	return rc;
}


static int bq275x0_atoi(const char *s)
{
	int k = 0;

	k = 0;
	while (*s != '\0' && *s >= '0' && *s <= '9') {
		k = 10 * k + (*s - '0');
		s++;
	}
	return k;
}

static unsigned long bq275x0_strtoul(const char *cp, unsigned int base)
{
	unsigned long result = 0,value;

	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) 
    {
		result = result*base + value;
		cp++;
	}

	return result;
}


static int bq275x0_firmware_program(struct i2c_client *client, const unsigned char *pgm_data, unsigned int filelen)
{
    unsigned int i = 0, j = 0, ulDelay = 0, ulReadNum = 0;
    unsigned int ulCounter = 0, ulLineLen = 0;
    unsigned char temp = 0; 
    unsigned char *p_cur;
    unsigned char pBuf[BSP_MAX_ASC_PER_LINE] = { 0 };
    unsigned char p_src[BSP_I2C_MAX_TRANSFER_LEN] = { 0 };
    unsigned char p_dst[BSP_I2C_MAX_TRANSFER_LEN] = { 0 };
    unsigned char ucTmpBuf[16] = { 0 };

bq275x0_firmware_program_begin:
    if(ulCounter > 10)
    {
        return -1;
    }
    
    p_cur = pgm_data;       

    while(1)
    {
        if((p_cur - pgm_data) >= filelen)
        {
            printk("Download success\n");
            break;
        }
            
        while (*p_cur == '\r' || *p_cur == '\n')
        {
            p_cur++;
        }
        
        i = 0;
        ulLineLen = 0;

        memset(p_src, 0x00, sizeof(p_src));
        memset(p_dst, 0x00, sizeof(p_dst));
        memset(pBuf, 0x00, sizeof(pBuf));

        /*获取一行数据，去除空格*/
        while(i < BSP_MAX_ASC_PER_LINE)
        {
            temp = *p_cur++;      
            i++;
            if(('\r' == temp) || ('\n' == temp))
            {
                break;  
            }
            if(' ' != temp)
            {
                pBuf[ulLineLen++] = temp;
            }
        }

        
        p_src[0] = pBuf[0];
        p_src[1] = pBuf[1];

        if(('W' == p_src[0]) || ('C' == p_src[0]))
        {
            for(i=2,j=0; i<ulLineLen; i+=2,j++)
            {
                memset(ucTmpBuf, 0x00, sizeof(ucTmpBuf));
                memcpy(ucTmpBuf, pBuf+i, 2);
                p_src[2+j] = bq275x0_strtoul(ucTmpBuf, 16);
            }

            temp = (ulLineLen -2)/2;
            ulLineLen = temp + 2;
        }
        else if('X' == p_src[0])
        {
            memset(ucTmpBuf, 0x00, sizeof(ucTmpBuf));
            memcpy(ucTmpBuf, pBuf+2, ulLineLen-2);
            ulDelay = bq275x0_atoi(ucTmpBuf);
        }
        else if('R' == p_src[0])
        {
            memset(ucTmpBuf, 0x00, sizeof(ucTmpBuf));
            memcpy(ucTmpBuf, pBuf+2, 2);
            p_src[2] = bq275x0_strtoul(ucTmpBuf, 16);
            memset(ucTmpBuf, 0x00, sizeof(ucTmpBuf));
            memcpy(ucTmpBuf, pBuf+4, 2);
            p_src[3] = bq275x0_strtoul(ucTmpBuf, 16);
            memset(ucTmpBuf, 0x00, sizeof(ucTmpBuf));
            memcpy(ucTmpBuf, pBuf+6, ulLineLen-6);
            ulReadNum = bq275x0_atoi(ucTmpBuf);
        }

        if(':' == p_src[1])
        {
            switch(p_src[0])
            {
                case 'W' :

                    #if 0
                    printk("W: ");
                    for(i=0; i<ulLineLen-4; i++)
                    {
                        printk("%x ", p_src[4+i]);
                    }
                    printk(KERN_ERR "\n");
                    #endif                    

                    if(bq275x0_i2c_bytes_write(client, p_src[3], &p_src[4], ulLineLen-4) < 0)
                    {
                         printk(KERN_ERR "[%s,%d] bq275x0_i2c_bytes_write failed len=%d\n",__FUNCTION__,__LINE__,ulLineLen-4);                        
                    }

                    break;
                
                case 'R' :
                    if(bq275x0_i2c_bytes_read(client, p_src[3], p_dst, ulReadNum) < 0)
                    {
                        printk(KERN_ERR "[%s,%d] bq275x0_i2c_bytes_read failed\n",__FUNCTION__,__LINE__);
                    }
                    break;
                    
                case 'C' :
                    if(bq275x0_i2c_bytes_read_and_compare(client, p_src[3], p_dst, &p_src[4], ulLineLen-4))
                    {
                        ulCounter++;
                        printk(KERN_ERR "[%s,%d] bq275x0_i2c_bytes_read_and_compare failed\n",__FUNCTION__,__LINE__);
                        goto bq275x0_firmware_program_begin;
                    }
                    break;
                    
                case 'X' :                    
                    mdelay(ulDelay);
                    break;
                  
                default:
                    return 0;
            }
        }
      
    }

    return 0;
    
}

static int bq275x0_firmware_download(struct i2c_client *client, const unsigned char *pgm_data, unsigned int len)
{
    int iRet;
  
    /*Enter Rom Mode */
    iRet = bq275x0_i2c_word_write(client, BSP_ENTER_ROM_MODE_CMD, BSP_ENTER_ROM_MODE_DATA);
    if(0 != iRet)
    {
        printk(KERN_ERR "[%s,%d] bq275x0_i2c_word_write failed\n",__FUNCTION__,__LINE__);
    }
    mdelay(10);

    /*change i2c addr*/
    g_battery_measure_by_bq275x0_i2c_client->addr = BSP_ROM_MODE_I2C_ADDR;

    /*program bqfs*/
    iRet = bq275x0_firmware_program(client, pgm_data, len);
    if(0 != iRet)
    {
        printk(KERN_ERR "[%s,%d] bq275x0_firmware_program failed\n",__FUNCTION__,__LINE__);
    }

    /*change i2c addr*/
    g_battery_measure_by_bq275x0_i2c_client->addr = BSP_NORMAL_MODE_I2C_ADDR;

    return iRet;
    
}


static int bq275x0_update_firmware(struct i2c_client *client, const char *pFilePath) 
{
    char *buf;
    struct file *filp;
    struct inode *inode = NULL;
    mm_segment_t oldfs;
    int length;
    int ret = 0;

    /* open file */
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    filp = filp_open(pFilePath, O_RDONLY, S_IRUSR);
    if (IS_ERR(filp)) 
    {
        printk(KERN_ERR "[%s,%d] filp_open failed\n",__FUNCTION__,__LINE__);
        set_fs(oldfs);
        return -1;
    }

    if (!filp->f_op) 
    {
        printk(KERN_ERR "[%s,%d] File Operation Method Error\n",__FUNCTION__,__LINE__);        
        filp_close(filp, NULL);
        set_fs(oldfs);
        return -1;
    }

    inode = filp->f_path.dentry->d_inode;
    if (!inode) 
    {
        printk(KERN_ERR "[%s,%d] Get inode from filp failed\n",__FUNCTION__,__LINE__);          
        filp_close(filp, NULL);
        set_fs(oldfs);
        return -1;
    }

    /* file's size */
    length = i_size_read(inode->i_mapping->host);
    printk("bq275x0 firmware image size is %d \n",length);
    if (!( length > 0 && length < BSP_FIRMWARE_FILE_SIZE))
    {
        printk(KERN_ERR "[%s,%d] Get file size error\n",__FUNCTION__,__LINE__);
        filp_close(filp, NULL);
        set_fs(oldfs);
        return -1;
    }

    /* allocation buff size */
    buf = vmalloc(length+(length%2));       /* buf size if even */
    if (!buf) 
    {
        printk(KERN_ERR "[%s,%d] Alloctation memory failed\n",__FUNCTION__,__LINE__);
        filp_close(filp, NULL);
        set_fs(oldfs);
        return -1;
    }

    /* read data */
    if (filp->f_op->read(filp, buf, length, &filp->f_pos) != length) 
    {
        printk(KERN_ERR "[%s,%d] File read error\n",__FUNCTION__,__LINE__);
        filp_close(filp, NULL);
        filp_close(filp, NULL);
        set_fs(oldfs);
        vfree(buf);
        return -1;
    }

    ret = bq275x0_firmware_download(client, buf, length);

    filp_close(filp, NULL);
    set_fs(oldfs);
    vfree(buf);
    
    return ret;
    
}


static ssize_t bq275x0_attr_store(struct device_driver *driver, char *buf)
{
    int iRet = 0;
   
    if(NULL == buf)
    {
        return -1;
    }
    
    if(0 == strncmp(buf, BSP_UPGRADE_FIRMWARE_BQFS_CMD, strlen(BSP_UPGRADE_FIRMWARE_BQFS_CMD)))
    {
        /*enter firmware bqfs download*/
        gBq275x0DownloadFirmwareFlag = BSP_FIRMWARE_DOWNLOAD_MODE;
        iRet = bq275x0_update_firmware(g_battery_measure_by_bq275x0_i2c_client, BSP_UPGRADE_FIRMWARE_BQFS_NAME);        
        gBq275x0DownloadFirmwareFlag = BSP_NORMAL_MODE;
    }    
    else if(0 == strncmp(buf, BSP_UPGRADE_FIRMWARE_DFFS_CMD, strlen(BSP_UPGRADE_FIRMWARE_DFFS_CMD)))
    {
        /*enter firmware dffs download*/
        gBq275x0DownloadFirmwareFlag = BSP_FIRMWARE_DOWNLOAD_MODE;
        iRet = bq275x0_update_firmware(g_battery_measure_by_bq275x0_i2c_client, BSP_UPGRADE_FIRMWARE_DFFS_NAME);
        gBq275x0DownloadFirmwareFlag = BSP_NORMAL_MODE;
    }
    i2c_smbus_write_word_data(g_battery_measure_by_bq275x0_i2c_client,0x00,0x0021);
    return iRet;
}


static ssize_t bq275x0_attr_show(struct device_driver *driver, char *buf)
{
    int iRet = 0;
   
    if(NULL == buf)
    {
        return -1;
    }
    
    mutex_lock(&bq275x0_battery_mutex);
	i2c_smbus_write_word_data(g_battery_measure_by_bq275x0_i2c_client,0x00,0x0008);
    mdelay(2);
	iRet = i2c_smbus_read_word_data(g_battery_measure_by_bq275x0_i2c_client,0x00);
    mutex_unlock(&bq275x0_battery_mutex);
	if(iRet < 0)
	{
        return sprintf(buf, "%s", "Coulometer Damaged or Firmware Error");
	}
    else
    {
        return sprintf(buf, "%x", iRet);
    }

}

static DRIVER_ATTR(state, S_IRUGO|S_IWUGO, bq275x0_attr_show, bq275x0_attr_store);
 


static int bq275x0_battery_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	int num;
	char *name;
	int retval = 0;
	struct bq275x0_device_info *di;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		BQ275x0_ERR("[%s,%d]: need I2C_FUNC_I2C\n",__FUNCTION__,__LINE__);
		return -ENODEV;
	}
   
	i2c_smbus_write_word_data(client,0x00,0x0008);
    mdelay(2);
	retval = i2c_smbus_read_word_data(client,0x00);
	if(retval<0)
	{
        printk(KERN_ERR "[%s,%d] Coulometer Damaged or Firmware Error\n",__FUNCTION__,__LINE__);
	}
    else
    {
        printk(KERN_ERR "Normal Mode and read Firmware version=%04x\n", retval);
    }
    
    retval = driver_create_file(&(bq275x0_battery_driver.driver), &driver_attr_state);
    if (0 != retval)
    {
		printk("failed to create sysfs entry(state): %d\n", retval);
        return -1;
    }

	power_set_batt_measurement_type(BATT_MEASURE_BY_BQ275x0);

	/* Get new ID for the new battery device */
	retval = idr_pre_get(&bq275x0_battery_id, GFP_KERNEL);
	if (retval == 0) {
		retval = -ENOMEM;
		goto batt_failed_0;
	}
	mutex_lock(&bq275x0_battery_mutex);
	retval = idr_get_new(&bq275x0_battery_id, client, &num);
	mutex_unlock(&bq275x0_battery_mutex);
	if (retval < 0) {
		goto batt_failed_0;
	}

	name = kasprintf(GFP_KERNEL, "bq275x0-%d", num);
	if (!name) {
		dev_err(&client->dev, "failed to allocate device name\n");
		retval = -ENOMEM;
		goto batt_failed_1;
	}

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		dev_err(&client->dev, "failed to allocate device info data\n");
		retval = -ENOMEM;
		goto batt_failed_2;
	}
	di->id = num;

	i2c_set_clientdata(client, di);
	di->dev = &client->dev;
	di->bat.name = name;
	di->client = client;

	bq275x0_powersupply_init(di);

	g_battery_measure_by_bq275x0_i2c_client = client;

	dev_info(&client->dev, "bq275x0 support ver. %s enabled\n", DRIVER_VERSION);

	return 0;

batt_failed_2:
	kfree(name);
batt_failed_1:
	mutex_lock(&bq275x0_battery_mutex);
	idr_remove(&bq275x0_battery_id, num);
	mutex_unlock(&bq275x0_battery_mutex);
batt_failed_0:

	power_set_batt_measurement_type(BATT_MEASURE_UNKNOW);

	return retval;
}

static int bq275x0_battery_remove(struct i2c_client *client)
{
	struct bq275x0_device_info *di = i2c_get_clientdata(client);

	free_irq(di->client->irq,di);
	power_supply_unregister(&di->bat);
	kfree(di->bat.name);

	mutex_lock(&bq275x0_battery_mutex);
	idr_remove(&bq275x0_battery_id, di->id);
	mutex_unlock(&bq275x0_battery_mutex);

	kfree(di);
	return 0;
}

/*
 * Module stuff
 */

static const struct i2c_device_id bq275x0_id[] = {
	{"bq275x0-battery",0},
	{},
};

static struct i2c_driver bq275x0_battery_driver = {
	.driver = {
		.name = "bq275x0-battery",
	},
	.probe = bq275x0_battery_probe,
	.remove = bq275x0_battery_remove,
	.id_table = bq275x0_id,
};

static int __init bq275x0_battery_init(void)
{
	int ret;

	ret = i2c_add_driver(&bq275x0_battery_driver);
	if (ret)
		BQ275x0_ERR("Unable to register BQ275x0 driver\n");

	return ret;
}

module_init(bq275x0_battery_init);

static void __exit bq275x0_battery_exit(void)
{
	i2c_del_driver(&bq275x0_battery_driver);
}
module_exit(bq275x0_battery_exit);

MODULE_AUTHOR("Rodolfo Giometti <giometti@linux.it>");
MODULE_DESCRIPTION("BQ275x0 battery monitor driver");
MODULE_LICENSE("GPL");
