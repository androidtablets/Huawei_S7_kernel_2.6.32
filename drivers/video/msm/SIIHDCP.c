/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/

#include <linux/kernel.h>
//#include <stdio.h>
//#include "MCU_Regs.h"
#include "SIIdefs.h"
#include "SIITypeDefs.h"
//#include "AMF_Lib.h"
#include "SIITPI_Regs.h"
#include "SIIConstants.h"
#include "Externals.h"
#include "SIIMacros.h"
#include "SIITPI_Access.h"
#include "SIITPI.h"
#include "SIIHDCP.h"
#include "Util.h"
#include "SIIAV_Config.h"

#ifdef DEV_SUPPORT_HDCP

#define AKSV_SIZE              5
#define NUM_OF_ONES_IN_KSV    20

bool HDCP_TxSupports;
bool HDCP_Started;
byte HDCP_LinkProtectionLevel;

static bool AreAKSV_OK(void);

#ifdef CHECKREVOCATIONLIST
static bool CheckRevocationList(void);
#endif

#ifdef READKSV
static bool GetKSV(void);
static bool IsRepeater(void);
#endif


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   HDCP_Init()
//
// PURPOSE      :   Tests Tx and Rx support of HDCP. If found, checks if
//                  and attempts to set the security level accordingly.
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :	HDCP_TxSupports - initialized to FALSE, set to TRUE if supported by this device
//					HDCP_Started - initialized to FALSE;
//					HDCP_LinkProtectionLevel - initialized to (EXTENDED_LINK_PROTECTION_NONE | LOCAL_LINK_PROTECTION_NONE);
//
// RETURNS      :   None
//
//////////////////////////////////////////////////////////////////////////////

void HDCP_Init (void) {

	TPI_TRACE_PRINT((">>HDCP_Init()\n"));

	HDCP_TxSupports = FALSE;
	HDCP_Started = FALSE;
	HDCP_LinkProtectionLevel = EXTENDED_LINK_PROTECTION_NONE | LOCAL_LINK_PROTECTION_NONE;

	// This is TX-related... need only be done once.
    if (!IsHDCP_Supported())
    {
		TPI_DEBUG_PRINT(("HDCP -> TX does not support HDCP\n"));
		return;
	}

	// This is TX-related... need only be done once.
    if (!AreAKSV_OK())
    {
        TPI_DEBUG_PRINT(("HDCP -> Illegal AKSV\n"));
        return;
    }

	// Both conditions above must pass or authentication will never be attempted. 
	TPI_DEBUG_PRINT(("HDCP -> Supported by TX\n"));
	HDCP_TxSupports = TRUE;
}


void HDCP_CheckStatus (byte InterruptStatusImage) {

	byte QueryData;
	byte LinkStatus;
	byte RegImage;
	byte NewLinkProtectionLevel;
#ifdef READKSV
	byte RiCnt;
#endif

	if (HDCP_TxSupports == TRUE)
	{
		if ((HDCP_LinkProtectionLevel == (EXTENDED_LINK_PROTECTION_NONE | LOCAL_LINK_PROTECTION_NONE)) && (HDCP_Started == FALSE))
		{
			QueryData = ReadByteTPI(TPI_HDCP_QUERY_DATA_REG);

			if (QueryData & PROTECTION_TYPE_MASK)   // Is HDCP avaialable
			{
				HDCP_On();
			}
		}

		// Check if Link Status has changed:
		if (InterruptStatusImage & SECURITY_CHANGE_EVENT)
		{
			TPI_DEBUG_PRINT (("HDCP -> "));

			LinkStatus = ReadByteTPI(TPI_HDCP_QUERY_DATA_REG);
			LinkStatus &= LINK_STATUS_MASK;

			ClearInterrupt(SECURITY_CHANGE_EVENT);

			switch (LinkStatus)
			{
				case LINK_STATUS_NORMAL:
					TPI_DEBUG_PRINT (("Link = Normal\n"));
					break;

				case LINK_STATUS_LINK_LOST:
					TPI_DEBUG_PRINT (("Link = Lost\n"));
					RestartHDCP();
					break;

				case LINK_STATUS_RENEGOTIATION_REQ:
					TPI_DEBUG_PRINT (("Link = Renegotiation Required\n"));
					HDCP_Off();
					HDCP_On();
					break;

				case LINK_STATUS_LINK_SUSPENDED:
					TPI_DEBUG_PRINT (("Link = Suspended\n"));
					HDCP_On();
					break;
			}
		}
		
		// Check if HDCP state has changed:
		if (InterruptStatusImage & HDCP_CHANGE_EVENT)
		{
			RegImage = ReadByteTPI(TPI_HDCP_QUERY_DATA_REG);
			
			NewLinkProtectionLevel = RegImage & (EXTENDED_LINK_PROTECTION_MASK | LOCAL_LINK_PROTECTION_MASK);
			if (NewLinkProtectionLevel != HDCP_LinkProtectionLevel)
			{
				TPI_DEBUG_PRINT (("HDCP -> "));				
				HDCP_LinkProtectionLevel = NewLinkProtectionLevel;
				switch (HDCP_LinkProtectionLevel)
				{					
					case (EXTENDED_LINK_PROTECTION_NONE | LOCAL_LINK_PROTECTION_NONE):						
						TPI_DEBUG_PRINT (("Protection = None\n"));
						RestartHDCP();
						break;
					case LOCAL_LINK_PROTECTION_SECURE:
#ifdef USE_BLACK_MODE
  						DelayMS(10); // Wait about half a frame for compliance.
						SetAudioMute(AUDIO_MUTE_NORMAL);
#ifndef USE_DE_GENERATOR
						SetInputColorSpace (INPUT_COLOR_SPACE_RGB);
#else
						SetInputColorSpace (INPUT_COLOR_SPACE_YCBCR422);
#endif
#else	//AV MUTE
						ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, AV_MUTE_MASK, AV_MUTE_NORMAL);
#endif
						TPI_DEBUG_PRINT (("Protection = Local, Video Unmuted\n"));
						break;

					case (EXTENDED_LINK_PROTECTION_SECURE | LOCAL_LINK_PROTECTION_SECURE):
						TPI_DEBUG_PRINT (("Protection = Extended\n"));
#ifdef READKSV
 						if (IsRepeater())
						{
							RiCnt = ReadIndexedRegister(INDEXED_PAGE_0, 0x25);
							while (RiCnt > 0x70)  // Frame 112
							{
								RiCnt = ReadIndexedRegister(INDEXED_PAGE_0, 0x25);
							}
							ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, 0x06, 0x06);
							GetKSV();
							RiCnt = ReadByteTPI(TPI_SYSTEM_CONTROL_DATA_REG);
							ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, 0x08, 0x00);
						}
#endif
						break;

					default:
						TPI_DEBUG_PRINT (("Protection = Extended but not Local?\n"));
						RestartHDCP();
						break;
				}
			}

			ClearInterrupt(HDCP_CHANGE_EVENT);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   IsHDCP_Supported()
//
// PURPOSE      :   Check Tx revision number to find if this Tx supports HDCP
//                  by reading the HDCP revision number from TPI register 0x30.
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   TRUE if Tx supports HDCP. FALSE if not.
//
//////////////////////////////////////////////////////////////////////////////

bool IsHDCP_Supported(void)
{
    byte HDCP_Rev;
	bool HDCP_Supported;

	TPI_TRACE_PRINT((">>IsHDCP_Supported()\n"));

	HDCP_Supported = TRUE;

	// Check Device ID
    HDCP_Rev = ReadByteTPI(TPI_HDCP_REVISION_DATA_REG);

    if (HDCP_Rev != (HDCP_MAJOR_REVISION_VALUE | HDCP_MINOR_REVISION_VALUE))
	{
    	HDCP_Supported = FALSE;
	}

#ifdef SiI_9022AYBT_DEVICEID_CHECK
	// Even if HDCP is supported check for incorrect Device ID
	HDCP_Rev = ReadByteTPI(TPI_AKSV_1_REG);
	if (HDCP_Rev == 0x90)
	{
		HDCP_Rev = ReadByteTPI(TPI_AKSV_2_REG);
		if (HDCP_Rev == 0x22)
		{
			HDCP_Rev = ReadByteTPI(TPI_AKSV_3_REG);
			if (HDCP_Rev == 0xA0)
			{
				HDCP_Rev = ReadByteTPI(TPI_AKSV_4_REG);
				if (HDCP_Rev == 0x00)
				{
					HDCP_Rev = ReadByteTPI(TPI_AKSV_5_REG);
					if (HDCP_Rev == 0x00)
					{
						HDCP_Supported = FALSE;
					}
				}
			}
		}
	}
#endif
	return HDCP_Supported;
}

#ifdef READKSV
//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   IsRepeater()
//
// PURPOSE      :   Test if sink is a repeater
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   None
//
// RETURNS      :   TRUE if sink is a repeater. FALSE if not.
//
//////////////////////////////////////////////////////////////////////////////
bool IsRepeater(void)
{
    byte RegImage;

	TPI_TRACE_PRINT((">>IsRepeater()\n"));

    RegImage = ReadByteTPI(TPI_HDCP_QUERY_DATA_REG);

    if (RegImage & HDCP_REPEATER_MASK)
        return TRUE;

    return FALSE;           // not a repeater
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   HDCP_On()
//
// PURPOSE      :   Switch hdcp on.
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   HDCP_Started set to TRUE
//
// RETURNS      :   None
//
//////////////////////////////////////////////////////////////////////////////

void HDCP_On(void)
{
	TPI_TRACE_PRINT((">>HDCP_On()\n"));
	TPI_DEBUG_PRINT(("HDCP Started\n"));

	WriteByteTPI(TPI_HDCP_CONTROL_DATA_REG, PROTECTION_LEVEL_MAX);

	HDCP_Started = TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   HDCP_Off()
//
// PURPOSE      :   Switch hdcp off.
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   HDCP_Started set to FALSE
//
// RETURNS      :   None
//
//////////////////////////////////////////////////////////////////////////////

void HDCP_Off(void)
{
	TPI_TRACE_PRINT((">>HDCP_Off()\n"));
	TPI_DEBUG_PRINT(("HDCP Stopped\n"));
#ifdef USE_BLACK_MODE
	SetInputColorSpace(INPUT_COLOR_SPACE_BLACK_MODE);
	SetAudioMute(AUDIO_MUTE_MUTED);
#else	// AV MUTE
	ReadModifyWriteTPI(TPI_SYSTEM_CONTROL_DATA_REG, AV_MUTE_MASK, AV_MUTE_MUTED);
#endif
	WriteByteTPI(TPI_HDCP_CONTROL_DATA_REG, PROTECTION_LEVEL_MIN);

	HDCP_Started = FALSE;
	HDCP_LinkProtectionLevel = EXTENDED_LINK_PROTECTION_NONE | LOCAL_LINK_PROTECTION_NONE;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION     :   GetKSV()
//
// PURPOSE      :   Collect all downstrean KSV for verification
//
// INPUT PARAMS :   None
//
// OUTPUT PARAMS:   None
//
// GLOBALS USED :   KSV_Array[]
//
// RETURNS      :   TRUE if KSVs collected successfully. False if not.
//
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// The buffer is limited to KSV_ARRAY_SIZE due to the 8051 implementation.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//////////////////////////////////////////////////////////////////////////////
#ifdef READKSV
static bool GetKSV(void)
{
	byte i;
    word KeyCount;
    byte KSV_Array[KSV_ARRAY_SIZE];

	TPI_TRACE_PRINT((">>GetKSV()\n"));
    ReadBlockHDCP(DDC_BSTATUS_ADDR_L, 1, &i);
    KeyCount = (i & DEVICE_COUNT_MASK) * 5;
	if (KeyCount != 0)
	{
		ReadBlockHDCP(DDC_KSV_FIFO_ADDR, KeyCount, KSV_Array);
	}

	/*
	TPI_TRACE_PRINT(("KeyCount = %d\n", (int) KeyCount));
	for (i=0; i<KeyCount; i++)
	{
		TPI_TRACE_PRINT(("KSV[%2d] = %2.2X\n", (int) i, (int) KSV_Array[i]));
	}
	*/

	 return TRUE;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  AreAKSV_OK()
//
// PURPOSE       :  Check if AKSVs contain 20 '0' and 20 '1'
//
// INPUT PARAMS  :  None
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  TBD
//
// RETURNS       :  TRUE if 20 zeros and 20 ones found in AKSV. FALSE OTHERWISE
//
//////////////////////////////////////////////////////////////////////////////
static bool AreAKSV_OK(void)
{
    byte B_Data[AKSV_SIZE];
    byte NumOfOnes = 0;

    byte i;
    byte j;

	TPI_TRACE_PRINT((">>AreAKSV_OK()\n"));

    ReadBlockTPI(TPI_AKSV_1_REG, AKSV_SIZE, B_Data);

    for (i=0; i < AKSV_SIZE; i++)
    {
        for (j=0; j < BYTE_SIZE; j++)
        {
            if (B_Data[i] & 0x01)
            {
                NumOfOnes++;
            }
            B_Data[i] >>= 1;
        }
     }
     if (NumOfOnes != NUM_OF_ONES_IN_KSV)
        return FALSE;

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  CheckRevocationList()
//
// PURPOSE       :  Compare KSVs to those included in a revocation list
//
// INPUT PARAMS  :  None
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  TBD
//
// RETURNS       :  TRUE if no illegal KSV found in BKSV list
//
// NOTE			 :	Currently this function is implemented as a place holder only
//
//////////////////////////////////////////////////////////////////////////////
#ifdef CHECKREVOCATIONLIST
static bool CheckRevocationList(void)
{
	TPI_TRACE_PRINT((">>CheckRevocationList()\n"));
    return TRUE;
}
#endif

#endif
