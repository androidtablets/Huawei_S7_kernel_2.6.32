/* Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
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

#include <mach/gpio.h>
#include <mach/io.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <mach/vreg.h>
#include <mach/mpp.h>

#if 0
#define DBG(fmt, args...) printk(KERN_INFO "[%s,%d] "fmt"\n", __FUNCTION__, __LINE__, ##args)
#else
#define DBG(fmt, args...) do {} while (0)
#endif
#define INFO(fmt, args...) printk(KERN_INFO "[%s,%d] "fmt"\n", __FUNCTION__, __LINE__, ##args)

#define TLMMADDR_TLMM_BASE			0xA8E00000
#define TLMMADDR_GPIO1_BASE     	0xA8E00000
#define TLMMADDR_GPIO2_BASE			0xA8F00400
#define TLMMADDR_GPIO1SHDW1_BASE	0xA9000800
#define TLMMADDR_GPIO2SHDW1_BASE	0xA9100C00
#define TLMMADDR_GPIO1SHDW2_BASE	0xA9201000
#define TLMMADDR_GPIO2SHDW2_BASE	0xA9301400

#define CLOCK_CONTROL_BASE			0xA8600000
#define PERPH_WEB_BASE_TCXO4		0xA9D00000

#define LCD_PANEL_BL_MIN			0
#define LCD_PANEL_BL_MAX			100

#if HUAWEI_HWID_L1(S7)
#if HUAWEI_HWID_L2(S7, S7201)
#define	HUAWEI_BRIGHTNESS_START		25
#define	HUAWEI_BRIGHTNESS_END		50
#else
#define	HUAWEI_BRIGHTNESS_START		25
#define	HUAWEI_BRIGHTNESS_END		54
#endif
#endif

#define GP_MN_MDIV					1
#define GP_MN_NDIV					4800

#if HWVERID_HIGHER(S70, T1)
#define GPIO_LCD_VLED		29
#endif
 
#if HWVERID_HIGHER(S70, A)
#define GPIO_LCD_VDD		26
#endif

#define GPIO_LCD_PWM_OUT	60

#define lcd_pwm_on() 	do { \
	    					gpio_tlmm_config(GPIO_CFG(GPIO_LCD_PWM_OUT, \
	    					                          2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), \
	    					                 GPIO_ENABLE); \
	    				} while(0)
#define lcd_pwm_off() 	do {gpio_tlmm_config(GPIO_CFG(GPIO_LCD_PWM_OUT, \
	    											  2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), \
	    									 GPIO_DISABLE); \
	    				} while(0)

static u32 gLcdBacklightLevel = 0;  
static DECLARE_MUTEX(mLcdBackLightSem);

extern int tps65023_set_ldo2(bool flag);
	
static int lcdc_panel_on(struct platform_device *pdev)
{
    DBG("***************lcdc_panel_on*******************");

//	mpp_config_digital_out(6, MPP_CFG(MPP_DLOGIC_LVL_MSMP, MPP_DLOGIC_OUT_CTRL_LOW));

#if 0
	/* Power on at resume in order to get more info from UART */
	tps65023_set_ldo2(1);
#endif

#if HWVERID_HIGHER(S70, T1)
    /* power on LCD */
    gpio_direction_output(GPIO_LCD_VLED, 1);
#endif

#if HWVERID_HIGHER(S70, A)
    gpio_direction_output(GPIO_LCD_VDD, 1);
#endif
    lcd_pwm_on();
	return 0;
}

static int lcdc_panel_off(struct platform_device *pdev)
{
	DBG("***************lcdc_panel_off*******************");
	lcd_pwm_off();
#if HWVERID_HIGHER(S70, A)
    gpio_direction_output(GPIO_LCD_VDD, 0);
#endif

#if HWVERID_HIGHER(S70, T1)
	/* power off LCD */
	gpio_direction_output(GPIO_LCD_VLED, 0);
#endif

#if 0
	/* Power off at suspend in order to get more info from UART */
	tps65023_set_ldo2(0);
#endif

    down(&mLcdBackLightSem);
    gLcdBacklightLevel = 0;
    up(&mLcdBackLightSem);

//    mpp_config_digital_out(6, MPP_CFG(MPP_DLOGIC_LVL_MSMP, MPP_DLOGIC_OUT_CTRL_LOW));
    
	return 0;
}

u32 bsp_lcdc_panel_get_backlight(void)
{
    u32 level;

    down(&mLcdBackLightSem);
    level = gLcdBacklightLevel;
    up(&mLcdBackLightSem);
    
    return level;    
}

EXPORT_SYMBOL(bsp_lcdc_panel_get_backlight);

static void lcdc_panel_set_backlight(struct msm_fb_data_type *mfd)
{
	unsigned int level;
	unsigned int virt_addr;

#if HUAWEI_HWID_L2(S7, S7201)
    unsigned int backLight;
    unsigned int level_numerator = 18;
    unsigned int level_denominator = 25;
    unsigned int level_intercept = 2200;
    unsigned int level_down = 22;
    unsigned int level_up  = 100;
#endif

	level = mfd->bl_level;
    level = (level > LCD_PANEL_BL_MAX) ? LCD_PANEL_BL_MAX : level;
    level = (level <= LCD_PANEL_BL_MIN) ? LCD_PANEL_BL_MIN : level;

#if HUAWEI_HWID_L2(S7, S7201)
    backLight = level_numerator * level * level /level_denominator + level_intercept;
	backLight /= level_up;

    if(level_down == level || level_up == level)
    {
	   backLight = level;
    }
#endif
    down(&mLcdBackLightSem);
    gLcdBacklightLevel = level;
    up(&mLcdBackLightSem);

    /* 默认将level 0 置为全灭,用于解决屏幕唤醒时闪屏问题 */
#if HUAWEI_HWID_L2(S7, S7201)
	if(0 == backLight)
	{
	    virt_addr = (unsigned int)ioremap(PERPH_WEB_BASE_TCXO4, 0x1000);
	    outpdw(virt_addr + 0x54, (GP_MN_NDIV * (LCD_PANEL_BL_MAX - backLight) 
	        							 / (LCD_PANEL_BL_MAX - LCD_PANEL_BL_MIN)));
	    iounmap((void *)virt_addr);

	    return;
	}
#else
    if(0 == level)
        {
            virt_addr = (unsigned int)ioremap(PERPH_WEB_BASE_TCXO4, 0x1000);
            outpdw(virt_addr + 0x54, (GP_MN_NDIV * (LCD_PANEL_BL_MAX - level) 
                                             / (LCD_PANEL_BL_MAX - LCD_PANEL_BL_MIN)));
            iounmap((void *)virt_addr);
    
            return;
        }
#endif


	virt_addr = (unsigned int)ioremap(PERPH_WEB_BASE_TCXO4, 0x1000);

	//outpdw(virt_addr + 0x54, (GP_MN_NDIV * (LCD_PANEL_BL_MAX - backLight) 
      //  							 / (LCD_PANEL_BL_MAX - LCD_PANEL_BL_MIN)));
#if HUAWEI_HWID_L2(S7, S7201)
	outpdw(virt_addr + 0x54, (GP_MN_NDIV * (LCD_PANEL_BL_MAX 
								- (backLight * (HUAWEI_BRIGHTNESS_END - HUAWEI_BRIGHTNESS_START) / 100 
								+ HUAWEI_BRIGHTNESS_START)) / (LCD_PANEL_BL_MAX - LCD_PANEL_BL_MIN))); 
#else
	outpdw(virt_addr + 0x54, (GP_MN_NDIV * (LCD_PANEL_BL_MAX 
								- (level * (HUAWEI_BRIGHTNESS_END - HUAWEI_BRIGHTNESS_START) / 100 
								+ HUAWEI_BRIGHTNESS_START)) / (LCD_PANEL_BL_MAX - LCD_PANEL_BL_MIN)));
#endif

    
	
    iounmap((void *)virt_addr);
#if HUAWEI_HWID_L2(S7, S7201)
	DBG("BackLight=%d%%", 100 - (LCD_PANEL_BL_MAX - backLight) * 100
        					 / (LCD_PANEL_BL_MAX - LCD_PANEL_BL_MIN));
#else
	DBG("BackLight=%d%%", 100 - (LCD_PANEL_BL_MAX - level) * 100
        					 / (LCD_PANEL_BL_MAX - LCD_PANEL_BL_MIN));
#endif

}

static int __init lcdc_panel_probe(struct platform_device *pdev)
{
	msm_fb_add_device(pdev);

	return 0;
}

#ifdef CONFIG_PM
// TODO: BUG, white screen when resume because of hardware, by h00131430
#if !HUAWEI_HWID_L2(S7, S7103) 
static int lcdc_panel_suspend(struct platform_device *pdev, pm_message_t state)
{
	//tps65023_set_ldo2(0);
	return 0;
}

static int lcdc_panel_resume(struct platform_device *pdev)
{
	//tps65023_set_ldo2(1);
	return 0;
}
#endif
#endif

static struct platform_driver this_driver = {
	.probe  = lcdc_panel_probe,
#ifdef CONFIG_PM
// TODO: BUG, white screen when resume because of hardware, by h00131430
#if !HUAWEI_HWID_L2(S7, S7103) 
	.suspend = lcdc_panel_suspend,
	.resume = lcdc_panel_resume,
#endif
#endif
	.driver = {
		.name   = "lcdc_panel",
	},
};

static struct msm_fb_panel_data lcdc_panel_data = {
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
	.set_backlight = lcdc_panel_set_backlight,
};

static int lcdc_dev_id;

int lcdc_device_register(struct msm_panel_info *pinfo)
{
	struct platform_device *pdev = NULL;
	int ret;

	pdev = platform_device_alloc("lcdc_panel", ++lcdc_dev_id);
	if (!pdev)
		return -ENOMEM;

	lcdc_panel_data.panel_info = *pinfo;
    lcdc_panel_data.panel_info.bl_min = LCD_PANEL_BL_MIN;
    lcdc_panel_data.panel_info.bl_max = LCD_PANEL_BL_MAX;
    
	ret = platform_device_add_data(pdev, &lcdc_panel_data,
		sizeof(lcdc_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init lcdc_panel_init(void)
{
    int rc = 0;
    unsigned int virt_addr;

#if HWVERID_HIGHER(S70, T1)
    if ((rc = gpio_request(GPIO_LCD_VLED, "lcdc_vled"))) {
        pr_err("gpio_request failed on pin %d\n", GPIO_LCD_VLED);
        return rc;
    }
#endif
#if HWVERID_HIGHER(S70, A)
    if ((rc = gpio_request(GPIO_LCD_VDD, "lcdc_vdd"))) {
        pr_err("gpio_request failed on pin %d\n", GPIO_LCD_VDD);
        return rc;
    }
#endif

    /* start PWM Clock: TCXO/4=4.8MHz, 
     * N=GP_MN_NDIV, M=GP_MN_MDIV, Fout=1KHz, Duty=0% */
    virt_addr = (unsigned int)ioremap(PERPH_WEB_BASE_TCXO4, 0x1000);
    outpdw(virt_addr + 0x4C, GP_MN_MDIV);
    outpdw(virt_addr + 0x50, (0x1FFF - (GP_MN_NDIV - GP_MN_MDIV)));

	/* set backlight as 0% */
    outpdw(virt_addr + 0x54, (GP_MN_NDIV * (LCD_PANEL_BL_MAX - LCD_PANEL_BL_MIN) 
                                     / (LCD_PANEL_BL_MAX - LCD_PANEL_BL_MIN)));
    iounmap((void *)virt_addr);

    down(&mLcdBackLightSem);
    gLcdBacklightLevel = 0;
    up(&mLcdBackLightSem);

    lcd_pwm_on();

	return platform_driver_register(&this_driver);
}

module_init(lcdc_panel_init);
