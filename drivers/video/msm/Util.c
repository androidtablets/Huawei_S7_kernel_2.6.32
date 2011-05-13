/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/

#include "SIITypeDefs.h"
#include "Util.h"

//////////////////////////////////////////////////////////////////////////////
//
// FUNCTION      :  DelayMS(word M_Sec)
//
// PURPOSE       :  Introduce a busy-wait delay equal, in milliseconds, to the
//                  input parameter.
//
// INPUT PARAMS  :  Length of required delay in milliseconds (max. 65535 ms)
//
// OUTPUT PARAMS :  None
//
// GLOBALS USED  :  None
//
// RETURNS       :  void
//
//////////////////////////////////////////////////////////////////////////////

void DelayMS(word M_Sec)
{
    int i;
    i = M_Sec * 100;

    while (i)
    {
        i--;
    }

}
