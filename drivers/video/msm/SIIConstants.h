/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/

// Generic Constants
//==================
#define FALSE                   0
#define TRUE                    1

#define OFF                     0
#define ON                      1

#define LOW                     0
#define HIGH                    1

#define AFTER_INIT              1
#define MODE_CHANGE             0

// Generic Masks
//==============
#define LOW_BYTE                0x00FF

#define LOW_NIBBLE              0x0F
#define HI_NIBBLE               0xF0

#define MSBIT                   0x80
#define LSBIT                   0x01

#define BIT_0                   0x01
#define BIT_1                   0x02
#define BIT_2                   0x04
#define BIT_3                   0x08
#define BIT_4                   0x10
#define BIT_5                   0x20
#define BIT_6                   0x40
#define BIT_7                   0x80

#define TWO_LSBITS              0x03
#define THREE_LSBITS            0x07
#define FOUR_LSBITS             0x0F
#define FIVE_LSBITS             0x1F
#define SEVEN_LSBITS            0x7F
#define TWO_MSBITS              0xC0
#define EIGHT_BITS              0xFF
#define BYTE_SIZE               0x08
#define BITS_1_0                0x03
#define BITS_2_1                0x06
#define BITS_2_1_0              0x07
#define BITS_3_2                0x0C
#define BITS_4_3_2              0x1C  
#define BITS_5_4                0x30
#define BITS_5_4_3              0x38
#define BITS_6_5                0x60
#define BITS_6_5_4              0x70
#define BITS_7_6                0xC0


// Interrupt Masks
//================
#define HOT_PLUG_EVENT          0x01
#define RX_SENSE_EVENT          0x02
#define HOT_PLUG_STATE          0x04
#define RX_SENSE_STATE          0x08

#define AUDIO_ERROR_EVENT       0x10
#define SECURITY_CHANGE_EVENT   0x20
#define V_READY_EVENT           0x40
#define HDCP_CHANGE_EVENT       0x80

#define NON_MASKABLE_INT        0xFF

// TPI Control Masks
// =================

#define CS_HDMI_RGB         0x00
#define CS_DVI_RGB          0x03
/// //#define OUTPT_MODE_DVI        0x00
#define OUTPT_MODE_HDMI     0x01

#define ENABLE_AND_REPEAT   0xC0
#define EN_AND_RPT_MPEG     0xC3

// Pixel Repetition Masks
//=======================
#define BIT_BUS_24          0x20
#define BIT_EDGE_RISE       0x10

//Audio Maps
//==========
#define BIT_AUDIO_MUTE      0x10

// Input/Output Format Masks
//==========================
#define BITS_IN_RGB         0x00
#define BITS_IN_YCBCR444    0x01
#define BITS_IN_YCBCR422    0x02

#define BITS_IN_AUTO_RANGE  0x00
#define BITS_IN_FULL_RANGE  0x04
#define BITS_IN_LTD_RANGE   0x08

#define BIT_EN_DITHER_10_8  0x40
#define BIT_EXTENDED_MODE   0x80

#define BITS_OUT_RGB        0x00
#define BITS_OUT_YCBCR444   0x01
#define BITS_OUT_YCBCR422   0x02

#define BITS_OUT_AUTO_RANGE 0x00
#define BITS_OUT_FULL_RANGE 0x04
#define BITS_OUT_LTD_RANGE  0x08

#define BIT_BT_709          0x10


// DE Generator Masks
//===================
#define BIT_EN_DE_GEN       0x40

// Embedded Sync Masks
//====================
#define BIT_EN_SYNC_EXTRACT 0x40

// Audio Modes
//============
#define AUD_PASS_BASIC      0x00
#define AUD_PASS_ALL        0x01
#define AUD_DOWN_SAMPLE     0x02
#define AUD_DO_NOT_CHECK    0x03

#define REFER_TO_STREAM_HDR     0x00
#define TWO_CHANNELS            0x00
#define EIGHT_CHANNELS          0x01
#define AUD_IF_SPDIF            0x40
#define AUD_IF_I2S              0x80
#define AUD_IF_DSD              0xC0
#define AUD_IF_HBR              0x04

#define TWO_CHANNEL_LAYOUT      0x00
#define EIGHT_CHANNEL_LAYOUT    0x20


// Configuration File Constants
//=============================
#define CONFIG_DATA_LEN     0x10
#define IDX_VIDEO_MODE      0x00

// I2C Slave Addresses
//====================
#define TPI_BASE_ADDR       (0x72>>1)
#define TX_SLAVE_ADDR       (0x72>>1)
#define CBUS_SLAVE_ADDR     (0xC8>>1)
#define HDCP_SLAVE_ADDR     (0x74>>1)
#define EDID_ROM_ADDR       (0xA0>>1)
#define EDID_SEG_ADDR       (0x60>>1)

// DDC Bus Addresses
//==================
#define DDC_BSTATUS_ADDR_L  0x41
#define DDC_BSTATUS_ADDR_H  0x42
#define DDC_KSV_FIFO_ADDR   0x43
#define KSV_ARRAY_SIZE      128

// DDC Bus Bit Masks
//==================
#define BIT_DDC_HDMI        0x80
#define BIT_DDC_REPEATER    0x40
#define BIT_DDC_FIFO_RDY    0x20
#define DEVICE_COUNT_MASK   0x7F

// KSV Buffer Size
//================
#define DEVICE_COUNT         128    // May be tweaked as needed

// InfoFrames
//===========
#define SIZE_AVI_INFOFRAME      0x0E     // including checksum byte
#define BITS_OUT_FORMAT         0x60    // Y1Y0 field

#define _4_To_3                 0x10    // Aspect ratio - 4:3  in InfoFrame DByte 1
#define _16_To_9                0x20    // Aspect ratio - 16:9 in InfoFrame DByte 1
#define SAME_AS_AR              0x08    // R3R2R1R0 - in AVI InfoFrame DByte 2

#define BT_601                  0x40
#define BT_709                  0x80

#define SIZE_AUDIO_INFOFRAME    0x0F

#define EN_AUDIO_INFOFRAMES         0xC2
#define TYPE_AUDIO_INFOFRAMES       0x84
#define AUDIO_INFOFRAMES_VERSION    0x01
#define AUDIO_INFOFRAMES_LENGTH     0x0A

#define MPEG_INFOFRAME_CODE         0x03

#define TYPE_GBD_INFOFRAME          0x0A

#define NEXT_FIELD                  0x80
#define GBD_PROFILE                 0x00
#define AFFECTED_GAMUT_SEQ_NUM      0x01

#define ONLY_PACKET                 0x30
#define CURRENT_GAMUT_SEQ_NUM       0x01


// FPLL Multipliers:
//==================

#define X1                      0x01

// 3D Constants
//=============

#define _3D_STRUC_PRESENT               0x02

// 3D_Stucture Constants
//=======================
#define FRAME_PACKING                   0x00
#define FIELD_ALTERNATIVE               0x01
#define LINE_ALTERNATIVE                0x02
#define SIDE_BY_SIDE_FULL               0x03
#define L_PLUS_DEPTH                    0x04
#define L_PLUS_DEPTH_PLUS_GRAPHICS      0x05
#define SIDE_BY_SIDE_HALF               0x08

// 3D_Ext_Data Constants
//======================
#define HORIZ_ODD_LEFT_ODD_RIGHT        0x00
#define HORIZ_ODD_LEFT_EVEN_RIGHT       0x01
#define HORIZ_EVEN_LEFT_ODD_RIGHT       0x02
#define HORIZ_EVEN_LEFT_EVEN_RIGHT      0x03

#define QUINCUNX_ODD_LEFT_EVEN_RIGHT    0x04
#define QUINCUNX_ODD_LEFT_ODD_RIGHT     0x05
#define QUINCUNX_EVEN_LEFT_ODD_RIGHT    0x06
#define QUINCUNX_EVEN_LEFT_EVEN_RIGHT   0x07

#define NO_3D_SUPPORT                   0x0F
