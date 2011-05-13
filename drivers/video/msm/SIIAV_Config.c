/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/

#include <linux/types.h>
#include <mach/gpio.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include "SIIdefs.h"
#include "SIITypeDefs.h"
#include "SIITPI_Regs.h"
#include "SIIConstants.h"
#include "Externals.h"
#include "SIIMacros.h"
#include "SIITPI_Access.h"
#include "SIITPI.h"
#include "SIIAV_Config.h"
#include "i2c_master_sw.h"
#include "SIIVideoModeTable.h"
#include "SerialPort.h"
#include "SIIHDCP.h"

#include "SIIEDID.h"

// VSIF Constants
//===============
#define VSIF_TYPE                       0x81
#define VSIF_VERSION                    0x01
#define VSIF_LEN                        0x07

byte AudioI2SConfig = 0x90;

byte I2SSDSet[4] =
{
    0x80,
    0x91,
    0xA2,
    0xB3
};

byte I2SStreamHeader[5] =
{
    0x00,
    0x00,
    0x00,
    0x02,
    0x02
};
byte VideoMode[9]=
{
    0x02,
    0x60,
    0x00,
    0x00,
    0x00,
    0x00,
    0x01,
    0x00,
    0x0F
};

byte AudioMode[6]=
{
    0x80,
    0x00,
    0x00,
    0x00,
    0x20,
    0x01
};
extern bool tmdsPoweredUp;
void TxPowerState(byte powerState);

#ifdef DEV_SUPPORT_EDID
void Set_VSIF(byte, byte);
#endif

#ifdef RX_ONBOARD
extern VideoFormat_t video_information;
extern AVIInfoFormat_t avi_information;
#endif


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  InitVideo()
//
// PURPOSE       :  Set the 9022/4 to the video mode determined by GetVideoMode()
//
// INPUT PARAMS  :  Index of video mode to set; Flag that distinguishes between
//                  calling this function after power up and after input
//                  resolution change
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  VModesTable, VideoCommandImage
//
// RETURNS       :  TRUE
//
//////////////////////////////////////////////////////////////////////////////
bool InitVideo(byte Mode, byte TclkSel, bool Init, byte _3D_Struct)
{
    byte V_Mode;
#ifdef DEEP_COLOR
    byte temp;
#endif
    //word W_Data[4];
    byte B_Data[8];
    
#ifdef DEV_EMBEDDED
    byte EMB_Status;
#endif
#ifdef USE_DE_GENERATOR
    byte DE_Status;
#endif
    
#ifndef DEV_INDEXED_PLL
    byte Pattern;
#endif
    TPI_TRACE_PRINT((">>InitVideo()\n"));
    
    V_Mode = ConvertVIC_To_VM_Index(Mode, _3D_Struct & FOUR_LSBITS);  // convert 861-D VIC into VModesTable[] index.
    
#ifdef DEV_INDEXED_PLL
    SetPLL(TclkSel);
#else
    Pattern = (TclkSel << 6) & TWO_MSBITS;              // Use TPI 0x08[7:6] for 9022A/24A video clock multiplier
    ReadSetWriteTPI(TPI_PIX_REPETITION, Pattern);
#endif
    
    // Take values from VModesTable[]:
    //W_Data[0] = VModesTable[V_Mode].PixClk;             // write Pixel clock to TPI registers 0x00, 0x01
    //W_Data[1] = VModesTable[V_Mode].Tag.VFreq;          // write Vertical Frequency to TPI registers 0x02, 0x03
    //W_Data[2] = VModesTable[V_Mode].Tag.Total.Pixels;   // write total number of pixels to TPI registers 0x04, 0x05
    //W_Data[3] = VModesTable[V_Mode].Tag.Total.Lines;    // write total number of lines to TPI registers 0x06, 0x07
    
    B_Data[0] = VModesTable[V_Mode].PixClk & 0x00FF;             // write Pixel clock to TPI registers 0x00, 0x01
    B_Data[1] = (VModesTable[V_Mode].PixClk >> 8) & 0xFF;
    
    B_Data[2] = VModesTable[V_Mode].Tag.VFreq & 0x00FF;          // write Vertical Frequency to TPI registers 0x02, 0x03
    B_Data[3] = (VModesTable[V_Mode].Tag.VFreq >> 8) & 0xFF;
    
    B_Data[4] = VModesTable[V_Mode].Tag.Total.Pixels & 0x00FF;   // write total number of pixels to TPI registers 0x04, 0x05
    B_Data[5] = (VModesTable[V_Mode].Tag.Total.Pixels >> 8) & 0xFF;
    
    B_Data[6] = VModesTable[V_Mode].Tag.Total.Lines & 0x00FF;    // write total number of lines to TPI registers 0x06, 0x07
    B_Data[7] = (VModesTable[V_Mode].Tag.Total.Lines >> 8) & 0xFF;
    
    //Result=TxTPI_WriteBlock(TPI_PIX_CLK_LSB, 8, (byte *)(&W_Data[0]));  // Write TPI Mode data in one burst (8 bytes);
    WriteBlockTPI(TPI_PIX_CLK_LSB, 8, B_Data);  // Write TPI Mode data in one burst (8 bytes);
    
#ifdef DEV_EMBEDDED
    EMB_Status = SetEmbeddedSync(Mode);
    EnableEmbeddedSync();
#endif
    
#ifdef USE_DE_GENERATOR
    ReadClearWriteTPI(TPI_SYNC_GEN_CTRL, MSBIT);       // set 0x60[7] = 0 for External Sync
    DE_Status = SetDE(Mode);                              // Call SetDE() with Video Mode as a parameter
#endif
    
#ifdef DEEP_COLOR
    switch (video_information.color_depth)
    {
    case 0: temp = 0x00; break;
    case 1: temp = 0x80; break;
    case 2: temp = 0xC0; break;
    case 3: temp = 0x40; break;
    default: temp = 0x00; break;
    }
    B_Data[0] = ReadByteTPI(0x09);
    B_Data[0] = ((B_Data[0] & 0x3F) | temp);
#endif
#ifdef DEV_EMBEDDED
    B_Data[0] = ((B_Data[0] & 0xFC) | 0x02);
#endif
    
    B_Data[1] = (BITS_OUT_RGB | BITS_OUT_AUTO_RANGE) & ~BIT_BT_709;
#ifdef DEEP_COLOR
    B_Data[1] = ((B_Data[1] & 0x3F) | temp);
#endif
    
#ifdef DEV_SUPPORT_EDID
    if (dvi_mode == TRUE)
    {
        B_Data[1] = ((B_Data[1] & 0xFC) | 0x03);
    }
    else
    {
        // Set YCbCr color space depending on EDID
        if (EDID_Data.YCbCr_4_4_4)
        {
            B_Data[1] = ((B_Data[1] & 0xFC) | 0x01);
        }
        else
        {
            if (EDID_Data.YCbCr_4_2_2)
            {
                B_Data[1] = ((B_Data[1] & 0xFC) | 0x02);
            }
        }
    }
#else
    B_Data[1] = 0x00;
#endif
    
    SetFormat(B_Data);
    
    if (Init)
    {
        B_Data[0] = (VModesTable[V_Mode].PixRep) & LOW_BYTE;        // Set pixel replication field of 0x08
        B_Data[0] |= BIT_BUS_24;                                    // Set 24 bit bus
        
#ifndef DEV_INDEXED_PLL
        B_Data[0] |= (TclkSel << 6) & TWO_MSBITS;
#endif
        
#ifdef CLOCK_EDGE_FALLING
        B_Data[0] &= ~BIT_EDGE_RISE;                                // Set to falling edge
#endif
        
#ifdef CLOCK_EDGE_RISING
        B_Data[0] |= BIT_EDGE_RISE;                                                                     // Set to rising edge
#endif
        
        WriteByteTPI(TPI_PIX_REPETITION, B_Data[0]);       // 0x08
        
#ifdef DEV_EMBEDDED
        EMB_Status = SetEmbeddedSync(Mode);
        EnableEmbeddedSync();
#endif
        // default to full range RGB at the input:
#ifndef USE_DE_GENERATOR
        B_Data[0] = (((BITS_IN_RGB | BITS_IN_AUTO_RANGE) & ~BIT_EN_DITHER_10_8) & ~BIT_EXTENDED_MODE);  // 0x09
#else
        B_Data[0] = (((BITS_IN_YCBCR422 | BITS_IN_AUTO_RANGE) & ~BIT_EN_DITHER_10_8) & ~BIT_EXTENDED_MODE);  // 0x09
#endif
        
#ifdef DEEP_COLOR
        switch (video_information.color_depth)
        {
        case 0: temp = 0x00; break;
        case 1: temp = 0x80; break;
        case 2: temp = 0xC0; break;
        case 3: temp = 0x40; break;
        default: temp = 0x00; break;
        }
        B_Data[0] = ((B_Data[0] & 0x3F) | temp);
#endif
#ifdef DEV_EMBEDDED
        B_Data[0] = ((B_Data[0] & 0xFC) | 0x02);
#endif
        B_Data[1] = (BITS_OUT_RGB | BITS_OUT_AUTO_RANGE) & ~BIT_BT_709;
#ifdef DEEP_COLOR
        B_Data[1] = ((B_Data[1] & 0x3F) | temp);
#endif
        
#ifdef DEV_SUPPORT_EDID
        if (dvi_mode == TRUE)
        {
            B_Data[1] = ((B_Data[1] & 0xFC) | 0x03);
        }
        else
        {
            // Set YCbCr color space depending on EDID
            if (EDID_Data.YCbCr_4_4_4)
            {
                B_Data[1] = ((B_Data[1] & 0xFC) | 0x01);
            }
            else
            {
                if (EDID_Data.YCbCr_4_2_2)
                {
                    B_Data[1] = ((B_Data[1] & 0xFC) | 0x02);
                }
            }
        }
#else
        B_Data[1] = 0x00;
#endif
        
        SetFormat(B_Data);
        
        ReadClearWriteTPI(TPI_SYNC_GEN_CTRL, BIT_2); // // Number HSync pulses from VSync active edge to Video Data Period should be 20 (VS_TO_VIDEO)
    }
    
#ifdef SOURCE_TERMINATION_ON
    V_Mode = ReadIndexedRegister(INDEXED_PAGE_1, TMDS_CONT_REG);
    V_Mode = (V_Mode & 0x3F) | 0x25;
    WriteIndexedRegister(INDEXED_PAGE_1, TMDS_CONT_REG, V_Mode);
#endif
    
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   SetFormat(byte * Data)
//
// PURPOSE      :   
//
// INPUT PARAMS :   
//
// OUTPUT PARAMS:   
//
// GLOBALS USED :   
//
// RETURNS      :   
//
//////////////////////////////////////////////////////////////////////////////
void SetFormat(byte *Data)
{
    if (dvi_mode)
    {
        ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, OUTPUT_MODE_MASK, OUTPUT_MODE_HDMI); // Set HDMI mode to allow color space conversion
    }
    
    WriteBlockTPI(TPI_INPUT_FORMAT_REG, 2, Data);   // Program TPI AVI Input and Output Format
    WriteByteTPI(TPI_END_RIGHT_BAR_MSB, 0x00);      // Set last byte of TPI AVI InfoFrame for TPI AVI I/O Format to take effect
    
    if (dvi_mode == TRUE) 
    {
        ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, OUTPUT_MODE_MASK, OUTPUT_MODE_DVI);
    }
    
#ifdef DEV_EMBEDDED
    EnableEmbeddedSync();                           // Last byte of TPI AVI InfoFrame resets Embedded Sync Extraction
#endif
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   SetEmbeddedSync(V_Mode)
//
// PURPOSE      :   Set the 9022/4 registers to extract embedded sync.
//
// INPUT PARAMS :   Index of video mode to set
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   VModesTable[]
//
// RETURNS      :   TRUE
//
//////////////////////////////////////////////////////////////////////////////
bool SetEmbeddedSync(byte V_Mode)
{
    word H_Bit_2_H_Sync;
    word Field2Offset;
    word H_SyncWidth;
    
    byte V_Bit_2_V_Sync;
    byte V_SyncWidth;
    byte B_Data[8];
    
    TPI_TRACE_PRINT((">>SetEmbeddedSync()\n"));
    
    ReadModifyWriteIndexedRegister(INDEXED_PAGE_0, 0x0A, 0x01, 0x01);
    
    ReadClearWriteTPI(TPI_SYNC_GEN_CTRL, MSBIT);     // set 0x60[7] = 0 for DE mode
    WriteByteTPI(0x63, 0x30);
    ReadSetWriteTPI(TPI_SYNC_GEN_CTRL, MSBIT);       // set 0x60[7] = 1 for Embedded Sync
    
    V_Mode = ConvertVIC_To_VM_Index(V_Mode, NO_3D_SUPPORT);                // convert 861-D VIC into VModesTable[] index. No 3D with embedded sync
    
    H_Bit_2_H_Sync = VModesTable[V_Mode]._656.HBit2HSync;
    Field2Offset = VModesTable[V_Mode]._656.Field2Offset;
    H_SyncWidth = VModesTable[V_Mode]._656.HLength;
    V_Bit_2_V_Sync = VModesTable[V_Mode]._656.VBit2VSync;
    V_SyncWidth = VModesTable[V_Mode]._656.VLength;
    
    B_Data[0] = H_Bit_2_H_Sync & LOW_BYTE;                  // Setup HBIT_TO_HSYNC 8 LSBits (0x62)
    
    B_Data[1] = (H_Bit_2_H_Sync >> 8) & TWO_LSBITS;         // HBIT_TO_HSYNC 2 MSBits
    //B_Data[1] |= BIT_EN_SYNC_EXTRACT;                     // and Enable Embedded Sync to 0x63
    
    EmbeddedSynPATCH = B_Data[1];
    
    B_Data[2] = Field2Offset & LOW_BYTE;                    // 8 LSBits of "Field2 Offset" to 0x64
    B_Data[3] = (Field2Offset >> 8) & LOW_NIBBLE;           // 2 MSBits of "Field2 Offset" to 0x65
    
    B_Data[4] = H_SyncWidth & LOW_BYTE;
    B_Data[5] = (H_SyncWidth >> 8) & TWO_LSBITS;                    // HWIDTH to 0x66, 0x67
    B_Data[6] = V_Bit_2_V_Sync;                                     // VBIT_TO_VSYNC to 0x68
    B_Data[7] = V_SyncWidth;                                        // VWIDTH to 0x69
    
    WriteBlockTPI(TPI_HBIT_TO_HSYNC_7_0, 8, &B_Data[0]);
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   EnableEmbeddedSync
//
// PURPOSE      :
//
// INPUT PARAMS :
//
// OUTPUT PARAMS:
//
// GLOBALS USED :
//
// RETURNS      :
//
//////////////////////////////////////////////////////////////////////////////
void EnableEmbeddedSync()
{
    TPI_TRACE_PRINT((">>EnableEmbeddedSync()\n"));
    
    ReadClearWriteTPI(TPI_SYNC_GEN_CTRL, MSBIT);     // set 0x60[7] = 0 for DE mode
    WriteByteTPI(0x63, 0x30);
    ReadSetWriteTPI(TPI_SYNC_GEN_CTRL, MSBIT);       // set 0x60[7] = 1 for Embedded Sync
    ReadSetWriteTPI(TPI_DE_CTRL, BIT_6);
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   SetDE(V_Mode)
//
// PURPOSE      :   Set the 9022/4 internal DE generator parameters
//
// INPUT PARAMS :   Index of video mode to set
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   DE_SET_OK
//
// NOTE         :   0x60[7] must be set to "0" for the follwing settings to
//                  take effect
//
//////////////////////////////////////////////////////////////////////////////
byte SetDE(byte V_Mode)
{
    byte RegValue;
    
    word H_StartPos;
    word V_StartPos;
    word Htotal;
    word Vtotal;
    word H_Res;
    word V_Res;
    
    byte Polarity;
    byte B_Data[12];
    
    TPI_TRACE_PRINT((">>SetDE()\n"));
    
    if (VModesTable[V_Mode]._3D_Struct != NO_3D_SUPPORT)
    {
        return DE_CANNOT_BE_SET_WITH_3D_MODE;
        TPI_TRACE_PRINT((">>SetDE() not allowed with 3D video format\n"));
        
    }
    // Make sure that External Sync method is set before enableing the DE Generator:
    RegValue = ReadByteTPI(TPI_SYNC_GEN_CTRL);
    //MessageIndex     if (!Result)
    //MessageIndex         EnqueueMessageIndex(TPI_READ_FAILURE);
    
    if (RegValue & BIT_7)
    {
        //MessageIndex         EnqueueMessageIndex(DE_CANNOT_BE_SET_WITH_EMBEDDED_SYNC);
        return DE_CANNOT_BE_SET_WITH_EMBEDDED_SYNC;
    }
    
    V_Mode = ConvertVIC_To_VM_Index(V_Mode, NO_3D_SUPPORT);                // convert 861-D VIC into VModesTable[] index. No 3D with internal DE
    
    H_StartPos = VModesTable[V_Mode].Pos.H;
    V_StartPos = VModesTable[V_Mode].Pos.V;
    
    Htotal = VModesTable[V_Mode].Tag.Total.Pixels;
    Vtotal = VModesTable[V_Mode].Tag.Total.Lines;
    
    Polarity = (~VModesTable[V_Mode].Tag.RefrTypeVHPol) & TWO_LSBITS;
    
    H_Res = VModesTable[V_Mode].Res.H;
    
    if ((VModesTable[V_Mode].Tag.RefrTypeVHPol & 0x04))
    {
        V_Res = (VModesTable[V_Mode].Res.V) >> 1;
    }
    else
    {
        V_Res = (VModesTable[V_Mode].Res.V);
    }
    
    B_Data[0] = H_StartPos & LOW_BYTE;              // 8 LSB of DE DLY in 0x62
    
    B_Data[1] = (H_StartPos >> 8) & TWO_LSBITS;     // 2 MSBits of DE DLY to 0x63
    B_Data[1] |= (Polarity << 4);                   // V and H polarity
    B_Data[1] |= BIT_EN_DE_GEN;                     // enable DE generator
    
    B_Data[2] = V_StartPos & SEVEN_LSBITS;      // DE_TOP in 0x64
    B_Data[3] = 0x00;                           // 0x65 is reserved
    B_Data[4] = H_Res & LOW_BYTE;               // 8 LSBits of DE_CNT in 0x66
    B_Data[5] = (H_Res >> 8) & LOW_NIBBLE;      // 4 MSBits of DE_CNT in 0x67
    B_Data[6] = V_Res & LOW_BYTE;               // 8 LSBits of DE_LIN in 0x68
    B_Data[7] = (V_Res >> 8) & THREE_LSBITS;    // 3 MSBits of DE_LIN in 0x69
    B_Data[8] = Htotal & LOW_BYTE;              // 8 LSBits of H_RES in 0x6A
    B_Data[9] = (Htotal >> 8) & LOW_NIBBLE;     // 4 MSBITS of H_RES in 0x6B
    B_Data[10] = Vtotal & LOW_BYTE;             // 8 LSBits of V_RES in 0x6C
    B_Data[11] = (Vtotal >> 8) & BITS_2_1_0;    // 3 MSBITS of V_RES in 0x6D
    
    WriteBlockTPI(TPI_DE_DLY, 12, &B_Data[0]);
    
    //MessageIndex     EnqueueMessageIndex(DE_SET_OK);                   // For GetStatus()
    return DE_SET_OK;                               // Write completed successfully
}



//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      : SetBasicAudio()
//
// PURPOSE       : Set the 9022/4 audio interface to basic audio.
//
// INPUT PARAMS  : None
//
// OUTPUT PARAMS : None
//
// GLOBALS USED  : None
//
// RETURNS       : void.
//
//////////////////////////////////////////////////////////////////////////////
void SetBasicAudio(void)
{
    
    TPI_TRACE_PRINT((">>SetBasicAudio()\n"));
    
#ifdef I2S_AUDIO
    WriteByteTPI(TPI_AUDIO_INTERFACE_REG,  AUD_IF_I2S);                             // 0x26
    WriteByteTPI(TPI_AUDIO_HANDLING, 0x08 | AUD_DO_NOT_CHECK);          // 0x25
#else
    WriteByteTPI(TPI_AUDIO_INTERFACE_REG, AUD_IF_SPDIF);                    // 0x26 = 0x40
    WriteByteTPI(TPI_AUDIO_HANDLING, AUD_PASS_BASIC);                   // 0x25 = 0x00
#endif
    
#ifndef F_9022A_9334
    SetChannelLayout(TWO_CHANNELS);             // Always 2 channesl in S/PDIF
#else
    ReadClearWriteTPI(TPI_AUDIO_INTERFACE_REG, BIT_5); // Use TPI 0x26[5] for 9022A/24A and 9334 channel layout
#endif
    
#ifdef I2S_AUDIO
    // I2S - Map channels - replace with call to API MAPI2S
    WriteByteTPI(TPI_I2S_EN, 0x80); // 0x1F
    //        WriteByteTPI(TPI_I2S_EN, 0x91);
    //        WriteByteTPI(TPI_I2S_EN, 0xA2);
    //        WriteByteTPI(TPI_I2S_EN, 0xB3);
    
    // I2S - Stream Header Settings - replace with call to API SetI2S_StreamHeader
    WriteByteTPI(TPI_I2S_CHST_0, 0x00); // 0x21
    WriteByteTPI(TPI_I2S_CHST_1, 0x00);
    WriteByteTPI(TPI_I2S_CHST_2, 0x00);
    WriteByteTPI(TPI_I2S_CHST_3, 0x02);
    WriteByteTPI(TPI_I2S_CHST_4, 0x02);
    
    // I2S - Input Configuration - replace with call to API ConfigI2SInput
    WriteByteTPI(TPI_I2S_IN_CFG, 0x10); // 0x20
#endif
    
    WriteByteTPI(TPI_AUDIO_SAMPLE_CTRL, TWO_CHANNELS);  // 0x27 = 0x01
    SetAudioInfoFrames(TWO_CHANNELS, 0x00, 0x00, 0x00, 0x00);
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  ChangeVideoMode()
//
// PURPOSE       :  Changes the 9022/4 video resolution following a command from
//                  System Control
//
// INPUT PARAMS  :  API_Cmd type structure with command parameters, sent from
//                  the system controller
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  RxCommand, F_IgnoreEDID
//
// RETURNS       :  Success message if video resolution changed successfully.
//                  Error Code if resolution change failed
//
// NOTE         :   Function calls InitVideo() with the 2nd parameter set to
//                  MODE_CHANGE (==0). That will initialized only the basic
//                  parameters (Pix. Clk; VFreq; H. # of pixels;...). Other
//                  parmeters will not be set by InitVideo(), and will be set
//                  by this function, based on the values passed in the
//                  API_Cmd typed parameter
//
//////////////////////////////////////////////////////////////////////////////
byte ChangeVideoMode(API_Cmd Command)
{
    byte Result;
    byte Status;
    
    TPI_TRACE_PRINT((">>ChangeVideoMode()\n"));
    
    vid_mode = Command.Arg[0];//zhy 2009 11 24
#ifdef DEV_SUPPORT_HDCP
    HDCP_Off();
#endif
    
    DisableTMDS();                  // turn off TMDS output
    msleep_interruptible(T_RES_CHANGE_DELAY);    // allow control InfoFrames to pass through to the sink device.
    
    
#if 0//ndef DEBUG_EDID // huangyq
    if (!F_IgnoreEDID)
    {
        Result = IsVideoModeSupported (vid_mode);
        if (!Result)
            return V_MODE_NOT_SUPPORTED;                // Sink does not support this video mode
    }
#endif
    // Do not change vid_mode via IForm
        //vid_mode = Command.Arg[0];
    
    InitVideo(vid_mode, ((Command.Arg[1] >> 6) & TWO_LSBITS), MODE_CHANGE, Command.Arg[8]);        // Will set values based on VModesTable[Arg[0])
    
#ifdef F_9136                                                   // Deep color for 9334 only
    if (((Command.Arg[2] & BITS_5_4) >> 4) == DC_48)         // 16 bit Deep Color. Forcer 0x08[5] to 0 for half pixel mode
        Command.Arg[1] &= ~BIT_5;
#endif
    
    //        WriteByteTPI(TPI_PIX_REPETITION, Command.Arg[1]);  // 0x08
    
#ifndef F_9136
    Command.Arg[2] &= ~BITS_5_4;                         // No Deep Color in 9022A/24A, 9022/4
#endif
    WriteByteTPI(TPI_INPUT_FORMAT_REG, Command.Arg[2]);     // Write output formats to register 0x0A
    
    if ((Command.Arg[3] & TWO_LSBITS) == CS_DVI_RGB)
    {
        ReadClearWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, OUTPT_MODE_HDMI);        // Set 0x1A[0] to DVI
        // ReadClearWriteTPI(TPI_OUTPUT_FORMAT, CS_DVI_RGB);              // Set 0x0A[1:0] to DVI RGB
        ReadModifyWriteTPI(TPI_OUTPUT_FORMAT_REG, TWO_LSBITS, CS_DVI_RGB);   // Set 0x0A[1:0] to DVI RGB
    }
    
    else if (((Command.Arg[3] & TWO_LSBITS) >= CS_HDMI_RGB) && ((Command.Arg[3] & TWO_LSBITS) < CS_DVI_RGB)) // An HDMI CS
    {
        
#ifdef DEV_SUPPORT_EDID
        if (IsHDMI_Sink())                                                  // sink suppurts HDMI
        {
            ReadSetWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, OUTPT_MODE_HDMI);  // Set 0x1A[0] to HDMI
            
            //zhy + Begin Out color space selected
            if (dvi_mode == TRUE)
            {
                Command.Arg[3] = ((Command.Arg[3] & 0xFC) | 0x03);
            }
            else
            {
                // Set YCbCr color space depending on EDID
                if (EDID_Data.YCbCr_4_4_4)
                {
                    Command.Arg[3] = ((Command.Arg[3] & 0xFC) | 0x01);
                }
                else
                {
                    if (EDID_Data.YCbCr_4_2_2)
                    {
                        Command.Arg[3] = ((Command.Arg[3] & 0xFC) | 0x02);
                    }
                    else
                    {
                        Command.Arg[3] = (Command.Arg[3] & 0xFC) ;
                    }
                }
            }
            //zhy + End Out color space selected
        }
        
        // No else?
#endif
    }
    
    WriteByteTPI(TPI_PIX_REPETITION, Command.Arg[1]);  // 0x08
    
    WriteByteTPI(TPI_OUTPUT_FORMAT_REG, Command.Arg[3]);   // Write input and output formats to registers 0x09, 0x0A
    
    if (Command.Arg[4] & MSBIT)             // set embedded sync,
    {
        ReadSetWriteTPI(TPI_SYNC_GEN_CTRL, MSBIT); // set 0x60[7] = 1 for Embedded Sync
        
        if (Command.Arg[5] & BIT_6)
        {
            Result = SetEmbeddedSync(vid_mode);       // Call SetEmbeddedSync() with Video Mode as a parameter
            if (!Result)
            {
                return SET_EMBEDDED_SYC_FAILURE;
            }
            EnableEmbeddedSync();
        }   
        else
        {
            ReadClearWriteTPI(TPI_DE_CTRL, BIT_6);     // clear 0x63[6] = 0 to disable internal DE
        }   
    }
    else                                    // Set external sync
    {
        ReadClearWriteTPI(TPI_SYNC_GEN_CTRL, MSBIT);       // set 0x60[7] = 0 for External Sync
        
        if (Command.Arg[5] & BIT_6)     // set Internal DE Generator only if 0x60[7] == 0
        {
            ReadSetWriteTPI(TPI_DE_CTRL, BIT_6);               // set 0x63[6] = 1 for DE
            
            Status = SetDE(vid_mode);         // Call SetDE() with Video Mode as a parameter
            if (Status != DE_SET_OK)
            {
                return Status;
            }
        }
        
        else if (!(Command.Arg[5] & BIT_6))
        {
            ReadClearWriteTPI(TPI_DE_CTRL, BIT_6);     // clear 0x63[6] = 0 to disable internal DE
        }
    }
    
    TxPowerState(TX_POWER_STATE_D0);//zhy +
#ifdef DEV_SUPPORT_EDID
    if (IsHDMI_Sink())
    {
        SetAVI_InfoFrames(Command);             // InfoFrames - only if output mode is HDMI
        
#ifdef F_9136
        if ((Command.Arg[8] & FOUR_LSBITS) != NO_3D_SUPPORT)
            Set_VSIF(Command.Arg[8] & FOUR_LSBITS, (Command.Arg[8] >> 4) & THREE_LSBITS);
#endif
    }
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!
    //
    
    // THIS PATCH IS NEEDED BECAUSE SETTING UP AVI InfoFrames CLEARS 0x63 and 0x60[5]
    
    WriteByteTPI(TPI_SYNC_GEN_CTRL, Command.Arg[4]);    // Set 0x60 according to Command.Arg[4]
    // THIS PATCH IS NEEDED BECAUSE SETTING UP AVI InfoFrames CLEARS 0x63 and 0x60[5]
    
    if(Command.Arg[4] & MSBIT)              // THIS PATCH IS NEEDED BECAUSE SETTING UP AVI InfoFrames CLEARS 0x63
    {
        WriteByteTPI(TPI_DE_CTRL, EmbeddedSynPATCH); // (set in function SetEmbeddedSync())TO 0.
    }
    //
    // PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!PATCH!!!
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#endif
    
    // Command.Arg[6][3:2] -> 0x0E[7:6] Colorimetry
    ReadModifyWriteTPI(TPI_AVI_BYTE_2, BITS_7_6, Command.Arg[6] << 4);
    
    // Command.Arg[6][6:4] -> 0x0F[6:4] Extended Colorimetry
    if ((Command.Arg[6] & BITS_3_2) == SET_EX_COLORIMETRY)
    {
        ReadModifyWriteTPI(TPI_AVI_BYTE_3, BITS_6_5_4, Command.Arg[6]);
    }
    
    //==========================================================
    
    
    // YC Input Mode Select - Command.Arg[7] - offset 0x0B
    //printf("before set Arg[7] = 0x%x\n", (int)Command.Arg[7]);
    
    WriteByteTPI(TPI_YC_Input_Mode, Command.Arg[7]);
    //      tmp = ReadByteTPI(TPI_YC_Input_Mode);
    //      printf("after set Arg[7] = 0x%x\n", (int)tmp);
    
    // This check needs to be changed to if HDCP is required by the content... once support has been added by RX-side library.
    if (0)//HDCP_TxSupports == TRUE)  //huangyq
    {
#ifdef USE_BLACK_MODE
        SetInputColorSpace (INPUT_COLOR_SPACE_BLACK_MODE);
        TPI_DEBUG_PRINT (("TMDS -> Enabled\n"));
        ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, LINK_INTEGRITY_MODE_MASK | TMDS_OUTPUT_CONTROL_MASK, LINK_INTEGRITY_DYNAMIC | TMDS_OUTPUT_CONTROL_ACTIVE);
#else   // AV MUTE
        ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, LINK_INTEGRITY_MODE_MASK | TMDS_OUTPUT_CONTROL_MASK | AV_MUTE_MASK, LINK_INTEGRITY_DYNAMIC | TMDS_OUTPUT_CONTROL_ACTIVE | AV_MUTE_MUTED);
#endif
        tmdsPoweredUp = TRUE;
    }
    else
    {
        EnableTMDS();
    }
    //EnableTMDS();
    
    return VIDEO_MODE_SET_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  SetAudioMode()
//
// PURPOSE       :  Changes the 9022/4 audio mode as defined by a command from
//                  the System Controller
//
// INPUT PARAMS  :  API_Cmd type structure with command parameters
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  Success message if audio mode set successfully. Error
//                  Code if failed
//
//////////////////////////////////////////////////////////////////////////////
byte SetAudioMode(API_Cmd Command)
{
    bool Result;
    
    TPI_TRACE_PRINT((">>SetAudioMode()\n"));
    
    Result = IsAudioModeSupported(Command.Arg[0] & LOW_NIBBLE);
    
    if (!Result)
    {
        //MessageIndex         EnqueueMessageIndex(AUD_MODE_NOT_SUPPORTED);
        return AUD_MODE_NOT_SUPPORTED;
    }
    else
    {
        SetAudioMute(AUDIO_MUTE_MUTED);
        
        //          ReadModifyWriteTPI(TPI_AUDIO_INTERFACE_REG, HI_NIBBLE, Command.Arg[0]);    // Arg[0][7:4] - 0x26[7:4] - Set audio interface, layout, and mute/unmute
        WriteByteTPI(TPI_AUDIO_INTERFACE_REG, Command.Arg[0]);
        
        /// //      WriteByteTPI(TPI_SPEAKER_CFG, Command.Arg[2]);              // 0x28 - Speaker Configuration - for both S/PDIF and I2S/DSD
        
        if ((Command.Arg[0] & TWO_MSBITS) == AUD_IF_SPDIF)          // S/PDIF audio interface
        {
            ReadModifyWriteTPI(TPI_AUDIO_HANDLING, BITS_1_0, Command.Arg[3]);       // 0x25[1:0] - Audio Handling (S/PDIF only)
            
            //          WriteByteTPI(TPI_AUDIO_SAMPLE_CTRL, Command.Arg[1]);   // 0x27 - Sample Frequency; Sample Size
            ReadModifyWriteTPI(TPI_AUDIO_SAMPLE_CTRL, BITS_7_6 | BITS_5_4_3, Command.Arg[1]); // Sample Frequency 0x27[5:3]; Sample Size 0x27[7:6]
            
            
#ifndef F_9022A_9334
            SetChannelLayout(TWO_CHANNELS);             // Always 2 channels in S/PDIF
#else
            ReadClearWriteTPI(TPI_AUDIO_INTERFACE_REG, BIT_5); // Use TPI 0x26[5] for 9022A/24A and 9334 channel layout
#endif
        }
        else                                                        // I2S or DSD Audio interface
        {
            WriteByteTPI(TPI_AUDIO_SAMPLE_CTRL, Command.Arg[1]);   // 0x27 - HBR; Sample Frequency; Sample Size
            
#ifndef F_9022A_9334
            SetChannelLayout(Command.Arg[0] & BIT_5);
#else
            if ((Command.Arg[0] & TWO_MSBITS) == AUD_IF_DSD)     // DSD audio interface
                ReadSetWriteTPI(TPI_AUDIO_INTERFACE_REG, BIT_5); // Use TPI 0x26[5] to set to Layout_1
            else if ((Command.Arg[0] & TWO_MSBITS) == AUD_IF_I2S) // I2S audio interface
                ReadModifyWriteTPI(TPI_AUDIO_INTERFACE_REG, BIT_5, Command.Arg[0]); // Set user's channel layout selection to TPI 0x26[5]
#endif
#ifndef F_9022A_9334                                                        // 9022/24 only. For 9022A/24A and 9334 input
            SetInputWordLength(Command.Arg[4] & LOW_NIBBLE);       // word length is set automatically when
#endif
/*Begin set the audio format register modified by nielimin*/
			WriteByteTPI(0xBC, 0x02);
			WriteByteTPI(0xBD, 0x24);
			WriteByteTPI(0xBE, 0x02);
/*End set the audio format register modified by nielimin*/

        }
        
        SetAudioMute(AUDIO_MUTE_NORMAL);
        /*
        if ((Command.Arg[0] & TWO_MSBITS) == AUD_IF_DSD) {
        // DSD
        SetAudioInfoFrames(Command.Arg[5] & THREE_LSBITS, Command.Arg[2], 0x09, 0x02, 0x00); //channel count, spk config,
        // Fs, SS.
        }
        else if (Command.Arg[1] & BIT_2) {
        // HBR
        SetAudioInfoFrames(Command.Arg[5] & THREE_LSBITS,
        Command.Arg[2],
        0x00,
        (Command.Arg[1] & BITS_5_4_3) >> 3,
        (Command.Arg[1] & BITS_7_6) >> 6
        );
        }
        else
        {
        SetAudioInfoFrames(Command.Arg[5] & THREE_LSBITS,
        Command.Arg[2],
        0x00,
        0x00,
        0x00
        );
        }
        */
        SetAudioInfoFrames(Command.Arg[5] & THREE_LSBITS, Command.Arg[0] & FOUR_LSBITS, Command.Arg[1] & BITS_7_6, Command.Arg[1] & BITS_5_4_3, Command.Arg[2]); //channel count, spk config,coding type, sample size, sample freq. 
        
        ReadModifyWriteTPI(TPI_AUDIO_INTERFACE_REG, BIT_4, Command.Arg[0]);     // Set audio mute/unmute bit.
        
        return AUDIO_MODE_SET_OK;
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  MapI2S()
//
// PURPOSE       :  Changes the 9022/4 I2S channel mapping as defined by a
//                  command sent from the System Controller
//
// INPUT PARAMS  :  API_Cmd type structure with command parameters
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  Success message if channel mapping successful. Error
//                  Code if failed
//
//////////////////////////////////////////////////////////////////////////////
byte MapI2S(API_Cmd Command)
{
    byte B_Data;
    int i;
    // byte temp;
    
    TPI_TRACE_PRINT((">>MapI2S()\n"));
    
    B_Data = ReadByteTPI(TPI_AUDIO_INTERFACE_REG);
    
    if ((B_Data & TWO_MSBITS) != AUD_IF_I2S)    // 0x26 not set to I2S interface
    {
        return I2S_NOT_SET;
    }
    
    //      WriteByteTPI(TPI_I2S_EN, Command.Arg[0]);
    
    //      printf("I2sMapping:\n");
    
    for (i = 0; i < I2S_MAPPING_CMD_LEN; i++)
    {
        WriteByteTPI(TPI_I2S_EN, Command.Arg[i]);
        
        //          temp = ReadByteTPI(TPI_I2S_EN);
        //          printf("    FIFO#%d = 0x%2X\n", i, (int)temp);
        
        if ((Command.Arg[i+1] & BITS_1_0) == 0)
            return 0;
    }
    return I2S_MAPPING_SUCCESSFUL;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  ConfigI2SInput()
//
// PURPOSE       :  Sets the 9022/4 I2S channel bit direction, justification
//                  and polarity as defined by acommand sent from the System
//                  Controller
//
// INPUT PARAMS  :  API_Cmd type structure with command parameters
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  Success message if video I2S channels configuredsuccessfully.
//                  Error Code if setting failed
//
//////////////////////////////////////////////////////////////////////////////
byte ConfigI2SInput(API_Cmd Command)
{
    byte B_Data;
    
    TPI_TRACE_PRINT((">>ConfigI2SInput()\n"));
    
    B_Data = ReadByteTPI(TPI_AUDIO_INTERFACE_REG);
    
    if ((B_Data & TWO_MSBITS) != AUD_IF_I2S)    // 0x26 not set to I2S interface
    {
        return I2S_NOT_SET;
    }
    
    WriteByteTPI(TPI_I2S_IN_CFG, Command.Arg[0]);
    
    return I2S_INPUT_CONFIG_SUCCESSFUL;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  SetI2S_StreamHeader()
//
// PURPOSE       :  Sets the 9022/4 I2S Channel Status bytes, as defined by
//                  a command sent from the System Controller
//
// INPUT PARAMS  :  API_Cmd type structure with command parameters
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  Success message if stream header set successfully. Error
//                  Code if failed
//
//////////////////////////////////////////////////////////////////////////////
byte SetI2S_StreamHeader(API_Cmd Command)
{
    byte B_Data;
    int i;
    
    TPI_TRACE_PRINT((">>SetI2S_StreamHeader()\n"));
    
    B_Data = ReadByteTPI(TPI_AUDIO_INTERFACE_REG);
    
    if ((B_Data & TWO_MSBITS) != AUD_IF_I2S)    // 0x26 not set to I2S interface
    {
        return I2S_NOT_SET;
    }
    
    for (i = 0; i < Command.CommandLength; i++)
    {
        WriteByteTPI(TPI_I2S_CHST_0 + i, Command.Arg[i]);
    }
    
    return I2S_HEADER_SET_SUCCESSFUL;
}


#ifdef F_9022A_9334

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   SetGBD_InfoFrame(()
//
// PURPOSE      :   Sets and sends the the 9022A/4A GBD InfoFrames according
//                  to data sent from the System Controller
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   Success message if GBD packet set successfully. Error
//                  Code if failed
//
// NOTE         :   Currently this function is a place holder. It always
//                  returns a Success message
//
//////////////////////////////////////////////////////////////////////////////
byte SetGBD_InfoFrame(void)
{
    byte CheckSum;
    
    TPI_TRACE_PRINT((">>SetGBD_InfoFrame()\n"));
    
    // Set MPEG InfoFrame Header to GBD InfoFrame Header values:
    WriteByteTPI(MISC_INFO_FRAMES_CTRL, MPEG_INFOFRAME_CODE);                       // 0xBF = Use MPEG      InfoFrame for GBD - 0x03
    WriteByteTPI(MISC_INFO_FRAMES_TYPE, TYPE_GBD_INFOFRAME);                        // 0xC0 = 0x0A
    WriteByteTPI(MISC_INFO_FRAMES_VER, NEXT_FIELD | GBD_PROFILE | AFFECTED_GAMUT_SEQ_NUM);   // 0x0C1 = 0x81
    WriteByteTPI(MISC_INFO_FRAMES_LEN, ONLY_PACKET | CURRENT_GAMUT_SEQ_NUM);                                // 0x0C2 = 0x31
    
    CheckSum = TYPE_GBD_INFOFRAME +
        NEXT_FIELD +
        GBD_PROFILE +
        AFFECTED_GAMUT_SEQ_NUM +
        ONLY_PACKET +
        CURRENT_GAMUT_SEQ_NUM;
    
    CheckSum = 0x100 - CheckSum;
    
    WriteByteTPI(MISC_INFO_FRAMES_CTRL, EN_AND_RPT_MPEG);  // Enable and Repeat MPEG InfoFrames
    WriteByteTPI(MISC_INFO_FRAMES_CHKSUM, CheckSum);                        // 0X00 - Send header only
    
    return GBD_SET_SUCCESSFULLY;
}

#endif

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  IsVideoModeSupported()
//
// PURPOSE       :  Checks if the video mode passed as parameter is supported
//                  by the connected sink (as read from its EDID).
//
// INPUT PARAMS  :  Video mode number
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  TRUE if mode is supported by the sink. FALSE if not.
//
//////////////////////////////////////////////////////////////////////////////
bool IsVideoModeSupported(byte V_Mode)
{
#ifdef DEV_SUPPORT_EDID
    int i;
    
    TPI_TRACE_PRINT((">>IsVideoModeSupported()\n"));
    
    for (i = 0; i < MAX_V_DESCRIPTORS; i++)
    {
        if ((EDID_Data.VideoDescriptor[i] & SEVEN_LSBITS) == V_Mode)
            return TRUE;
    }
    return FALSE;
#else
    V_Mode = V_Mode;        // dummy usage of parameter to avoid compiler warning
    TPI_TRACE_PRINT((">>IsVideoModeSupported()\n"));
    return TRUE;            // Just return true for now... what other mechanism should be used to determine support?
#endif
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  IsAudioModeSupported()
//
// PURPOSE       :  Checks if the audio mode passed as parameter is supported
//                  by the connected sink (as read from its EDID).
//
// INPUT PARAMS  :  Video mode number
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  TRUE if mode is supporter by the sink. FALSE if not.
//
//////////////////////////////////////////////////////////////////////////////
byte IsAudioModeSupported(byte A_Mode)
{
#ifdef DEV_SUPPORT_EDID
    int i;
    
    TPI_TRACE_PRINT((">>IsAudioModeSupported()\n"));
    
    for (i = 0; i < MAX_A_DESCRIPTORS; i++)
    {
        if ((((EDID_Data.AudioDescriptor[i][0]) & SEVEN_LSBITS) >> 3) == A_Mode)
            return TRUE;
    }
    return FALSE;
#else
    A_Mode = A_Mode;        // dummy usage of parameter to avoid compiler warning
    TPI_TRACE_PRINT((">>IsAudioModeSupported()\n"));
    return TRUE;            // Just return true for now... what other mechanism should be used to determine support?
#endif
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  SetAVI_InfoFrames()
//
// PURPOSE       :  Load AVI InfoFrame data into registers and send to sink
//
// INPUT PARAMS  :  An API_Cmd parameter that holds the data to be sent
//                  in the InfoFrames
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  TRUE
//
//////////////////////////////////////////////////////////////////////////////
bool SetAVI_InfoFrames(API_Cmd Command)
{
    byte B_Data[SIZE_AVI_INFOFRAME];
    byte VideoMode;                     // pointer to VModesTable[]
    byte i;
    byte TmpVal;
    
    TPI_TRACE_PRINT((">>SetAVI_InfoFrames()\n"));
    
    for (i = 0; i < SIZE_AVI_INFOFRAME; i++)
        B_Data[i] = 0;
    
    //if (Command.CommandLength > VIDEO_SETUP_CMD_LEN)    // Command length > 9. AVI InfoFrame is set by the host
    //{
    //     for (i = 1; i < Command.CommandLength - VIDEO_SETUP_CMD_LEN; i++)
    //         B_Data[i] = Command.Arg[VIDEO_SETUP_CMD_LEN + i - 1];
    //}
    //else                                                // Command length == 7. AVI InfoFrame is set by the FW
    {
        if ((Command.Arg[3] & TWO_LSBITS) == 1)         // AVI InfoFrame DByte1
            TmpVal = 2;
        else if ((Command.Arg[3] & TWO_LSBITS) == 2)
            TmpVal = 1;
        else
            TmpVal = 0;
        
        B_Data[1] = (TmpVal << 5) & BITS_OUT_FORMAT;                    // AVI Byte1: Y1Y0 (output format)
        
        if (((Command.Arg[6] >> 2) & TWO_LSBITS) == 3)                  // Extended colorimetry - xvYCC
        {
            B_Data[2] = 0xC0;                                           // Extended colorimetry info (B_Data[3] valid (CEA-861D, Table 11)
            
            if (((Command.Arg[6] >> 4) & THREE_LSBITS) == 0)            // xvYCC601
                B_Data[3] &= ~BITS_6_5_4;
            
            else if (((Command.Arg[6] >> 4) & THREE_LSBITS) == 1)       // xvYCC709
                B_Data[3] = (B_Data[3] & ~BITS_6_5_4) | BIT_4;
        }
        
        else if (((Command.Arg[6] >> 2) & TWO_LSBITS) == 2)             // BT.709
            B_Data[2] = 0x80;                                           // AVI Byte2: C1C0
        
        else if (((Command.Arg[6] >> 2) & TWO_LSBITS) == 1)             // BT.601
            B_Data[2] = 0x40;                                           // AVI Byte2: C1C0
        
        else                                                            // Carries no data
        {                                                               // AVI Byte2: C1C0
            B_Data[2] &= ~BITS_7_6;                                     // colorimetry = 0
            B_Data[3] &= ~BITS_6_5_4;                                   // Extended colorimetry = 0
        }
        
        VideoMode = ConvertVIC_To_VM_Index(vid_mode, Command.Arg[8] & LOW_NIBBLE);              /// //
        
        B_Data[4] = vid_mode;
        
#ifdef RX_ONBOARD
        if ((avi_information.byte_2 & PICTURE_ASPECT_RATIO_MASK) == PICTURE_ASPECT_RATIO_16x9)
        {
            B_Data[2] |= _16_To_9;                          // AVI Byte2: M1M0
            if (VModesTable[VideoMode].AspectRatio == _4or16 && AspectRatioTable[vid_mode-1] == _4)
            {
                vid_mode++;
                B_Data[4]++;
            }
        }
        else
        {
            B_Data[2] |= _4_To_3;                           // AVI Byte4: VIC
        }
#else
        B_Data[2] |= _4_To_3;                           // AVI Byte4: VIC
#endif
        
        B_Data[2] |= SAME_AS_AR;                                        // AVI Byte2: R3..R1
        B_Data[5] = VModesTable[VideoMode].PixRep;                      // AVI Byte5: Pixel Replication - PR3..PR0
    }
    
    B_Data[0] = 0x82 + 0x02 +0x0D;                                          // AVI InfoFrame ChecKsum
    
    for (i = 1; i < SIZE_AVI_INFOFRAME; i++)
        B_Data[0] += B_Data[i];
    
    B_Data[0] = 0x100 - B_Data[0];
    
    WriteBlockTPI(TPI_AVI_BYTE_0, SIZE_AVI_INFOFRAME, B_Data);
#ifdef DEV_EMBEDDED
    EnableEmbeddedSync();
#endif
    
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  SetAudioInfoFrames()
//
// PURPOSE       :  Load Audio InfoFrame data into registers and send to sink
//
// INPUT PARAMS  :  (1) Channel count (2) speaker configuration per CEA-861D
//                  Tables 19, 20 (3) Coding type: 0x09 for DSD Audio. 0 (refer
//                                      to stream header) for all the rest (4) Sample Frequency. Non
//                                      zero for HBR only (5) Audio Sample Length. Non zero for HBR
//                                      only.
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  TRUE
//
//////////////////////////////////////////////////////////////////////////////
bool SetAudioInfoFrames(byte ChannelCount, byte CodingType, byte SS, byte Fs, byte SpeakerConfig)
{
    byte B_Data[SIZE_AUDIO_INFOFRAME];  // 14
    byte i;
    //byte TmpVal = 0;  //removed by h00163450
    
    TPI_TRACE_PRINT((">>SetAudioInfoFrames()\n"));
    
    for (i = 0; i < SIZE_AUDIO_INFOFRAME +1; i++)
        B_Data[i] = 0;
    
    B_Data[0] = EN_AUDIO_INFOFRAMES;        // 0xC2
    B_Data[1] = TYPE_AUDIO_INFOFRAMES;      // 0x84
    B_Data[2] = AUDIO_INFOFRAMES_VERSION;   // 0x01
    B_Data[3] = AUDIO_INFOFRAMES_LENGTH;    // 0x0A
    
    B_Data[5] = ChannelCount;               // 0 for "Refer to Stream Header" or for 2 Channels. 0x07 for 8 Channels
    B_Data[5] |= (CodingType << 4);                 // 0xC7[7:4] == 0b1001 for DSD Audio
    B_Data[4] = 0x84 + 0x01 + 0x0A;         // Calculate checksum
    
    //    B_Data[6] = (Fs << 2) | SS;
    B_Data[6] = (Fs >> 1) | (SS >> 6);
    
    //write Fs to 0x27[5:3] and SS to 0x27[7:6] to update the IForm with the current value.
    //  ReadModifyWriteTPI(TPI_AUDIO_SAMPLE_CTRL, BITS_7_6 | BITS_5_4_3, (B_Data[6] & BITS_1_0) << 6 | (B_Data[6] & 0x1C) << 1);
    
    B_Data[8] = SpeakerConfig;
    
    for (i = 5; i < SIZE_AUDIO_INFOFRAME; i++)
        B_Data[4] += B_Data[i];
    
    B_Data[4] = 0x100 - B_Data[4];
    g_audio_Checksum = B_Data[4];   // Audio checksum for global use
    
    WriteBlockTPI(TPI_AUDIO_BYTE_0, SIZE_AUDIO_INFOFRAME, B_Data);
#ifdef DEV_EMBEDDED
    EnableEmbeddedSync();
#endif
    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  Set_VSIF(()
//
// PURPOSE       :  Construct Vendor Specific InfoFrame for3D support. use
//                                      MPEG InfoFrame
//
// INPUT PARAMS  :  (1) 3D_Structure value per HDMI 1.4, table H-2.
//                                      (2) 3D_Ext_Data value per HDMI 1.4, table H-6.
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  void
//
//////////////////////////////////////////////////////////////////////////////
#ifdef DEV_SUPPORT_EDID
void Set_VSIF(byte _3D_Struct, byte _3D_Ext_Data)
{
    byte i;
    byte Data[16];
       
    Data[0] = VSIF_TYPE;                // 0x81
    Data[1] = VSIF_VERSION;             // 0x01
    Data[2] = VSIF_LEN;                 // 5
    
    Data[3] = VSIF_TYPE+                // partial checksum
        VSIF_VERSION+
        VSIF_LEN;
    
    Data[4] = 0x03;                     // HDMI Signature LS Byte
    Data[5] = 0x0C;                     // HDMI Signature middle byte
    Data[6] = 0x00;                     // HDMI Signature MS Byte
    
    Data[7] = _3D_STRUC_PRESENT << 5;   // 3D format indication present. 3D_Structure follows.
    Data[8] = _3D_Struct << 4;   // 3D format indication present. 3D_Structure follows.
    
    if (_3D_Struct == SIDE_BY_SIDE_HALF)
        Data[9] = _3D_Ext_Data << 4;       // 3D_Structure - 0x00 for Frame Packing
    
    for (i = 4; i < 10; i++)
        Data[3] += Data[i];
    
    Data[3] %= 0x100;
    Data[3] = 100 - Data[3];            // Final checksum
    
    WriteByteTPI(MISC_INFO_FRAMES_CTRL, EN_AND_RPT_MPEG);                           // Enable and Repeat MPEG InfoFrames
    WriteBlockTPI(MISC_INFO_FRAMES_TYPE, 10, Data );               // Write VSIF to MPEG registers and start transmission
}
#endif

void SetI2S(void)
{
    API_Cmd Command;
    
    Command.Arg[0] = AudioI2SConfig;
    ConfigI2SInput(Command);
    Command.Arg[0] = I2SSDSet[0];
    Command.Arg[1] = I2SSDSet[1];
    Command.Arg[2] = I2SSDSet[2];
    Command.Arg[3] = I2SSDSet[3];
    MapI2S(Command);
    Command.Arg[0] = I2SStreamHeader[0];
    Command.Arg[1] = I2SStreamHeader[1];
    Command.Arg[2] = I2SStreamHeader[2];
    Command.Arg[3] = I2SStreamHeader[3];
    Command.Arg[4] = I2SStreamHeader[4];
    Command.CommandLength = 5;
    SetI2S_StreamHeader(Command);
}

void VideoModeSetbymain(void)
{
    API_Cmd Command;
    
    
    Command.Arg[0] = VideoMode[0];
    Command.Arg[1] = VideoMode[1];
    Command.Arg[2] = VideoMode[2];
    Command.Arg[3] = VideoMode[3];
    Command.Arg[4] = VideoMode[4];
    Command.Arg[5] = VideoMode[5];
    Command.Arg[6] = VideoMode[6];
    Command.Arg[7] = VideoMode[7];
    Command.Arg[8] = VideoMode[8];
    ChangeVideoMode(Command);
}

void AudioModeSetbymain(void)
{
    API_Cmd Command;
    
    Command.Arg[0] = AudioMode[0];
    Command.Arg[1] = AudioMode[1];
    Command.Arg[2] = AudioMode[2];
    Command.Arg[3] = AudioMode[3];
    Command.Arg[4] = AudioMode[4];
    Command.Arg[5] = AudioMode[5];
    
    SetAudioMode(Command);
    
    if((AudioMode[0]>>6)==2)
    {
        SetI2S();
    }
    
}

