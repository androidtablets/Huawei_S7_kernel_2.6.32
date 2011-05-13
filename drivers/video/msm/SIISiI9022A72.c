/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/
#include "SIIdefs.h"
#include "SIITypeDefs.h"
#include "i2c_master_sw.h"

void TXHAL_InitMicroGpios (void) 
{
}


void TXHAL_InitPreReset (void) 
{
}


void TXHAL_InitPostReset (void) 
{
    I2C_WriteByte(0x72>>1, 0x82, 0x25);                    // Terminations
    I2C_WriteByte(0x72>>1, 0x08, 0x36);
}
