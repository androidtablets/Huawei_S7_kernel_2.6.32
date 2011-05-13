/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/

// API Command Lengths
//=======================

// These are here because thet are referenced in av_config.c

#define VIDEO_SETUP_CMD_LEN     0x15
#define AUDIO_SETUP_CMD_LEN 	0x07
#define I2S_MAPPING_CMD_LEN     0x04
#define CFG_I2S_INPUT_CMD_LEN   0x01
#define I2S_STREAM_HDR_CMD_LEN  0x05
#define API_VERSION_CMD_LEN		0x03
#define TPI_FW_VERSION_CMD_LEN	(int)sizeof(TPI_FW_VERSION)
#define EHDMI_MODE_CMD_LEN		0x01 

