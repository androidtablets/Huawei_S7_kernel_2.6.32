/*****************************************************************************
 *               Copyright (C) 2009, Huawei Tech. Co., Ltd.               
 *                           ALL RIGHTS RESERVED                               
 * FileName: bsp_led.h                                                          
 * Version: 1.0                                                                
 * Description:  led¶ÔÍâ½Ó¿ÚÍ·ÎÄ¼þ
 *                                                                             
 * History:                                                                    
******************************************************************************/

#ifndef __BSP_LED_H__
#define __BSP_LED_H__

/*****************************************************************************/
/*                              宏常量定义                                   */
/*****************************************************************************/
#define BSP_LED_DEVICE_NAME                "incident_led"
#define BSP_LED_DEVICE_PATH                "/dev/incident_led"

/*****************************************************************************/
/*                              枚举定义                                     */
/*****************************************************************************/
/*功能描述：LED状态枚举*/
typedef enum tagBSP_LED_E_STATUS
{
  BSP_LED_E_ON = 0,
  BSP_LED_E_OFF,
  BSP_LED_E_FLASH,
  BSP_LED_E_BUTT
}BSP_LED_E_STATUS;

/*****************************************************************************/
/*                                结构定义                                   */
/*****************************************************************************/
/*LED闪烁控制*/
typedef struct tagBSP_LED_S_CTRL
{
  unsigned short usOnTime;      /*单位ms*/
  unsigned short usOffTime;     /*单位ms*/
}BSP_LED_S_CTRL;


#endif  /*__BSP_LED_H__*/

