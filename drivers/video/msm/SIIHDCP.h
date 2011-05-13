/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/

#ifdef DEV_SUPPORT_HDCP

extern bool HDCP_TxSupports;			// True if the TX supports HDCP and has legal AKSVs... False otherwise.
extern bool HDCP_Started;				// True if the HDCP enable bit has been set... False otherwise.

extern byte HDCP_LinkProtectionLevel;	// The most recently set link protection level from 0x29[7:6]

void HDCP_Init (void);

void HDCP_CheckStatus (byte InterruptStatusImage);


bool IsHDCP_Supported(void);

void HDCP_On(void);
void HDCP_Off(void);

#else

#define HDCP_TxSupports FALSE

#endif