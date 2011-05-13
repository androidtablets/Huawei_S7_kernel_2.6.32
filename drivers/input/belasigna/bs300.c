/** 
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
 
/**
 *  bs300-audio I2C driver.
 *  Belasigna BS300 (MIC noise reducer) driver 
 */
 
#include <linux/module.h>
#include <linux/param.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <mach/gpio-i2c-adpt.h>

#include "bs300.h"
#include "belasigna300_debug_protocol.h"
#include "download_data.h"

struct i2c_client* bs300_i2c_client = NULL;

static unsigned int 
bs300_send_receive( int sendCount, int receiveCount,
        unsigned char *pSendData, unsigned char *pReceiveData )
{
	int err;
	struct i2c_msg msg[1];
	struct i2c_client *client = bs300_i2c_client;

	
	if (!client->adapter)
	{
		BS300_ERR("[%s,%d]invalid i2c client\n",__FUNCTION__,__LINE__);
		return BS300_FAIL;
	}

	gpio_i2c_adpt_set_state(STATE_MIC_NOISE_REDUCE);
	if(sendCount != 0)
	{
		msg->addr = client->addr;
		msg->len = sendCount;
		msg->flags = 0;		/*i2c write flag*/
		msg->buf = pSendData;
		
		err = i2c_transfer(client->adapter, msg, 1);
		if(err<0)
		{
			BS300_ERR("[%s,%d]i2c write byte fail, addr = %x, err = %d\n"
						,__FUNCTION__,__LINE__,msg->addr,err);
			goto out_err;
		}
	}

	if(receiveCount != 0)
	{
		msg->addr = client->addr;
		msg->len = receiveCount;
		msg->flags = I2C_M_RD;
		msg->buf = pReceiveData;

		err = i2c_transfer(client->adapter, msg, 1);
		if(err<0)
		{
			BS300_ERR("[%s,%d]i2c read byte fail, addr = %x, err = %d\n"
						,__FUNCTION__,__LINE__,msg->addr,err);
			goto out_err;
		}
	}

	return BS300_SUCCESS;

out_err:

	return BS300_FAIL;
}

static unsigned int bs300_get_status(unsigned short *pStatusWord)
{
	unsigned char command[] = {CMD_GET_STATUS};
	unsigned char statusbytes[2] = {0};

	if(pStatusWord == NULL)
	{	
		BS300_ERR("[%s,%d] fail with null pointer\n",__FUNCTION__,__LINE__);
		return BS300_FAIL;
	}
	/*read status info*/ 
	if(bs300_send_receive(1,2, command,statusbytes))
	{						
		if( ((statusbytes[0] & 0x80) == 0x80) && ((statusbytes[1] & 0x80) == 0x00) ) 
		{			
			*pStatusWord = (unsigned short)(statusbytes[0] << 8) | statusbytes[1];
			return BS300_SUCCESS;
		}
	}
	return BS300_FAIL;
}

static unsigned int bs300_connect(void)
{
	unsigned short statusWord = 0;
	unsigned int i = 0;
	const unsigned int maxRetryCount = 400;
	unsigned char stop_command[] = {CMD_STOP_CORE};
       unsigned char Cmd_Reset_Monitor[] = {0x43 /* 'C' */};
	unsigned char Cmd_reset_lp[] = {CMD_WRITE_SPECIAL_REGISTERS/* '2' */
										, SPECIAL_REGISTER_LP/*0x0c*/, 0x00, 0x00};
	unsigned char Cmd_reset_sr[] = {CMD_WRITE_NORMAL_REGISTERS/* 'F' */
										, NORMAL_REGISTER_SR/*0x32*/, 0x00, 0x00, 0x00};

	for(i=0;i<maxRetryCount;i++)
	{
		if((bs300_get_status(&statusWord) )&& 
			((statusWord & STATUS_SECURITY_MODE) == STATUS_SECURITY_UNRESTRICTED))
		{
			break;
		}
		mdelay(15);	/*sleep 15 ms*/ 
	}
	/*max retry get_status fail*/
	if(i == maxRetryCount)		
	{
		BS300_ERR("[%s,%d] maximum retry exceeds\n",__FUNCTION__,__LINE__);
		return BS300_FAIL;
	}
	/*Stop DSP*/
	if(!bs300_send_receive(1,0,stop_command,NULL))	
	{
		BS300_ERR("[%s,%d] stop DSP failed\n",__FUNCTION__,__LINE__);
		return BS300_FAIL;
	}

	if(!bs300_send_receive(1,0,Cmd_Reset_Monitor,NULL))	
	{
		BS300_ERR("[%s,%d] reset monitor failed\n",__FUNCTION__,__LINE__);
		return BS300_FAIL;
	}	

	if(!bs300_send_receive(sizeof(Cmd_reset_lp), 0, Cmd_reset_lp, NULL))	
	{
		BS300_ERR("[%s,%d] reset lp failed\n",__FUNCTION__,__LINE__);
		return BS300_FAIL;
	}	

	if(!bs300_send_receive(sizeof(Cmd_reset_sr), 0, Cmd_reset_sr, NULL))	
	{
		BS300_ERR("[%s,%d] reset sr failed\n",__FUNCTION__,__LINE__);
		return BS300_FAIL;
	}	
	
	return BS300_SUCCESS;
}

static unsigned int bs300_download(void)
{
	unsigned int i = 0;
	unsigned char crcBuffer[2] = {0};
	unsigned short receivedCrc = 0;
	unsigned char crc_command[] = {CMD_READ_AND_RESET_CRC};	

	for( i=0; i < DOWNLOAD_BLOCK_COUNT; i ++ )
	{
		/*set crc to 0xFFFF before data transfer*/
		if(!bs300_send_receive(1, 0, crc_command, NULL))	
		{
			BS300_ERR("[%s,%d] send CRC CMD failed\n",__FUNCTION__,__LINE__);
			return BS300_FAIL;
		}
		/*write data transfer*/
		if(!bs300_send_receive(downloadBlocks[i].byteCount, 0,downloadBlocks[i].formattedData, NULL))	
		{
			BS300_ERR("[%s,%d] send DOWNLOAD BLOCKS failed\n",__FUNCTION__,__LINE__);
			return BS300_FAIL;
		}
	  	/*read crc after data transfer*/
		if(!bs300_send_receive(1, 2, crc_command, crcBuffer))
		{
			BS300_ERR("[%s,%d] send & read CRC CMD failed\n",__FUNCTION__,__LINE__);
			return BS300_FAIL;
		}
							
		receivedCrc = (unsigned short)((crcBuffer[0] << 8)|crcBuffer[1]);
		/*check crc*/
		if( downloadBlocks[i].crc != receivedCrc ) 
		{
			BS300_ERR("[%s,%d] crc not match i= %d, downloadBlocks[i].crc = %x, receivedCrc = %x\n"
							,__FUNCTION__,__LINE__,i,downloadBlocks[i].crc,receivedCrc);
			return BS300_FAIL;
		}	
		
		
	}

	return BS300_SUCCESS;
}

static unsigned int bs300_run(void)
{
	unsigned char reset_lp_command[] ={CMD_WRITE_SPECIAL_REGISTERS
											, SPECIAL_REGISTER_LP, 0x00, 0x00};
	unsigned char reset_sr_command[] = {CMD_WRITE_NORMAL_REGISTERS
											, NORMAL_REGISTER_SR, 0x00, 0x00, 0x00};
	unsigned char set_pc_command[] = {CMD_EXECUTE_INSTRUCTION
											, 0x3B, 0x20, 0x10, 0x00 /* start at 0x1000 */};
	unsigned char go_command[] = {CMD_START_CORE};

	/*write special register(LP)*/
	if(!bs300_send_receive(sizeof(reset_lp_command), 0, reset_lp_command, NULL))	
	{
		BS300_ERR("[%s,%d] reset lp CMD failed\n",__FUNCTION__,__LINE__);
		return BS300_FAIL;
	}
	/*write normal register(SR)*/
	if(!bs300_send_receive(sizeof(reset_sr_command), 0, reset_sr_command, NULL))	
	{
		BS300_ERR("[%s,%d] reset sr CMD failed\n",__FUNCTION__,__LINE__);
		return BS300_FAIL;
	}
	/*write execute*/
	if( !bs300_send_receive( sizeof(set_pc_command), 0, set_pc_command, NULL ) )  
	{
		BS300_ERR("[%s,%d] set pc CMD failed\n",__FUNCTION__,__LINE__);
		return BS300_FAIL;
	}
	/*start DSP core*/
	if( !bs300_send_receive( sizeof(go_command), 0, go_command, NULL ) )		
	{
		BS300_ERR("[%s,%d] go CMD failed\n",__FUNCTION__,__LINE__);
		return BS300_FAIL;
	}	

	return BS300_SUCCESS;
}

/**
 * I2C kernel driver module
 */
static int bs300_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	int retval = 0;
	unsigned int uiRet=0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		BS300_ERR("[%s,%d]: need I2C_FUNC_I2C\n",__FUNCTION__,__LINE__);
		return -ENODEV;
	}

	bs300_i2c_client = client;

	uiRet = bs300_connect();
	if(!uiRet)
	{
		BS300_ERR("[%s,%d]: bs300_connect return fail\n",__FUNCTION__,__LINE__);
		retval = -EFAULT;
		goto bs300_failed_0;
	}

	uiRet = bs300_download();
	if(!uiRet)
	{
		BS300_ERR("[%s,%d]: bs300_download return fail\n",__FUNCTION__,__LINE__);
		retval = -EFAULT;
		goto bs300_failed_0;
	}

	uiRet = bs300_run();
	if(!uiRet)
	{
		BS300_ERR("[%s,%d]: bs300_run return fail\n",__FUNCTION__,__LINE__);
		retval = -EFAULT;
		goto bs300_failed_0;
	}
	BS300_DBG("[%s,%d]: bs300 Connect, Download, Run SUCCESS\n",__FUNCTION__,__LINE__);

	return 0;

bs300_failed_0:

	return retval;
}

static int bs300_remove(struct i2c_client *client)
{
	bs300_i2c_client = NULL;
	return 0;
}


static const struct i2c_device_id bs300_id[] = {
	{"bs300-audio",0},
	{},
};

static struct i2c_driver bs300_driver = {
	.driver = {
		.name = "bs300-audio",
	},
	.probe = bs300_probe,
	.remove = bs300_remove,
	.id_table = bs300_id,
};

static int __init bs300_init(void)
{
	int ret;

	ret = i2c_add_driver(&bs300_driver);
	if (ret)
		BS300_ERR("Unable to register BS300 driver\n");

	return ret;
}

late_initcall(bs300_init);

static void __exit bs300_exit(void)
{
	i2c_del_driver(&bs300_driver);
}
module_exit(bs300_exit);

MODULE_AUTHOR("HUAWEI Tech. Co., Ltd. Qiu Hanning");
MODULE_DESCRIPTION("Belasigna BS300 (MIC noise reducer) driver");
MODULE_LICENSE("GPL");
