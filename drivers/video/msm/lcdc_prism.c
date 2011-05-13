/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include "msm_fb.h"

#ifdef CONFIG_FB_MSM_TRY_MDDI_CATCH_LCDC_PRISM
 #include "mddihosti.h"
#endif

#ifdef CONFIG_MSM_HDMI
 #include "hdmi.h"
#endif

//#define 720P_60HZ	1
static int __init lcdc_prism_init(void)
{
    int ret;
    struct msm_panel_info pinfo;

#ifdef CONFIG_FB_MSM_TRY_MDDI_CATCH_LCDC_PRISM
    ret = msm_fb_detect_client("lcdc_prism_wvga");
    if (ret == -ENODEV)
    {
        return 0;
    }

    if (ret && (mddi_get_client_id() != 0))
    {
        return 0;
    }
#endif

#if 0
// 720*480
	pinfo.xres  = 720;
    pinfo.yres  = 480;

	pinfo.type  = LCDC_PANEL;
    pinfo.pdest = DISPLAY_1;
    pinfo.wait_cycle = 0;
    pinfo.bpp = 24;
    pinfo.fb_num   = 2;

	pinfo.clk_rate = 27027000;
	pinfo.bl_min = 0;
    pinfo.bl_max = 255;
	pinfo.lcdc.h_back_porch  = 60;
	pinfo.lcdc.h_front_porch = 16;
	pinfo.lcdc.h_pulse_width = 62;
	pinfo.lcdc.v_back_porch  = 30;
	pinfo.lcdc.v_front_porch = 6;
	pinfo.lcdc.v_pulse_width = 9;

    pinfo.lcdc.border_clr = 0;/* blk */
    pinfo.lcdc.underflow_clr = 0xff;/* blue */
    pinfo.lcdc.hsync_skew = 0;
#endif

#if 1
// 800*480
	 pinfo.xres  = 800; 
     pinfo.yres  = 480;
     pinfo.type  = LCDC_PANEL;
     pinfo.pdest = DISPLAY_1;
     pinfo.wait_cycle = 0;
     pinfo.bpp = 32;
     pinfo.fb_num   = 2;
     pinfo.clk_rate = 30720000;//38460000;
     pinfo.bl_min = 0;
     pinfo.bl_max = 255;
  #if 0  //LCD // ÏûÒþ   nielimin
	 pinfo.lcdc.h_back_porch  = 26;
	 pinfo.lcdc.h_front_porch = 36;	 
	 pinfo.lcdc.h_pulse_width = 20;
     pinfo.lcdc.v_back_porch  = 19;
	 pinfo.lcdc.v_front_porch = 9;     
 	 pinfo.lcdc.v_pulse_width = 4; 
  #else
	 pinfo.lcdc.h_back_porch  = 55;
     pinfo.lcdc.h_front_porch = 36;
     pinfo.lcdc.h_pulse_width = 29;     
     pinfo.lcdc.v_back_porch  = 32;
     pinfo.lcdc.v_front_porch = 9;
     pinfo.lcdc.v_pulse_width = 4;
  #endif
	 pinfo.lcdc.border_clr = 0;/* blk */
	 pinfo.lcdc.underflow_clr = 0xff;/* blue */
     pinfo.lcdc.hsync_skew = 0;
#endif

#if 0 //1280*720
    pinfo.xres = 1280;
    pinfo.yres = 720;

    pinfo.type  = LCDC_PANEL;
    pinfo.pdest = DISPLAY_1;
    pinfo.wait_cycle = 0;
    pinfo.bpp = 24;
    pinfo.fb_num = 2;

    pinfo.clk_rate = 74250000;
    pinfo.clk_min = 74250000;
    pinfo.clk_max = 74250000;
    pinfo.bl_min = 0;
    pinfo.bl_max = 255;
    pinfo.lcdc.h_back_porch  = 220;
    pinfo.lcdc.h_front_porch = 110;
    pinfo.lcdc.h_pulse_width = 40;
    pinfo.lcdc.v_back_porch  = 20;
    pinfo.lcdc.v_front_porch = 5;
    pinfo.lcdc.v_pulse_width = 5;

    pinfo.lcdc.border_clr = 0;/* blk */
    pinfo.lcdc.underflow_clr = 0xff;/* blue */
    pinfo.lcdc.hsync_skew = 0;
#endif
	ret = lcdc_device_register(&pinfo);
    if (ret)
    {
        printk(KERN_ERR "%s: failed to register device!\n", __func__);
    }
    return ret;
}
module_init(lcdc_prism_init);
