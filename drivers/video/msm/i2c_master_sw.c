/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include "SIIdefs.h"
#include "SIIConstants.h"
#include "SIITypeDefs.h"


extern struct i2c_client *gHdmi_client;
#define  BSP_HDMI_I2C_MAX_RETRY      2

int chip_i2c_txdata(unsigned short saddr, unsigned char *txdata, int length)
{
    struct i2c_msg msg[] = 
    {
        {
            .addr  = saddr,
                .flags = 0,
                .len = length,
                .buf = txdata,
        },
    };
    
    if (i2c_transfer(gHdmi_client->adapter, msg, 1) < 0) 
    {
        printk(KERN_ERR "hdmi_i2c_txdata failed\n");
        return -EIO;
    }
    
    return 0;   
}
 
int chip_i2c_write_client_reg(unsigned short saddr, char client_reg, char bdata)
{
    unsigned char buf[2];
    
    struct i2c_msg msg[] = 
    {
        {
            .addr  = saddr,
            .flags = 0,
            .len = 2,
            .buf = buf,
        },
    };
     
    memset(buf, 0, sizeof(buf));
    buf[0] = client_reg;
    buf[1] = bdata; 
    
    if (i2c_transfer(gHdmi_client->adapter, msg, 1) < 0) 
    {
        printk(KERN_ERR "HDMI: hdmi_i2c_txdata failed\n");
        return -EIO;
    }
    
    
   return 0;
}       

int hdmi_i2c_read_client_reg(byte saddr, byte client_reg, 
                             byte *bdata, unsigned int length)
{
    struct i2c_msg msgs[] = 
    {
        {   .addr   = saddr,
            .flags = 0,
            .len   = 1,
            .buf   = &client_reg,
        },
        {   .addr   = saddr,
        .flags = I2C_M_RD,
        .len   = length,
        .buf   = bdata,
        },
    };  
    if (i2c_transfer(gHdmi_client->adapter, msgs, 2) < 0) 
    {
        printk(KERN_INFO "HDMI: hdmi_i2c_read_client_reg failed!\n");
        return -EIO;
    }
    return 0;
}

//------------------------------------------------------------------------------
// Function: I2C_WriteByte
// Description:
//------------------------------------------------------------------------------
void I2C_WriteByte(byte deviceID, byte offset, byte value)
{
    int iRet = 0, i = 0;

    for(i=0;i<BSP_HDMI_I2C_MAX_RETRY;i++)
    {
        iRet = chip_i2c_write_client_reg(deviceID, offset, value);
        if(0 == iRet)
        {
            break;
        }
        udelay(10);
    }
}


//------------------------------------------------------------------------------
// Function: I2C_ReadByte
// Description:
//------------------------------------------------------------------------------
byte I2C_ReadByte(byte deviceID, byte offset)
{
    byte abyte = 0;
    int iRet = 0, i = 0;
    
    for(i=0;i<BSP_HDMI_I2C_MAX_RETRY;i++)
    {
        iRet = hdmi_i2c_read_client_reg(deviceID, offset, &abyte, 1);
        if(0 == iRet)
        {
            break;
        }
        udelay(10);
        abyte = 0;
    }
    
    return (abyte);
}


//------------------------------------------------------------------------------
// Function: I2C_WriteBlock
// Description:
//------------------------------------------------------------------------------
byte I2C_WriteBlock(byte deviceID, byte offset, byte* buffer, word length)
{
    byte *buffer_temp = (byte *)kzalloc(length + 1, GFP_KERNEL);
    memcpy(buffer_temp, &offset, 1);
    memcpy(buffer_temp + 1, buffer, length);
    chip_i2c_txdata(/*(unsigned short)*/deviceID, buffer_temp, (int)length +1);
    kfree(buffer_temp);
    buffer_temp = NULL;
    return (0);
}


//------------------------------------------------------------------------------
// Function: I2C_ReadBlock
// Description:
//------------------------------------------------------------------------------
byte I2C_ReadBlock(byte deviceID, byte offset, byte *buffer, word length)
{
    hdmi_i2c_read_client_reg(deviceID, offset, buffer, length);;
    return (0);
}


//------------------------------------------------------------------------------
// Function: I2C_ReadSegmentBlock
// Description:
//------------------------------------------------------------------------------
#ifndef CBUS_EDID
char I2C_ReadSegmentBlock(char deviceID, char segment, char offset, char *buffer, word length)
{
    chip_i2c_write_client_reg((unsigned short)EDID_SEG_ADDR, 0x01, segment);
    I2C_ReadBlock(deviceID, offset, buffer, length);
    return (0);
}
#endif
