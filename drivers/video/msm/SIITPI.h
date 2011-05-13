/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/

bool TPI_Init (void);           // Document purpose, usage
void TPI_Poll (void);           // Document purpose, usage, rename
void TPI_Power_Standby(void);

void EnableTMDS (void);         // Document purpose, usage
void DisableTMDS (void);        // Document purpose, usage

void RestartHDCP (void);        // Document purpose, usage

void SetInputColorSpace (byte inputColorSpace);

void SetAudioMute (byte audioMute);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

void HotPlugService (void);     // Why is this being called form outside TPI.c?? Re-architect
bool EnableInterrupts (byte);   // Why is this being called form outside TPI.c?? Re-architect


// TPI Firmware Version
//=====================
static const char TPI_FW_VERSION[] = {'0', '.', '2', '8'};

