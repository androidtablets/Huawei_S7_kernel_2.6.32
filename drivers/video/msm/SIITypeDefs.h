/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/

//typedef unsigned char     bool;  //removed by h00163450
typedef unsigned char       byte;
typedef unsigned short      word;
typedef unsigned long       dword;

#define code


#define MAX_COMMAND_ARGUMENTS       24

// API Interface Data Structures
//==============================
typedef struct 
{
    byte    OpCode;
    byte    CommandLength;
    byte    Arg[MAX_COMMAND_ARGUMENTS];
    byte    CheckSum;
} API_Cmd;


typedef enum 
{
    SS_FATAL_ERROR,
    SS_RX_NOT_READY,
    SS_RX_NEW_VIDEO_MODE,
    SS_TX_READY
} SYSTEM_STATE;
