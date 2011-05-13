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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <mach/hardware.h>
#include <linux/io.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>

#include <asm/system.h>
#include <asm/mach-types.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/clk.h>

#include "msm_fb.h"
#include "tvenc.h"


#define PAL_TV_DIMENSION_WIDTH      720
#define PAL_TV_DIMENSION_HEIGHT     576
static unsigned int cvbs_flag_pal_ntsc = 0;

#if 0
static int pal_on(struct platform_device *pdev)
{
	uint32 reg = 0;
	int ret = 0;
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	TV_OUT(TV_ENC_CTL, 0);	/* disable TV encoder */

	switch (mfd->panel.id) {
	case PAL_BDGHIN:
		/* Cr gain 11, Cb gain C6, y_gain 97 */
		TV_OUT(TV_GAIN, 0x0088c1a0);
		TV_OUT(TV_CGMS, 0x00012345);
		TV_OUT(TV_TEST_MUX, 0x0);
		/*  PAL Timing */
		TV_OUT(TV_SYNC_1, 0x00180097);
		TV_OUT(TV_SYNC_2, 0x011f06c0);
		TV_OUT(TV_SYNC_3, 0x0005000a);
		TV_OUT(TV_SYNC_4, 0x00320271);
		TV_OUT(TV_SYNC_5, 0x005602f9);
		TV_OUT(TV_SYNC_6, 0x0005000a);
		TV_OUT(TV_SYNC_7, 0x0000000f);
		TV_OUT(TV_BURST_V1, 0x0012026e);
		TV_OUT(TV_BURST_V2, 0x0011026d);
		TV_OUT(TV_BURST_V3, 0x00100270);
		TV_OUT(TV_BURST_V4, 0x0013026f);
		TV_OUT(TV_BURST_H, 0x00af00ea);
		TV_OUT(TV_SOL_REQ_ODD, 0x0030026e);
		TV_OUT(TV_SOL_REQ_EVEN, 0x0031026f);

		reg |= TVENC_CTL_TV_MODE_PAL_BDGHIN;
		break;
	case PAL_M:
		/* Cr gain 11, Cb gain C6, y_gain 97 */
		TV_OUT(TV_GAIN, 0x0081b697);
		TV_OUT(TV_CGMS, 0x000af317);
		TV_OUT(TV_TEST_MUX, 0x000001c3);
		TV_OUT(TV_TEST_MODE, 0x00000002);
		/*  PAL Timing */
		TV_OUT(TV_SYNC_1, 0x0020009e);
		TV_OUT(TV_SYNC_2, 0x011306b4);
		TV_OUT(TV_SYNC_3, 0x0006000c);
		TV_OUT(TV_SYNC_4, 0x0028020D);
		TV_OUT(TV_SYNC_5, 0x005e02fb);
		TV_OUT(TV_SYNC_6, 0x0006000c);
		TV_OUT(TV_SYNC_7, 0x00000012);
		TV_OUT(TV_BURST_V1, 0x0012020b);
		TV_OUT(TV_BURST_V2, 0x0016020c);
		TV_OUT(TV_BURST_V3, 0x00150209);
		TV_OUT(TV_BURST_V4, 0x0013020c);
		TV_OUT(TV_BURST_H, 0x00bf010b);
		TV_OUT(TV_SOL_REQ_ODD, 0x00280208);
		TV_OUT(TV_SOL_REQ_EVEN, 0x00290209);

		reg |= TVENC_CTL_TV_MODE_PAL_M;
		break;
	case PAL_N:
		/* Cr gain 11, Cb gain C6, y_gain 97 */
		TV_OUT(TV_GAIN, 0x0081b697);
		TV_OUT(TV_CGMS, 0x000af317);
		TV_OUT(TV_TEST_MUX, 0x000001c3);
		TV_OUT(TV_TEST_MODE, 0x00000002);
		/*  PAL Timing */
		TV_OUT(TV_SYNC_1, 0x00180097);
		TV_OUT(TV_SYNC_2, 0x12006c0);
		TV_OUT(TV_SYNC_3, 0x0005000a);
		TV_OUT(TV_SYNC_4, 0x00320271);
		TV_OUT(TV_SYNC_5, 0x005602f9);
		TV_OUT(TV_SYNC_6, 0x0005000a);
		TV_OUT(TV_SYNC_7, 0x0000000f);
		TV_OUT(TV_BURST_V1, 0x0012026e);
		TV_OUT(TV_BURST_V2, 0x0011026d);
		TV_OUT(TV_BURST_V3, 0x00100270);
		TV_OUT(TV_BURST_V4, 0x0013026f);
		TV_OUT(TV_BURST_H, 0x00af00fa);
		TV_OUT(TV_SOL_REQ_ODD, 0x0030026e);
		TV_OUT(TV_SOL_REQ_EVEN, 0x0031026f);

		reg |= TVENC_CTL_TV_MODE_PAL_N;
		break;

	default:
		return -ENODEV;
	}

	reg |= TVENC_CTL_Y_FILTER_EN |
	    TVENC_CTL_CR_FILTER_EN |
	    TVENC_CTL_CB_FILTER_EN | TVENC_CTL_SINX_FILTER_EN;
#ifdef CONFIG_FB_MSM_TVOUT_SVIDEO
	reg |= TVENC_CTL_S_VIDEO_EN;
#endif

	TV_OUT(TV_LEVEL, 0x00000000);	/* DC offset to 0. */
	TV_OUT(TV_OFFSET, 0x008080f0);

#ifdef CONFIG_FB_MSM_MDP31
	TV_OUT(TV_DAC_INTF, 0x29);
#endif
	TV_OUT(TV_ENC_CTL, reg);

	//reg |= TVENC_CTL_ENC_EN;
	//TV_OUT(TV_ENC_CTL, reg);

	return ret;
}
#endif

static int pal_on(struct platform_device *pdev)
{
	uint32 reg = 0;
	
    if(2 == cvbs_flag_pal_ntsc)
    {
	    /* Open CVBS in NTSC mode */
	    TV_OUT(TV_ENC_CTL, 0);  /* disable TV encoder */

	    /* NTSC_M */
	    /* Cr gain 11, Cb gain C6, y_gain 97 */
	    TV_OUT(TV_GAIN, 0x0081B697);
	                    
	    TV_OUT(TV_CGMS, 0x0);
	    /* NTSC Timing */
	    TV_OUT(TV_SYNC_1, 0x0020009e);
	    TV_OUT(TV_SYNC_2, 0x011306B4);
	    TV_OUT(TV_SYNC_3, 0x0006000C);
	    TV_OUT(TV_SYNC_4, 0x0028020D);
	    TV_OUT(TV_SYNC_5, 0x005E02FB);
	    TV_OUT(TV_SYNC_6, 0x0006000C);
	    TV_OUT(TV_SYNC_7, 0x00000012);
	    TV_OUT(TV_BURST_V1, 0x0013020D);
	    TV_OUT(TV_BURST_V2, 0x0014020C);
	    TV_OUT(TV_BURST_V3, 0x0013020D);
	    TV_OUT(TV_BURST_V4, 0x0014020C);
	    TV_OUT(TV_BURST_H, 0x00AE00F2);
	    TV_OUT(TV_SOL_REQ_ODD, 0x00280208);
	    TV_OUT(TV_SOL_REQ_EVEN, 0x00290209);

	    reg |= TVENC_CTL_TV_MODE_NTSC_M_PAL60;

	    reg |= TVENC_CTL_Y_FILTER_EN |
	        TVENC_CTL_CR_FILTER_EN |
	        TVENC_CTL_CB_FILTER_EN | TVENC_CTL_SINX_FILTER_EN;
#ifdef CONFIG_FB_MSM_TVOUT_SVIDEO
	    reg |= TVENC_CTL_S_VIDEO_EN;
#endif
    }
	else
	{
        /* Open CVBS in PAL mode */
        TV_OUT(TV_ENC_CTL, 0);	/* disable TV encoder */
        
        /* Cr gain 11, Cb gain C6, y_gain 97 */
		TV_OUT(TV_GAIN, 0x0081b697);
		TV_OUT(TV_CGMS, 0x000af317);
		TV_OUT(TV_TEST_MUX, 0x000001c3);
		TV_OUT(TV_TEST_MODE, 0x00000002);
        
		/*  PAL Timing */
		TV_OUT(TV_SYNC_1, 0x0020009e);
		TV_OUT(TV_SYNC_2, 0x011306b4);
		TV_OUT(TV_SYNC_3, 0x0006000c);
		TV_OUT(TV_SYNC_4, 0x0028020D);
		TV_OUT(TV_SYNC_5, 0x005e02fb);
		TV_OUT(TV_SYNC_6, 0x0006000c);
		TV_OUT(TV_SYNC_7, 0x00000012);
		TV_OUT(TV_BURST_V1, 0x0012020b);
		TV_OUT(TV_BURST_V2, 0x0016020c);
		TV_OUT(TV_BURST_V3, 0x00150209);
		TV_OUT(TV_BURST_V4, 0x0013020c);
		TV_OUT(TV_BURST_H, 0x00bf010b);
		TV_OUT(TV_SOL_REQ_ODD, 0x00280208);
		TV_OUT(TV_SOL_REQ_EVEN, 0x00290209);

		reg |= TVENC_CTL_TV_MODE_PAL_M;
        
        reg |= TVENC_CTL_Y_FILTER_EN |
            TVENC_CTL_CR_FILTER_EN |
            TVENC_CTL_CB_FILTER_EN | TVENC_CTL_SINX_FILTER_EN;
 #ifdef CONFIG_FB_MSM_TVOUT_SVIDEO
        reg |= TVENC_CTL_S_VIDEO_EN;
 #endif           
	}

    TV_OUT(TV_LEVEL, 0x00000000);   /* DC offset to 0. */
    TV_OUT(TV_OFFSET, 0x008080f0); 
		
 #ifdef CONFIG_FB_MSM_MDP31                                             
    TV_OUT(TV_DAC_INTF, 0x29);                     
 #endif
    TV_OUT(TV_ENC_CTL, reg);
    
    reg |= TVENC_CTL_ENC_EN;
                   
    if(0 != cvbs_flag_pal_ntsc)
    {
		TV_OUT(TV_ENC_CTL, reg);
    }

	return 0;
}

static int pal_off(struct platform_device *pdev)
{
	TV_OUT(TV_ENC_CTL, 0);	/* disable TV encoder */
	return 0;
}

static int __init pal_probe(struct platform_device *pdev)
{
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = pal_probe,
	.driver = {
		.name   = "tv_pal",
	},
};

static struct msm_fb_panel_data pal_panel_data = {
	.panel_info.xres = PAL_TV_DIMENSION_WIDTH,
	.panel_info.yres = PAL_TV_DIMENSION_HEIGHT,
	.panel_info.type = TV_PANEL,
	.panel_info.pdest = DISPLAY_1,
	.panel_info.wait_cycle = 0,
	.panel_info.bpp = 16,
	.panel_info.fb_num = 2,	
	.on = pal_on,
	.off = pal_off,
};

static struct platform_device this_device = {
	.name   = "tv_pal",
    #if 0
	.id	= 0,
    #else
	.id	= PAL_M,   
    #endif
    
	.dev	= {
		.platform_data = &pal_panel_data,
	}
};

static int DRV_TvDevOpen(struct inode *inode, struct file *file);
static int DRV_TvDevClose(struct inode *inode, struct file *file);
static int DRV_TvDevIoctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);

#define BSP_IMX51_TV_DEVICE_NAME "tv_ctrl"

#define BSP_CVBS_MODE_CRTL    0x11223366
#define BSP_CVBS_OPEN_CRTL    0x11223377
#define BSP_CVBS_CLOSE_CRTL   0x11223388

#define BSP_TV_ENC_BASE_ADDR  0xaa400000

#define REG_READ(addr)	     (*((volatile u32 *)(addr)))

static void __iomem *TV_base;

struct file_operations iMX51Tv_fops = 
{
    .ioctl   = DRV_TvDevIoctl,
    .open    = DRV_TvDevOpen,
    .release = DRV_TvDevClose
};

static struct miscdevice iMX51TV_dev = {
	MISC_DYNAMIC_MINOR,
	BSP_IMX51_TV_DEVICE_NAME,
	&iMX51Tv_fops,
};

/* Open the device */
static int DRV_TvDevOpen(struct inode *inode, struct file *file)
{
    return 0;
}

/* Close the device */
static int DRV_TvDevClose(struct inode *inode, struct file *file)
{
    return 0;
}

/* Control the device */
static int DRV_TvDevIoctl(struct inode *inode, struct file *file,
                                    unsigned int cmd, unsigned long arg)
{
    unsigned int ulRet = 0;
    unsigned timeout = 200000;
    unsigned int ulIoAddr, data;
    unsigned int TV_MODE;
          
    switch(cmd)
    {
       case BSP_CVBS_MODE_CRTL: 
            ulRet = copy_from_user((void *)&TV_MODE,(void __user *)arg, sizeof(TV_MODE));
            if (0 != ulRet)
            {
                printk("%s: copy_from_user error.\n", __func__);     
                return -1;
            }
            else
            {                             
                uint32 reg = 0;
                if(0 != TV_MODE)
                {
                    /* change to PAL mode */
                    cvbs_flag_pal_ntsc = 1;

                    TV_OUT(TV_ENC_CTL, 0);	/* disable TV encoder */
                    
                    /* Cr gain 11, Cb gain C6, y_gain 97 */
            		TV_OUT(TV_GAIN, 0x0081b697);
            		TV_OUT(TV_CGMS, 0x000af317);
            		TV_OUT(TV_TEST_MUX, 0x000001c3);
            		TV_OUT(TV_TEST_MODE, 0x00000002);
                    
            		/*  PAL Timing */
            		TV_OUT(TV_SYNC_1, 0x0020009e);
            		TV_OUT(TV_SYNC_2, 0x011306b4);
            		TV_OUT(TV_SYNC_3, 0x0006000c);
            		TV_OUT(TV_SYNC_4, 0x0028020D);
            		TV_OUT(TV_SYNC_5, 0x005e02fb);
            		TV_OUT(TV_SYNC_6, 0x0006000c);
            		TV_OUT(TV_SYNC_7, 0x00000012);
            		TV_OUT(TV_BURST_V1, 0x0012020b);
            		TV_OUT(TV_BURST_V2, 0x0016020c);
            		TV_OUT(TV_BURST_V3, 0x00150209);
            		TV_OUT(TV_BURST_V4, 0x0013020c);
            		TV_OUT(TV_BURST_H, 0x00bf010b);
            		TV_OUT(TV_SOL_REQ_ODD, 0x00280208);
            		TV_OUT(TV_SOL_REQ_EVEN, 0x00290209);

            		reg |= TVENC_CTL_TV_MODE_PAL_M;
                    
                    reg |= TVENC_CTL_Y_FILTER_EN |
                        TVENC_CTL_CR_FILTER_EN |
                        TVENC_CTL_CB_FILTER_EN | TVENC_CTL_SINX_FILTER_EN;
             #ifdef CONFIG_FB_MSM_TVOUT_SVIDEO
                    reg |= TVENC_CTL_S_VIDEO_EN;
             #endif
                    
                    TV_OUT(TV_LEVEL, 0x00000000);   /* DC offset to 0. */
                    TV_OUT(TV_OFFSET, 0x008080f0);
                    
             #ifdef CONFIG_FB_MSM_MDP31                                             
                    TV_OUT(TV_DAC_INTF, 0x29);                     
             #endif
                    TV_OUT(TV_ENC_CTL, reg);
                
                    reg |= TVENC_CTL_ENC_EN;

                    /* After delay, then ofen TV encoder */
                    while (--timeout);              
                    TV_OUT(TV_ENC_CTL, reg);
                }
                else
                {                
                    /* change to NTSC mode */
                    cvbs_flag_pal_ntsc = 2;

                    TV_OUT(TV_ENC_CTL, 0);  /* disable TV encoder */
                
                    /* NTSC_M */
                    /* Cr gain 11, Cb gain C6, y_gain 97 */
                    TV_OUT(TV_GAIN, 0x0081B697);
                                    
                    TV_OUT(TV_CGMS, 0x0);
                    /* NTSC Timing */
                    TV_OUT(TV_SYNC_1, 0x0020009e);
                    TV_OUT(TV_SYNC_2, 0x011306B4);
                    TV_OUT(TV_SYNC_3, 0x0006000C);
                    TV_OUT(TV_SYNC_4, 0x0028020D);
                    TV_OUT(TV_SYNC_5, 0x005E02FB);
                    TV_OUT(TV_SYNC_6, 0x0006000C);
                    TV_OUT(TV_SYNC_7, 0x00000012);
                    TV_OUT(TV_BURST_V1, 0x0013020D);
                    TV_OUT(TV_BURST_V2, 0x0014020C);
                    TV_OUT(TV_BURST_V3, 0x0013020D);
                    TV_OUT(TV_BURST_V4, 0x0014020C);
                    TV_OUT(TV_BURST_H, 0x00AE00F2);
                    TV_OUT(TV_SOL_REQ_ODD, 0x00280208);
                    TV_OUT(TV_SOL_REQ_EVEN, 0x00290209);
                
                    reg |= TVENC_CTL_TV_MODE_NTSC_M_PAL60;
                
                    reg |= TVENC_CTL_Y_FILTER_EN |
                        TVENC_CTL_CR_FILTER_EN |
                        TVENC_CTL_CB_FILTER_EN | TVENC_CTL_SINX_FILTER_EN;
             #ifdef CONFIG_FB_MSM_TVOUT_SVIDEO
                    reg |= TVENC_CTL_S_VIDEO_EN;
             #endif
                
                    TV_OUT(TV_LEVEL, 0x00000000);   /* DC offset to 0. */
                    TV_OUT(TV_OFFSET, 0x008080f0);
                
             #ifdef CONFIG_FB_MSM_MDP31                                             
                    TV_OUT(TV_DAC_INTF, 0x29);                      
             #endif
                    TV_OUT(TV_ENC_CTL, reg);
                
                    reg |= TVENC_CTL_ENC_EN;

                    // After delay, then ofen TV encoder
                    while (--timeout);
                    
                    TV_OUT(TV_ENC_CTL, reg);
                }                              
            }
            break;

       case BSP_CVBS_OPEN_CRTL:
           cvbs_flag_pal_ntsc = 1; /* flag that PAM mode */
           ulIoAddr = (unsigned int)((unsigned char *)TV_base);
           data = REG_READ(ulIoAddr);
           data |= TVENC_CTL_ENC_EN; 
           TV_OUT(TV_ENC_CTL, data);
           break;
           

       case BSP_CVBS_CLOSE_CRTL:  
           cvbs_flag_pal_ntsc = 0; /* flag that close the cvbs */
           ulIoAddr = (unsigned int)((unsigned char *)TV_base);
           data = REG_READ(ulIoAddr);
           data &= 0xfffffffb; 
           TV_OUT(TV_ENC_CTL, data);
           break;
                      
       default:
           printk("Error Ioctl operation.\r\n");
           return -1;
           
    }
    return 0;
}

static ssize_t cvbs_attr_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    ssize_t iLen = 0;

    if(1 == cvbs_flag_pal_ntsc){
        iLen += sprintf(buf, "%s", "PALM\n");
    }else if(2 == cvbs_flag_pal_ntsc){
        iLen += sprintf(buf, "%s", "NTSC\n");
    }else if(0 == cvbs_flag_pal_ntsc){
        iLen += sprintf(buf, "%s", "STOP\n");
    }
    
    return iLen;
}

/* Define the device attributes */
 static struct kobj_attribute cvbs_attribute =
     __ATTR(cvbs_mode, 0666, cvbs_attr_show, NULL);

 static struct attribute* cvbs_attributes[] =
 {
     &cvbs_attribute.attr,
     NULL
 };

 static struct attribute_group cvbs_defattr_group =
 {
     .attrs = cvbs_attributes,
 };


static int __init pal_init(void)
{
	int ret;

    //for make sys flag
    struct kobject *kobj = NULL;    
    kobj = kobject_create_and_add("cvbs", NULL);
    if (kobj == NULL) {
        printk(" kobject_create_and_add() return -1  \n");
        return -1;
    }
   
    /* Create the files associated with this kobject */
    if (sysfs_create_group(kobj, &cvbs_defattr_group)) {
        kobject_put(kobj);
        printk(" sysfs_create_group() return -1  \n");
        return -1;
    }
    
    TV_base = ioremap(BSP_TV_ENC_BASE_ADDR, PAGE_SIZE);
    if(NULL == TV_base)
    {
        printk(" ioremap() return -1  \n");
        printk(KERN_ERR "ioremap error\n");
        return -1;
    } 
    
	ret = platform_driver_register(&this_driver);
	if (!ret) {
		ret = platform_device_register(&this_device);
		if (ret)
			platform_driver_unregister(&this_driver);
	}

    ret = misc_register(&iMX51TV_dev);
    if (ret < 0)
    {
        printk("iMX51TV_dev register error.\n");
        return ret;
    }

	return ret;
}

module_init(pal_init);

