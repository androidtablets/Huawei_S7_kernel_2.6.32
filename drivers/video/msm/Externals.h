/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/
#define bool unsigned char

extern byte CmdTableIndex;                  // Command Table index
extern byte vid_mode;

// CEC Globals
//============

extern bool CEC_DeviceList[];              // List of logical addresses and their status (taken/free)
extern byte CEC_Initiator;                  // Logical address of THIS device

// Patches
//========
extern byte EmbeddedSynPATCH;

//UART
//====
extern byte TXBusy;

extern byte IDX_InChar;
extern byte NumOfArgs;
extern bool F_SBUF_DataReady;
extern bool F_CollectingData;


#ifndef DEBUG_EDID
extern bool F_IgnoreEDID;                   // Allow setting of any video input format regardless of Sink's EDID (for debuggin puroses)
#endif

extern byte pvid_mode;
extern bool dvi_mode;

extern byte g_audio_Checksum;   // Audio checksum

