/***********************************************************************************/
/*  Copyright (c) 2002-2009, Silicon Image, Inc.  All rights reserved.             */
/*  No part of this work may be reproduced, modified, distributed, transmitted,    */
/*  transcribed, or translated into any language or computer format, in any form   */
/*  or by any means without written permission of: Silicon Image, Inc.,            */
/*  1060 East Arques Avenue, Sunnyvale, California 94085                           */
/***********************************************************************************/

void TXHAL_InitMicroGpios (void);
void TXHAL_InitPreReset (void);
void TXHAL_InitPostReset (void);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SiI9022A/24A 81BGA
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define C_9022A_OR_C_9024A_81BGA

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Micro Controller Pins
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//sbit PinI2CSCL = P1^7;
//sbit PinI2CSDA = P1^6;

//sbit GRANT = P2^2;            // GPIO3 on BB Header pin 9
//sbit REQUEST = P2^3;          // GPIO4 on BB Header pin 10
//sbit CYC_MICRO_RESET = P2^1;  // Connected to 9022A/4A pin C3 (CRST#)
//sbit DEBUG = P1^0;                // Jumper JP16 - Jumper ON = low / Jumper OFF = high
//sbit Master = P1^4;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Project Definitions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define T_MONITORING_PERIOD     10
#define R_INIT_PERIOD           10
#define R_MONITORING_PERIOD     200

#define TX_HW_RESET_PERIOD      200
#define RX_HW_RESET_PERIOD      600

#define INT_CONTROL 0x00 // Interrupt pin is push-pull and active high (this is normally 0x06)

#define SiI_DEVICE_ID           0xB0
#define SiI_DEVICE_STRING       "SiI 9022A/9024A 81BGA\n"
#define T_HW_RESET              1
//#define RX_ONBOARD
#define SiI_9022AYBT_DEVICEID_CHECK
//#define PLL_BACKDOOR          // TPI does not support programming of Output Clock PLL
#define F_9022A_9334            // for now, use new TPI features...
#define CLOCK_EDGE_RISING
#define SOURCE_TERMINATION_ON

#define DEV_INDEXED_READ
#define DEV_INDEXED_WRITE
#define DEV_INDEXED_RMW

#define DEV_SUPPORT_EDID
//#define DEV_SUPPORT_HDCP
//#define DEV_SUPPORT_CEC
#define DEV_ACNU_BOARD

#define I2S_AUDIO // huangyq

/*\
| | TPI API Version
\*/

#define TPI_API_MAJ_VERSION 1
#define TPI_API_MIN_VERSION 4   // feature set
#define TPI_API_SUB_VERSION 1   // command length change

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Debug Definitions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define DISABLE 0x00
#define ENABLE  0xFF

#if 0
// Compile debug prints inline or not
#define CONF__TPI_DEBUG_PRINT  (ENABLE)   // h00163450
#define CONF__CPI_DEBUG_PRINT  (DISABLE)  // (ENABLE) // h00163450
#define CONF__TPI_EDID_PRINT   (ENABLE)  //(DISABLE)  // (ENABLE) // h00163450

//#define CONF__TPI_TRACE_PRINT (DISABLE)
#define CONF__TPI_TRACE_PRINT   (ENABLE)  // h00163450
#else
// Compile debug prints inline or not
#define CONF__TPI_DEBUG_PRINT  (DISABLE)   // h00163450
#define CONF__CPI_DEBUG_PRINT  (DISABLE)  // (ENABLE) // h00163450
#define CONF__TPI_EDID_PRINT   (DISABLE)  //(DISABLE)  // (ENABLE) // h00163450
//#define CONF__TPI_TRACE_PRINT (DISABLE)
#define CONF__TPI_TRACE_PRINT   (DISABLE)  // h00163450
#endif
/*\
| | Trace Print Macro
| |
| | Note: TPI_DEBUG_PRINT Requires double parenthesis
| | Example:  TPI_DEBUG_PRINT(("hello, world!\n"));
\*/

#if (CONF__TPI_TRACE_PRINT == ENABLE)
#define TPI_TRACE_PRINT(x)     printk x
#else
#define TPI_TRACE_PRINT(x)
#endif

/*\
| | Debug Print Macro
| |
| | Note: TPI_DEBUG_PRINT Requires double parenthesis
| | Example:  TPI_DEBUG_PRINT(("hello, world!\n"));
\*/

#if (CONF__TPI_DEBUG_PRINT == ENABLE)
#define TPI_DEBUG_PRINT(x)      printk x
#else
#define TPI_DEBUG_PRINT(x)
#endif

/*\
| | CPI Debug Print Macro
| |
| | Note: To enable CPI description printing, both CONF__CPI_DEBUG_PRINT and CONF__TPI_DEBUG_PRINT must be enabled
| |
| | Note: CPI_DEBUG_PRINT Requires double parenthesis
| | Example:  CPI_DEBUG_PRINT(("hello, world!\n"));
\*/

#if (CONF__CPI_DEBUG_PRINT == ENABLE)
#define CPI_DEBUG_PRINT(x)      TPI_DEBUG_PRINT(x)
#else
#define CPI_DEBUG_PRINT(x)
#endif

/*\
| | EDID Print Macro
| |
| | Note: To enable EDID description printing, both CONF__TPI_EDID_PRINT and CONF__TPI_DEBUG_PRINT must be enabled
| |
| | Note: TPI_EDID_PRINT Requires double parenthesis
| | Example:  TPI_EDID_PRINT(("hello, world!\n"));
\*/

#if (CONF__TPI_EDID_PRINT == ENABLE)
#define TPI_EDID_PRINT(x)       TPI_DEBUG_PRINT(x)
#else
#define TPI_EDID_PRINT(x)
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//#define TEST_CEC                    // Define when testing CEC by sending SiIMon commands

