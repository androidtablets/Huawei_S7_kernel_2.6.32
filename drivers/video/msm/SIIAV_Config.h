/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/

enum AV_ConfigErrorCodes 
{
    DE_CANNOT_BE_SET_WITH_EMBEDDED_SYNC,
    V_MODE_NOT_SUPPORTED,
    SET_EMBEDDED_SYC_FAILURE,
    I2S_MAPPING_SUCCESSFUL,
    I2S_INPUT_CONFIG_SUCCESSFUL,
    I2S_HEADER_SET_SUCCESSFUL,
    EHDMI_ARC_SINGLE_SET_SUCCESSFUL,
    EHDMI_ARC_COMMON_SET_SUCCESSFUL,
    EHDMI_HEC_SET_SUCCESSFUL,
    EHDMI_ARC_CM_WITH_HEC_SET_SUCCESSFUL,
    AUD_MODE_NOT_SUPPORTED,
    I2S_NOT_SET,
    DE_SET_OK,
    VIDEO_MODE_SET_OK,
    AUDIO_MODE_SET_OK,
    GBD_SET_SUCCESSFULLY,
    DE_CANNOT_BE_SET_WITH_3D_MODE,
};

// Time Constants Used in AV_Config.c only
//========================================
#define T_RES_CHANGE_DELAY      128         // delay between turning TMDS bus off and changing output resolution

// Deep Color Constants
//=====================
#define DC_48                                   3

// Prototype Declarations
//=======================
void SetBasicAudio(void);
bool InitVideo(byte, byte, bool, byte);
void SetFormat(byte *Data);
bool SetEmbeddedSync(byte);
void EnableEmbeddedSync(void);
byte SetDE(byte);

byte ChangeVideoMode(API_Cmd);
byte MapI2S(API_Cmd);
byte ConfigI2SInput(API_Cmd);
byte SetAudioMode(API_Cmd);

byte SetI2S_StreamHeader(API_Cmd);
byte SetEHDMI_ARC_sm(API_Cmd);
byte SetEHDMI_ARC_cm(API_Cmd);
byte SetEHDMI_HEC(API_Cmd);
byte SetEHDMI_ARCcm_with_HEC(API_Cmd);

bool IsVideoModeSupported(byte);
byte IsAudioModeSupported(byte);
bool SetAVI_InfoFrames(API_Cmd);
bool SetAudioInfoFrames(byte, byte, byte, byte, byte);

#ifdef F_9022A_9334
byte SetGBD_InfoFrame(void);
#endif

