/*****************************************************************************
 *               Copyright (C) 2006, Huawei Tech. Co., Ltd.
 *                           ALL RIGHTS RESERVED
 * FileName: bsp_interface.h
 * Version: 1.0
 *
 * History:
 * 1. 2010-12-19,z00101756 Create this file.
******************************************************************************/

#ifndef __BSP_INTERFACE_H__
#define __BSP_INTERFACE_H__

/*****************************************************************************/
/*****************************************************************************/

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#define BSP_REG_DEVICE_NAME          "reg"
#define BSP_GPIO_DEVICE_NAME         "gpio"

#define BSP_REG_DEVICE_PATH          "/dev/reg"
#define BSP_GPIO_DEVICE_PATH         "/dev/gpio"



#define BSP_GPIO_DEBUG_CRTL                  0x50000001  
#define BSP_REG_DEBUG_CRTL                   0x50000002   

#define BSP_GPIO_SET_VALUE                   0x00
#define BSP_GPIO_GET_VALUE                   0x01


typedef struct tagBSP_GPIO_DEVICE_S_PARA
{
    unsigned int usOpType; 
    unsigned char  ucgpio_num;  
    unsigned char  ucgpio_cfg;                                 
    unsigned char  ucval;    
}BSP_GPIO_DEVICE_S_PARA,*BSP_GPIO_DEVICE_S_PARA_PTR;

typedef struct tagBSP_REG_DEVICE_S_PARA
{
    unsigned int  ulOpType; 
    unsigned int  uladdr;   
    unsigned int  ulvalue; 
}BSP_REG_DEVICE_S_PARA,*BSP_REG_DEVICE_S_PARA_PTR;


#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */


#endif  /* __BSP_INTERFACE_H__ */

