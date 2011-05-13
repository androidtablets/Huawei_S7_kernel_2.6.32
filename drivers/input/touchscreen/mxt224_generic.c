/*
 * drivers/input/touchscreen/mxt422_generic.c
 *
 * Copyright (c) 2011 Huawei Device Co., Ltd.
 *
 * Using code from:
 *  The code is originated from Atmel Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/types.h> 
#include <linux/i2c.h>    
#include <linux/mxt224.h>
#include "mxt224_generic.h"

#if 0
#define DBG(fmt, args...) printk(KERN_INFO "[%s,%d] "fmt"\n", __FUNCTION__, __LINE__, ##args)
#else
#define DBG(fmt, args...) do {} while (0)
#endif

#define QT_printf   		DBG
#define malloc(size)   		kzalloc(size, GFP_KERNEL)
#define free(ptr)   		kfree(ptr)
#define OPTION_WRITE_CONFIG     /* uncomment to force chip setup at startup */

static struct mxt224 *mxt224_generic_tsc;

//General Object
gen_powerconfig_t7_config_t power_config = {0};                 //Power config settings.
gen_acquisitionconfig_t8_config_t acquisition_config = {0};     // Acquisition config. 

//Touch Object
touch_multitouchscreen_t9_config_t touchscreen_config = {0};    //Multitouch screen config.
touch_keyarray_t15_config_t keyarray_config = {0};              //Key array config.
touch_proximity_t23_config_t proximity_config = {0};        //Proximity config. 

//Signal Processing Objects
proci_gripfacesuppression_t20_config_t gripfacesuppression_config = {0};    //Grip / face suppression config.
procg_noisesuppression_t22_config_t noise_suppression_config = {0};         //Noise suppression config.
proci_onetouchgestureprocessor_t24_config_t onetouch_gesture_config = {0};  //One-touch gesture config. 
proci_twotouchgestureprocessor_t27_config_t twotouch_gesture_config = {0};  //Two-touch gesture config.

//Support Objects
spt_gpiopwm_t19_config_t  gpiopwm_config = {0};             //GPIO/PWM config
spt_selftest_t25_config_t selftest_config = {0};            //Selftest config.
spt_cteconfig_t28_config_t cte_config = {0};                //Capacitive touch engine config.
spt_comcconfig_t18_config_t   comc_config = {0};            //Communication config settings.

typedef struct
{
    uint8_t *txbuf;
    uint16_t txlen;
    uint8_t *rxbuf;
    uint16_t rxlen;
} i2c_package_t;

void delay_ms(unsigned short dly)
{
//	msleep(dly);
}

uint8_t write_mem(uint16_t Address, uint8_t ByteCount, uint8_t *Data)
{
    uint8_t status;
    uint8_t *txtmp;
    i2c_package_t packet;

    if ((txtmp = malloc(2 + ByteCount)) == NULL)
        return WRITE_MEM_FAILED;
    memset(txtmp, 0, 2 + ByteCount);

    *txtmp = (uint8_t)(Address & 0xFF);
    *(txtmp + 1) = (uint8_t) (Address >> 8);

    memcpy(txtmp + 2, Data, ByteCount);

    packet.txbuf = (void *)txtmp;
    packet.txlen = 2 + ByteCount;
    packet.rxbuf = NULL;
    packet.rxlen = 0;

    if (i2c_smbus_write_i2c_block_data(mxt224_generic_tsc->client, 
        							   *packet.txbuf, 
        							   packet.txlen - 1, 
        							   packet.txbuf + 1) < 0)
    {
        status = WRITE_MEM_FAILED;
	DBG("writing data failed");
    }
    else
    {
        status = WRITE_MEM_OK;
	DBG("writing data succeed");
    }

    free(txtmp);

    return status;
}

uint8_t read_mem(uint16_t Address, uint8_t ByteCount, uint8_t *Data)
{
    uint8_t status;
    uint8_t addr[2];
    s32 len;
    i2c_package_t packet;

    *addr = (uint8_t)(Address & 0xFF);
    *(addr + 1) = (uint8_t) (Address >> 8);
    packet.txbuf = (void *)addr;
    packet.txlen = 2;
    packet.rxbuf = NULL;
    packet.rxlen = 0;

    if (i2c_smbus_write_i2c_block_data(mxt224_generic_tsc->client, 
                                       *packet.txbuf, 
                                       packet.txlen - 1, 
                                       packet.txbuf + 1) < 0) {
		DBG("writing memory address failed");
        return READ_MEM_FAILED;
    }

    DBG("writing memory address succeed");
    packet.txbuf = NULL;
    packet.txlen = 0;
    packet.rxbuf = Data;
    packet.rxlen = ByteCount;

    if ((len = i2c_smbus_read_i2c_block_data(mxt224_generic_tsc->client, 
                                      0, 
                                      packet.rxlen, 
                                      packet.rxbuf)) < 0) {
      DBG("reading data failed");
    	status = READ_MEM_FAILED;
    } else {
    	  DBG("reading data succeed");
        status = READ_MEM_OK;
    }

    return status;
}

uint8_t read_uint16_t(uint16_t Address, uint16_t *Data)
{
    uint8_t Temp[2];
    uint8_t Status;

    Status = read_mem(Address, 2, Temp);
    *Data = ((uint16_t)Temp[1] << 8) + (uint16_t)Temp[0];

    return Status;
}

uint8_t address_slave(void)
{
	uint8_t tmp;
    
    if (read_mem(0, 1, &tmp) == READ_MEM_OK)
        return CONNECT_OK;
    else
        return CONNECT_ERROR;
}

uint8_t init_touch_driver(uint8_t I2C_address, void (*handler)(uint8_t *, uint8_t))
{
    uint16_t i;
    uint8_t tmp;
    uint16_t current_address;
    uint16_t crc_address;
    enum driver_setup_t driver_setup = DRIVER_SETUP_INCOMPLETE;

    uint8_t status;
    int current_report_id = 0;

    /* Poll device */
    status = address_slave();
    if (status == CONNECT_OK)
        QT_printf("I2C chip IC detected");
    else {
        QT_printf("I2C chip IC not detected");
        return DRIVER_SETUP_INCOMPLETE;
    }

    if ((info_block = malloc(sizeof(info_block_t))) == NULL)
        return DRIVER_SETUP_INCOMPLETE;

    /* Read the info block data. */
    if (read_id_block(&info_block->info_id) != READ_MEM_OK) {
        DBG("Reading ID block failed");
        goto err_info_block;
    }

    /* Read object table. */
    if ((info_block->objects = (object_t *)malloc(info_block->info_id.num_declared_objects 
                                      		 * sizeof(object_t))) == NULL)
        goto err_info_block;

    /* Reading the whole object table block to memory directly doesn't work cause sizeof object_t
    isn't necessarily the same on every compiler/platform due to alignment issues. Endianness
    can also cause trouble. */
    current_address = OBJECT_TABLE_START_ADDRESS;
    max_report_id = 0;
    for (i = 0; i < info_block->info_id.num_declared_objects; i++) {
        status = read_mem(current_address, 1, &info_block->objects[i].object_type);
        if (status != READ_MEM_OK) 
            goto err_object_table;
        current_address++;

        status = read_uint16_t(current_address, &info_block->objects[i].i2c_address);
        if (status != READ_MEM_OK)
            goto err_object_table;
        current_address += 2;

        status = read_mem(current_address, 1, &info_block->objects[i].size);
        if (status != READ_MEM_OK)
            goto err_object_table;
        current_address++;

        status = read_mem(current_address, 1, &info_block->objects[i].instances);
        if (status != READ_MEM_OK)
            goto err_object_table;
        current_address++;

        status = read_mem(current_address, 1, &info_block->objects[i].num_report_ids);
        if (status != READ_MEM_OK)
            goto err_object_table;
        current_address++;

        max_report_id += info_block->objects[i].num_report_ids;

        /* Find out the maximum message length. */
        if (info_block->objects[i].object_type == GEN_MESSAGEPROCESSOR_T5)
            max_message_length = info_block->objects[i].size + 1;

        DBG("T%d \tA%04x \tS%d \tI%d \tR%d", 
            info_block->objects[i].object_type, 
            info_block->objects[i].i2c_address, 
            info_block->objects[i].size, 
        	info_block->objects[i].instances,
            info_block->objects[i].num_report_ids);

		{
			unsigned char tempinfo[5] = {0};
			 read_mem(info_block->objects[i].i2c_address, 5, tempinfo);
			 DBG("%d OBJ: %d %d %d %d %d",
			 	info_block->objects[i].object_type,tempinfo[0],tempinfo[1],tempinfo[2],tempinfo[3],tempinfo[4]);
		}
    }

    /* Check that message processor was found. */
    if (max_message_length == 0)
        goto err_object_table;

    crc_address = OBJECT_TABLE_START_ADDRESS +
			      info_block->info_id.num_declared_objects * OBJECT_TABLE_ELEMENT_SIZE;
    status = read_mem(crc_address, 1u, &tmp);
    if (status != READ_MEM_OK)
        goto err_object_table;
    info_block->CRC = tmp;

    status = read_mem(crc_address + 1u, 1u, &tmp);
    if (status != READ_MEM_OK)
        goto err_object_table;
    info_block->CRC |= (tmp << 8u);

    status = read_mem(crc_address + 2, 1, &info_block->CRC_hi);
    if (status != READ_MEM_OK)
        goto err_object_table;

    /* Store message processor address, it is needed often on message reads. */
    message_processor_address = get_object_address(GEN_MESSAGEPROCESSOR_T5, 0);
    if (message_processor_address == 0)
        goto err_object_table;

    /* Store command processor address. */
    command_processor_address = get_object_address(GEN_COMMANDPROCESSOR_T6, 0);
    if (command_processor_address == 0)
        goto err_object_table;
    
    if ((mxt224_message = malloc(max_message_length)) == NULL)
        goto err_object_table;

    /* Allocate memory for report id map now that the number of report id's
     * is known. */
    if ((report_id_map = malloc(sizeof(report_id_map_t) * max_report_id + 1)) == NULL)
        goto err_msg;

    /* Report ID 0 is reserved, so start from 1. */
    report_id_map[0].instance = 0;
    report_id_map[0].object_type = 0;
    current_report_id = 1;

    for (i = 0; i < info_block->info_id.num_declared_objects; i++) {
        if (info_block->objects[i].num_report_ids != 0) {
            int instance;
            for (instance = 0; instance <= info_block->objects[i].instances; instance++) {
                int start_report_id = current_report_id;
                for (; current_report_id < (start_report_id + info_block->objects[i].num_report_ids); current_report_id++) {
                    report_id_map[current_report_id].instance = instance;
                    report_id_map[current_report_id].object_type =
                        info_block->objects[i].object_type;
                    //report_id_map[current_report_id].first_rid = start_report_id;
                }
            }
        }
    }

    if (info_block->info_id.version == 0x14 || info_block->info_id.version == 0x15) {
        if ((info_block->info_id.version == 0x14 && info_block->info_id.build == 0x0B)
            || (info_block->info_id.version == 0x15 && info_block->info_id.build == 0xAA)) {
            /* Store communication cfg processor address. */
            comc_cfg_address = get_object_address(SPT_COMCONFIG_T18, 0);
            if (command_processor_address == 0) 
                goto err_report_id_map;
        }

        diagnostic_addr = get_object_address(DEBUG_DIAGNOSTIC_T37, 0);
        if (diagnostic_addr == 0)
            goto err_report_id_map;

        diagnostic_size = get_object_size(DEBUG_DIAGNOSTIC_T37);        
    }

    driver_setup = DRIVER_SETUP_OK;
    goto err_ok;

err_report_id_map:
    free(report_id_map);
err_msg:
    free(mxt224_message);
err_object_table:
	free(info_block->objects);
err_info_block:
    free(info_block);
err_ok:
    return driver_setup;
}

uint8_t exit_touch_driver(void)
{
    free(info_block->objects);
    free(info_block);
    free(mxt224_message);
    free(report_id_map);

    return 0;
}

uint8_t reset_chip(void)
{
    uint8_t data = 1u;
    return write_mem(command_processor_address + RESET_OFFSET, 1, &data);
}

uint8_t calibrate_chip(void)
{
    uint8_t data = 1u;
    return write_mem(command_processor_address + CALIBRATE_OFFSET, 1, &data);
}

uint8_t diagnostic_chip(uint8_t mode)
{
    uint8_t status;
    status = write_mem(command_processor_address + DIAGNOSTIC_OFFSET, 1, &mode);
    return status;
}

uint8_t backup_config(void)
{
    /* Write 0x55 to BACKUPNV register to initiate the backup. */
    uint8_t data = 0x55u;
    return write_mem(command_processor_address + BACKUP_OFFSET, 1, &data);
}

uint8_t get_version(uint8_t *version)
{
    if (info_block) {
        *version = info_block->info_id.version;
		DBG("MX224 ic version = %d", *version);
    } else {
    	DBG("MX224 ic get version failed");
        return ID_DATA_NOT_AVAILABLE;
    }
    return ID_DATA_OK;
}

uint8_t get_family_id(uint8_t *family_id)
{
    if (info_block)
    {
        *family_id = info_block->info_id.family_id;
	  	DBG("MX224 ic family id = %d", *family_id);
    }
    else
    {
    	DBG("MX224 ic get family id failed");
        return ID_DATA_NOT_AVAILABLE;
    }
    
    return ID_DATA_OK;
}

uint8_t get_build_number(uint8_t *build)
{
    if (info_block)
    {
        *build = info_block->info_id.build;
		DBG("MX224 ic build number = %d", *build);
    }
    else
   {
   		DBG("MX224 ic get build number failed");
        return ID_DATA_NOT_AVAILABLE;
    }
    return ID_DATA_OK;
}

uint8_t get_variant_id(uint8_t *variant)
{
    if (info_block)
    {
        *variant = info_block->info_id.variant_id;
	   	DBG("MX224 ic variant id = %d", *variant);
    }
    else
    {
    	DBG("MX224 ic get variant id failed");
        return ID_DATA_NOT_AVAILABLE;
    }
    return ID_DATA_OK;
}

uint8_t write_power_config(gen_powerconfig_t7_config_t cfg)
{
    return write_simple_config(GEN_POWERCONFIG_T7, 0, (void *)&cfg);
}

uint8_t write_acquisition_config(gen_acquisitionconfig_t8_config_t cfg)
{
    return write_simple_config(GEN_ACQUISITIONCONFIG_T8, 0, (void *)&cfg);
}

uint8_t write_multitouchscreen_config(uint8_t instance, touch_multitouchscreen_t9_config_t cfg)
{
    uint16_t object_address;
    uint8_t *tmp;
    uint8_t status;
    uint8_t object_size;

    object_size = get_object_size(TOUCH_MULTITOUCHSCREEN_T9);
    if (object_size == 0)
        return CFG_WRITE_FAILED;

    tmp = (uint8_t *)malloc(object_size);
    if (tmp == NULL)
        return CFG_WRITE_FAILED;

    memset(tmp,0,object_size);

    /* 18 elements at beginning are 1 byte. */
    memcpy(tmp, &cfg, 18);

    /* Next two are 2 bytes. */
    *(tmp + 18) = (uint8_t)(cfg.xrange &  0xFF);
    *(tmp + 19) = (uint8_t)(cfg.xrange >> 8);

    *(tmp + 20) = (uint8_t)(cfg.yrange &  0xFF);
    *(tmp + 21) = (uint8_t)(cfg.yrange >> 8);

    /* And the last 4(8) 1 bytes each again. */
    *(tmp + 22) = cfg.xloclip;
    *(tmp + 23) = cfg.xhiclip;
    *(tmp + 24) = cfg.yloclip;
    *(tmp + 25) = cfg.yhiclip;

#if defined(__VER_1_4__) || defined(__VER_1_5__) ||defined(__VER_2_0__)
    *(tmp + 26) = cfg.xedgectrl;
    *(tmp + 27) = cfg.xedgedist;
    *(tmp + 28) = cfg.yedgectrl;
    *(tmp + 29) = cfg.yedgedist;
#endif

    object_address = get_object_address(TOUCH_MULTITOUCHSCREEN_T9, instance);

    if (object_address == 0)
        return CFG_WRITE_FAILED;
    status = write_mem(object_address, object_size, tmp);
    free(tmp);
    return status;
}

uint8_t write_keyarray_config(uint8_t instance, touch_keyarray_t15_config_t cfg)
{
    return write_simple_config(TOUCH_KEYARRAY_T15, instance, (void *)&cfg);
}

uint8_t write_linearization_config(uint8_t instance, proci_linearizationtable_t17_config_t cfg)
{
    uint16_t object_address;
    uint8_t *tmp;
    uint8_t status;
    uint8_t object_size;

    object_size = get_object_size(PROCI_LINEARIZATIONTABLE_T17);
    if (object_size == 0)
        return CFG_WRITE_FAILED;
    tmp = (uint8_t *)malloc(object_size);

    if (tmp == NULL)
        return CFG_WRITE_FAILED;

    memset(tmp,0,object_size);

    *(tmp + 0) = cfg.ctrl;
    *(tmp + 1) = (uint8_t)(cfg.xoffset & 0x00FF);
    *(tmp + 2) = (uint8_t)(cfg.xoffset >> 8);

    memcpy((tmp+3), &cfg.xsegment, 16);

    *(tmp + 19) = (uint8_t)(cfg.yoffset & 0x00FF);
    *(tmp + 20) = (uint8_t)(cfg.yoffset >> 8);

    memcpy((tmp+21), &cfg.ysegment, 16);

    object_address = get_object_address(PROCI_LINEARIZATIONTABLE_T17,
        instance);

    if (object_address == 0)
        return CFG_WRITE_FAILED;

    status = write_mem(object_address, object_size, tmp);
    free(tmp);

    return status;
}

uint8_t write_comc_config(uint8_t instance, spt_comcconfig_t18_config_t cfg)
{
    return(write_simple_config(SPT_COMCONFIG_T18, instance, (void *)&cfg));
}

uint8_t write_gpio_config(uint8_t instance, spt_gpiopwm_t19_config_t cfg)
{
    return(write_simple_config(SPT_GPIOPWM_T19, instance, (void *)&cfg));
}

uint8_t write_gripsuppression_config(uint8_t instance, proci_gripfacesuppression_t20_config_t cfg)
{
    return(write_simple_config(PROCI_GRIPFACESUPPRESSION_T20, instance,
        (void *)&cfg));
}

uint8_t write_noisesuppression_config(uint8_t instance, procg_noisesuppression_t22_config_t cfg)
{
    return(write_simple_config(PROCG_NOISESUPPRESSION_T22, instance,
        (void *)&cfg));
}

uint8_t write_proximity_config(uint8_t instance, touch_proximity_t23_config_t cfg)
{
    uint16_t object_address;
    uint8_t *tmp;
    uint8_t status;
    uint8_t object_size;

    object_size = get_object_size(TOUCH_PROXIMITY_T23);
    if (object_size == 0)
        return CFG_WRITE_FAILED;

    tmp = (uint8_t *)malloc(object_size);
    if (tmp == NULL)
        return CFG_WRITE_FAILED;

    memset(tmp,0,object_size);

    *(tmp + 0) = cfg.ctrl;
    *(tmp + 1) = cfg.xorigin;
    *(tmp + 2) = cfg.yorigin;
    *(tmp + 3) = cfg.xsize;
    *(tmp + 4) = cfg.ysize;
    *(tmp + 5) = cfg.reserved_for_future_aks_usage;
    *(tmp + 6) = cfg.blen;

    *(tmp + 7) = (uint8_t)(cfg.tchthr & 0x00FF);
    *(tmp + 8) = (uint8_t)(cfg.tchthr >> 8);

    *(tmp + 9) = cfg.tchdi;
    *(tmp + 10) = cfg.average;

    *(tmp + 11) = (uint8_t)(cfg.rate & 0x00FF);
    *(tmp + 12) = (uint8_t)(cfg.rate >> 8);

    object_address = get_object_address(TOUCH_PROXIMITY_T23, instance);
    if (object_address == 0)
        return CFG_WRITE_FAILED;

    status = write_mem(object_address, object_size, tmp);
    free(tmp);

    return status;
}

uint8_t write_onetouchgesture_config(uint8_t instance, proci_onetouchgestureprocessor_t24_config_t cfg)
{

    uint16_t object_address;
    uint8_t *tmp;
    uint8_t status;
    uint8_t object_size;

    object_size = get_object_size(PROCI_ONETOUCHGESTUREPROCESSOR_T24);
    if (object_size == 0)
        return CFG_WRITE_FAILED;

    tmp = (uint8_t *)malloc(object_size);
    if (tmp == NULL)
        return CFG_WRITE_FAILED;

    memset(tmp,0,object_size);

    *(tmp + 0) = cfg.ctrl;
#if defined(__VER_1_2__)
    *(tmp + 1) = 0;
#else
    *(tmp + 1) = cfg.numgest;
#endif

    *(tmp + 2) = (uint8_t)(cfg.gesten & 0x00FF);
    *(tmp + 3) = (uint8_t)(cfg.gesten >> 8);

    memcpy((tmp+4), &cfg.pressproc, 7);

    *(tmp + 11) = (uint8_t)(cfg.flickthr & 0x00FF);
    *(tmp + 12) = (uint8_t)(cfg.flickthr >> 8);

    *(tmp + 13) = (uint8_t)(cfg.dragthr & 0x00FF);
    *(tmp + 14) = (uint8_t)(cfg.dragthr >> 8);

    *(tmp + 15) = (uint8_t)(cfg.tapthr & 0x00FF);
    *(tmp + 16) = (uint8_t)(cfg.tapthr >> 8);

    *(tmp + 17) = (uint8_t)(cfg.throwthr & 0x00FF);
    *(tmp + 18) = (uint8_t)(cfg.throwthr >> 8);

    object_address = get_object_address(PROCI_ONETOUCHGESTUREPROCESSOR_T24,
        instance);

    if (object_address == 0)
        return CFG_WRITE_FAILED;

    status = write_mem(object_address, object_size, tmp);

    free(tmp);

    return status;
}

uint8_t write_selftest_config(uint8_t instance, spt_selftest_t25_config_t cfg)
{

    uint16_t object_address;
    uint8_t *tmp;
    uint8_t status;
    uint8_t object_size;

    object_size = get_object_size(SPT_SELFTEST_T25);
    if (object_size == 0)
        return CFG_WRITE_FAILED;
    tmp = (uint8_t *)malloc(object_size);

    if (tmp == NULL)
        return CFG_WRITE_FAILED;

    memset(tmp,0,object_size);

    *(tmp + 0) = cfg.ctrl;
    *(tmp + 1) = cfg.cmd;
#if defined(NUM_OF_TOUCH_OBJECTS)
    *(tmp + 2) = (uint8_t)(cfg.upsiglim & 0x00FF);
    *(tmp + 3) = (uint8_t)(cfg.upsiglim >> 8);

    *(tmp + 2) = (uint8_t)(cfg.losiglim & 0x00FF);
    *(tmp + 3) = (uint8_t)(cfg.losiglim >> 8);
#endif
    object_address = get_object_address(SPT_SELFTEST_T25,
        instance);

    if (object_address == 0)
        return CFG_WRITE_FAILED;

    status = write_mem(object_address, object_size, tmp);

    free(tmp);
    return status;
}

uint8_t write_twotouchgesture_config(uint8_t instance, proci_twotouchgestureprocessor_t27_config_t cfg)
{

    uint16_t object_address;
    uint8_t *tmp;
    uint8_t status;
    uint8_t object_size;

    object_size = get_object_size(PROCI_TWOTOUCHGESTUREPROCESSOR_T27);
    if (object_size == 0)
        return CFG_WRITE_FAILED;
    tmp = (uint8_t *)malloc(object_size);

    if (tmp == NULL)
        return CFG_WRITE_FAILED;

    memset(tmp,0,object_size);

    *(tmp + 0) = cfg.ctrl;

#if defined(__VER_1_2__)
    *(tmp + 1) = 0;
#else
    *(tmp + 1) = cfg.numgest;
#endif

    *(tmp + 2) = 0;

    *(tmp + 3) = cfg.gesten;

    *(tmp + 4) = cfg.rotatethr;

    *(tmp + 5) = (uint8_t)(cfg.zoomthr & 0x00FF);
    *(tmp + 6) = (uint8_t)(cfg.zoomthr >> 8);

    object_address = get_object_address(PROCI_TWOTOUCHGESTUREPROCESSOR_T27,
        instance);

    if (object_address == 0)
        return CFG_WRITE_FAILED;

    status = write_mem(object_address, object_size, tmp);
    free(tmp);

    return status;

}

uint8_t write_CTE_config(spt_cteconfig_t28_config_t cfg)
{

    return write_simple_config(SPT_CTECONFIG_T28, 0, (void *)&cfg);
}

uint8_t write_simple_config(uint8_t object_type, uint8_t instance, void *cfg)
{
    uint16_t object_address;
    uint8_t object_size;

    object_address = get_object_address(object_type, instance);
    object_size = get_object_size(object_type);

    if (object_size == 0 || object_address == 0)
        return CFG_WRITE_FAILED;

    return write_mem(object_address, object_size, cfg);
}

uint8_t get_object_size(uint8_t object_type)
{
    uint8_t object_table_index = 0;
    uint8_t object_found = 0;
    uint16_t size = OBJECT_NOT_FOUND;

    object_t *object_table;
    object_t obj;
    object_table = info_block->objects;
    while ((object_table_index < info_block->info_id.num_declared_objects) &&
        !object_found) {
        obj = object_table[object_table_index];
        
        /* Does object type match? */
        if (obj.object_type == object_type) {
            object_found = 1;
            size = obj.size + 1;
        }
        object_table_index++;
    }

    return size;
}

uint8_t type_to_report_id(uint8_t object_type, uint8_t instance)
{
    uint8_t report_id = 1;
    int8_t report_id_found = 0;

    while ((report_id <= max_report_id) && (report_id_found == 0)) {
        if((report_id_map[report_id].object_type == object_type) &&
            (report_id_map[report_id].instance == instance)) {
            report_id_found = 1;
        } else {
            report_id++;
        }
    }
    
    if (report_id_found) 
        return report_id;
    else 
        return ID_MAPPING_FAILED;
}

uint8_t report_id_to_type(uint8_t report_id, uint8_t *instance)
{
    if (report_id <= max_report_id) {
        *instance = report_id_map[report_id].instance;
        return report_id_map[report_id].object_type;
    } else
        return ID_MAPPING_FAILED;
}

uint8_t read_id_block(info_id_t *id)
{
    uint8_t status;

    status = read_mem(0, 1, (void *)&id->family_id);
    if (status != READ_MEM_OK) 
        return status;

    status = read_mem(1, 1, (void *)&id->variant_id);
    if (status != READ_MEM_OK)
        return status;

    status = read_mem(2, 1, (void *)&id->version);
    if (status != READ_MEM_OK)
        return status;

    status = read_mem(3, 1, (void *)&id->build);
    if (status != READ_MEM_OK)
        return status;

    status = read_mem(4, 1, (void *)&id->matrix_x_size);
    if (status != READ_MEM_OK)
        return status;

    status = read_mem(5, 1, (void *)&id->matrix_y_size);
    if (status != READ_MEM_OK)
        return status;

    status = read_mem(6, 1, (void *)&id->num_declared_objects);

	DBG("family_id = %d, variant_id = %d,version = %d, build = %d, matrix_x_size = %d, matrix_y_size = %d, num_declared_objects = %d ",
		id->family_id,id->variant_id,id->version,id->build,id->matrix_x_size,id->matrix_y_size,id->num_declared_objects);

	return status;
}

uint8_t get_max_report_id(void)
{
    return max_report_id;
}

uint16_t get_object_address(uint8_t object_type, uint8_t instance)
{
    uint8_t object_table_index = 0;
    uint8_t address_found = 0;
    uint16_t address = OBJECT_NOT_FOUND;

    object_t *object_table;
    object_t obj;
    object_table = info_block->objects;
    while ((object_table_index < info_block->info_id.num_declared_objects) &&
           !address_found) {
        obj = object_table[object_table_index];
        /* Does object type match? */
        if (obj.object_type == object_type) {
            address_found = 1;

            /* Are there enough instances defined in the FW? */
            if (obj.instances >= instance) 
                address = obj.i2c_address + (obj.size + 1) * instance;
        }
        object_table_index++;
    }

    return address;
}

uint32_t get_stored_infoblock_crc(void)
{
    uint32_t crc;
    crc = (uint32_t)(((uint32_t)info_block->CRC_hi) << 16);
    crc = crc | info_block->CRC;
    return crc;
}

uint8_t calculate_infoblock_crc(uint32_t *crc_pointer)
{

    uint32_t crc = 0;
    uint16_t crc_area_size;
    uint8_t *mem;

    uint8_t i;
    uint8_t status;

    /* 7 bytes of version data, 6 * NUM_OF_OBJECTS bytes of object table. */
    crc_area_size = 7 + info_block->info_id.num_declared_objects * 6;

    mem = (uint8_t *)malloc(crc_area_size);
    if (mem == NULL)
        return CRC_CALCULATION_FAILED;

    status = read_mem(0, crc_area_size, mem);
    if (status != READ_MEM_OK)
        return CRC_CALCULATION_FAILED;

	{
		unsigned int count = 0;
		unsigned char *ptemp = mem;
		while (count < crc_area_size)
		{
			DBG("calculate_infoblock_crc:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
				*ptemp,*(ptemp+1),*(ptemp+2),*(ptemp+3),
				*(ptemp+4),*(ptemp+5),*(ptemp+6),*(ptemp+7));
			count+=8;
			ptemp += 8;
		}
 	}
    i = 0;
    while (i < (crc_area_size - 1)) {
        crc = CRC_24(crc, *(mem + i), *(mem + i + 1));
        i += 2;
    }

    crc = CRC_24(crc, *(mem + i), 0);

    free(mem);

    /* Return only 24 bit CRC. */
    *crc_pointer = (crc & 0x00FFFFFF);
    return CRC_CALCULATION_OK;
}

uint32_t CRC_24(uint32_t crc, uint8_t byte1, uint8_t byte2)
{
    static const uint32_t crcpoly = 0x80001B;
    uint32_t result;
    uint16_t data_word;

    data_word = (uint16_t)((uint16_t)(byte2 << 8u) | byte1);
    result = ((crc << 1u) ^ (uint32_t)data_word);

    if (result & 0x1000000)
        result ^= crcpoly;

    return result;
}

void qt_Power_Config_Init(void)
{
    /* Set Idle Acquisition Interval to 32 ms. */
    power_config.idleacqint = 50;

    /* Set Active Acquisition Interval to 16 ms. */
    power_config.actvacqint = 255;

    /* Set Active to Idle Timeout to 4 s (one unit = 200ms). */
    power_config.actv2idleto = 50;

    /* Write power config to chip. */
    if (write_power_config(power_config) != CFG_WRITE_OK)
        QT_printf("Power configuration failed");
}

void qt_Acquisition_Config_Init(void)
{
    acquisition_config.chrgtime = 10; // 2us
    acquisition_config.reserved = 0;
    acquisition_config.tchdrift = 5; // 4s
    acquisition_config.driftst = 10; // 4s
    acquisition_config.tchautocal = 0; // infinite
    acquisition_config.sync = 0; // disabled

#if defined(__VER_1_4__) || defined(__VER_1_5__) ||defined(__VER_2_0__)
    acquisition_config.atchcalst = 5;
    acquisition_config.atchcalsthr = 10;
#endif

    if (write_acquisition_config(acquisition_config) != CFG_WRITE_OK)
        QT_printf("Acquisition configuration failed");
}

void qt_Multitouchscreen_Init(void)
{
    touchscreen_config.ctrl = 3;//0x8F; // enable + message-enable
    touchscreen_config.xorigin = 0;
    touchscreen_config.yorigin = 0;
    touchscreen_config.xsize = 19;
    touchscreen_config.ysize = 11;
    touchscreen_config.akscfg = 0;
    touchscreen_config.blen = 0;  // step in 2 between 0x20~0x50
    touchscreen_config.tchthr = 35;  // threshold step in 10 between 20 and 200
    touchscreen_config.tchdi = 4;
    touchscreen_config.orient = 2;
    touchscreen_config.mrgtimeout = 0;
    touchscreen_config.movhysti = 5;
    touchscreen_config.movhystn = 1;
    touchscreen_config.movfilter = 0;
    touchscreen_config.numtouch= MAX_FINGERS; 
    touchscreen_config.mrghyst = 5;
    touchscreen_config.mrgthr = 25;
    touchscreen_config.amphyst = 0;
    //touchscreen_config.xrange = mxt224_generic_tsc->x_max;
    //touchscreen_config.yrange = mxt224_generic_tsc->y_max;
    touchscreen_config.xrange = 860;
    touchscreen_config.yrange = 480;
    touchscreen_config.xloclip = 0;
    touchscreen_config.xhiclip = 0;
    touchscreen_config.yloclip = 0;
    touchscreen_config.yhiclip = 0;
#if defined(__VER_1_4__) || defined(__VER_1_5__) ||defined(__VER_2_0__)
    touchscreen_config.xedgectrl = 0;
    touchscreen_config.xedgedist = 0;
    touchscreen_config.yedgectrl = 0;
    touchscreen_config.yedgedist = 0;
#endif

    if (write_multitouchscreen_config(0, touchscreen_config) != CFG_WRITE_OK)
        QT_printf("Multi touch configuration failed");
}

void qt_KeyArray_Init(void)
{
    keyarray_config.ctrl = 0;
    keyarray_config.xorigin = 0;
    keyarray_config.yorigin = 0;
    keyarray_config.xsize = 0;
    keyarray_config.ysize = 0;
    keyarray_config.akscfg = 0;
    keyarray_config.blen = 0;
    keyarray_config.tchthr = 0;
    keyarray_config.tchdi = 0;
    keyarray_config.reserved[0] = 0;
    keyarray_config.reserved[1] = 0;

    if (write_keyarray_config(0, keyarray_config) != CFG_WRITE_OK) 
        QT_printf("Key array configuration failed");
}

void qt_ComcConfig_Init(void)
{
    comc_config.ctrl = 0x01;
    comc_config.cmd = COMM_MODE1;

    if (get_object_address(SPT_COMCONFIG_T18, 0) != OBJECT_NOT_FOUND) {
        if (write_comc_config(0, comc_config) != CFG_WRITE_OK)
            QT_printf("Communication configuration failed");
    }
}

void qt_Gpio_Pwm_Init(void)
{
    gpiopwm_config.ctrl = 0;
    gpiopwm_config.reportmask = 0;
    gpiopwm_config.dir = 0;
    gpiopwm_config.intpullup = 0;
    gpiopwm_config.out = 0;
    gpiopwm_config.wake = 0;
    gpiopwm_config.pwm = 0;
    gpiopwm_config.period = 0;
    gpiopwm_config.duty[0] = 0;
    gpiopwm_config.duty[1] = 0;
    gpiopwm_config.duty[2] = 0;
    gpiopwm_config.duty[3] = 0;

    if (write_gpio_config(0, gpiopwm_config) != CFG_WRITE_OK) 
        QT_printf("GPIO PWM configuration failed");
}

void qt_Grip_Face_Suppression_Config_Init(void)
{
    gripfacesuppression_config.ctrl = 0;
    gripfacesuppression_config.xlogrip = 0;
    gripfacesuppression_config.xhigrip = 0;
    gripfacesuppression_config.ylogrip = 0;
    gripfacesuppression_config.yhigrip = 0;
    gripfacesuppression_config.maxtchs = 0;
    gripfacesuppression_config.reserved = 0;
    gripfacesuppression_config.szthr1 = 85;
    gripfacesuppression_config.szthr2 = 40;
    gripfacesuppression_config.shpthr1 = 4;
    gripfacesuppression_config.shpthr2 = 15;

#if defined(__VER_1_4__) || defined(__VER_1_5__) ||defined(__VER_2_0__)
    gripfacesuppression_config.supextto = 0;
#endif

    /* Write grip suppression config to chip. */
    if (get_object_address(PROCI_GRIPFACESUPPRESSION_T20, 0) != OBJECT_NOT_FOUND) {
        if (write_gripsuppression_config(0, gripfacesuppression_config) !=
            CFG_WRITE_OK) {
            QT_printf("Grip face suppression configuration failed");
        }
    }
}

void qt_Noise_Suppression_Config_Init(void)
{
    noise_suppression_config.ctrl = 13;  

#if defined(__VER_1_2__)
    noise_suppression_config.outflen = 0;
#elif defined(__VER_1_4__) || defined(__VER_1_5__) ||defined(__VER_2_0__)
    noise_suppression_config.reserved = 0;
#endif

    noise_suppression_config.reserved1 = 0;
    noise_suppression_config.gcaful = 0;
    noise_suppression_config.gcafll = 0;

#if defined(__VER_1_2__)
    noise_suppression_config.gcaflcount = 0;
#elif defined(__VER_1_4__) || defined(__VER_1_5__) ||defined(__VER_2_0__)
    noise_suppression_config.actvgcafvalid = 0;
#endif

   noise_suppression_config.noisethr = 30;
    noise_suppression_config.freqhopscale = 0;
	
#if defined(__VER_1_2__)
    noise_suppression_config.freq0 = 0;
    noise_suppression_config.freq1 = 0;
    noise_suppression_config.freq2 = 0;
#elif defined(__VER_1_4__) || defined(__VER_1_5__) ||defined(__VER_2_0__)
    noise_suppression_config.freq[0] = 0;
    noise_suppression_config.freq[1] = 10;
    noise_suppression_config.freq[2] = 25;
    noise_suppression_config.freq[3] = 255;
    noise_suppression_config.freq[4] = 255;
    noise_suppression_config.idlegcafvalid = 3;
#endif

    /* Write Noise suppression config to chip. */
    if (get_object_address(PROCG_NOISESUPPRESSION_T22, 0) != OBJECT_NOT_FOUND) {
        if (write_noisesuppression_config(0,noise_suppression_config) != CFG_WRITE_OK) {
            QT_printf("Noise suppression configuration failed");
        }
    }
}

void qt_Proximity_Config_Init(void)
{
    proximity_config.ctrl = 0;
    proximity_config.xorigin = 0;
    proximity_config.xsize = 0;
    proximity_config.ysize = 0;
    proximity_config.reserved_for_future_aks_usage = 0;
    proximity_config.blen = 0;
    proximity_config.tchthr = 0;
    proximity_config.tchdi = 0;
    proximity_config.average = 0;
    proximity_config.rate = 0;

    if (get_object_address(TOUCH_PROXIMITY_T23, 0) != OBJECT_NOT_FOUND) {
        if (write_proximity_config(0, proximity_config) != CFG_WRITE_OK) {
            QT_printf("Proximity configuration failed");
        }
    }
}

void qt_One_Touch_Gesture_Config_Init(void)
{
    /* Disable one touch gestures. */
    onetouch_gesture_config.ctrl = 0;
#if defined(__VER_1_2__)
    onetouch_gesture_config.reserved_1 = 0;
#elif defined(__VER_1_4__) || defined(__VER_1_5__) ||defined(__VER_2_0__)
    onetouch_gesture_config.numgest = 0;
#endif

    onetouch_gesture_config.gesten = 0;
    onetouch_gesture_config.pressproc = 0;
    onetouch_gesture_config.tapto = 0;
    onetouch_gesture_config.flickto = 0;
    onetouch_gesture_config.dragto = 0;
    onetouch_gesture_config.spressto = 0;
    onetouch_gesture_config.lpressto = 0;
    onetouch_gesture_config.reppressto = 0;
    onetouch_gesture_config.flickthr = 0;
    onetouch_gesture_config.dragthr = 0;
    onetouch_gesture_config.tapthr = 0;
    onetouch_gesture_config.throwthr = 0;

    if (get_object_address(PROCI_ONETOUCHGESTUREPROCESSOR_T24, 0) !=
        OBJECT_NOT_FOUND) {
        if (write_onetouchgesture_config(0, onetouch_gesture_config) !=
            CFG_WRITE_OK) {
            QT_printf("One touch gesture configuration failed");
        }
    }
}

void qt_Selftest_Init(void)
{
    selftest_config.ctrl = 0;
    selftest_config.cmd = 0;

#if defined(NUM_OF_TOUCH_OBJECTS)
    siglim.upsiglim[0] = 12000;
    siglim.losiglim[0] = 7000;
#endif
    if (get_object_address(SPT_SELFTEST_T25, 0) != OBJECT_NOT_FOUND) {
        if (write_selftest_config(0,selftest_config) != CFG_WRITE_OK) {
            QT_printf("Selftest gesture configuration failed");
        }
    }
}

void qt_Two_touch_Gesture_Config_Init(void)
{
    /* Disable two touch gestures. */
    twotouch_gesture_config.ctrl = 0;
#if defined(__VER_1_2__)
    twotouch_gesture_config.reserved1 = 0;
#elif defined(__VER_1_4__) || defined(__VER_1_5__) ||defined(__VER_2_0__)
    twotouch_gesture_config.numgest = 0;
#endif
    twotouch_gesture_config.reserved2 = 0;
    twotouch_gesture_config.gesten = 0;
    twotouch_gesture_config.rotatethr = 0;
    twotouch_gesture_config.zoomthr = 0;

    if (get_object_address(PROCI_TWOTOUCHGESTUREPROCESSOR_T27, 0) !=
        OBJECT_NOT_FOUND) {
        if (write_twotouchgesture_config(0, twotouch_gesture_config) !=
            CFG_WRITE_OK) {
            QT_printf("Two touch gesture configuration failed");
        }
    }
}

void qt_CTE_Config_Init(void)
{
    /* Set CTE config */
    cte_config.ctrl = 0;
    cte_config.cmd = 0;
    cte_config.mode = 3;
    cte_config.idlegcafdepth = 8;
    cte_config.actvgcafdepth = 8;

    /* Write CTE config to chip. */
    if (get_object_address(SPT_CTECONFIG_T28, 0) != OBJECT_NOT_FOUND) {
        if (write_CTE_config(cte_config) != CFG_WRITE_OK) {
            QT_printf("CTE configuration failed");
        }
    }
} 

unsigned char Comm_Config_Process(unsigned char change_en)
{
    if(change_en == 1) {
        change_en = 0;
        if (get_object_address(SPT_COMCONFIG_T18, 0) != OBJECT_NOT_FOUND) {
            if (comc_config.cmd == COMM_MODE3) {
                if (1) { //(PIND_Bit2 == 1) {
                    comc_config.cmd = COMM_MODE1;
                    return change_en;
                }
            }
            if (write_comc_config(0, comc_config) != CFG_WRITE_OK)
                return change_en;
        }
    }
    
    return change_en;
}

uint8_t config_disable_mxt244(void)
{
    uint16_t object_address;
    uint8_t data = 0u;
    
 	/*clear ENABLE bit of TOUCH_MULTITOUCHSCREEN_T9*/    
    object_address = get_object_address(TOUCH_MULTITOUCHSCREEN_T9, 0);
    if (object_address == 0)
        return CFG_WRITE_FAILED;
    return write_mem(object_address, 1, &data);
}
uint8_t config_enable_mxt244(void)
{
    uint16_t object_address;
    uint8_t data = 3u;

    /*set ENABLE bit of TOUCH_MULTITOUCHSCREEN_T9*/
    object_address = get_object_address(TOUCH_MULTITOUCHSCREEN_T9, 0);
    if (object_address == 0)
        return CFG_WRITE_FAILED;
    return write_mem(object_address, 1, &data);
}

void get_message(struct ts_event *tch)
{
        uint8_t tmsg[9];
        DBG("Enter get_message");
        if (read_mem(message_processor_address, max_message_length, tmsg) == READ_MEM_OK) {
            DBG(" read mem succeed!");
            DBG("%02x %02x %02x %02x %02x %02x %02x %02x %02x ", 
                tmsg[0], tmsg[1], tmsg[2], tmsg[3], 
                tmsg[4], tmsg[5], tmsg[6], tmsg[7], 
                tmsg[8]);
            tch->touch_number_id=tmsg[0];
            if (tmsg[0] > 1 && tmsg[0] < MAX_FINGERS+2) {
                tch->tchstatus = tmsg[1];
                tch->x = ((tmsg[2]) << 2) + ((tmsg[4] & 0xC0) >> 6);
                tch->y = ((tmsg[3]) << 2) + ((tmsg[4] & 0x0C) >> 2);
                tch->tcharea = tmsg[5];
                tch->tchamp = tmsg[6];
                tch->tchvector = tmsg[7];
            } else if (tmsg[0] == 0xFF) {
    
            }
        }
        else
            DBG(" read mem failed!");
}

uint8_t init_touch_app(void)
{
    uint8_t return_val = TRUE;
    uint8_t report_id;
    uint8_t report_id_sum;
    uint8_t object_type, instance;
    uint32_t crc, stored_crc;
    uint8_t version = 0, family_id = 0, variant = 0, build = 0;

    if (init_touch_driver(0xFF,  NULL) != DRIVER_SETUP_OK) {
        QT_printf("Connect/info block read failed!");
        return FALSE;
    } else {
        get_family_id(&family_id);
        get_variant_id(&variant);
        get_version(&version);
        get_build_number(&build);

        QT_printf("Version:        0x%x", version);
        QT_printf("Family:         0x%x", family_id);
        QT_printf("Variant:        0x%x", variant);
        QT_printf("Build number:   0x%x", build);
        QT_printf("Matrix X size : %d", info_block->info_id.matrix_x_size);
        QT_printf("Matrix Y size : %d", info_block->info_id.matrix_y_size);
        
        if (calculate_infoblock_crc(&crc) != CRC_CALCULATION_OK) {
            QT_printf("Calculating CRC failed, skipping check!");
        } else {
            QT_printf("Calced CRC: \t0x%08x", crc);
        }

        stored_crc = get_stored_infoblock_crc();
        QT_printf("Stored CRC: \t0x%08x", stored_crc);
        if (stored_crc != crc) {
            QT_printf("Warning: info block CRC value doesn't match the calculated!");
        } else {
            QT_printf("Info block CRC value OK.");
        }

        /* Test the report id to object type / instance mapping: get the maximum
        * report id and print the report id map. */
        QT_printf("Report ID to Object type/instance mapping:");
        report_id_sum = get_max_report_id();
        for (report_id = 1; report_id <= report_id_sum; report_id++) {
            object_type = report_id_to_type(report_id, &instance);
            QT_printf("Report ID : %d, Object Type : T%d, Instance : %d",
                      report_id , object_type, instance);
        }

#ifdef OPTION_WRITE_CONFIG
        QT_printf("Chip config...");
		qt_Power_Config_Init();
		qt_Acquisition_Config_Init();
		qt_Multitouchscreen_Init();
		qt_KeyArray_Init();
		qt_ComcConfig_Init();
		qt_Gpio_Pwm_Init();
		qt_Grip_Face_Suppression_Config_Init();
		qt_Noise_Suppression_Config_Init();
		qt_Proximity_Config_Init();
		qt_One_Touch_Gesture_Config_Init();
		qt_Selftest_Init();
		qt_Two_touch_Gesture_Config_Init();
		qt_CTE_Config_Init();

        /* Backup settings to NVM. */
        if (backup_config() != WRITE_MEM_OK) {
            QT_printf("Backing up configs failed");
            return_val = FALSE;
        } else {
            QT_printf("Backing up configs OK"); 
        }
#else
        QT_printf("Chip setup sequence was bypassed!");
#endif /* OPTION_WRITE_CONFIG */

        /* Calibrate the touch IC. */
        if (calibrate_chip() != WRITE_MEM_OK) {
            QT_printf("Failed to calibrate, exiting...");
            return_val = FALSE;
        } else {
            QT_printf("Chip calibrated!");
        }

        QT_printf("Waiting for touch chip messages...");
    }

    if (return_val == FALSE)
	    exit_touch_driver();

    return return_val;
}

int mxt224_generic_probe(struct mxt224 *tsc)
{
    uint8_t version, family_id, variant, build;

	if (tsc)
		mxt224_generic_tsc = tsc;
    else
        return -1;
    
    if (init_touch_app() == TRUE) {
        get_family_id(&family_id);
        get_variant_id(&variant);
        get_version(&version);
        get_build_number(&build);
    } else
    	return -1;
    
    return 0;
}
EXPORT_SYMBOL(mxt224_generic_probe);

void mxt224_generic_remove(struct mxt224 *tsc)
{
    exit_touch_driver();
}
EXPORT_SYMBOL(mxt224_generic_remove);

void mxt224_get_message(struct mxt224 *tsc)
{
	DBG("Enter mxt224_get_message");
    get_message(&tsc->tc);
}
EXPORT_SYMBOL(mxt224_get_message);


unsigned char read_diagnostic_debug(debug_diagnositc_t37_t *dbg, unsigned char mode, unsigned char page)
{
    unsigned char status;

    diagnostic_addr = get_object_address(DEBUG_DIAGNOSTIC_T37, 0);
    diagnostic_size = get_object_size(DEBUG_DIAGNOSTIC_T37);

    diagnostic_chip(mode);
    status = read_mem(diagnostic_addr, 2, &dbg->mode);

    do {
		status = read_mem(diagnostic_addr, 2, &dbg->mode);
		//QT_printf("[TSP] DBG_PAGE = %d ",dbg->page);
		if(status == READ_MEM_OK) {
			if (dbg->page > 128) {
				if (page > 128) {
					if(dbg->page < page) {
						status = diagnostic_chip(QT_PAGE_UP);
						delay_ms(1);
					} else if(dbg->page > page) {
						diagnostic_chip(QT_PAGE_DOWN);
						delay_ms(1);
					}
				} else {
					if (dbg->page > page) {
					    diagnostic_chip(QT_PAGE_UP);
					    delay_ms(1);
					} else if(dbg->page < page) {
					    diagnostic_chip(QT_PAGE_DOWN);
					    delay_ms(1);
					}
				}
			} else {
                if (page < 128) {
                    if (dbg->page < page)
                    {
                        diagnostic_chip(QT_PAGE_UP);
                        delay_ms(1);
                    }
                    else if (dbg->page > page)
                    {
                        diagnostic_chip(QT_PAGE_DOWN);
                        delay_ms(1);
                    }
                }
                else
                {
                    if(dbg->page > page)
                    {
                        diagnostic_chip(QT_PAGE_UP);
                        delay_ms(1);
                    }
                    else if(dbg->page < page)
                    {
                        diagnostic_chip(QT_PAGE_DOWN);
                        delay_ms(1);
                    }
                }
            }
        }

    }while(dbg->page != page);

    status = read_mem(diagnostic_addr,diagnostic_size, &dbg->mode);

    return status;
}


unsigned char read_diagnostic_delta(debug_diagnositc_t37_delta_t * dbg, unsigned char page)
{
    unsigned char status;

    diagnostic_chip(QT_DELTA_MODE);

    diagnostic_addr = get_object_address(DEBUG_DIAGNOSTIC_T37, 0);
    diagnostic_size = get_object_size(DEBUG_DIAGNOSTIC_T37);

    read_mem(diagnostic_addr,2, &dbg->mode);

    do
    {
        status = read_mem(diagnostic_addr,2, & dbg->mode);

        if(status == READ_MEM_OK)
        {
            if(dbg->page > 128)
            {
                if(page > 128)
                {
                    if(dbg->page < page)
                    {
                        diagnostic_chip(QT_PAGE_UP);
                    }
                    else if(dbg->page > page)
                    {
                        diagnostic_chip(QT_PAGE_DOWN);
                    }
                }
                else
                {
                    if(dbg->page > page)
                    {
                        diagnostic_chip(QT_PAGE_UP);
                    }
                    else if(dbg->page < page)
                    {
                        diagnostic_chip(QT_PAGE_DOWN);
                    }
                }
            }
            else
            {
                if(page < 128)
                {
                    if(dbg->page < page)
                    {
                        diagnostic_chip(QT_PAGE_UP);
                    }
                    else if(dbg->page > page)
                    {
                        diagnostic_chip(QT_PAGE_DOWN);
                    }
                }
                else
                {
                    if(dbg->page > page)
                    {
                        diagnostic_chip(QT_PAGE_UP);
                    }
                    else if(dbg->page < page)
                    {
                        diagnostic_chip(QT_PAGE_DOWN);
                    }
                }
            }
        }
        delay_ms(1);
    }while(dbg->page != page);

    status = read_mem(diagnostic_addr,diagnostic_size, &dbg->mode);

    return status;
}

unsigned char read_diagnostic_reference(debug_diagnositc_t37_reference_t * dbg,unsigned char page)
{
    unsigned char status;

    diagnostic_chip(QT_REFERENCE_MODE);

    diagnostic_addr = get_object_address(DEBUG_DIAGNOSTIC_T37, 0);
    diagnostic_size = get_object_size(DEBUG_DIAGNOSTIC_T37);

    read_mem(diagnostic_addr,2, &dbg->mode);

    do
    {
        status = read_mem(diagnostic_addr,2, & dbg->mode);

        if(status == READ_MEM_OK)
        {
            if(dbg->page > 128)
            {
                if(page > 128)
                {
                    if(dbg->page < page)
                    {
                        diagnostic_chip(QT_PAGE_UP);
                    }
                    else if(dbg->page > page)
                    {
                        diagnostic_chip(QT_PAGE_DOWN);
                    }
                }
                else
                {
                    if(dbg->page > page)
                    {
                        diagnostic_chip(QT_PAGE_UP);
                    }
                    else if(dbg->page < page)
                    {
                        diagnostic_chip(QT_PAGE_DOWN);
                    }
                }
            }
            else
            {
                if(page < 128)
                {
                    if(dbg->page < page)
                    {
                        diagnostic_chip(QT_PAGE_UP);
                    }
                    else if(dbg->page > page)
                    {
                        diagnostic_chip(QT_PAGE_DOWN);
                    }
                }
                else
                {
                    if(dbg->page > page)
                    {
                        diagnostic_chip(QT_PAGE_UP);
                    }
                    else if(dbg->page < page)
                    {
                        diagnostic_chip(QT_PAGE_DOWN);
                    }
                }
            }
        }
        delay_ms(1);
    }while(dbg->page != page);

    status = read_mem(diagnostic_addr,diagnostic_size, &dbg->mode);

    return status;
}



