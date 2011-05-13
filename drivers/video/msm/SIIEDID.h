/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/
#include <linux/types.h>

#ifdef DEV_SUPPORT_EDID

#define MAX_V_DESCRIPTORS           20
#define MAX_A_DESCRIPTORS           10
#define MAX_SPEAKER_CONFIGURATIONS   4
#define AUDIO_DESCR_SIZE             3

typedef struct 
{                               // for storing EDID parsed data
    byte VideoDescriptor[MAX_V_DESCRIPTORS];    // maximum number of video descriptors
    byte AudioDescriptor[MAX_A_DESCRIPTORS][3]; // maximum number of audio descriptors
    byte SpkrAlloc[MAX_SPEAKER_CONFIGURATIONS]; // maximum number of speaker configurations
    bool UnderScan;                             // "1" if DTV monitor underscans IT video formats by default
    bool BasicAudio;                            // Sink supports Basic Audio
    bool YCbCr_4_4_4;                           // Sink supports YCbCr 4:4:4
    bool YCbCr_4_2_2;                           // Sink supports YCbCr 4:2:2
    bool HDMI_Sink;                             // "1" if HDMI signature found
    byte CEC_A_B;                               // CEC Physical address. See HDMI 1.3 Table 8-6
    byte CEC_C_D;
    byte ColorimetrySupportFlags;               // IEC 61966-2-4 colorimetry support: 1 - xvYCC601; 2 - xvYCC709 
    byte MetadataProfile;
    bool _3D_Supported;  
} Type_EDID_Descriptors;

enum EDID_ErrorCodes 
{
    EDID_OK,
    EDID_INCORRECT_HEADER,
    EDID_CHECKSUM_ERROR,
    EDID_NO_861_EXTENSIONS,
    EDID_SHORT_DESCRIPTORS_OK,
    EDID_LONG_DESCRIPTORS_OK,
    EDID_EXT_TAG_ERROR,
    EDID_REV_ADDR_ERROR,
    EDID_V_DESCR_OVERFLOW,
    EDID_UNKNOWN_TAG_CODE,
    EDID_NO_DETAILED_DESCRIPTORS,
    EDID_DDC_BUS_REQ_FAILURE,
    EDID_DDC_BUS_RELEASE_FAILURE
};

byte DoEdidRead (void);

#define ReadBlockEDID(a,b,c)    I2C_ReadBlock(EDID_ROM_ADDR, a, c, b)

#define IsHDMI_Sink()           (EDID_Data.HDMI_Sink)
#define IsCEC_DEVICE()          (((EDID_Data.CEC_A_B != 0xFF) && (EDID_Data.CEC_C_D != 0xFF)) ? TRUE : FALSE)

extern Type_EDID_Descriptors EDID_Data;     // holds parsed EDID data needed by the FW
extern byte EDID_TempData[];

extern bool edidDataValid;

#else

#define IsHDMI_Sink()           (TRUE)
#define IsCEC_DEVICE()          (FALSE)

#endif
