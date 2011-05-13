/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/
#include <linux/types.h>

#include "SIIdefs.h"
#include "SIITypeDefs.h"
#include "SIIConstants.h"


byte CmdTableIndex;                 // Command Table index
byte vid_mode;


// CEC Globals
//============
bool CEC_DeviceList[16];                // List of logical addresses and their status (taken/free)
byte CEC_Initiator;                 // Logical address of THIS device


// Patches
//========
byte EmbeddedSynPATCH;


//UART
//====
byte TXBusy;

byte IDX_InChar;
byte NumOfArgs;
bool F_SBUF_DataReady;
bool F_CollectingData;



#ifndef DEBUG_EDID
bool F_IgnoreEDID;                  // Allow setting of any video input format regardless of Sink's EDID (for debuggin puroses)
#endif

byte pvid_mode;
bool dvi_mode;

// Checksums
byte g_audio_Checksum;  // Audio checksum

