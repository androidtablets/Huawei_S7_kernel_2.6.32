/* Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/android_pmem.h>
#include <linux/bootmem.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/mfd/tps65023.h>
#include <linux/bma150.h>
#include <linux/power_supply.h>
#include <linux/clk.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/io.h>
#include <asm/setup.h>

#include <asm/mach/mmc.h>
#include <mach/vreg.h>
#include <mach/mpp.h>
#include <mach/gpio.h>
#include <mach/board.h>
#include <mach/sirc.h>
#include <mach/dma.h>
#include <mach/rpc_hsusb.h>
#include <mach/rpc_pmapp.h>
#include <mach/msm_hsusb.h>
#include <mach/msm_hsusb_hw.h>
#include <mach/msm_serial_hs.h>
#include <mach/msm_touchpad.h>
#include <mach/msm_i2ckbd.h>
#include <mach/pmic.h>
#include <mach/camera.h>
#include <mach/memory.h>
#include <mach/msm_spi.h>
#include <mach/msm_tsif.h>
#include <mach/msm_battery.h>
#include <mach/rpc_server_handset.h>
#include <linux/delay.h>

#include "devices.h"
#include "timer.h"
#include "socinfo.h"
#include "msm-keypad-devices.h"
#include "pm.h"
#include "proc_comm.h"
#include "smd_private.h"
#include <linux/msm_kgsl.h>
#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android.h>
#endif

#if defined(CONFIG_MOUSE_OFN_HID) || defined(CONFIG_MOUSE_OFN_HID_MODULE)
#include <mach/ofn_hid.h> 
#endif

#if defined(CONFIG_SENSORS_AKM8973) || defined(CONFIG_SENSORS_AKM8973_MODULE)
#include <mach/akm8972_board.h>
#endif

#if HUAWEI_HWID_L2(S7, S7201)
#if defined(CONFIG_BATTERY_BQ275X0) ||defined(CONFIG_BATTERY_BQ275X0_MODULE)
#include <linux/i2c/bq275x0_battery.h>
#endif
#endif

#include "mach/msm_gsensor.h"
#include "mach/qsd_bcm4325.h"
#include "mach/msm_touch.h"

#if defined(CONFIG_HUAWEI_INCIDENT_LED)
#include "msm_incident_led.h"
#endif

#if defined(CONFIG_INPUT_PEDESTAL) || defined(CONFIG_INPUT_PEDESTAL_MODULE)
#include <mach/pedestal.h>
#endif

#if defined(CONFIG_DOCK_DET)
#include <linux/input/dock_det.h>
#endif

#if defined(CONFIG_CYPRESS120) || defined(CONFIG_CYPRESS120_MODULE)
#include <mach/cypress120_ts_dev.h>
#endif

#if HUAWEI_HWID_L2(S7, S7201)
#if defined(CONFIG_TOUCHSCREEN_T1320) || defined(CONFIG_TOUCHSCREEN_T1320_MODULE)
#include <linux/t1320.h>
#endif
#endif

#if defined(CONFIG_TOUCHSCREEN_MXT224) || defined(CONFIG_TOUCHSCREEN_MXT224_MODULE)
#include <linux/mxt224.h>
#endif
#if 1
#define DBG(fmt, args...) printk(KERN_INFO "[%s,%d] "fmt"\n", __FUNCTION__, __LINE__, ##args)
#else
#define DBG(fmt, args...) do {} while (0)
#endif

#define I(fmt, args...) printk(KERN_INFO "[%s,%d] "fmt"\n", __FUNCTION__, __LINE__, ##args)

#if HUAWEI_HWID_L2(S7, S7201)
#define GPIO_BS300_WAKEUP	103
#endif

#if HUAWEI_HWID(S70)
#if HWVERID_HIGHER(S70, B)
#define GPIO_BCM4325_WL_WAKE_HOST	38
#ifdef CONFIG_HUAWEI_CDMA_EVDO
#define GPIO_BCM4325_WL_RST_N		81
#else
#define GPIO_BCM4325_WL_RST_N		57
#endif
#elif HWVERID_HIGHER(S70, A)
#define GPIO_BCM4325_WL_WAKE_HOST	38
#define GPIO_BCM4325_WL_RST_N		17
#else
#define GPIO_BCM4325_WL_WAKE_HOST	34
#define GPIO_BCM4325_WL_RST_N		32
#endif

#if HWVERID_HIGHER(S70, T1)
#define GPIO_BCM4325_REG_ON			30
#define GPIO_BCM4325_HOST_WAKE_WL	109
#define GPIO_BCM4325_HOST_WAKE_BT	42
#define GPIO_BCM4325_BT_WAKE_HOST	21
#define GPIO_BCM4325_BT_RST_N		27
#define GPIO_GPS_LNA_EN				35
#define GPIO_GSENSOR_INT1			22
#define TOUCHPAD_SUSPEND 			34
#define TOUCHPAD_IRQ 				38
#endif

#if defined(CONFIG_MOUSE_OFN_HID) || defined(CONFIG_MOUSE_OFN_HID_MODULE)
#if HWVERID_HIGHER(S70, B)
#ifdef CONFIG_HUAWEI_CDMA_EVDO
#define GPIO_OPTNAV_IRQ             77
#else
#define GPIO_OPTNAV_IRQ             58
#endif
#define GPIO_OPTNAV_RESET           104
#define GPIO_OPTNAV_SHUTDOWN        105
#define GPIO_OPTNAV_DOME1           34
#else
#define GPIO_OPTNAV_IRQ             18
#define GPIO_OPTNAV_RESET           19
#define GPIO_OPTNAV_SHUTDOWN        20
#define GPIO_OPTNAV_DOME1           34
#endif
#endif

// add gpion define for compass
#if defined(CONFIG_SENSORS_AKM8973) || defined(CONFIG_SENSORS_AKM8973_MODULE)
#if HWVERID_HIGHER(S70, B)
#define GPIO_COMPASS_IRQ             20
#define GPIO_COMPASS_RESET           18
#else
#error -- No compass driver
#endif
#endif

#if defined(CONFIG_DOCK_DET) || defined(CONFIG_DOCK_DET_MODULE)
#define GPIO_DOCK_DET	(106)
#endif

#if defined(CONFIG_CYPRESS120) || defined(CONFIG_CYPRESS120_MODULE)
#define GPIO_CTP_INT   			(28)
#define GPIO_CTP_POWER   		(101)
#define GPIO_CTP_RESET       	(158)
#define CTP_I2C_ADDR   			(0x26)
#endif

#if defined(CONFIG_GPIO_I2C_ADPT)
#define GPIO_I2C_SCL		98		
#define GPIO_I2C_SDA		100
#endif
#endif

#define MSM_PMEM_SF_SIZE	0x1700000

#define SMEM_SPINLOCK_I2C	"S:6"

#define MSM_PMEM_ADSP_SIZE	0x2B96000
#ifdef CONFIG_MSM_HDMI
#define MSM_FB_SIZE         0x800000  //0x500000  //0x2EE000  // modified for 24bits LCD display
#else
#define MSM_FB_SIZE         0x500000  //0x2EE000  // modified for 24bits LCD display
#endif
#define MSM_AUDIO_SIZE		0x80000
#define MSM_GPU_PHYS_SIZE 	SZ_2M

#ifdef CONFIG_MSM_SOC_REV_A
#define MSM_SMI_BASE		0xE0000000
#else
#define MSM_SMI_BASE		0x00000000
#endif

#define MSM_SHARED_RAM_PHYS	(MSM_SMI_BASE + 0x00100000)

#define MSM_PMEM_SMI_BASE	(MSM_SMI_BASE + 0x02B00000)
#define MSM_PMEM_SMI_SIZE	0x01500000

#define MSM_FB_BASE		MSM_PMEM_SMI_BASE
#define MSM_GPU_PHYS_BASE 	(MSM_FB_BASE + MSM_FB_SIZE)
#define MSM_PMEM_SMIPOOL_BASE	(MSM_GPU_PHYS_BASE + MSM_GPU_PHYS_SIZE)
#define MSM_PMEM_SMIPOOL_SIZE	(MSM_PMEM_SMI_SIZE - MSM_FB_SIZE \
					- MSM_GPU_PHYS_SIZE)

#define PMEM_KERNEL_EBI1_SIZE	0x28000

#define PMIC_VREG_WLAN_LEVEL	2600
#define PMIC_VREG_GP6_LEVEL	2900

#define FPGA_SDCC_STATUS	0x70000280

static inline void mmc_delay(unsigned int ms)
{
	if (ms < 1000 / HZ) {
		cond_resched();
		mdelay(ms);
	} else {
		msleep(ms);
	}
}

#if defined(CONFIG_SMC91X) || defined(CONFIG_SMC91X_MODULE)
static struct resource smc91x_resources[] = {
	[0] = {
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.flags  = IORESOURCE_IRQ,
	},
};
#endif

#ifdef CONFIG_USB_FUNCTION
static struct usb_mass_storage_platform_data usb_mass_storage_pdata = {
	.nluns          = 0x02,
	.buf_size       = 16384,
	.vendor         = "GOOGLE",
	.product        = "Mass storage",
	.release        = 0xffff,
};

static struct platform_device mass_storage_device = {
	.name           = "usb_mass_storage",
	.id             = -1,
	.dev            = {
		.platform_data          = &usb_mass_storage_pdata,
	},
};
#endif

#ifdef CONFIG_USB_ANDROID

#if HUAWEI_HWID(S70)
/* dynamic composition */
static struct usb_composition usb_func_composition[] = {
	{
		/* MSC */
		.product_id         = USBPID_HUAWEI_S7_MSC,
		.functions	    = 0x02,
		.adb_product_id     = USBPID_HUAWEI_S7_MSC_ADB,
		.adb_functions	    = 0x12
	},
#ifdef CONFIG_USB_F_SERIAL
	{
		/* MODEM */
		.product_id         = USBPID_HUAWEI_S7_GMDM,
		.functions	    = 0x06,
		.adb_product_id     = USBPID_HUAWEI_S7_GMDM_ADB,
		.adb_functions	    = 0x16,
	},
#endif
#ifdef CONFIG_USB_ANDROID_DIAG
	{
		/* DIAG */
		.product_id         = USBPID_HUAWEI_S7_DIAG_MSC,
		.functions	    = 0x24,
		.adb_product_id     = USBPID_HUAWEI_S7_DIAG_ADB_MSC,
		.adb_functions	    = 0x214,
	},
#endif
#if defined(CONFIG_USB_ANDROID_DIAG) && defined(CONFIG_USB_F_SERIAL)
	{
		/* DIAG + MODEM */
		.product_id         = USBPID_HUAWEI_S7_DIAG_GMDM,
		.functions	    = 0x64,
		.adb_product_id     = USBPID_HUAWEI_S7_DIAG_ADB_GMDM,
		.adb_functions	    = 0x0614,
	},
	{
		/* DIAG + MODEM + NMEA*/
		.product_id         = USBPID_HUAWEI_S7_DIAG_GMDM_GNMEA,
		.functions	    = 0x764,
		.adb_product_id     = USBPID_HUAWEI_S7_DIAG_ADB_GMDM_GNMEA,
		.adb_functions	    = 0x7614,
	},
	{
		/* DIAG + MODEM + NMEA + MSC */
		.product_id         = USBPID_HUAWEI_S7_DIAG_GMDM_GNMEA_MSC,
		.functions	    = 0x2764,
		.adb_product_id     = USBPID_HUAWEI_S7_DIAG_ADB_GMDM_GNMEA_MSC,
		.adb_functions	    = 0x27614,
	},
#endif
#ifdef CONFIG_USB_ANDROID_CDC_ECM
	{
		/* MSC + CDC-ECM */
		.product_id         = USBPID_HUAWEI_S7_MSC_CDCECM,
		.functions	    = 0x82,
		.adb_product_id     = USBPID_HUAWEI_S7_MSC_ADB_CDCECM,
		.adb_functions	    = 0x812,
	},
#endif
#ifdef CONFIG_USB_ANDROID_RMNET
	{
		/* DIAG + RMNET */
		.product_id         = USBPID_HUAWEI_S7_DIAG_RMNET,
		.functions	    = 0x94,
		.adb_product_id     = USBPID_HUAWEI_S7_DIAG_ADB_RMNET,
		.adb_functions	    = 0x914,
	},
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	{
		/* RNDIS */
		.product_id         = USBPID_HUAWEI_S7_RNDIS_MSC_DIAG,
		.functions	    = 0x42A,
		.adb_product_id     = USBPID_HUAWEI_S7_RNDIS_ADB_MSC_DIAG,
		.adb_functions	    = 0x421A,
	},
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	{
		/* RNDIS */
		.product_id         = USBPID_HUAWEI_S7_RNDIS,
		.functions	    =0x2A,
		.adb_product_id     = USBPID_HUAWEI_S7_ADB_RNDIS,
		.adb_functions	    =0x21A,
	},
#endif
};
#else
/* dynamic composition */
static struct usb_composition usb_func_composition[] = {
	{
		/* MSC */
		.product_id         = 0xF000,
		.functions	    = 0x02,
		.adb_product_id     = 0x9015,
		.adb_functions	    = 0x12
	},
#ifdef CONFIG_USB_F_SERIAL
	{
		/* MODEM */
		.product_id         = 0xF00B,
		.functions	    = 0x06,
		.adb_product_id     = 0x901E,
		.adb_functions	    = 0x16,
	},
#endif
#ifdef CONFIG_USB_ANDROID_DIAG
	{
		/* DIAG */
		.product_id         = 0x900E,
		.functions	    = 0x04,
		.adb_product_id     = 0x901D,
		.adb_functions	    = 0x14,
	},
#endif
#if defined(CONFIG_USB_ANDROID_DIAG) && defined(CONFIG_USB_F_SERIAL)
	{
		/* DIAG + MODEM */
		.product_id         = 0x9004,
		.functions	    = 0x64,
		.adb_product_id     = 0x901F,
		.adb_functions	    = 0x0614,
	},
	{
		/* DIAG + MODEM + NMEA*/
		.product_id         = 0x9016,
		.functions	    = 0x764,
		.adb_product_id     = 0x9020,
		.adb_functions	    = 0x7614,
	},
	{
		/* DIAG + MODEM + NMEA + MSC */
		.product_id         = 0x9017,
		.functions	    = 0x2764,
		.adb_product_id     = 0x9018,
		.adb_functions	    = 0x27614,
	},
#endif
#ifdef CONFIG_USB_ANDROID_CDC_ECM
	{
		/* MSC + CDC-ECM */
		.product_id         = 0x9014,
		.functions	    = 0x82,
		.adb_product_id     = 0x9023,
		.adb_functions	    = 0x812,
	},
#endif
#ifdef CONFIG_USB_ANDROID_RMNET
	{
		/* DIAG + RMNET */
		.product_id         = 0x9021,
		.functions	    = 0x94,
		.adb_product_id     = 0x9022,
		.adb_functions	    = 0x914,
	},
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	{
		/* RNDIS */
		.product_id         = 0xF00E,
		.functions	    = 0xA,
		.adb_product_id     = 0x9024,
		.adb_functions	    = 0x1A,
	},
#endif
};
#endif

static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 2,
#if 0
	.vendor		= "GOOGLE",
	.product	= "Mass Storage",
#else 
	.vendor		= "Huawei",
	.product	= "Ideos Tablet",
#endif	
	.release	= 0xFFFF,
};
static struct platform_device mass_storage_device = {
	.name           = "usb_mass_storage",
	.id             = -1,
	.dev            = {
		.platform_data          = &mass_storage_pdata,
	},
};

static struct android_usb_platform_data android_usb_pdata = {
#if HUAWEI_HWID(S70)
	.vendor_id	= USBVID_HUAWEI,
#else
	.vendor_id	= 0x05C6,
#endif
	.version	= 0x0100,
	.compositions   = usb_func_composition,
	.num_compositions = ARRAY_SIZE(usb_func_composition),
#if HUAWEI_HWID(S70)
	.product_name	= "Ideos Tablet",
	.manufacturer_name = "Huawei Incorporated",

#ifdef HUAWEI_MSC_VENDOR_PRODUCT
	.msc_vendor = "HUAWEI",  // NOTICE: 8 characters limit in Win XP
	.msc_product = "Ideos Tablet",  // NOTICE: 16 characters limit in Wind XP
#endif
#else
	.product_name	= "Qualcomm HSUSB Device",
	.manufacturer_name = "Qualcomm Incorporated",
	.nluns = 1,
#endif
};

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};
#endif

#if defined(CONFIG_SMC91X) || defined(CONFIG_SMC91X_MODULE)
static struct platform_device smc91x_device = {
	.name           = "smc91x",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(smc91x_resources),
	.resource       = smc91x_resources,
};
#endif


#ifdef CONFIG_USB_FUNCTION
static struct usb_function_map usb_functions_map[] = {
	{"diag", 0},
	{"adb", 1},
	{"modem", 2},
	{"nmea", 3},
	{"mass_storage", 4},
	{"ethernet", 5},
#if defined(CONFIG_USB_FUNCTION_SERIAL_CONSOLE)
	{"usbtty",6},				
#endif
};

/* dynamic composition */
#if !HUAWEI_HWID(S70)
static struct usb_composition usb_func_composition[] = {
	{
		.product_id         = 0x9012,
		.functions	    = 0x5, /* 0101 */
	},

	{
		.product_id         = 0x9013,
		.functions	    = 0x15, /* 10101 */
	},

	{
		.product_id         = 0x9014,
		.functions	    = 0x30, /* 110000 */
	},

	{
		.product_id         = 0x9015,
		.functions	    = 0x12, /* 10010 */
	},

	{
		.product_id         = 0x9016,
		.functions	    = 0xD, /* 01101 */
	},

	{
		.product_id         = 0x9017,
		.functions	    = 0x1D, /* 11101 */
	},

	{
		.product_id         = 0xF000,
		.functions	    = 0x10, /* 10000 */
	},

	{
		.product_id         = 0xF009,
		.functions	    = 0x20, /* 100000 */
	},

	{
		.product_id         = 0x9018,
		.functions	    = 0x1F, /* 011111 */
	},

	{
		.product_id         = 0x901A,
		.functions	    = 0x0F, /* 01111 */
	},
};
#else
static struct usb_composition usb_func_composition[] = {
	{
		.product_id         = 0x9012,
		.functions	    = 0x5, /* 0101 */
	},

	{
		.product_id         = 0x9013,
		.functions	    = 0x15, /* 10101 */
	},

	{
		.product_id         = 0x9014,
		.functions	    = 0x30, /* 110000 */
	},

	{
		.product_id         = 0x9015,
		.functions	    = 0x12, /* 10010 */
	},

	{
		.product_id         = 0x9016,
		.functions	    = 0xD, /* 01101 */
	},

	{
		.product_id         = 0x9017,
		.functions	    = 0x1D, /* 11101 */
	},

	{
		.product_id         = 0xF000,
		.functions	    = 0x10, /* 10000 */
	},

	{
		.product_id         = 0xF009,
		.functions	    = 0x20, /* 100000 */
	},

	{
		.product_id         = USBPID_HUAWEI_S7,
		.functions	    = 0x1F, /* 011111 */
	},

	{
		.product_id         = 0x901A,
		.functions	    = 0x0F, /* 01111 */
	},
};
#endif
#endif

static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "s7_handset",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_device = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

#ifdef CONFIG_HUAWEI_MSM_VIBRATOR 
static struct platform_device vibrator_device = {
	.name		= "time_vibrator",
	.id		= -1,
};

static int init_vibrator_device(void)
{
   return platform_device_register(&vibrator_device);
}
#endif

#ifdef CONFIG_USB_FS_HOST
static struct msm_gpio fsusb_config[] = {
	{ GPIO_CFG(139, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), "fs_dat" },
	{ GPIO_CFG(140, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), "fs_se0" },
	{ GPIO_CFG(141, 3, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), "fs_oe_n" },
};

static int fsusb_gpio_init(void)
{
	return msm_gpios_request(fsusb_config, ARRAY_SIZE(fsusb_config));
}

static void msm_fsusb_setup_gpio(unsigned int enable)
{
	if (enable)
		msm_gpios_enable(fsusb_config, ARRAY_SIZE(fsusb_config));
	else
		msm_gpios_disable(fsusb_config, ARRAY_SIZE(fsusb_config));

}
#endif

#define MSM_USB_BASE              ((unsigned)addr)

static struct msm_hsusb_platform_data msm_hsusb_pdata = {
#ifdef CONFIG_USB_FUNCTION
	.version	= 0x0100,
	.phy_info	= (USB_PHY_INTEGRATED | USB_PHY_MODEL_180NM),
#if HUAWEI_HWID(S70)
	.vendor_id			= USBVID_HUAWEI,
	.product_name       = "S70",
	.serial_number      = "1234567890ABCDEF",
	.manufacturer_name  = "HUAWEI",
#else
	.vendor_id          = 0x5c6,
	.product_name       = "Qualcomm HSUSB Device",
	.serial_number      = "1234567890ABCDEF",
	.manufacturer_name  = "Qualcomm Incorporated",
#endif
	.compositions	= usb_func_composition,
	.num_compositions = ARRAY_SIZE(usb_func_composition),
	.function_map   = usb_functions_map,
	.num_functions	= ARRAY_SIZE(usb_functions_map),
	.config_gpio    = NULL,

#endif
};

static struct vreg *vreg_usb;
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{

	switch (PHY_TYPE(phy_info)) {
	case USB_PHY_INTEGRATED:
		if (on)
			msm_hsusb_vbus_powerup();
		else
			msm_hsusb_vbus_shutdown();
		break;
	case USB_PHY_SERIAL_PMIC:
		if (on)
			vreg_enable(vreg_usb);
		else
			vreg_disable(vreg_usb);
		break;
	default:
		pr_err("%s: undefined phy type ( %X ) \n", __func__,
						phy_info);
	}

}

static struct msm_usb_host_platform_data msm_usb_host_pdata = {
	.phy_info	= (USB_PHY_INTEGRATED | USB_PHY_MODEL_180NM),
	.vbus_power = msm_hsusb_vbus_power,
};

#ifdef CONFIG_USB_FS_HOST
static struct msm_usb_host_platform_data msm_usb_host2_pdata = {
	.phy_info	= USB_PHY_SERIAL_PMIC,
	.config_gpio = msm_fsusb_setup_gpio,
	.vbus_power = msm_hsusb_vbus_power,
};
#endif

static struct android_pmem_platform_data android_pmem_kernel_ebi1_pdata = {
	.name = PMEM_KERNEL_EBI1_DATA_NAME,
	/* if no allocator_type, defaults to PMEM_ALLOCATORTYPE_BITMAP,
	 * the only valid choice at this time. The board structure is
	 * set to all zeros by the C runtime initialization and that is now
	 * the enum value of PMEM_ALLOCATORTYPE_BITMAP, now forced to 0 in
	 * include/linux/android_pmem.h.
	 */
	.cached = 0,
};

#ifdef CONFIG_KERNEL_PMEM_SMI_REGION

static struct android_pmem_platform_data android_pmem_kernel_smi_pdata = {
	.name = PMEM_KERNEL_SMI_DATA_NAME,
	/* if no allocator_type, defaults to PMEM_ALLOCATORTYPE_BITMAP,
	 * the only valid choice at this time. The board structure is
	 * set to all zeros by the C runtime initialization and that is now
	 * the enum value of PMEM_ALLOCATORTYPE_BITMAP, now forced to 0 in
	 * include/linux/android_pmem.h.
	 */
	.cached = 0,
};

#endif

static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 1,
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};

static struct android_pmem_platform_data android_pmem_smipool_pdata = {
	.name = "pmem_smipool",
	.start = MSM_PMEM_SMIPOOL_BASE,
	.size = MSM_PMEM_SMIPOOL_SIZE,
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};


static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &android_pmem_pdata },
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

static struct platform_device android_pmem_smipool_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_smipool_pdata },
};


static struct platform_device android_pmem_kernel_ebi1_device = {
	.name = "android_pmem",
	.id = 3,
	.dev = { .platform_data = &android_pmem_kernel_ebi1_pdata },
};

#ifdef CONFIG_KERNEL_PMEM_SMI_REGION
static struct platform_device android_pmem_kernel_smi_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_kernel_smi_pdata },
};
#endif

static struct resource msm_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

static int msm_fb_detect_panel(const char *name)
{
	int ret = -EPERM;

	if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa()) {
		if (!strncmp(name, "mddi_toshiba_wvga_pt", 20))
			ret = 0;
		else
			ret = -ENODEV;
	} else if ((machine_is_qsd8x50_surf() || machine_is_qsd8x50a_surf())
			&& !strcmp(name, "lcdc_external"))
		ret = 0;

	return ret;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_fb_resources),
	.resource       = msm_fb_resources,
	.dev    = {
		.platform_data = &msm_fb_pdata,
	}
};


#if defined(CONFIG_SPI_QSD) || defined(CONFIG_SPI_QSD_MODULE)
static struct msm_gpio bma_spi_gpio_config_data[] = {
	{ GPIO_CFG(22, 0, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA), "bma_irq" },
};

static int msm_bma_gpio_setup(struct device *dev)
{
	int rc;

	rc = msm_gpios_request_enable(bma_spi_gpio_config_data,
		ARRAY_SIZE(bma_spi_gpio_config_data));

	return rc;
}

static void msm_bma_gpio_teardown(struct device *dev)
{
	msm_gpios_disable_free(bma_spi_gpio_config_data,
		ARRAY_SIZE(bma_spi_gpio_config_data));
}

static struct bma150_platform_data bma_pdata = {
	.setup    = msm_bma_gpio_setup,
	.teardown = msm_bma_gpio_teardown,
};

static struct resource qsd_spi_resources[] = {
	{
		.name   = "spi_irq_in",
		.start	= INT_SPI_INPUT,
		.end	= INT_SPI_INPUT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_irq_out",
		.start	= INT_SPI_OUTPUT,
		.end	= INT_SPI_OUTPUT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_irq_err",
		.start	= INT_SPI_ERROR,
		.end	= INT_SPI_ERROR,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.name   = "spi_base",
		.start	= 0xA1200000,
		.end	= 0xA1200000 + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.name   = "spidm_channels",
		.flags  = IORESOURCE_DMA,
	},
	{
		.name   = "spidm_crci",
		.flags  = IORESOURCE_DMA,
	},
};

static struct platform_device qsd_device_spi = {
	.name	        = "spi_qsd",
	.id	        = 0,
	.num_resources	= ARRAY_SIZE(qsd_spi_resources),
	.resource	= qsd_spi_resources,
};

static struct spi_board_info msm_spi_board_info[] __initdata = {
	{
		.modalias	= "bma150",
		.mode		= SPI_MODE_3,
		.irq		= MSM_GPIO_TO_INT(22),
		.bus_num	= 0,
		.chip_select	= 0,
		.max_speed_hz	= 10000000,
		.platform_data	= &bma_pdata,
	},
};

#define CT_CSR_PHYS		0xA8700000
#define TCSR_SPI_MUX		(ct_csr_base + 0x54)
static int msm_qsd_spi_dma_config(void)
{
	void __iomem *ct_csr_base = 0;
	u32 spi_mux;
	int ret = 0;

	ct_csr_base = ioremap(CT_CSR_PHYS, PAGE_SIZE);
	if (!ct_csr_base) {
		pr_err("%s: Could not remap %x\n", __func__, CT_CSR_PHYS);
		return -1;
	}

	spi_mux = readl(TCSR_SPI_MUX);
	switch (spi_mux) {
	case (1):
		qsd_spi_resources[4].start  = DMOV_HSUART1_RX_CHAN;
		qsd_spi_resources[4].end    = DMOV_HSUART1_TX_CHAN;
		qsd_spi_resources[5].start  = DMOV_HSUART1_RX_CRCI;
		qsd_spi_resources[5].end    = DMOV_HSUART1_TX_CRCI;
		break;
	case (2):
		qsd_spi_resources[4].start  = DMOV_HSUART2_RX_CHAN;
		qsd_spi_resources[4].end    = DMOV_HSUART2_TX_CHAN;
		qsd_spi_resources[5].start  = DMOV_HSUART2_RX_CRCI;
		qsd_spi_resources[5].end    = DMOV_HSUART2_TX_CRCI;
		break;
	case (3):
		qsd_spi_resources[4].start  = DMOV_CE_OUT_CHAN;
		qsd_spi_resources[4].end    = DMOV_CE_IN_CHAN;
		qsd_spi_resources[5].start  = DMOV_CE_OUT_CRCI;
		qsd_spi_resources[5].end    = DMOV_CE_IN_CRCI;
		break;
	default:
		ret = -1;
	}

	iounmap(ct_csr_base);
	return ret;
}

static struct msm_gpio qsd_spi_gpio_config_data[] = {
	{ GPIO_CFG(17, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA), "spi_clk" },
	{ GPIO_CFG(18, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA), "spi_mosi" },
	{ GPIO_CFG(19, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA), "spi_miso" },
	{ GPIO_CFG(20, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA), "spi_cs0" },
	{ GPIO_CFG(21, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_16MA), "spi_pwr" },
};

static int msm_qsd_spi_gpio_config(void)
{
	int rc;

	rc = msm_gpios_request_enable(qsd_spi_gpio_config_data,
		ARRAY_SIZE(qsd_spi_gpio_config_data));
	if (rc)
		return rc;

	/* Set direction for SPI_PWR */
	gpio_direction_output(21, 1);

	return 0;
}

static void msm_qsd_spi_gpio_release(void)
{
	msm_gpios_disable_free(qsd_spi_gpio_config_data,
		ARRAY_SIZE(qsd_spi_gpio_config_data));
}

static struct msm_spi_platform_data qsd_spi_pdata = {
	.max_clock_speed = 19200000,
	.clk_name = "spi_clk",
	.gpio_config  = msm_qsd_spi_gpio_config,
	.gpio_release = msm_qsd_spi_gpio_release,
	.dma_config = msm_qsd_spi_dma_config,
};

static void __init msm_qsd_spi_init(void)
{
	qsd_device_spi.dev.platform_data = &qsd_spi_pdata;
}
#endif

static int mddi_toshiba_pmic_bl(int level)
{
	int ret = -EPERM;

	if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa()) {
		ret = pmic_set_led_intensity(LED_LCD, level);

		if (ret)
			printk(KERN_WARNING "%s: can't set lcd backlight!\n",
						__func__);
	}

	return ret;
}

static struct msm_panel_common_pdata mddi_toshiba_pdata = {
	.pmic_backlight = mddi_toshiba_pmic_bl,
};

static struct platform_device mddi_toshiba_device = {
	.name   = "mddi_toshiba",
	.id     = 0,
	.dev    = {
		.platform_data = &mddi_toshiba_pdata,
	}
};

static void msm_fb_vreg_config(const char *name, int on)
{
	struct vreg *vreg;
	int ret = 0;

	vreg = vreg_get(NULL, name);
	if (IS_ERR(vreg)) {
		printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
		__func__, name, PTR_ERR(vreg));
		return;
	}

	ret = (on) ? vreg_enable(vreg) : vreg_disable(vreg);
	if (ret)
		printk(KERN_ERR "%s: %s(%s) failed!\n",
			__func__, (on) ? "vreg_enable" : "vreg_disable", name);
}

#define MDDI_RST_OUT_GPIO 100

static int mddi_power_save_on;
static int msm_fb_mddi_power_save(int on)
{
	int flag_on = !!on;
	int ret = 0;


	if (mddi_power_save_on == flag_on)
		return ret;

	mddi_power_save_on = flag_on;

	if (!flag_on && (machine_is_qsd8x50_ffa()
				|| machine_is_qsd8x50a_ffa())) {
		gpio_set_value(MDDI_RST_OUT_GPIO, 0);
		mdelay(1);
	}

	ret = pmic_lp_mode_control(flag_on ? OFF_CMD : ON_CMD,
		PM_VREG_LP_MSME2_ID);
	if (ret)
		printk(KERN_ERR "%s: pmic_lp_mode_control failed!\n", __func__);

	msm_fb_vreg_config("gp5", flag_on);
	msm_fb_vreg_config("boost", flag_on);

	if (flag_on && (machine_is_qsd8x50_ffa()
			|| machine_is_qsd8x50a_ffa())) {
		gpio_set_value(MDDI_RST_OUT_GPIO, 0);
		mdelay(1);
		gpio_set_value(MDDI_RST_OUT_GPIO, 1);
		gpio_set_value(MDDI_RST_OUT_GPIO, 1);
		mdelay(1);
	}

	return ret;
}

static int msm_fb_mddi_sel_clk(u32 *clk_rate)
{
	*clk_rate *= 2;
	return 0;
}

static struct mddi_platform_data mddi_pdata = {
	.mddi_power_save = msm_fb_mddi_power_save,
	.mddi_sel_clk = msm_fb_mddi_sel_clk,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 98,
};

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("pmdh", &mddi_pdata);
	msm_fb_register_device("emdh", &mddi_pdata);
	msm_fb_register_device("tvenc", 0);
	msm_fb_register_device("lcdc", 0);
}

static struct resource msm_audio_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	},
	{
		.name   = "aux_pcm_dout",
		.start  = 68,
		.end    = 68,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_din",
		.start  = 69,
		.end    = 69,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_syncout",
		.start  = 70,
		.end    = 70,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "aux_pcm_clkin_a",
		.start  = 71,
		.end    = 71,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "sdac_din",
		.start  = 144,
		.end    = 144,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "sdac_dout",
		.start  = 145,
		.end    = 145,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "sdac_wsout",
		.start  = 143,
		.end    = 143,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "cc_i2s_clk",
		.start  = 142,
		.end    = 142,
		.flags  = IORESOURCE_IO,
	},
	{
		.name   = "audio_master_clkout",
		.start  = 146,
		.end    = 146,
		.flags  = IORESOURCE_IO,
	},
	{
		.name	= "audio_base_addr",
		.start	= 0xa0700000,
		.end	= 0xa0700000 + 4,
		.flags	= IORESOURCE_MEM,
	},

};

static unsigned audio_gpio_on[] = {
	GPIO_CFG(68, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_DOUT */
	GPIO_CFG(69, 1, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),	/* PCM_DIN */
	GPIO_CFG(70, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_SYNC */
	GPIO_CFG(71, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* PCM_CLK */
	GPIO_CFG(142, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* CC_I2S_CLK */
	GPIO_CFG(143, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* SADC_WSOUT */
#if !HUAWEI_HWID_L1(S7)
	GPIO_CFG(144, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),	/* SADC_DIN */
#endif
	GPIO_CFG(145, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* SDAC_DOUT */
	GPIO_CFG(146, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),	/* MA_CLK_OUT */
};

static void __init audio_gpio_init(void)
{
	int pin, rc;

	for (pin = 0; pin < ARRAY_SIZE(audio_gpio_on); pin++) {
		rc = gpio_tlmm_config(audio_gpio_on[pin],
			GPIO_ENABLE);
		if (rc) {
			printk(KERN_ERR
				"%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, audio_gpio_on[pin], rc);
			return;
		}
	}
}

static struct platform_device msm_audio_device = {
	.name   = "msm_audio",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(msm_audio_resources),
	.resource       = msm_audio_resources,
};

#if HWVERID_HIGHER(S70, T1)
static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= GPIO_BCM4325_BT_WAKE_HOST,
		.end	= GPIO_BCM4325_BT_WAKE_HOST,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= GPIO_BCM4325_HOST_WAKE_BT,
		.end	= GPIO_BCM4325_HOST_WAKE_BT,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "host_wake",
		.start	= MSM_GPIO_TO_INT(GPIO_BCM4325_BT_WAKE_HOST),
		.end	= MSM_GPIO_TO_INT(GPIO_BCM4325_BT_WAKE_HOST),
		.flags	= IORESOURCE_IRQ,
	},
};
#else
static struct resource bluesleep_resources[] = {
	{
		.name	= "gpio_host_wake",
		.start	= 21,
		.end	= 21,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "gpio_ext_wake",
		.start	= 19,
		.end	= 19,
		.flags	= IORESOURCE_IO,
	},
	{
		.name	= "host_wake",
		.start	= MSM_GPIO_TO_INT(21),
		.end	= MSM_GPIO_TO_INT(21),
		.flags	= IORESOURCE_IRQ,
	},
};
#endif

static struct platform_device msm_bluesleep_device = {
	.name = "bluesleep",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(bluesleep_resources),
	.resource	= bluesleep_resources,
};

#ifdef CONFIG_BT
static struct platform_device msm_bt_power_device = {
	.name = "bt_power",
};

enum {
	BT_SYSRST,
	BT_WAKE,
	BT_HOST_WAKE,
	BT_VDD_IO,
	BT_RFR,
	BT_CTS,
	BT_RX,
	BT_TX,
	BT_VDD_FREG
};

#if HWVERID_HIGHER(S70, T1)
static struct msm_gpio bt_config_power_off[] = {
	{ GPIO_CFG(GPIO_BCM4325_BT_RST_N, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"BT SYSRST" },
	{ GPIO_CFG(GPIO_BCM4325_BT_WAKE_HOST, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"HOST WAKE" },
	{ GPIO_CFG(GPIO_BCM4325_HOST_WAKE_BT, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"BT WAKE" },
	{ GPIO_CFG(43, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"UART1DM_RFR" },
	{ GPIO_CFG(44, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"UART1DM_CTS" },
	{ GPIO_CFG(45, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"UART1DM_RX" },
	{ GPIO_CFG(46, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"UART1DM_TX" }
};

static struct msm_gpio bt_config_power_on[] = {
	{ GPIO_CFG(GPIO_BCM4325_BT_RST_N, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
		"BT SYSRST" },
	{ GPIO_CFG(GPIO_BCM4325_BT_WAKE_HOST, 0, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),
		"HOST WAKE" },
	{ GPIO_CFG(GPIO_BCM4325_HOST_WAKE_BT, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
		"BT WAKE" },
	{ GPIO_CFG(43, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
		"UART1DM_RFR" },
	{ GPIO_CFG(44, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),
		"UART1DM_CTS" },
	{ GPIO_CFG(45, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),
		"UART1DM_RX" },
	{ GPIO_CFG(46, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
		"UART1DM_TX" }
};

static int bluetooth_power(int on)
{
	int rc;
	unsigned mpp_bt_reg_on;

#if HWVERID_HIGHER(S70, A)
	mpp_bt_reg_on = 6 - 1;
#else
    mpp_bt_reg_on = 4 - 1;
#endif
    if (!mpp_bt_reg_on) 
		printk(KERN_ERR "%s: get BT_REG_ON failed \n",__func__);

	if (on) {
		rc = msm_gpios_enable(bt_config_power_on,
					ARRAY_SIZE(bt_config_power_on));
		if (rc < 0) {
			printk(KERN_ERR
				"%s: bt power on gpio config failed: %d\n",
				__func__, rc);
			return rc;
		}

		/* BT PWR ON and Reset*/
		rc = mpp_config_digital_out(mpp_bt_reg_on,
				     	MPP_CFG(MPP_DLOGIC_LVL_MSMP,
				     	MPP_DLOGIC_OUT_CTRL_HIGH));
		if (rc) {
			printk(KERN_ERR "%s: config BT_REG_ON failed\n", __func__);
			return rc;
		}

		mmc_delay(100);

		/*gpio 27 is  BT_PWR_ON. For BT Reset*/
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_BCM4325_BT_RST_N, 0, GPIO_OUTPUT, 
					GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		if (rc)
			printk(KERN_ERR "%s: config BT_RESET failed\n", __func__);

		gpio_set_value(GPIO_BCM4325_BT_RST_N, 1);
		/* reset again */
		mmc_delay(100);
		gpio_set_value(GPIO_BCM4325_BT_RST_N, 0);
		mmc_delay(100);
		gpio_set_value(GPIO_BCM4325_BT_RST_N, 1);

	} else {
		rc = msm_gpios_enable(bt_config_power_off,
					ARRAY_SIZE(bt_config_power_off));
		if (rc < 0) {
			printk(KERN_ERR
				"%s: bt power off gpio config failed: %d\n",
				__func__, rc);
			return rc;
		}

		/*gpio 27 is  BT_PWR_ON. For BT Reset*/
		gpio_tlmm_config(GPIO_CFG(GPIO_BCM4325_BT_RST_N, 0, GPIO_OUTPUT, 
					GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		gpio_set_value(GPIO_BCM4325_BT_RST_N, 0);

		rc = mpp_config_digital_out(mpp_bt_reg_on,
				     	MPP_CFG(MPP_DLOGIC_LVL_MSMP,
				     	MPP_DLOGIC_OUT_CTRL_LOW));
		if (rc)
	     		printk(KERN_ERR "%s: config BT_REG_ON failed: %d\n",__func__, rc);
	}

	printk(KERN_DEBUG "Bluetooth power switch: %d\n", on);

	return 0;
}

static void __init bt_power_init(void)
{
	int rc;
    
	if ((rc = gpio_request(GPIO_BCM4325_BT_RST_N, "bcm4325_wl_rstn"))) {
		pr_err("gpio_request failed on pin %d\n", GPIO_BCM4325_BT_RST_N);
		goto exit;
	}

	if (bluetooth_power(0)) {
		gpio_free(GPIO_BCM4325_BT_RST_N);
		goto exit;
    }

	msm_bt_power_device.dev.platform_data = &bluetooth_power;

	printk(KERN_DEBUG "Bluetooth power switch: initialized\n");

exit:
	return;
}

#else

static struct msm_gpio bt_config_power_off[] = {
	{ GPIO_CFG(18, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"BT SYSRST" },
	{ GPIO_CFG(19, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"BT WAKE" },
	{ GPIO_CFG(21, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"HOST WAKE" },
	{ GPIO_CFG(22, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"BT VDD_IO" },
	{ GPIO_CFG(43, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"UART1DM_RFR" },
	{ GPIO_CFG(44, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"UART1DM_CTS" },
	{ GPIO_CFG(45, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"UART1DM_RX" },
	{ GPIO_CFG(46, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"UART1DM_TX" }
};

static struct msm_gpio bt_config_power_on[] = {
	{ GPIO_CFG(18, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
		"BT SYSRST" },
	{ GPIO_CFG(19, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
		"BT WAKE" },
	{ GPIO_CFG(21, 0, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),
		"HOST WAKE" },
	{ GPIO_CFG(22, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
		"BT VDD_IO" },
	{ GPIO_CFG(43, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
		"UART1DM_RFR" },
	{ GPIO_CFG(44, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),
		"UART1DM_CTS" },
	{ GPIO_CFG(45, 2, GPIO_INPUT,  GPIO_NO_PULL, GPIO_2MA),
		"UART1DM_RX" },
	{ GPIO_CFG(46, 2, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
		"UART1DM_TX" }
};

static struct msm_gpio wlan_config_power_off[] = {
	{ GPIO_CFG(62, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"SDC2_CLK" },
	{ GPIO_CFG(63, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"SDC2_CMD" },
	{ GPIO_CFG(64, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"SDC2_D3" },
	{ GPIO_CFG(65, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"SDC2_D2" },
	{ GPIO_CFG(66, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"SDC2_D1" },
	{ GPIO_CFG(67, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"SDC2_D0" },
	{ GPIO_CFG(113, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"VDD_WLAN" },
	{ GPIO_CFG(138, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),
		"WLAN_PWD" }
};

static struct msm_gpio wlan_config_power_on[] = {
	{ GPIO_CFG(62, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
		"SDC2_CLK" },
	{ GPIO_CFG(63, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),
		"SDC2_CMD" },
	{ GPIO_CFG(64, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),
		"SDC2_D3" },
	{ GPIO_CFG(65, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),
		"SDC2_D2" },
	{ GPIO_CFG(66, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),
		"SDC2_D1" },
	{ GPIO_CFG(67, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),
		"SDC2_D0" },
	{ GPIO_CFG(113, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
		"VDD_WLAN" },
	{ GPIO_CFG(138, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA),
		"WLAN_PWD" }
};

static int bluetooth_power(int on)
{
	int rc;
	struct vreg *vreg_wlan;

	vreg_wlan = vreg_get(NULL, "wlan");

	if (IS_ERR(vreg_wlan)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_wlan));
		return PTR_ERR(vreg_wlan);
	}

	if (on) {
		/* units of mV, steps of 50 mV */
		rc = vreg_set_level(vreg_wlan, PMIC_VREG_WLAN_LEVEL);
		if (rc) {
			printk(KERN_ERR "%s: vreg wlan set level failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}
		rc = vreg_enable(vreg_wlan);
		if (rc) {
			printk(KERN_ERR "%s: vreg wlan enable failed (%d)\n",
			       __func__, rc);
			return -EIO;
		}

		rc = msm_gpios_enable(bt_config_power_on,
					ARRAY_SIZE(bt_config_power_on));
		if (rc < 0) {
			printk(KERN_ERR
				"%s: bt power on gpio config failed: %d\n",
				__func__, rc);
			return rc;
		}

		if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa()) {
			rc = msm_gpios_enable
					(wlan_config_power_on,
					 ARRAY_SIZE(wlan_config_power_on));
			if (rc < 0) {
				printk
				 (KERN_ERR
				 "%s: wlan power on gpio config failed: %d\n",
					__func__, rc);
				return rc;
			}
		}

		gpio_set_value(22, on); /* VDD_IO */
		gpio_set_value(18, on); /* SYSRST */

		if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa()) {
			gpio_set_value(138, 0); /* WLAN: CHIP_PWD */
			gpio_set_value(113, on); /* WLAN */
		}
	} else {
		if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa()) {
			gpio_set_value(138, on); /* WLAN: CHIP_PWD */
			gpio_set_value(113, on); /* WLAN */
		}

		gpio_set_value(18, on); /* SYSRST */
		gpio_set_value(22, on); /* VDD_IO */

		rc = vreg_disable(vreg_wlan);
		if (rc) {
			printk(KERN_ERR "%s: vreg wlan disable failed (%d)\n",
					__func__, rc);
			return -EIO;
		}

		rc = msm_gpios_enable(bt_config_power_off,
					ARRAY_SIZE(bt_config_power_off));
		if (rc < 0) {
			printk(KERN_ERR
				"%s: bt power off gpio config failed: %d\n",
				__func__, rc);
			return rc;
		}

		if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa()) {
			rc = msm_gpios_enable
					(wlan_config_power_off,
					 ARRAY_SIZE(wlan_config_power_off));
			if (rc < 0) {
				printk
				 (KERN_ERR
				 "%s: wlan power off gpio config failed: %d\n",
					__func__, rc);
				return rc;
			}
		}
	}

	printk(KERN_DEBUG "Bluetooth power switch: %d\n", on);

	return 0;
}

static void __init bt_power_init(void)
{
	struct vreg *vreg_bt;
	int rc;

	if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa()) {
		gpio_set_value(138, 0); /* WLAN: CHIP_PWD */
		gpio_set_value(113, 0); /* WLAN */
	}

	gpio_set_value(18, 0); /* SYSRST */
	gpio_set_value(22, 0); /* VDD_IO */

	/* do not have vreg bt defined, gp6 is the same */
	/* vreg_get parameter 1 (struct device *) is ignored */
	vreg_bt = vreg_get(NULL, "gp6");

	if (IS_ERR(vreg_bt)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_bt));
		goto exit;
	}

	/* units of mV, steps of 50 mV */
	rc = vreg_set_level(vreg_bt, PMIC_VREG_GP6_LEVEL);
	if (rc) {
		printk(KERN_ERR "%s: vreg bt set level failed (%d)\n",
		       __func__, rc);
		goto exit;
	}
	rc = vreg_enable(vreg_bt);
	if (rc) {
		printk(KERN_ERR "%s: vreg bt enable failed (%d)\n",
		       __func__, rc);
		goto exit;
	}

	if (bluetooth_power(0))
		goto exit;

	msm_bt_power_device.dev.platform_data = &bluetooth_power;

	printk(KERN_DEBUG "Bluetooth power switch: initialized\n");

exit:
	return;
}
#endif
#else
#define bt_power_init(x) do {} while (0)
#endif

static struct resource kgsl_resources[] = {
       {
		.name  = "kgsl_reg_memory",
		.start = 0xA0000000,
		.end = 0xA001ffff,
		.flags = IORESOURCE_MEM,
       },
       {
		.name   = "kgsl_phys_memory",
		.start = MSM_GPU_PHYS_BASE,
		.end = MSM_GPU_PHYS_BASE + MSM_GPU_PHYS_SIZE - 1,
		.flags = IORESOURCE_MEM,
       },
       {
		.name = "kgsl_yamato_irq",
		.start = INT_GRAPHICS,
		.end = INT_GRAPHICS,
		.flags = IORESOURCE_IRQ,
       },
};
static struct kgsl_platform_data kgsl_pdata = {
	.high_axi_3d = 128000, /* Max for 8K */
	.max_grp2d_freq = 0,
	.min_grp2d_freq = 0,
	.set_grp2d_async = NULL,
	.max_grp3d_freq = 0,
	.min_grp3d_freq = 0,
	.set_grp3d_async = NULL,
	.imem_clk_name = "imem_clk",
	.grp3d_clk_name = "grp_clk",
	.grp2d_clk_name = NULL,
};

static struct platform_device msm_device_kgsl = {
       .name = "kgsl",
       .id = -1,
       .num_resources = ARRAY_SIZE(kgsl_resources),
       .resource = kgsl_resources,
	.dev = {
		.platform_data = &kgsl_pdata,
	},
};

static struct platform_device msm_device_pmic_leds = {
	.name	= "pmic-leds",
	.id	= -1,
};

/* TSIF begin */
#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)

#define TSIF_A_SYNC      GPIO_CFG(106, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA)
#define TSIF_A_DATA      GPIO_CFG(107, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA)
#define TSIF_A_EN        GPIO_CFG(108, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA)
#define TSIF_A_CLK       GPIO_CFG(109, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA)

static const struct msm_gpio tsif_gpios[] = {
	{ .gpio_cfg = TSIF_A_CLK,  .label =  "tsif_clk", },
	{ .gpio_cfg = TSIF_A_EN,   .label =  "tsif_en", },
	{ .gpio_cfg = TSIF_A_DATA, .label =  "tsif_data", },
	{ .gpio_cfg = TSIF_A_SYNC, .label =  "tsif_sync", },
};

static struct msm_tsif_platform_data tsif_platform_data = {
	.num_gpios = ARRAY_SIZE(tsif_gpios),
	.gpios = tsif_gpios,
	.tsif_clk = "tsif_clk",
	.tsif_ref_clk = "tsif_ref_clk",
};

#endif /* defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE) */
/* TSIF end   */

#ifdef CONFIG_QSD_SVS
#define TPS65023_MAX_DCDC1	1600
#else
#define TPS65023_MAX_DCDC1	CONFIG_QSD_PMIC_DEFAULT_DCDC1
#endif

static int qsd8x50_tps65023_set_dcdc1(int mVolts)
{
	int rc = 0;
#ifdef CONFIG_QSD_SVS
	rc = tps65023_set_dcdc1_level(mVolts);
	/* By default the TPS65023 will be initialized to 1.225V.
	 * So we can safely switch to any frequency within this
	 * voltage even if the device is not probed/ready.
	 */
	if (rc == -ENODEV && mVolts <= CONFIG_QSD_PMIC_DEFAULT_DCDC1)
		rc = 0;
#else
	/* Disallow frequencies not supported in the default PMIC
	 * output voltage.
	 */
	if (mVolts > CONFIG_QSD_PMIC_DEFAULT_DCDC1)
		rc = -EFAULT;
#endif
	return rc;
}

static struct msm_acpu_clock_platform_data qsd8x50_clock_data = {
	.acpu_switch_time_us = 20,
	.max_speed_delta_khz = 256000,
	.vdd_switch_time_us = 62,
	.max_vdd = TPS65023_MAX_DCDC1,
	.acpu_set_vdd = qsd8x50_tps65023_set_dcdc1,
};


#if !HUAWEI_HWID(S70)
static void touchpad_gpio_release(void)
{
	gpio_free(TOUCHPAD_IRQ);
	gpio_free(TOUCHPAD_SUSPEND);
}

static int touchpad_gpio_setup(void)
{
	int rc;
	int suspend_pin = TOUCHPAD_SUSPEND;
	int irq_pin = TOUCHPAD_IRQ;
	unsigned suspend_cfg =
		GPIO_CFG(suspend_pin, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA);
	unsigned irq_cfg =
		GPIO_CFG(irq_pin, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA);

	rc = gpio_request(irq_pin, "msm_touchpad_irq");
	if (rc) {
		pr_err("gpio_request failed on pin %d (rc=%d)\n",
		       irq_pin, rc);
		goto err_gpioconfig;
	}
	rc = gpio_request(suspend_pin, "msm_touchpad_suspend");
	if (rc) {
		pr_err("gpio_request failed on pin %d (rc=%d)\n",
		       suspend_pin, rc);
		goto err_gpioconfig;
	}
	rc = gpio_tlmm_config(suspend_cfg, GPIO_ENABLE);
	if (rc) {
		pr_err("gpio_tlmm_config failed on pin %d (rc=%d)\n",
		       suspend_pin, rc);
		goto err_gpioconfig;
	}
	rc = gpio_tlmm_config(irq_cfg, GPIO_ENABLE);
	if (rc) {
		pr_err("gpio_tlmm_config failed on pin %d (rc=%d)\n",
		       irq_pin, rc);
		goto err_gpioconfig;
	}
	return rc;

err_gpioconfig:
	touchpad_gpio_release();
	return rc;
}

static struct msm_touchpad_platform_data msm_touchpad_data = {
	.gpioirq     = TOUCHPAD_IRQ,
	.gpiosuspend = TOUCHPAD_SUSPEND,
	.gpio_setup  = touchpad_gpio_setup,
	.gpio_shutdown = touchpad_gpio_release
};
#endif

#if !HUAWEI_HWID(S70)
#define KBD_RST 35
#define KBD_IRQ 36

static void kbd_gpio_release(void)
{
	gpio_free(KBD_IRQ);
	gpio_free(KBD_RST);
}

static int kbd_gpio_setup(void)
{
	int rc;
	int respin = KBD_RST;
	int irqpin = KBD_IRQ;
	unsigned rescfg =
		GPIO_CFG(respin, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA);
	unsigned irqcfg =
		GPIO_CFG(irqpin, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA);

	rc = gpio_request(irqpin, "gpio_keybd_irq");
	if (rc) {
		pr_err("gpio_request failed on pin %d (rc=%d)\n",
			irqpin, rc);
		goto err_gpioconfig;
	}
	rc = gpio_request(respin, "gpio_keybd_reset");
	if (rc) {
		pr_err("gpio_request failed on pin %d (rc=%d)\n",
			respin, rc);
		goto err_gpioconfig;
	}
	rc = gpio_tlmm_config(rescfg, GPIO_ENABLE);
	if (rc) {
		pr_err("gpio_tlmm_config failed on pin %d (rc=%d)\n",
			respin, rc);
		goto err_gpioconfig;
	}
	rc = gpio_tlmm_config(irqcfg, GPIO_ENABLE);
	if (rc) {
		pr_err("gpio_tlmm_config failed on pin %d (rc=%d)\n",
			irqpin, rc);
		goto err_gpioconfig;
	}
	return rc;

err_gpioconfig:
	kbd_gpio_release();
	return rc;
}

/* use gpio output pin to toggle keyboard external reset pin */
static void kbd_hwreset(int kbd_mclrpin)
{
	gpio_direction_output(kbd_mclrpin, 0);
	gpio_direction_output(kbd_mclrpin, 1);
}

static struct msm_i2ckbd_platform_data msm_kybd_data = {
	.hwrepeat = 0,
	.scanset1 = 1,
	.gpioreset = KBD_RST,
	.gpioirq = KBD_IRQ,
	.gpio_setup = kbd_gpio_setup,
	.gpio_shutdown = kbd_gpio_release,
	.hw_reset = kbd_hwreset,
};
#endif

#if defined(CONFIG_MOUSE_OFN_HID) || defined(CONFIG_MOUSE_OFN_HID_MODULE)
static void __init ofn_power_init(void)
{
	struct vreg *vreg_ofn_AVDD;
    struct vreg *vreg_ofn_IOVDD;
	int rc;

	vreg_ofn_AVDD = vreg_get(NULL, "gp4");

	if (IS_ERR(vreg_ofn_AVDD)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_ofn_AVDD));
		goto exit;
	}

    vreg_ofn_IOVDD = vreg_get(NULL, "gp5");

	if (IS_ERR(vreg_ofn_IOVDD)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_ofn_IOVDD));
		goto exit;
	}

	/* units of mV, steps of 50 mV */
	rc = vreg_set_level(vreg_ofn_AVDD, 2800);//2.8v
	if (rc) {
		printk(KERN_ERR "%s: vreg ofn set level failed (%d)\n",
		       __func__, rc);
		goto exit;
	}
	rc = vreg_enable(vreg_ofn_AVDD);
	if (rc) {
		printk(KERN_ERR "%s: vreg ofn enable failed (%d)\n",
		       __func__, rc);
		goto exit;
	}

    /* units of mV, steps of 50 mV */
	rc = vreg_set_level(vreg_ofn_IOVDD, 2600);//2.6v
	if (rc) {
		printk(KERN_ERR "%s: vreg ofn set level failed (%d)\n",
		       __func__, rc);
		goto exit;
	}
	rc = vreg_enable(vreg_ofn_IOVDD);
	if (rc) {
		printk(KERN_ERR "%s: vreg ofn enable failed (%d)\n",
		       __func__, rc);
		goto exit;
	}

	printk(KERN_DEBUG "OFN power  initialized\n");

exit:
	return;
}

static struct msm_gpio optnav_config_data[] = {
	{ GPIO_CFG(GPIO_OPTNAV_IRQ, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA),"optnav_motion" },
	{ GPIO_CFG(GPIO_OPTNAV_SHUTDOWN, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),"optnav_shutdown" },
	{ GPIO_CFG(GPIO_OPTNAV_RESET, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), "optnav_reset" },
	{ GPIO_CFG(GPIO_OPTNAV_DOME1, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA),"optnav_dome1" },
};

static int optnav_gpio_setup(void)
{
	int rc = -ENODEV;

	rc = msm_gpios_request_enable(optnav_config_data,
					      ARRAY_SIZE(optnav_config_data));
	return rc;
}

static void optnav_gpio_release(void)
{
	msm_gpios_disable_free(optnav_config_data,
			       ARRAY_SIZE(optnav_config_data));
}

static struct ofn_hid_platform_data optnav_data = {
	.irq_button_center  	= MSM_GPIO_TO_INT(GPIO_OPTNAV_DOME1),
    .shutdown = GPIO_OPTNAV_SHUTDOWN,
    .reset = GPIO_OPTNAV_RESET,
	.gpio_button_center 	= GPIO_OPTNAV_DOME1,
	.gpio_setup    		= optnav_gpio_setup,
	.gpio_release  		= optnav_gpio_release,
	.rotate_xy     			= 0,
};

#endif

#ifdef CONFIG_DOCK_DET
static struct msm_gpio dock_dect_cfg = {
	  GPIO_CFG(GPIO_DOCK_DET, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA),"dock_dect_irq" 
};

static int dock_dect_setup(void)
{
	int rc = -ENODEV;
	rc = msm_gpios_request_enable(&dock_dect_cfg,1);
	return rc;
}

static struct platform_device dock_det_device = {
	.name		= "dock_detect",
	.id		= -1,
};

static struct dock_det_platform_data dock_pdata = {
	.det_gpio		= GPIO_DOCK_DET,
    .det_irq_gpio   = MSM_GPIO_TO_INT(GPIO_DOCK_DET),
};

static int init_dock_detect(void)
{
	struct platform_device	*pdev;
    
    dock_dect_setup();

    pdev = &dock_det_device;
	pdev->dev.platform_data = &dock_pdata;
	return platform_device_register(pdev);
}

#endif

#if defined(CONFIG_CYPRESS120) || defined(CONFIG_CYPRESS120_MODULE)
static struct msm_gpio ctp_cfg[] = {
	{ GPIO_CFG(GPIO_CTP_INT, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA),"ctp_irq" },
	{ GPIO_CFG(GPIO_CTP_RESET, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA),"ctp_reset" },
	{ GPIO_CFG(GPIO_CTP_POWER, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA), "ctp_power" },
};

static int ctp_vbus_ctrl(int on)
{
    struct vreg *vreg_gp5;
    int rc=0;

    vreg_gp5 = vreg_get(NULL, "gp5");
    if (IS_ERR(vreg_gp5)) 
       {
        printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
                __func__, "gp5", PTR_ERR(vreg_gp5));
        return -EIO;
    }

    rc = vreg_set_level(vreg_gp5, 2600);
    if (rc) 
       {
        printk(KERN_ERR "%s: GP5 set_level failed (%d)\n",
                __func__, rc);
        return -EIO;
    }

	if(on)
    {
	    rc = vreg_enable(vreg_gp5);
	    if (rc)
	    {
	        printk(KERN_ERR "vreg gp5 enable failed\n");
	        return -EIO;
	    }
    }
    else
    {
	    rc = vreg_disable(vreg_gp5);
	    if (rc)
	    {
	        printk(KERN_ERR "vreg gp5 disable failed\n");
	        return -EIO;
	    }
    }        
     return 0;
}
static int ctp_gpio_init(void)
{
	int rc = -ENODEV;

	rc = msm_gpios_request_enable(ctp_cfg, ARRAY_SIZE(ctp_cfg));
    if (rc < 0) {
	    printk(KERN_ERR "[%s,%d]: setup gpio failed. error code %d\n", __func__, __LINE__, rc);
    }
    ctp_vbus_ctrl(1);
    msleep(1);
    ctp_vbus_ctrl(0);
    msleep(10);
	gpio_set_value(GPIO_CTP_POWER, 1);	/*enable synaptics vdd*/
	msleep(30);
    ctp_vbus_ctrl(1);
	return rc;
}

void ctp_gpio_free(void)
{
	msm_gpios_disable_free(ctp_cfg, ARRAY_SIZE(ctp_cfg));
}

#endif

#if defined(CONFIG_INPUT_PEDESTAL) || defined(CONFIG_INPUT_PEDESTAL_MODULE)

static struct msm_gpio pedestal_dect_cfg = 
	{ GPIO_CFG(PEDESTAL_DECT_GPIO, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA),"pedestal_dect_irq" };


static int pedestal_dect_setup(void)
{
	int rc = -ENODEV;

	rc = msm_gpios_request_enable(&pedestal_dect_cfg,1);
	return rc;
}

static void pedestal_dect_release(void)
{
	msm_gpios_disable_free(&pedestal_dect_cfg,1);
}

static struct pedestal_platform_data pedestal_pdata = {
	.dect_gpio 		= PEDESTAL_DECT_GPIO,
	.interrupt_gpio = PEDESTAL_INT_GPIO,
	.gpio_setup    	= pedestal_dect_setup,
	.gpio_release  	= pedestal_dect_release,
};
#endif

#if defined(CONFIG_CYPRESS120) || defined(CONFIG_CYPRESS120_MODULE)
static struct cypress120_ts_i2c_platform_data cypress120_ts_platform_data = {
	.dect_irq		= MSM_GPIO_TO_INT(GPIO_CTP_INT),
    .reset_gpio		= GPIO_CTP_RESET,
    .power_gpio		= GPIO_CTP_POWER,
    .gpio_free		= ctp_gpio_free,
};
#endif

#if defined(CONFIG_SENSORS_AKM8973) || defined(CONFIG_SENSORS_AKM8973_MODULE)
static struct compass_platform_data compass_pdata = {
  .irq = MSM_GPIO_TO_INT(GPIO_COMPASS_IRQ),
  .reset = GPIO_COMPASS_RESET,
};
#endif

#if defined(CONFIG_TOUCHSCREEN_T1320)  || defined(CONFIG_TOUCHSCREEN_T1320_MODULE) \
	|| defined(CONFIG_TOUCHSCREEN_MXT224) || defined(CONFIG_TOUCHSCREEN_MXT224_MODULE)
#define GPIO_CTP_INT   		(28)
#define GPIO_CTP_POWER   		(105)
#define GPIO_CTP_RESET       	(158)

static struct msm_gpio ctp_cfg[] = {
	{ GPIO_CFG(GPIO_CTP_INT, 0, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA),"ctp_irq" },
	{ GPIO_CFG(GPIO_CTP_RESET, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA),"ctp_reset" },
	{ GPIO_CFG(GPIO_CTP_POWER, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA), "ctp_power" },
};

static int ctp_vbus_ctrl(int on)
{
	struct vreg *vreg_gp5;
	int rc=0;

	vreg_gp5 = vreg_get(NULL, "gp5");
	if (IS_ERR(vreg_gp5)) 
	{
		printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",__func__, "gp5", PTR_ERR(vreg_gp5));
		return -EIO;
	}
	
	rc = vreg_set_level(vreg_gp5, 2600);
	if (rc) 
	{
		printk(KERN_ERR "%s: GP5 set_level failed (%d)\n",__func__, rc);
		return -EIO;
	}
	
	if(on)
	{
		rc = vreg_enable(vreg_gp5);
		if (rc)
		{
			printk(KERN_ERR "vreg gp5 enable failed\n");
			return -EIO;
		}
	}
	else
	{
		rc = vreg_disable(vreg_gp5);
		if (rc)
		{
			printk(KERN_ERR "vreg gp5 disable failed\n");
			return -EIO;
		}
	}        
	return 0;
}

static void ctp_exit_platform_hw(void)
{
	ctp_vbus_ctrl(0);
	msm_gpios_disable_free(ctp_cfg, ARRAY_SIZE(ctp_cfg));
}

static int ctp_init_platform_hw(void)
{
	int rc = -ENODEV;

	rc = msm_gpios_request_enable(ctp_cfg, ARRAY_SIZE(ctp_cfg));
	if (rc < 0) {
		printk(KERN_ERR "[%s,%d]: setup gpio failed. error code %d\n", __func__, __LINE__, rc);
	}  
	
	ctp_vbus_ctrl(1);
	gpio_set_value(GPIO_CTP_POWER, 1);
	mdelay(5);
	gpio_set_value(GPIO_CTP_RESET, 0);
	mdelay(5);
	gpio_set_value(GPIO_CTP_RESET, 1);
	mdelay(50);
	return rc;
}
#endif
#if defined(CONFIG_TOUCHSCREEN_T1320) || defined(CONFIG_TOUCHSCREEN_T1320_MODULE)
static struct t1320 t1320_pdata = {
	.client = NULL,
	.input_dev = NULL,
	.use_irq = 0,
	.data_reg = 0xff,
	.data_length = 0,
	.data = NULL,
	.hasF11 = false,
	.f11_has_gestures = 0,
	.f11_has_relative = 0,
	.f11_max_x = 0,
	.f11_max_y = 0,
	.f11_egr = NULL,
	.hasEgrPinch = 0,
	.hasEgrPress = 0,
	.hasEgrFlick = 0,
	.hasEgrEarlyTap = 0,
	.hasEgrDoubleTap = 0,
	.hasEgrTapAndHold = 0,
	.hasEgrSingleTap = 0,
	.hasEgrPalmDetect = 0,
	.f11_fingers = NULL,
	.hasF19 = false,
	.hasF30 = false,
	.enable = 0,
	.init_platform_hw = ctp_init_platform_hw,
	.exit_platform_hw = ctp_exit_platform_hw,	
};
#endif
#if defined(CONFIG_TOUCHSCREEN_MXT224) || defined(CONFIG_TOUCHSCREEN_MXT224_MODULE)
static struct mxt224_platform_data mxt224_pdata = {
	.init_platform_hw = ctp_init_platform_hw,
	.exit_platform_hw = ctp_exit_platform_hw,
};
#endif

static struct i2c_board_info msm_i2c_board_info[] __initdata = {

#if defined(CONFIG_CYPRESS120) || defined(CONFIG_CYPRESS120_MODULE)
	{
		I2C_BOARD_INFO(CYPRESS120_I2C_TS_NAME, CTP_I2C_ADDR),
		.irq           =  MSM_GPIO_TO_INT(GPIO_CTP_INT),
		.platform_data = &cypress120_ts_platform_data,
	},
#endif

#if HUAWEI_HWID_L2(S7, S7201)
#if defined(CONFIG_TOUCHSCREEN_T1320) || defined(CONFIG_TOUCHSCREEN_T1320_MODULE)
	{
			I2C_BOARD_INFO("t1320", 0x24),
        	.irq           =  MSM_GPIO_TO_INT(GPIO_CTP_INT),
        	.platform_data = &t1320_pdata,
	},
#endif
#endif
#if defined(CONFIG_TOUCHSCREEN_MXT224) || defined(CONFIG_TOUCHSCREEN_MXT224_MODULE)
	{
		I2C_BOARD_INFO("mxt224", 0x4A),  //0x4b
		.platform_data = &mxt224_pdata,
		.irq = MSM_GPIO_TO_INT(GPIO_CTP_INT),
	},
#endif

#if !HUAWEI_HWID(S70)
	{
		I2C_BOARD_INFO("glidesensor", 0x2A),
		.irq           =  MSM_GPIO_TO_INT(TOUCHPAD_IRQ),
		.platform_data = &msm_touchpad_data
	},
	{
		I2C_BOARD_INFO("msm-i2ckbd", 0x3A),
		.type           = "msm-i2ckbd",
		.irq           =  MSM_GPIO_TO_INT(KBD_IRQ),
		.platform_data  = &msm_kybd_data
	},
#endif
#ifdef CONFIG_MT9D112
	{
		I2C_BOARD_INFO("mt9d112", 0x78 >> 1),
	},
#endif
#ifdef CONFIG_S5K3E2FX
	{
		I2C_BOARD_INFO("s5k3e2fx", 0x20 >> 1),
	},
#endif
#ifdef CONFIG_MT9P012
	{
		I2C_BOARD_INFO("mt9p012", 0x6C >> 1),
	},
#endif
#ifdef CONFIG_MT9P012_KM
	{
		I2C_BOARD_INFO("mt9p012_km", 0x6C >> 2),
	},
#endif
#if defined(CONFIG_MT9T013) || defined(CONFIG_SENSORS_MT9T013)
	{
		I2C_BOARD_INFO("mt9t013", 0x6C),
	},
#endif

	{
		I2C_BOARD_INFO("tps65023", 0x48),
	},

#if defined(CONFIG_MOUSE_OFN_HID) || defined(CONFIG_MOUSE_OFN_HID_MODULE)
	{
		I2C_BOARD_INFO("fo1w", 0x33),
		.irq           =  MSM_GPIO_TO_INT(GPIO_OPTNAV_IRQ),
		.platform_data = &optnav_data
	},
#endif

	#if defined(CONFIG_RMT_CTRL_NEC) || defined(CONFIG_RMT_CTRL_NEC_MODULE)
	{
		I2C_BOARD_INFO(RMT_CTRL_DEVICE_NAME, 0x51),
		.irq           =  MSM_GPIO_TO_INT(RMT_CTRL_IRQ),
		.platform_data = &rmt_ctrl_platform_data
	},
	#endif
		
#if defined(CONFIG_SENSORS_LIS3XX) || defined(CONFIG_SENSORS_LIS3XX_MODULE)
	{
		I2C_BOARD_INFO("lis3xx", 0x1C),
		.platform_data = &msm_gsensor_data
	},
#endif
#if defined(CONFIG_MT9V113) || defined(CONFIG_MT9V113_MODULE)
    {
        I2C_BOARD_INFO("mt9v113", 0x78 >> 1),
    },
#endif
#if defined(CONFIG_OV5630) || defined(CONFIG_OV5630_MODULE)
    {
        I2C_BOARD_INFO("ov5630", 0x6C >> 1),
    },
#endif

#ifdef CONFIG_MT9D113
	{
		I2C_BOARD_INFO("mt9d113", 0x78 >> 1),
	},
#endif

#ifdef CONFIG_MT9D113_BACK
	{
		I2C_BOARD_INFO("mt9d113_back", 0x78 ),// in mt9d113_back driver,will >> 1
	},
#endif

#ifdef CONFIG_OV2650
	{
		I2C_BOARD_INFO("ov2650", 0x60 >> 1),
	},
#endif

#ifdef CONFIG_S5K5CAG
	{
		I2C_BOARD_INFO("s5k5cag", 0x5A >> 1),
	},
#endif


#ifdef CONFIG_MT9V114
	{
		I2C_BOARD_INFO("mt9v114", 0x7A >> 1),
	},
#endif


#if defined(CONFIG_ACCELEROMETER_ST_L1S35DE) \
    || defined(CONFIG_ACCELEROMETER_ST_L1S35DE_MODULE)
	{
		I2C_BOARD_INFO("gs_st", 0x3A >> 1),   
	   .irq = MSM_GPIO_TO_INT(22)
	},
#endif

#if defined(CONFIG_SENSORS_AKM8973) \
    || defined(CONFIG_SENSORS_AKM8973_MODULE)
	{
	  I2C_BOARD_INFO("akm8973", 0x3c >> 1), //7 bit addr, no write bit
	  .irq = MSM_GPIO_TO_INT(GPIO_COMPASS_IRQ),  // is that irq needed ?? yes, to handle data when it's ready on chip
	  .platform_data = &compass_pdata,
	},
#endif

#if defined(CONFIG_ACCELEROMETER_ADXL345) \
        || defined(CONFIG_ACCELEROMETER_ADXL345_MODULE)
	{
		I2C_BOARD_INFO("adxl345", 0xA6 >> 1),   
	   .irq = MSM_GPIO_TO_INT(22)
	},
#endif
#ifdef CONFIG_MSM_HDMI 
	{
            I2C_BOARD_INFO("hdmi_i2c", 0x72 >> 1),
	},
#endif

#if defined(CONFIG_INPUT_PEDESTAL) || defined(CONFIG_INPUT_PEDESTAL_MODULE)
	{
		I2C_BOARD_INFO(PEDESTAL_DEVICE_NAME, PEDESTAL_I2C_ADDR),
		.irq           =  MSM_GPIO_TO_INT(PEDESTAL_INT_GPIO),
		.platform_data = &pedestal_pdata
	},
#endif
#if HUAWEI_HWID_L2(S7, S7201)
#if defined(CONFIG_BATTERY_BQ275X0) || defined(CONFIG_BATTERY_BQ275X0_MODULE)
	{
		I2C_BOARD_INFO("bq275x0-battery",0x55),	//(0xAA >> 1)
	},
#endif 
#endif
};

static struct i2c_board_info gpio_i2c_board_info[] __initdata = {
#if defined(CONFIG_BELASIGNA_BS300)||defined(CONFIG_BELASIGNA_BS300_MODULE)
	{
		I2C_BOARD_INFO("bs300-audio",0x60),
	},
#endif
};

#if HWVERID_HIGHER(S70, A)
#define GPIO_MCAM_PWDN			107
#define GPIO_MCAM_RST			108
#define GPIO_MCAM_VCM_PWDN		150

#define VREG_CAMERA_AVDD			"gp1"
#define VREG_CAMERA_DVDD			"gp2"
#define VREG_CAMERA_IOVDD			"gp3"    

#elif HWVERID_HIGHER(S70, T1)
#undef GPIO_MCAM_PWDN
#undef GPIO_MCAM_RST
#undef GPIO_MCAM_VCM_PWDN

#define VREG_CAMERA_AVDD			"gp1"
#define VREG_CAMERA_DVDD			"gp2"
#define VREG_CAMERA_IOVDD			"gp3"

#else
#define GPIO_MCAM_PWDN			85
#define GPIO_MCAM_RST			17
#define GPIO_MCAM_VCM_PWDN		88
#endif

#if HWVERID_HIGHER(S70, T1)
/*
static struct vreg *vreg_camera_avdd;
static struct vreg *vreg_camera_dvdd;

#if defined(CONFIG_MSM_CAMERA)
static struct vreg *vreg_camera_iovdd;
#endif

static int qsd8x50_camera_init(void)
{
#if defined(CONFIG_MSM_CAMERA)
	if ((vreg_camera_iovdd = vreg_get(NULL, VREG_CAMERA_IOVDD)) != ERR_PTR(-ENOENT)) {
        DBG("Set Camera IOVDD OK");
		vreg_set_level(vreg_camera_iovdd, 2600);
		vreg_enable(vreg_camera_iovdd);
    } else {
        DBG("Set Camera IOVDD failed");
    }
#endif

	if ((vreg_camera_dvdd = vreg_get(NULL, VREG_CAMERA_DVDD)) != ERR_PTR(-ENOENT)) {
        DBG("Set Camera DVDD OK");
		vreg_set_level(vreg_camera_dvdd, 1800);
		vreg_enable(vreg_camera_dvdd);
    } else {
        DBG("Set Camera DVDD failed");
    }

	if ((vreg_camera_avdd = vreg_get(NULL, VREG_CAMERA_AVDD)) != ERR_PTR(-ENOENT)) {
        DBG("Set Camera AVDD OK");
		vreg_set_level(vreg_camera_avdd, 2850);
		vreg_enable(vreg_camera_avdd);
    } else {
        DBG("Set Camera AVDD failed");
    }

	return 0;
}
*/
#else
#define qsd8x50_camera_init(x)  do {} while (0)
#endif

#ifdef CONFIG_MSM_CAMERA
#if HUAWEI_HWID_L2(S7, S7201)
static uint32_t camera_off_gpio_table[] = {
	GPIO_CFG(4,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
	GPIO_CFG(5,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
	GPIO_CFG(6,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
	GPIO_CFG(7,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
	GPIO_CFG(8,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT8 */
	GPIO_CFG(9,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT9 */
	GPIO_CFG(10, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT10 */
	GPIO_CFG(11, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT11 */
	GPIO_CFG(12, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* PCLK */
	GPIO_CFG(13, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
	GPIO_CFG(15, 0, GPIO_OUTPUT,GPIO_PULL_DOWN, GPIO_2MA), /* MCLK */
	GPIO_CFG(107, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(108, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(85, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(84, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
};

static uint32_t camera_on_gpio_table[] = {
	GPIO_CFG(4,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
	GPIO_CFG(5,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
	GPIO_CFG(6,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
	GPIO_CFG(7,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
	GPIO_CFG(8,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT8 */
	GPIO_CFG(9,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT9 */
	GPIO_CFG(10, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT10 */
	GPIO_CFG(11, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT11 */
	GPIO_CFG(12, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* PCLK */
	GPIO_CFG(13, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
	GPIO_CFG(15, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), /* MCLK */
	GPIO_CFG(107, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(108, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(85, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
	GPIO_CFG(84, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),

};
#else
static uint32_t camera_off_gpio_table[] = {
	/* parallel CAMERA interfaces */
	GPIO_CFG(0,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT0 */
	GPIO_CFG(1,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT1 */
	GPIO_CFG(2,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT2 */
	GPIO_CFG(3,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT3 */
	GPIO_CFG(4,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
	GPIO_CFG(5,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
	GPIO_CFG(6,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
	GPIO_CFG(7,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
	GPIO_CFG(8,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT8 */
	GPIO_CFG(9,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT9 */
	GPIO_CFG(10, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT10 */
	GPIO_CFG(11, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT11 */
	GPIO_CFG(12, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* PCLK */
	GPIO_CFG(13, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
	//GPIO_CFG(15, 0, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_2MA), /* MCLK */
	GPIO_CFG(15, 0, GPIO_OUTPUT,GPIO_PULL_DOWN, GPIO_2MA), /* MCLK */
    /* FFA front Sensor Standby */
	GPIO_CFG(107, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
    /* FFA front Sensor Reset */
	GPIO_CFG(108, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
	/*lijuan add gpio 95 96 107 109 end*/

	/* FFA front Sensor Standby */
	GPIO_CFG(85, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
    /* FFA front Sensor Reset */
	GPIO_CFG(84, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),

};

static uint32_t camera_on_gpio_table[] = {
	/* parallel CAMERA interfaces */
	GPIO_CFG(0,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT0 */
	GPIO_CFG(1,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT1 */
	GPIO_CFG(2,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT2 */
	GPIO_CFG(3,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT3 */
	GPIO_CFG(4,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT4 */
	GPIO_CFG(5,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT5 */
	GPIO_CFG(6,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT6 */
	GPIO_CFG(7,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT7 */
	GPIO_CFG(8,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT8 */
	GPIO_CFG(9,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT9 */
	GPIO_CFG(10, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT10 */
	GPIO_CFG(11, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* DAT11 */
	GPIO_CFG(12, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* PCLK */
	GPIO_CFG(13, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* HSYNC_IN */
	GPIO_CFG(14, 1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA), /* VSYNC_IN */
	GPIO_CFG(15, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_16MA), /* MCLK */

	/* FFA front Sensor Standby */
	GPIO_CFG(107, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
    /* FFA front Sensor Reset */
    GPIO_CFG(108, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),

   /* FFA front Sensor Standby */
    GPIO_CFG(85, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
   /* FFA front Sensor Reset */
    GPIO_CFG(84, 0, GPIO_OUTPUT, GPIO_PULL_DOWN, GPIO_2MA),
};
#endif

#if !HUAWEI_HWID_L1(S7)
static uint32_t camera_on_gpio_ffa_table[] = {
	/* parallel CAMERA interfaces */
	GPIO_CFG(95,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_16MA), /* I2C_SCL */
	GPIO_CFG(96,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_16MA), /* I2C_SDA */
	/* FFA front Sensor Reset */
	GPIO_CFG(137,  1, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_16MA),
};

static uint32_t camera_off_gpio_ffa_table[] = {
	/* FFA front Sensor Reset */
	GPIO_CFG(137,  0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_16MA),
};
#endif

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_ENABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

#if HUAWEI_HWID_L1(S7) 
static void msm_camera_vreg_config(int vreg_en)
{
	struct vreg *vreg_gp1,*vreg_gp2, *vreg_gp3;
	int rc;
	vreg_gp1=NULL;
	vreg_gp2=NULL;
	vreg_gp3=NULL;

    if (vreg_gp1 == NULL){
	    vreg_gp1 = vreg_get(NULL, "gp1");
	    if (IS_ERR(vreg_gp1)) {
		printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
			__func__, "gp1", PTR_ERR(vreg_gp1));
		return;
	    }

	    rc = vreg_set_level(vreg_gp1, 2800);
	    if (rc) {
	    	printk(KERN_ERR "%s: vreg gp1 set level failed (%d)\n",
			__func__, rc);
	    }
    }
	if (vreg_gp2 == NULL) {
		vreg_gp2 = vreg_get(NULL, "gp2");
		if (IS_ERR(vreg_gp2)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
				__func__, "gp2", PTR_ERR(vreg_gp2));
			return;
		}

		rc = vreg_set_level(vreg_gp2, 1800);
		if (rc) {
			printk(KERN_ERR "%s: GP2 set_level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_gp3 == NULL) {
		vreg_gp3 = vreg_get(NULL, "gp3");
		if (IS_ERR(vreg_gp3)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
				__func__, "gp3", PTR_ERR(vreg_gp3));
			return;
		}

	    rc = vreg_set_level(vreg_gp3, 2600);/*lijuan change 2800*/
		if (rc) {
			printk(KERN_ERR "%s: GP3 set level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_en) {
		rc = vreg_enable(vreg_gp1);
		if (rc) {
			printk(KERN_ERR "%s: vreg gp1 enable failed (%d)\n",
				 __func__, rc);
		}
		rc = vreg_enable(vreg_gp2);
		if (rc) {
			printk(KERN_ERR "%s: GP2 enable failed (%d)\n",
				 __func__, rc);
		}

		rc = vreg_enable(vreg_gp3);
		if (rc) {
			printk(KERN_ERR "%s: GP3 enable failed (%d)\n",
				__func__, rc);
		}
	} else {
		rc = vreg_disable(vreg_gp1);
		if (rc) {
			printk(KERN_ERR "%s: vreg gp1 enable failed (%d)\n",
				 __func__, rc);
		}
		rc = vreg_disable(vreg_gp2);
		if (rc) {
			printk(KERN_ERR "%s: GP2 disable failed (%d)\n",
				 __func__, rc);
		}
		rc = vreg_disable(vreg_gp3);
		if (rc) {
			printk(KERN_ERR "%s: GP3 disable failed (%d)\n",
				__func__, rc);
		}
	}
}
#ifdef CONFIG_S5K5CAG
static void msm_camera_vreg_config_s5k5cag(int vreg_en)
{
	struct vreg *vreg_gp1,*vreg_gp2, *vreg_gp3;
	struct vreg *vreg_pa;
	int rc;
	vreg_gp1=NULL;
	vreg_gp2=NULL;
	vreg_gp3=NULL;
	vreg_pa  =NULL;
	
      if (vreg_pa == NULL){
	    vreg_pa = vreg_get(NULL, "pa");
	    if (IS_ERR(vreg_pa)) {
		printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
			__func__, "pa", PTR_ERR(vreg_pa));
		return;
	    }

	    rc = vreg_set_level(vreg_pa, 1800);
	    if (rc) {
	    	printk(KERN_ERR "%s: vreg gp1 set level failed (%d)\n",
			__func__, rc);
	    }
	}

	if (vreg_gp2 == NULL) {
	vreg_gp2 = vreg_get(NULL, "gp2");
	if (IS_ERR(vreg_gp2)) {
		printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
			__func__, "gp2", PTR_ERR(vreg_gp2));
		return;
	}

	rc = vreg_set_level(vreg_gp2, 1800);
	if (rc) {
		printk(KERN_ERR "%s: GP2 set_level failed (%d)\n",
			__func__, rc);
		}
	}
	
      if (vreg_gp1 == NULL){
	    vreg_gp1 = vreg_get(NULL, "gp1");
	    if (IS_ERR(vreg_gp1)) {
		printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
			__func__, "gp1", PTR_ERR(vreg_gp1));
		return;
	    }

	    rc = vreg_set_level(vreg_gp1, 2800);
	    if (rc) {
	    	printk(KERN_ERR "%s: vreg gp1 set level failed (%d)\n",
			__func__, rc);
	    }
	}

	if (vreg_gp3 == NULL) {
		vreg_gp3 = vreg_get(NULL, "gp3");
		if (IS_ERR(vreg_gp3)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
				__func__, "gp3", PTR_ERR(vreg_gp3));
			return;
		}

	    rc = vreg_set_level(vreg_gp3, 2800);
		if (rc) {
			printk(KERN_ERR "%s: GP3 set level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_en) {
		rc = vreg_enable(vreg_pa);
		if (rc) {
			printk(KERN_ERR "%s: vreg pa enable failed (%d)\n",
				 __func__, rc);
		}
		
		rc = vreg_enable(vreg_gp2);
		if (rc) {
			printk(KERN_ERR "%s: GP2 enable failed (%d)\n",
				 __func__, rc);
		}
		
		rc = vreg_enable(vreg_gp1);
		if (rc) {
			printk(KERN_ERR "%s: vreg gp1 enable failed (%d)\n",
				 __func__, rc);
		}

		rc = vreg_enable(vreg_gp3);
		if (rc) {
			printk(KERN_ERR "%s: GP3 enable failed (%d)\n",
				__func__, rc);
		}
	} else {

		rc = vreg_disable(vreg_gp3);
		if (rc) {
			printk(KERN_ERR "%s: GP3 disable failed (%d)\n",
				__func__, rc);
		}
		rc = vreg_disable(vreg_gp1);
		if (rc) {
			printk(KERN_ERR "%s: vreg gp1 enable failed (%d)\n",
				 __func__, rc);
		}
		rc = vreg_disable(vreg_gp2);
		if (rc) {
			printk(KERN_ERR "%s: GP2 disable failed (%d)\n",
				 __func__, rc);
		}		
	}
}
#endif

#ifdef CONFIG_MT9V114
static void msm_camera_vreg_config_mt9v114(int vreg_en)
{
	struct vreg *vreg_gp1,*vreg_gp2, *vreg_gp3;
	struct vreg *vreg_pa;
	int rc;
	vreg_gp1=NULL;
	vreg_gp2=NULL;
	vreg_gp3=NULL;
	vreg_pa  =NULL;
	
      if (vreg_pa == NULL){
	    vreg_pa = vreg_get(NULL, "pa");
	    if (IS_ERR(vreg_pa)) {
		printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
			__func__, "pa", PTR_ERR(vreg_pa));
		return;
	    }

	    rc = vreg_set_level(vreg_pa, 1800);
	    if (rc) {
	    	printk(KERN_ERR "%s: vreg gp1 set level failed (%d)\n",
			__func__, rc);
	    }
	}
      if (vreg_gp1 == NULL){
	    vreg_gp1 = vreg_get(NULL, "gp1");
	    if (IS_ERR(vreg_gp1)) {
		printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
			__func__, "gp1", PTR_ERR(vreg_gp1));
		return;
	    }

	    rc = vreg_set_level(vreg_gp1, 2800);
	    if (rc) {
	    	printk(KERN_ERR "%s: vreg gp1 set level failed (%d)\n",
			__func__, rc);
	    }
	}

	if (vreg_gp3 == NULL) {
		vreg_gp3 = vreg_get(NULL, "gp3");
		if (IS_ERR(vreg_gp3)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
				__func__, "gp3", PTR_ERR(vreg_gp3));
			return;
		}

	    rc = vreg_set_level(vreg_gp3, 2800);
		if (rc) {
			printk(KERN_ERR "%s: GP3 set level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_gp2 == NULL) {
	vreg_gp2 = vreg_get(NULL, "gp2");
	if (IS_ERR(vreg_gp2)) {
		printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
			__func__, "gp2", PTR_ERR(vreg_gp2));
		return;
	}

	rc = vreg_set_level(vreg_gp2, 1800);
	if (rc) {
		printk(KERN_ERR "%s: GP2 set_level failed (%d)\n",
			__func__, rc);
		}
	}	


	if (vreg_en) {

		rc = vreg_enable(vreg_gp1);
		if (rc) {
			printk(KERN_ERR "%s: vreg gp1 enable failed (%d)\n",
				 __func__, rc);
		}

		rc = vreg_enable(vreg_gp3);
		if (rc) {
			printk(KERN_ERR "%s: GP3 enable failed (%d)\n",
				__func__, rc);
		}
		rc = vreg_enable(vreg_pa);
		if (rc) {
			printk(KERN_ERR "%s: vreg pa enable failed (%d)\n",
				 __func__, rc);
		}
		
		rc = vreg_enable(vreg_gp2);
		if (rc) {
			printk(KERN_ERR "%s: GP2 enable failed (%d)\n",
				 __func__, rc);
		}
		

	} else {

		rc = vreg_disable(vreg_gp2);
		if (rc) {
			printk(KERN_ERR "%s: GP2 disable failed (%d)\n",
				 __func__, rc);
		}		
		rc = vreg_disable(vreg_gp3);
		if (rc) {
			printk(KERN_ERR "%s: GP3 disable failed (%d)\n",
				__func__, rc);
		}
		rc = vreg_disable(vreg_gp1);
		if (rc) {
			printk(KERN_ERR "%s: vreg gp1 enable failed (%d)\n",
				 __func__, rc);
		}

	}
}
#endif

static void config_camera_on_gpios(void)
{
   /*	int vreg_en = 1;

	if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa()) {
		config_gpio_table(camera_on_gpio_ffa_table,
		ARRAY_SIZE(camera_on_gpio_ffa_table));

		msm_camera_vreg_config(vreg_en);
		gpio_set_value(137, 0);
	}*/
	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));
}

static void config_camera_off_gpios(void)
{
/*	int vreg_en = 0;

	if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa()) {
		config_gpio_table(camera_off_gpio_ffa_table,
		ARRAY_SIZE(camera_off_gpio_ffa_table));

		msm_camera_vreg_config(vreg_en);
	}*/

	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));
}


#else

static struct vreg *vreg_gp2;
static struct vreg *vreg_gp3;

static void msm_camera_vreg_config(int vreg_en)
{
	int rc;

	if (vreg_gp2 == NULL) {
		vreg_gp2 = vreg_get(NULL, "gp2");
		if (IS_ERR(vreg_gp2)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
				__func__, "gp2", PTR_ERR(vreg_gp2));
			return;
		}

		rc = vreg_set_level(vreg_gp2, 1800);
		if (rc) {
			printk(KERN_ERR "%s: GP2 set_level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_gp3 == NULL) {
		vreg_gp3 = vreg_get(NULL, "gp3");
		if (IS_ERR(vreg_gp3)) {
			printk(KERN_ERR "%s: vreg_get(%s) failed (%ld)\n",
				__func__, "gp3", PTR_ERR(vreg_gp3));
			return;
		}

		rc = vreg_set_level(vreg_gp3, 2800);
		if (rc) {
			printk(KERN_ERR "%s: GP3 set level failed (%d)\n",
				__func__, rc);
		}
	}

	if (vreg_en) {
		rc = vreg_enable(vreg_gp2);
		if (rc) {
			printk(KERN_ERR "%s: GP2 enable failed (%d)\n",
				 __func__, rc);
		}

		rc = vreg_enable(vreg_gp3);
		if (rc) {
			printk(KERN_ERR "%s: GP3 enable failed (%d)\n",
				__func__, rc);
		}
	} else {
		rc = vreg_disable(vreg_gp2);
		if (rc) {
			printk(KERN_ERR "%s: GP2 disable failed (%d)\n",
				 __func__, rc);
		}

		rc = vreg_disable(vreg_gp3);
		if (rc) {
			printk(KERN_ERR "%s: GP3 disable failed (%d)\n",
				__func__, rc);
		}
	}
}

static void config_camera_on_gpios(void)
{
	int vreg_en = 1;

	if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa()) {
		config_gpio_table(camera_on_gpio_ffa_table,
		ARRAY_SIZE(camera_on_gpio_ffa_table));

		msm_camera_vreg_config(vreg_en);
		gpio_set_value(137, 0);
	}
	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));
}

static void config_camera_off_gpios(void)
{
	int vreg_en = 0;

	if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa()) {
		config_gpio_table(camera_off_gpio_ffa_table,
		ARRAY_SIZE(camera_off_gpio_ffa_table));

		msm_camera_vreg_config(vreg_en);
	}
	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));
}
#endif

#ifdef CONFIG_MSM_CAMERA
static struct resource msm_camera_resources[] = {
	{
		.start	= 0xA0F00000,
		.end	= 0xA0F00000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VFE,
		.end	= INT_VFE,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct msm_camera_device_platform_data msm_camera_device_data = {
	.camera_gpio_on  = config_camera_on_gpios,
	.camera_gpio_off = config_camera_off_gpios,
	.ioext.mdcphy = MSM_MDC_PHYS,
	.ioext.mdcsz  = MSM_MDC_SIZE,
	.ioext.appphy = MSM_CLK_CTL_PHYS,
	.ioext.appsz  = MSM_CLK_CTL_SIZE,
};
#endif

#if defined(CONFIG_OV5630) || defined(CONFIG_OV5630_MODULE)
static struct msm_camera_sensor_info msm_camera_sensor_ov5630_data = {
    .sensor_reset   = GPIO_MCAM_RST,
    .sensor_pwd 	= GPIO_MCAM_PWDN,
	.vcm_pwd        = GPIO_MCAM_VCM_PWDN,
	.sensor_name    = "ov5630",
	.pdata          = &msm_camera_device_data,
	.flash_type     = MSM_CAMERA_FLASH_LED,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources)
};

static struct platform_device msm_camera_sensor_ov5630 = {
	.name      = "msm_camera_ov5630",
	.dev       = {
		.platform_data = &msm_camera_sensor_ov5630_data,
	},
};
#endif

#if !HUAWEI_HWID_L1(S7)
static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_PMIC,
	._fsrc.pmic_src.low_current  = 30,
	._fsrc.pmic_src.high_current = 100,
};
#endif

#ifdef CONFIG_MT9D112
static struct msm_camera_sensor_flash_data flash_mt9d112 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9d112_data = {
	.sensor_name    = "mt9d112",
    .sensor_reset   = GPIO_MCAM_RST,
    .sensor_pwd     = GPIO_MCAM_PWDN,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9d112
};

static struct platform_device msm_camera_sensor_mt9d112 = {
	.name      = "msm_camera_mt9d112",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9d112_data,
	},
};
#endif

#ifdef CONFIG_MT9D113_BACK
static struct msm_camera_sensor_flash_data  msm_camera_sensor_flash_data_mt9d113_back = {
		 .flash_type  = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9d113_back_data = {
	.sensor_name    = "mt9d113_back",
	.vreg_enable_func=msm_camera_vreg_config,
	.sensor_reset   =85, 
	.sensor_pwd     = 84,
	.vcm_pwd        = 0,
	.slave_sensor   =0,
	.pdata          = &msm_camera_device_data,
	.flash_data =&msm_camera_sensor_flash_data_mt9d113_back,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources)
};

static struct platform_device msm_camera_sensor_mt9d113_back= {
	.name      = "msm_camera_mt9d113_back",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9d113_back_data,
	},
};
#endif

#ifdef CONFIG_MT9D113
static struct msm_camera_sensor_flash_data  msm_camera_sensor_flash_data_mt9d113 = {
		//add by zj
		 .flash_type  = MSM_CAMERA_FLASH_NONE,
		//.flash_type = MSM_CAMERA_FLASH_LED,
		//.flash_src  = &msm_flash_src,
        //end by zj
};		
static struct msm_camera_sensor_info msm_camera_sensor_mt9d113_data = {
	.sensor_name    = "mt9d113",
	.vreg_enable_func=msm_camera_vreg_config,
	.sensor_reset   = GPIO_MCAM_RST,
	.sensor_pwd     = GPIO_MCAM_PWDN,
	.vcm_pwd        = 0,
	.slave_sensor   =1,
	.pdata          = &msm_camera_device_data,
	.flash_data =&msm_camera_sensor_flash_data_mt9d113,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources)
};

static struct platform_device msm_camera_sensor_mt9d113 = {
	.name      = "msm_camera_mt9d113",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9d113_data,
	},
};
#endif

#ifdef CONFIG_OV2650
static struct msm_camera_sensor_flash_data  msm_camera_sensor_flash_data_ov2650 = {
		 .flash_type  = MSM_CAMERA_FLASH_NONE,
};
static struct msm_camera_sensor_info msm_camera_sensor_ov2650_data = {
	.sensor_name    = "ov2650",
	.vreg_enable_func=msm_camera_vreg_config,
	.sensor_reset   =85, 
	.sensor_pwd     = 84,
	.vcm_pwd        = 0,
	.slave_sensor   =0,
	.pdata          = &msm_camera_device_data,
	.flash_data =&msm_camera_sensor_flash_data_ov2650,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources)
};

static struct platform_device msm_camera_sensor_ov2650= {
	.name      = "msm_camera_ov2650",
	.dev       = {
		.platform_data = &msm_camera_sensor_ov2650_data,
	},
};
#endif

#ifdef CONFIG_S5K5CAG
static struct msm_camera_sensor_flash_data  msm_camera_sensor_flash_data_s5k5cag = {
		 .flash_type  = MSM_CAMERA_FLASH_NONE,
};
static struct msm_camera_sensor_info msm_camera_sensor_s5k5cag_data = {
	.sensor_name    = "s5k5cag",
	.vreg_enable_func=msm_camera_vreg_config_s5k5cag,
	.sensor_reset   =85, 
	.sensor_pwd     = 84,
	.vcm_pwd        = 0,
	.slave_sensor   =0,
	.pdata          = &msm_camera_device_data,
	.flash_data =&msm_camera_sensor_flash_data_s5k5cag,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources)
};

static struct platform_device msm_camera_sensor_s5k5cag= {
	.name      = "msm_camera_s5k5cag",
	.dev       = {
		.platform_data = &msm_camera_sensor_s5k5cag_data,
	},
};
#endif

#ifdef CONFIG_MT9V114
static struct msm_camera_sensor_flash_data  msm_camera_sensor_flash_data_mt9v114 = {
		 .flash_type  = MSM_CAMERA_FLASH_NONE,
};		
static struct msm_camera_sensor_info msm_camera_sensor_mt9v114_data = {
	.sensor_name    = "mt9v114",
	.vreg_enable_func=msm_camera_vreg_config_mt9v114,
	.sensor_reset   = 108,
	.sensor_pwd     = 107,
	.vcm_pwd        = 0,
	.slave_sensor   =1,
	.pdata          = &msm_camera_device_data,
	.flash_data =&msm_camera_sensor_flash_data_mt9v114,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources)
};

static struct platform_device msm_camera_sensor_mt9v114 = {
	.name      = "msm_camera_mt9v114",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9v114_data,
	},
};
#endif

#ifdef CONFIG_S5K3E2FX
static struct msm_camera_sensor_flash_data flash_s5k3e2fx = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3e2fx_data = {
	.sensor_name    = "s5k3e2fx",
    .sensor_reset   = GPIO_MCAM_RST,
    .sensor_pwd     = GPIO_MCAM_PWDN,
	/*.vcm_pwd = 31, */  /* CAM1_VCM_EN, enabled in a9 */
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_s5k3e2fx
};

static struct platform_device msm_camera_sensor_s5k3e2fx = {
	.name      = "msm_camera_s5k3e2fx",
	.dev       = {
		.platform_data = &msm_camera_sensor_s5k3e2fx_data,
	},
};
#endif

#ifdef CONFIG_MT9P012
static struct msm_camera_sensor_flash_data flash_mt9p012 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9p012_data = {
	.sensor_name    = "mt9p012",
    .sensor_reset   = GPIO_MCAM_RST,
    .sensor_pwd     = GPIO_MCAM_PWDN,
	.vcm_pwd        = GPIO_MCAM_VCM_PWDN,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9p012
};

static struct platform_device msm_camera_sensor_mt9p012 = {
	.name      = "msm_camera_mt9p012",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9p012_data,
	},
};
#endif

#ifdef CONFIG_MT9P012_KM
static struct msm_camera_sensor_flash_data flash_mt9p012_km = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9p012_km_data = {
	.sensor_name    = "mt9p012_km",
	.sensor_reset   = 17,
	.sensor_pwd     = 85,
	.vcm_pwd        = 88,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9p012_km
};

static struct platform_device msm_camera_sensor_mt9p012_km = {
	.name      = "msm_camera_mt9p012_km",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9p012_km_data,
	},
};
#endif

#ifdef CONFIG_MT9T013
static struct msm_camera_sensor_flash_data flash_mt9t013 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9t013_data = {
	.sensor_name    = "mt9t013",
    .sensor_reset   = GPIO_MCAM_RST,
    .sensor_pwd     = GPIO_MCAM_PWDN,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data,
	.resource       = msm_camera_resources,
	.num_resources  = ARRAY_SIZE(msm_camera_resources),
	.flash_data     = &flash_mt9t013
};

static struct platform_device msm_camera_sensor_mt9t013 = {
	.name      = "msm_camera_mt9t013",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9t013_data,
	},
};
#endif
#endif /*CONFIG_MSM_CAMERA*/

#if !HUAWEI_HWID(S70)
static u32 msm_calculate_batt_capacity(u32 current_voltage);

static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design 	= 3200,
	.voltage_max_design	= 4200,
	.avail_chg_sources   	= AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
	.calculate_capacity	= &msm_calculate_batt_capacity,
};

static u32 msm_calculate_batt_capacity(u32 current_voltage)
{
	u32 low_voltage   = msm_psy_batt_data.voltage_min_design;
	u32 high_voltage  = msm_psy_batt_data.voltage_max_design;

	return (current_voltage - low_voltage) * 100
		/ (high_voltage - low_voltage);
}
#else
static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design 	= 3200,
	.voltage_max_design	= 4200,
	.avail_chg_sources   	= AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
};

#endif

static struct platform_device msm_batt_device = {
	.name 		    = "msm-battery",
	.id		    = -1,
	.dev.platform_data  = &msm_psy_batt_data,
};

static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}

#ifdef CONFIG_USB_FS_HOST
	 #if HUAWEI_HWID_L2(S7, S7201)
		static struct msm_otg_platform_data msm_otg_pdata = {
			.rpc_connect	= hsusb_rpc_connect,
			//.pmic_notif_init         = msm_pm_app_rpc_init,   
			//.pmic_notif_deinit       = msm_pm_app_rpc_deinit,
			//.pmic_register_vbus_sn   = msm_pm_app_register_vbus_sn,
			//.pmic_unregister_vbus_sn = msm_pm_app_unregister_vbus_sn,
			.pmic_enable_ldo         = msm_pm_app_enable_usb_ldo,
			.pemp_level              = PRE_EMPHASIS_WITH_10_PERCENT,
			.cdr_autoreset           = CDR_AUTO_RESET_DEFAULT,
			.drv_ampl                = HS_DRV_AMPLITUDE_5_PERCENT,
		};
	 #else
		static struct msm_otg_platform_data msm_otg_pdata = {
			.rpc_connect	= hsusb_rpc_connect,
			.pmic_notif_init         = msm_pm_app_rpc_init,
			.pmic_notif_deinit       = msm_pm_app_rpc_deinit,
			.pmic_register_vbus_sn   = msm_pm_app_register_vbus_sn,
			.pmic_unregister_vbus_sn = msm_pm_app_unregister_vbus_sn,
			.pmic_enable_ldo         = msm_pm_app_enable_usb_ldo,
			.pemp_level              = PRE_EMPHASIS_WITH_10_PERCENT,
			.cdr_autoreset           = CDR_AUTO_RESET_DEFAULT,
			.drv_ampl                = HS_DRV_AMPLITUDE_5_PERCENT,
		};
	 #endif
#else
		static struct msm_otg_platform_data msm_otg_pdata = {
			.rpc_connect	= hsusb_rpc_connect,
			.pmic_notif_init         = msm_pm_app_rpc_init,
			.pmic_notif_deinit       = msm_pm_app_rpc_deinit,
			.pmic_register_vbus_sn   = msm_pm_app_register_vbus_sn,
			.pmic_unregister_vbus_sn = msm_pm_app_unregister_vbus_sn,
			.pmic_enable_ldo         = msm_pm_app_enable_usb_ldo,
			.pemp_level              = PRE_EMPHASIS_WITH_10_PERCENT,
			.cdr_autoreset           = CDR_AUTO_RESET_DEFAULT,
			.drv_ampl                = HS_DRV_AMPLITUDE_5_PERCENT,
		};
#endif

static struct msm_hsusb_gadget_platform_data msm_gadget_pdata;

#if defined(CONFIG_HUAWEI_INCIDENT_LED)
static struct platform_device incident_led_device = {
	.name		= "time_incident_led",
	.id		= -1,
};
#endif

static struct platform_device *early_devices[] __initdata = {
#ifdef CONFIG_GPIOLIB
	&msm_gpio_devices[0],
	&msm_gpio_devices[1],
	&msm_gpio_devices[2],
	&msm_gpio_devices[3],
	&msm_gpio_devices[4],
	&msm_gpio_devices[5],
	&msm_gpio_devices[6],
	&msm_gpio_devices[7],
#endif
};

static struct platform_device *devices[] __initdata = {
	&msm_fb_device,
	&mddi_toshiba_device,
        
#if defined(CONFIG_SMC91X) || defined(CONFIG_SMC91X_MODULE)
	&smc91x_device,
#endif	

	&msm_device_smd,
	&msm_device_dmov,
	&android_pmem_kernel_ebi1_device,
#ifdef CONFIG_KERNEL_PMEM_SMI_REGION
	&android_pmem_kernel_smi_device,
#endif
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_smipool_device,
	&msm_device_nand,
	&msm_device_i2c,
	&device_gpio_i2c_adpt,
#if defined(CONFIG_SPI_QSD) || defined(CONFIG_SPI_QSD_MODULE)
	&qsd_device_spi,
#endif	
#ifdef CONFIG_USB_FUNCTION
	&mass_storage_device,
#endif
#ifdef CONFIG_USB_ANDROID
	&mass_storage_device,
	&android_usb_device,
#endif
#if !HUAWEI_HWID(S70)
	&msm_device_tssc,
#endif
	&msm_audio_device,
	&msm_device_uart_dm1,
	&msm_bluesleep_device,
#ifdef CONFIG_BT
	&msm_bt_power_device,
#endif
#if !defined(CONFIG_MSM_SERIAL_DEBUGGER)
	&msm_device_uart3,
#endif
	&msm_device_pmic_leds,
	&msm_device_kgsl,
	&hs_device,
#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)
	&msm_device_tsif,
#endif
#ifdef CONFIG_MT9T013
	&msm_camera_sensor_mt9t013,
#endif
#ifdef CONFIG_MT9D112
	&msm_camera_sensor_mt9d112,
#endif

#ifdef CONFIG_MT9D113_BACK
	&msm_camera_sensor_mt9d113_back,  
#endif

#ifdef CONFIG_MT9D113
	&msm_camera_sensor_mt9d113,  /*lijuan add*/
#endif

#ifdef CONFIG_OV2650
	&msm_camera_sensor_ov2650,  
#endif

#ifdef CONFIG_S5K5CAG
	&msm_camera_sensor_s5k5cag,  
#endif

#ifdef CONFIG_MT9V114
	&msm_camera_sensor_mt9v114,  
#endif
#ifdef CONFIG_S5K3E2FX
	&msm_camera_sensor_s5k3e2fx,
#endif
#ifdef CONFIG_MT9P012
	&msm_camera_sensor_mt9p012,
#endif
#ifdef CONFIG_MT9P012_KM
	&msm_camera_sensor_mt9p012_km,
#endif
#if defined(CONFIG_OV5630) || defined(CONFIG_OV5630_MODULE)
	&msm_camera_sensor_ov5630,
#endif
	&msm_batt_device,
#if 0
#if defined(CONFIG_HUAWEI_INCIDENT_LED)
	&incident_led_device,
#endif
#endif
};

static void __init qsd8x50_init_irq(void)
{
	msm_init_irq();
	msm_init_sirc();
}

static void kgsl_phys_memory_init(void)
{
	request_mem_region(kgsl_resources[1].start,
		resource_size(&kgsl_resources[1]), "kgsl");
}

static void usb_mpp_init(void)
{
	unsigned rc;
    
#if HUAWEI_HWID_L1(S7)
	unsigned mpp_usb = 7 - 1;
	if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50_surf()) {
#else
	unsigned mpp_usb = 20;
	if (machine_is_qsd8x50_ffa()) {
#endif
		rc = mpp_config_digital_out(mpp_usb,
			MPP_CFG(MPP_DLOGIC_LVL_VDD,
				MPP_DLOGIC_OUT_CTRL_HIGH));
		if (rc)
			pr_err("%s: configuring mpp pin"
				"to enable 3.3V LDO failed\n", __func__);
	}
}

static void __init qsd8x50_init_usb(void)
{
	usb_mpp_init();

#ifdef CONFIG_USB_MSM_OTG_72K
	platform_device_register(&msm_device_otg);
#endif

#ifdef CONFIG_USB_FUNCTION_MSM_HSUSB
	platform_device_register(&msm_device_hsusb_peripheral);
#endif

#ifdef CONFIG_USB_MSM_72K
	platform_device_register(&msm_device_gadget_peripheral);
#endif

	if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa())
		return;

	vreg_usb = vreg_get(NULL, "boost");

	if (IS_ERR(vreg_usb)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_usb));
		return;
	}

	platform_device_register(&msm_device_hsusb_otg);
	msm_add_host(0, &msm_usb_host_pdata);
#ifdef CONFIG_USB_FS_HOST
	if (fsusb_gpio_init())
		return;
	msm_add_host(1, &msm_usb_host2_pdata);
#endif
}

static struct vreg *vreg_emmc;
static struct vreg *vreg_mmc;

#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC3_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC4_SUPPORT))

struct sdcc_gpio {
	struct msm_gpio *cfg_data;
	uint32_t size;
};

static struct msm_gpio sdc1_cfg_data[] = {
	{GPIO_CFG(51, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc1_dat_3"},
	{GPIO_CFG(52, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc1_dat_2"},
	{GPIO_CFG(53, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc1_dat_1"},
	{GPIO_CFG(54, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc1_dat_0"},
	{GPIO_CFG(55, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc1_cmd"},
	{GPIO_CFG(56, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), "sdc1_clk"},
};

static struct msm_gpio sdc2_cfg_data[] = {
	{GPIO_CFG(62, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), "sdc2_clk"},
	{GPIO_CFG(63, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_cmd"},
	{GPIO_CFG(64, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_3"},
	{GPIO_CFG(65, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_2"},
	{GPIO_CFG(66, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_1"},
	{GPIO_CFG(67, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc2_dat_0"},
};

static struct msm_gpio sdc3_cfg_data[] = {
	{GPIO_CFG(88, 1, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), "sdc3_clk"},
	{GPIO_CFG(89, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_cmd"},
	{GPIO_CFG(90, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_3"},
	{GPIO_CFG(91, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_2"},
	{GPIO_CFG(92, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_1"},
	{GPIO_CFG(93, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_0"},

#ifdef CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT
	{GPIO_CFG(158, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_4"},
	{GPIO_CFG(159, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_5"},
	{GPIO_CFG(160, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_6"},
	{GPIO_CFG(161, 1, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc3_dat_7"},
#endif
};

static struct msm_gpio sdc4_cfg_data[] = {
	{GPIO_CFG(142, 3, GPIO_OUTPUT, GPIO_NO_PULL, GPIO_8MA), "sdc4_clk"},
	{GPIO_CFG(143, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_cmd"},
	{GPIO_CFG(144, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_dat_0"},
	{GPIO_CFG(145, 2, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_dat_1"},
	{GPIO_CFG(146, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_dat_2"},
	{GPIO_CFG(147, 3, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_8MA), "sdc4_dat_3"},
};

static struct sdcc_gpio sdcc_cfg_data[] = {
	{
		.cfg_data = sdc1_cfg_data,
		.size = ARRAY_SIZE(sdc1_cfg_data),
	},
	{
		.cfg_data = sdc2_cfg_data,
		.size = ARRAY_SIZE(sdc2_cfg_data),
	},
	{
		.cfg_data = sdc3_cfg_data,
		.size = ARRAY_SIZE(sdc3_cfg_data),
	},
	{
		.cfg_data = sdc4_cfg_data,
		.size = ARRAY_SIZE(sdc4_cfg_data),
	},
};

static unsigned long vreg_sts = 0, gpio_sts = 0;

static void msm_sdcc_setup_gpio(int dev_id, unsigned int enable)
{
	int rc = 0;
	struct sdcc_gpio *curr;

	curr = &sdcc_cfg_data[dev_id - 1];
	if (!(test_bit(dev_id, &gpio_sts)^enable))
		return;

	if (enable) {
		set_bit(dev_id, &gpio_sts);
		rc = msm_gpios_request_enable(curr->cfg_data, curr->size);
		if (rc)
			printk(KERN_ERR "%s: Failed to turn on GPIOs for slot %d\n",
				__func__,  dev_id);
	} else {
		clear_bit(dev_id, &gpio_sts);
		msm_gpios_disable_free(curr->cfg_data, curr->size);
	}
}

#if HUAWEI_HWID(S70)
#define GPIO_SD1_CD			157
static struct semaphore sdcc_sts_sem;
static int vreg_mmc_control(int sdcc_id, int on)
{
	int rc = 0;

    if (on == 0) {
		if (vreg_sts) {
		    clear_bit(sdcc_id, &vreg_sts);
		    if (!vreg_sts) {
		        rc = vreg_disable(vreg_mmc);
                I("Switching %s vreg_mmc %s", 
                    on ? "ON" : "OFF", rc ? "failed" : "OK");
                mmc_delay(10);
		    }
        }
	} else {
	    if (!vreg_sts) {
	        rc = vreg_set_level(vreg_mmc, PMIC_VREG_GP6_LEVEL);
            mmc_delay(10);
	        if (!rc) {
	            rc = vreg_enable(vreg_mmc);
                mmc_delay(10);
	        }
            I("Switching %s vreg_mmc %s", 
                on ? "ON" : "OFF", rc ? "failed" : "OK");
	    }
        set_bit(sdcc_id, &vreg_sts);
	}

//    I("vreg_mmc control status: vreg_sts=%08x sdccid=%d on=%d", 
//       (uint32_t)vreg_sts, sdcc_id, on);
    
    return rc;
}
#endif

#if !HUAWEI_HWID(S70)
static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
	msm_sdcc_setup_gpio(pdev->id, !!vdd);

	if (vdd == 0) {
		if (!vreg_sts)
			return 0;

		clear_bit(pdev->id, &vreg_sts);

		if (!vreg_sts) {
			rc = vreg_disable(vreg_mmc);
			if (rc)
				printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
		}
		return 0;
	}

	if (!vreg_sts) {
		rc = vreg_set_level(vreg_mmc, PMIC_VREG_GP6_LEVEL);
		if (!rc)
			rc = vreg_enable(vreg_mmc);
		if (rc)
			printk(KERN_ERR "%s: return val: %d \n",
					__func__, rc);
	}
	set_bit(pdev->id, &vreg_sts);
	return 0;
}
#else
static uint32_t msm_sdcc_setup_power(struct device *dv, unsigned int vdd)
{
	struct platform_device *pdev;
    
	pdev = container_of(dv, struct platform_device, dev);

    down(&sdcc_sts_sem);

	msm_sdcc_setup_gpio(pdev->id, !!vdd);
	mmc_delay(10);

	/* 
	 * !!! we should always keep SD Card(VREG_P6) on because of bug of PMIC7540 
	 *     and avoiding SD Card re-detect by vold result in causing remounting, 
     *     sounding and screen on
	 */
#ifdef CONFIG_MMC_MSM_NOT_REMOVE_CARD_WHEN_SUSPEND
	vreg_mmc_control(pdev->id, 1);
#else
	vreg_mmc_control(pdev->id, !!vdd);
#endif
    up(&sdcc_sts_sem);

	return 0;
}
#endif
#endif

#if (defined(CONFIG_MMC_MSM_SDC1_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC2_SUPPORT)\
	|| defined(CONFIG_MMC_MSM_SDC4_SUPPORT))
static int msm_sdcc_get_wpswitch(struct device *dv)
{
	void __iomem *wp_addr = 0;
	uint32_t ret = 0;
	struct platform_device *pdev;

	if (!(machine_is_qsd8x50_surf() || machine_is_qsd8x50a_surf()))
		return -1;

	pdev = container_of(dv, struct platform_device, dev);

	wp_addr = ioremap(FPGA_SDCC_STATUS, 4);
	if (!wp_addr) {
		pr_err("%s: Could not remap %x\n", __func__, FPGA_SDCC_STATUS);
		return -ENOMEM;
	}

	ret = (readl(wp_addr) >> ((pdev->id - 1) << 1)) & (0x03);
	pr_info("%s: WP/CD Status for Slot %d = 0x%x \n", __func__,
							pdev->id, ret);
	iounmap(wp_addr);
	return ((ret == 0x02) ? 1 : 0);

}
#endif

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
#if defined(CONFIG_MMC_MSM_CARD_HW_DETECTION) && HUAWEI_HWID(S70)
static unsigned int qsd8x50_sdc1_slot_status(struct device *dev)
{
	return !(unsigned int)gpio_get_value(GPIO_SD1_CD);
}
#endif

static struct mmc_platform_data qsd8x50_sdc1_data = {
	.ocr_mask	= MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.wpswitch	= msm_sdcc_get_wpswitch,
#ifdef CONFIG_MMC_MSM_SDC1_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
#if defined(CONFIG_MMC_MSM_CARD_HW_DETECTION) && HUAWEI_HWID(S70)
    .status = qsd8x50_sdc1_slot_status,
    .status_irq = MSM_GPIO_TO_INT(GPIO_SD1_CD),
	.irq_flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 25000000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 0,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
#if !HUAWEI_HWID(S70)

static struct mmc_platform_data qsd8x50_sdc2_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.wpswitch	= msm_sdcc_get_wpswitch,
#ifdef CONFIG_MMC_MSM_SDC2_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 25000000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 1,
};

#else

#define MODE_BCM4325_REG_ON_LEVEL		0
#define MODE_BCM4325_WL_RST_N_LEVEL		1
#define MODE_BCM4325_HOST_WAKE_WL		2

static unsigned int bcm4325_level_status = 0;
static int bcm4325_init_flag = 0;
static struct semaphore bcm4325_pmsem;

static uint32_t msm_pm_setup_status(struct device *dv, unsigned int vdd)
{
	struct platform_device *pdev;

	pdev = container_of(dv, struct platform_device, dev);
    down(&sdcc_sts_sem);
	msm_sdcc_setup_gpio(pdev->id, !!vdd);
    mmc_delay(10);

    /* 
     * !!! we should always keep SD Card(VREG_P6) on because of bug of PMIC7540 
     *     and avoiding SD Card re-detect by vold result in causing remounting, 
     *     sounding and screen on
     */
#ifdef CONFIG_MMC_MSM_NOT_REMOVE_CARD_WHEN_SUSPEND
	vreg_mmc_control(pdev->id, 1);
#else
	vreg_mmc_control(pdev->id, !!vdd);
#endif

    up(&sdcc_sts_sem);

	return 0;
}

#if defined(CONFIG_EMBEDDED_SDIO_DATA)
static struct sdio_embedded_func wlan_func = {
	.f_class		= SDIO_CLASS_WLAN,
	.f_maxblksize	= 64,
};
#endif

static void (*wlan_status_cb)(int card_present, void *dev_id);
static void *wlan_status_cb_devid;
static int qsd8x50_wlan_cd = 0;

static int qsd8x50_register_status_notify(void (*callback)(int card_present,
							  							   void *dev_id),
					 					  void *dev_id)
{
	if (wlan_status_cb)
		return -EAGAIN;
	wlan_status_cb = callback;
	wlan_status_cb_devid = dev_id;
	return 0;
}

static unsigned int qsd8x50_wlan_status(struct device *dev)
{
	return qsd8x50_wlan_cd;
}

int qsd8x50_wlan_set_carddetect(int val)
{
	qsd8x50_wlan_cd = val;
	if (wlan_status_cb)
		wlan_status_cb(val, wlan_status_cb_devid);
	else
		I("Nobody to notify");
	return 0;
}
EXPORT_SYMBOL(qsd8x50_wlan_set_carddetect);

#if 0
int qsd8x50_wlan_get_carddetect(void)
{
	return qsd8x50_wlan_status(NULL);
}
EXPORT_SYMBOL(qsd8x50_wlan_get_carddetect);
#endif

static int bcm4325_pm_init(void)
{
	int rc;

	if ((rc = gpio_request(GPIO_BCM4325_HOST_WAKE_WL, "bcm4325_wl_waked"))) {
	    pr_err("gpio_request failed on pin %d\n", GPIO_BCM4325_HOST_WAKE_WL);
	    goto err_bcm4325_pm_init2;
	}
    
	if ((rc = gpio_request(GPIO_BCM4325_WL_WAKE_HOST, "bcm4325_wl_wake_host"))) {
	    pr_err("gpio_request failed on pin %d\n", GPIO_BCM4325_WL_WAKE_HOST);
	    goto err_bcm4325_pm_init3;
	}
    
	if ((rc = gpio_request(GPIO_BCM4325_WL_RST_N, "bcm4325_wl_rstn"))) {
	    pr_err("gpio_request failed on pin %d\n", GPIO_BCM4325_WL_RST_N);
	    goto err_bcm4325_pm_init4;
	}

    init_MUTEX(&bcm4325_pmsem);
    if (down_interruptible(&bcm4325_pmsem))
		return -1;
    bcm4325_init_flag = 1;
    bcm4325_level_status = 0;
    up(&bcm4325_pmsem);

	return 0;

	gpio_free(GPIO_BCM4325_WL_RST_N);    
err_bcm4325_pm_init4:
	gpio_free(GPIO_BCM4325_WL_WAKE_HOST);
err_bcm4325_pm_init3:
	gpio_free(GPIO_BCM4325_HOST_WAKE_WL);
err_bcm4325_pm_init2:
    bcm4325_init_flag = 0;
    return -1;
}

/* power control of wifi, on_level = 1: power on, 2: reset on, 3: wake on */
void bcm_wlan_power_on(int on_level)
{
	int rc;

	unsigned mpp_wl_reg_on;
#if HWVERID_HIGHER(S70, A)
    mpp_wl_reg_on = 3 - 1;
#else
    mpp_wl_reg_on = 19 - 1;
#endif

	pr_info("[%s,%d] on_level = %d\n", __FUNCTION__, __LINE__, on_level);

	/* put HOST_WAKE_WL to HIGH */
	if (on_level == 1 || on_level == 3) {
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_BCM4325_HOST_WAKE_WL, 0, GPIO_OUTPUT, 
					GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		if (rc) 
			printk(KERN_ERR "%s: config HOST_WAKE_WL failed: %d\n",__func__, rc);
		gpio_set_value(GPIO_BCM4325_HOST_WAKE_WL, 1);
		if (on_level == 3)
			return;
	}

	if (on_level == 1) {
		rc = mpp_config_digital_out(mpp_wl_reg_on,
				     	MPP_CFG(MPP_DLOGIC_LVL_MSMP,
				     	MPP_DLOGIC_OUT_CTRL_HIGH));
		if (rc) 
			printk(KERN_ERR "%s: config WL_REG_ON failed: %d\n",__func__, rc);

		mmc_delay(160);

		/* gpio 32 is for WL_WOW - WiFi reset */
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_BCM4325_WL_RST_N, 0, GPIO_OUTPUT, 
					GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		if (rc) {
			printk(KERN_ERR "%s: config WL_WOW failed: %d\n",__func__, rc);
		}
		gpio_set_value(GPIO_BCM4325_WL_RST_N, 1);
		/* reset again */
/* reduce response time */
#if 0
		mmc_delay(160);
		gpio_set_value(GPIO_BCM4325_WL_RST_N, 0);
		mmc_delay(50);
		gpio_set_value(GPIO_BCM4325_WL_RST_N, 1);
#endif
	} else if (on_level == 2) {
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_BCM4325_WL_RST_N, 0, GPIO_OUTPUT, 
					GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
		if (rc) 
			printk(KERN_ERR "%s: config WLAN_WOW failed: %d\n",__func__, rc);

		gpio_set_value(GPIO_BCM4325_WL_RST_N, 1);
	}

	mmc_delay(160);
}
EXPORT_SYMBOL(bcm_wlan_power_on);

/* power control of wifi, off_level = 1: power off, 2: reset off, 3: wake off */
void bcm_wlan_power_off(int off_level)
{
	int rc;

	unsigned mpp_wl_reg_on;

#if HWVERID_HIGHER(S70, A)
	mpp_wl_reg_on = 3 - 1;
#else
    mpp_wl_reg_on = 19 - 1;
#endif

	pr_info("[%s,%d] off_level = %d\n", __FUNCTION__, __LINE__, off_level);

	if (off_level == 1) {
		/*gpio 32 is  WLAN_WOW - WiFi Reset*/
		gpio_set_value(GPIO_BCM4325_WL_RST_N, 0);
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_BCM4325_WL_RST_N, 0, GPIO_OUTPUT, 
					GPIO_NO_PULL, GPIO_2MA), GPIO_DISABLE);
		if (rc)
			printk(KERN_ERR "%s: config WLAN_WOW failedi: %d\n", __func__, rc);

		rc = mpp_config_digital_out(mpp_wl_reg_on,
				     	MPP_CFG(MPP_DLOGIC_LVL_MSMP,
				     	MPP_DLOGIC_OUT_CTRL_LOW));
		if (rc)
     		printk(KERN_ERR "%s: config WL_REG_ON failed: %d\n",__func__, rc);
	} else if (off_level == 2) {
		gpio_set_value(GPIO_BCM4325_WL_RST_N, 0);
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_BCM4325_WL_RST_N, 0, GPIO_OUTPUT, 
					GPIO_NO_PULL, GPIO_2MA), GPIO_DISABLE);
		if (rc)
			printk(KERN_ERR "%s: config WLAN_WOW failedi: %d\n", __func__, rc);
	}

	/* put HOST_WAKE_WL to LOW */
	if (off_level == 1 || off_level == 3) {
		gpio_set_value(GPIO_BCM4325_HOST_WAKE_WL, 0);
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_BCM4325_HOST_WAKE_WL, 0, GPIO_OUTPUT, 
					GPIO_NO_PULL, GPIO_2MA), GPIO_DISABLE);
		if (rc)
			printk(KERN_ERR "%s: config WLAN_WOW failedi: %d\n", __func__, rc);

		if (off_level == 3)
			return;
	}
}
EXPORT_SYMBOL(bcm_wlan_power_off);

static struct mmc_platform_data qsd8x50_wlan_data = {
	.ocr_mask	= MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd	= msm_pm_setup_status,
    .mmc_bus_width  = MMC_CAP_4_BIT_DATA,
    .wpswitch   = msm_sdcc_get_wpswitch,
#ifdef CONFIG_MMC_MSM_SDC2_DUMMY52_REQUIRED
    .dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 25000000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 1,
	.status			= qsd8x50_wlan_status,
	.register_status_notify	= qsd8x50_register_status_notify,
};
#endif
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
#if HWVERID_HIGHER(S70, T1)
static uint32_t msm_sdcc3_setup_power(struct device *dv, unsigned int vdd)
{
	int rc = 0;
	struct platform_device *pdev;
    int on = !!vdd;

	pdev = container_of(dv, struct platform_device, dev);

    down(&sdcc_sts_sem);

	msm_sdcc_setup_gpio(pdev->id, !!vdd);
    mmc_delay(10);

    /* 
     * !!! we should always keep SD Card(VREG_P6) on because of bug of PMIC7540 
     *     and avoiding SD Card re-detect by vold result in causing remounting, 
     *     sounding and screen on
     */
#ifdef CONFIG_MMC_MSM_NOT_REMOVE_CARD_WHEN_SUSPEND
	on = 1;
#endif

	if (on == 0) {
		if (test_bit(pdev->id, &vreg_sts)) {
	        if (vreg_emmc) {
	            rc = vreg_disable(vreg_emmc);
	            mmc_delay(10);            
		        I("Switching %s vreg_emmc in %s %s", 
		            on ? "ON" : "OFF", __FUNCTION__, rc ? "failed" : "OK");
	        }
            vreg_mmc_control(pdev->id, on);
        }
	} else {
		if (!test_bit(pdev->id, &vreg_sts)) {
            vreg_mmc_control(pdev->id, on);
		    if (vreg_emmc) {
		        rc = vreg_set_level(vreg_emmc, 2850);
	            mmc_delay(10);
	            rc = vreg_enable(vreg_emmc);
	            mmc_delay(10);
		        I("Switching %s vreg_emmc in %s %s", 
		            on ? "ON" : "OFF", __FUNCTION__, rc ? "failed" : "OK");
		    }
        }
    }

    up(&sdcc_sts_sem);
    
	return 0;
}
#endif

static struct mmc_platform_data qsd8x50_sdc3_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
#if HWVERID_HIGHER(S70, T1)
    .translate_vdd  = msm_sdcc3_setup_power,
#else
    .translate_vdd  = msm_sdcc_setup_power,
#endif
#ifdef CONFIG_MMC_MSM_SDC3_8_BIT_SUPPORT
	.mmc_bus_width  = MMC_CAP_8_BIT_DATA,
#else
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
#endif
#ifdef CONFIG_MMC_MSM_SDC3_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 25000000,
	.msmsdcc_fmax	= 49152000,//25000000,
	.nonremovable	= 1,
};
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct mmc_platform_data qsd8x50_sdc4_data = {
	.ocr_mask       = MMC_VDD_27_28 | MMC_VDD_28_29,
	.translate_vdd  = msm_sdcc_setup_power,
	.mmc_bus_width  = MMC_CAP_4_BIT_DATA,
	.wpswitch	= msm_sdcc_get_wpswitch,
#ifdef CONFIG_MMC_MSM_SDC4_DUMMY52_REQUIRED
	.dummy52_required = 1,
#endif
	.msmsdcc_fmin	= 144000,
	.msmsdcc_fmid	= 25000000,
	.msmsdcc_fmax	= 49152000,
	.nonremovable	= 0,
};
#endif

static void __init qsd8x50_init_mmc(void)
{
#if HWVERID_HIGHER(S70, T1)
    init_MUTEX(&sdcc_sts_sem);
	vreg_mmc = vreg_get(NULL, "gp6");
#else
	if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa())
		vreg_mmc = vreg_get(NULL, "gp6");
	else
		vreg_mmc = vreg_get(NULL, "gp5");
#endif

	if (IS_ERR(vreg_mmc)) {
		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
		       __func__, PTR_ERR(vreg_mmc));
		return;
	}

#if defined(CONFIG_MMC_MSM_CARD_HW_DETECTION) && HUAWEI_HWID(S70)
    if (gpio_request(GPIO_SD1_CD, "sdc1_status_irq"))
        I("request GPIO %d for sdc1_status_irq failed", GPIO_SD1_CD);
    else
        I("request GPIO %d OK", GPIO_SD1_CD);
    if (gpio_tlmm_config(GPIO_CFG(GPIO_SD1_CD, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA), 
        				 GPIO_ENABLE)) {
        I("configure GPIO %d failed", GPIO_SD1_CD);
        gpio_free(GPIO_SD1_CD);
        return;
    } else 
        I("configure GPIO %d OK", GPIO_SD1_CD);
#endif

#ifdef CONFIG_MMC_MSM_SDC1_SUPPORT
	msm_add_sdcc(1, &qsd8x50_sdc1_data);
#endif

	if (machine_is_qsd8x50_surf() || machine_is_qsd8x50a_surf()) {
#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
#if HWVERID_HIGHER(S70, T1)
		msm_add_sdcc(2, &qsd8x50_wlan_data);
#else
		msm_add_sdcc(2, &qsd8x50_sdc2_data);
#endif
#endif

#ifdef CONFIG_MMC_MSM_SDC3_SUPPORT
#if HWVERID_HIGHER(S70, A)
        vreg_emmc = vreg_get(NULL, "wlan");
        if (IS_ERR(vreg_emmc)) {
    		printk(KERN_ERR "%s: vreg get failed (%ld)\n",
    		       __func__, PTR_ERR(vreg_emmc));
	    }
#endif
		msm_add_sdcc(3, &qsd8x50_sdc3_data);
#endif
#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
		msm_add_sdcc(4, &qsd8x50_sdc4_data);
#endif
	} else
		printk(KERN_ERR "%s: invalid machine type\n", __func__);
}

#if defined(CONFIG_SMC91X) || defined(CONFIG_SMC91X_MODULE)
static void __init qsd8x50_cfg_smc91x(void)
{
	int rc = 0;

	if (machine_is_qsd8x50_surf() || machine_is_qsd8x50a_surf()) {
		smc91x_resources[0].start = 0x70000300;
		smc91x_resources[0].end = 0x700003ff;
		smc91x_resources[1].start = MSM_GPIO_TO_INT(156);
		smc91x_resources[1].end = MSM_GPIO_TO_INT(156);
	} else if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa()) {
		smc91x_resources[0].start = 0x84000300;
		smc91x_resources[0].end = 0x840003ff;
		smc91x_resources[1].start = MSM_GPIO_TO_INT(87);
		smc91x_resources[1].end = MSM_GPIO_TO_INT(87);

		rc = gpio_tlmm_config(GPIO_CFG(87, 0, GPIO_INPUT,
					       GPIO_PULL_DOWN, GPIO_2MA),
					       GPIO_ENABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config=%d\n",
					__func__, rc);
		}
	} else
		printk(KERN_ERR "%s: invalid machine type\n", __func__);
}
#endif

static struct msm_pm_platform_data msm_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].latency = 8594,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE].residency = 23740,

	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].supported = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].latency = 4594,
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN].residency = 23740,

	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].suspend_enabled
		= 1,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].idle_enabled = 0,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency = 443,
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].residency = 1098,

	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].supported = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].suspend_enabled = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].idle_enabled = 1,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].latency = 2,
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT].residency = 0,
};

static void
msm_i2c_gpio_config(int iface, int config_type)
{
	int gpio_scl;
	int gpio_sda;

#if HUAWEI_HWID(S70)
	gpio_scl = 95;
	gpio_sda = 96;
#else
	if (iface) {
		gpio_scl = 60;
		gpio_sda = 61;
	} else {
		gpio_scl = 95;
		gpio_sda = 96;
	}
#endif

	if (config_type) {
		gpio_tlmm_config(GPIO_CFG(gpio_scl, 1, GPIO_INPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
		gpio_tlmm_config(GPIO_CFG(gpio_sda, 1, GPIO_INPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
	} else {
		gpio_tlmm_config(GPIO_CFG(gpio_scl, 0, GPIO_OUTPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
		gpio_tlmm_config(GPIO_CFG(gpio_sda, 0, GPIO_OUTPUT,
					GPIO_NO_PULL, GPIO_16MA), GPIO_ENABLE);
	}
}

static struct msm_i2c_platform_data msm_i2c_pdata = {
	.clk_freq = 100000,
	.rsl_id = SMEM_SPINLOCK_I2C,
	.pri_clk = 95,
	.pri_dat = 96,
	.aux_clk = 60,
	.aux_dat = 61,
	.msm_i2c_config_gpio = msm_i2c_gpio_config,
};

#if defined(CONFIG_GPIO_I2C_ADPT)
static void gpio_i2c_adpt_config_gpio(unsigned int gpio_scl, unsigned int gpio_sda)
{
	gpio_tlmm_config(GPIO_CFG(gpio_scl, 0, GPIO_OUTPUT,
				GPIO_PULL_UP, GPIO_2MA), GPIO_ENABLE);
	gpio_tlmm_config(GPIO_CFG(gpio_sda, 0, GPIO_OUTPUT,
				GPIO_PULL_UP, GPIO_2MA), GPIO_ENABLE);
}

static struct gpio_i2c_adpt_platform_data gpio_i2c_adpt_pdata = {
	.dely_usec = 1,				/*delay 5 usec for 100kHZ i2c clock frequency*/
	.gpio_i2c_clk = GPIO_I2C_SCL,
	.gpio_i2c_dat = GPIO_I2C_SDA,
	.gpio_i2c_adpt_config_gpio = gpio_i2c_adpt_config_gpio,
};
#endif
static void __init msm_device_i2c_init(void)
{
	if (gpio_request(95, "i2c_pri_clk"))
		pr_err("failed to request gpio i2c_pri_clk\n");
	if (gpio_request(96, "i2c_pri_dat"))
		pr_err("failed to request gpio i2c_pri_dat\n");

#if !HUAWEI_HWID(S70)
	if (gpio_request(60, "i2c_sec_clk"))
		pr_err("failed to request gpio i2c_sec_clk\n");
	if (gpio_request(61, "i2c_sec_dat"))
		pr_err("failed to request gpio i2c_sec_dat\n");
#endif

	msm_i2c_pdata.rmutex = (uint32_t)smem_alloc(SMEM_I2C_MUTEX, 8);
	msm_i2c_pdata.pm_lat =
		msm_pm_data[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN]
		.latency;
	msm_device_i2c.dev.platform_data = &msm_i2c_pdata;
}

#if defined(CONFIG_GPIO_I2C_ADPT)
static void __init msm_device_gpio_i2c_adpt_init(void)
{
	if (gpio_request(gpio_i2c_adpt_pdata.gpio_i2c_clk, "gpio_i2c_clk"))
		pr_err("failed to request gpio gpio_i2c_clk\n");
	if (gpio_request(gpio_i2c_adpt_pdata.gpio_i2c_dat, "gpio_i2c_dat"))
		pr_err("failed to request gpio gpio_i2c_dat\n");

	device_gpio_i2c_adpt.dev.platform_data = &gpio_i2c_adpt_pdata;
}
#endif

static unsigned pmem_kernel_ebi1_size = PMEM_KERNEL_EBI1_SIZE;
static void __init pmem_kernel_ebi1_size_setup(char **p)
{
	pmem_kernel_ebi1_size = memparse(*p, p);
}
__early_param("pmem_kernel_ebi1_size=", pmem_kernel_ebi1_size_setup);

#ifdef CONFIG_KERNEL_PMEM_SMI_REGION
static unsigned pmem_kernel_smi_size = MSM_PMEM_SMIPOOL_SIZE;
static void __init pmem_kernel_smi_size_setup(char **p)
{
	pmem_kernel_smi_size = memparse(*p, p);

	/* Make sure that we don't allow more SMI memory then is
	   available - the kernel mapping code has no way of knowing
	   if it has gone over the edge */

	if (pmem_kernel_smi_size > MSM_PMEM_SMIPOOL_SIZE)
		pmem_kernel_smi_size = MSM_PMEM_SMIPOOL_SIZE;
}
__early_param("pmem_kernel_smi_size=", pmem_kernel_smi_size_setup);
#endif

static unsigned pmem_sf_size = MSM_PMEM_SF_SIZE;
static void __init pmem_sf_size_setup(char **p)
{
	pmem_sf_size = memparse(*p, p);
}
__early_param("pmem_sf_size=", pmem_sf_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;
static void __init pmem_adsp_size_setup(char **p)
{
	pmem_adsp_size = memparse(*p, p);
}
__early_param("pmem_adsp_size=", pmem_adsp_size_setup);


static unsigned audio_size = MSM_AUDIO_SIZE;
static void __init audio_size_setup(char **p)
{
	audio_size = memparse(*p, p);
}
__early_param("audio_size=", audio_size_setup);

#if HUAWEI_HWID(S70)
static struct msm_ts_platform_data msm_ts_pdata = {
	.x_max = 1024,
	.y_max = 1024,
	.pressure_max = 64,
};

static int msm_add_tscc(void)
{
	struct platform_device	*pdev;

	pdev = &msm_device_tssc;
	pdev->dev.platform_data = &msm_ts_pdata;
	return platform_device_register(pdev);
}
#endif

static int bs300_power_init(int on)
 {
	struct vreg *vreg_mmc; 
#if HUAWEI_HWID_L2(S7, S7201)    
      struct vreg *vreg_gp4;
#endif
	int ret;

#if HUAWEI_HWID_L2(S7, S7201)    
	struct vreg *vreg_pa;
	struct vreg *vreg_msme2;
        
	vreg_pa = vreg_get(NULL, "pa");
    	if (IS_ERR(vreg_pa)) 
	{
		printk(KERN_ERR "%s: vreg_get pa failed (%ld)\n",
		    __func__, PTR_ERR(vreg_pa));
		return -EIO;
	}
        
       ret = vreg_set_level(vreg_pa, 1800);
	if (ret) 
	{
		printk(KERN_ERR "%s: vreg pa set level failed (%d)\n",
		    __func__, ret);
		return -EIO;
	}
	
	if ((ret = gpio_request(GPIO_BS300_WAKEUP, "bs300_wakeup"))) {
	    printk(KERN_ERR "%s: gpio_request failed on pin %d\n",__func__, GPIO_BS300_WAKEUP);
	}
	/* gpio 103 is for BS300_WAKEUP  */
	ret = gpio_tlmm_config(GPIO_CFG(GPIO_BS300_WAKEUP, 0, GPIO_OUTPUT, 
				GPIO_PULL_UP, GPIO_2MA), GPIO_ENABLE);
	if (ret) {
		printk(KERN_ERR "%s: config BS300_WAKEUP failed: %d\n",__func__, ret);
	}
	gpio_set_value(GPIO_BS300_WAKEUP, 1);
#endif    

	/* 1.8 V */
	vreg_mmc = vreg_get(NULL, "mmc");
	if (IS_ERR(vreg_mmc)) 
	{
		printk(KERN_ERR "%s: vreg_get mmc failed (%ld)\n",
		    __func__, PTR_ERR(vreg_mmc));
		return -EIO;
	}
	ret = vreg_set_level(vreg_mmc, 1800);
	if (ret) 
	{
		printk(KERN_ERR "%s: vreg mmc set level failed (%d)\n",
		    __func__, ret);
		return -EIO;
	}

#if HUAWEI_HWID_L2(S7, S7201) 
	vreg_gp4 = vreg_get(NULL, "gp4");
	if (IS_ERR(vreg_mmc)) 
	{
		printk(KERN_ERR "%s: vreg_get gp4 failed (%ld)\n",
		    __func__, PTR_ERR(vreg_gp4));
		return -EIO;
	}
	ret = vreg_set_level(vreg_gp4, 1800);
	if (ret) 
	{
		printk(KERN_ERR "%s: vreg gp4 set level failed (%d)\n",
		    __func__, ret);
		return -EIO;
	} 
#endif

#if HUAWEI_HWID_L2(S7, S7201)          
       vreg_msme2 = vreg_get(NULL, "msme2");
       if (IS_ERR(vreg_msme2)) 
	{
		printk(KERN_ERR "%s: vreg_get pa failed (%ld)\n",
		    __func__, PTR_ERR(vreg_msme2));
		return -EIO;
	}

       ret = vreg_set_level(vreg_msme2, 1800);
	if (ret) 
	{
		printk(KERN_ERR "%s: vreg msme2 set level failed (%d)\n",
		    __func__, ret);
		return -EIO;
	}
#endif 
	if (on)
	{
#if HUAWEI_HWID_L2(S7, S7201)               
		ret = vreg_enable(vreg_pa);
		if (ret)
		{
			printk(KERN_ERR "vreg pa enable failed");
			return -EIO;
		}
#endif        

		ret = vreg_enable(vreg_mmc);
		if (ret)
		{
			printk(KERN_ERR "vreg mmc enable failed");
			return -EIO;
		}
#if HUAWEI_HWID_L2(S7, S7201) 
		ret = vreg_enable(vreg_gp4);
		if (ret)
		{
			printk(KERN_ERR "vreg gp4 enable failed");
			return -EIO;
		}
#endif


#if HUAWEI_HWID_L2(S7, S7201)   
		ret = vreg_enable(vreg_msme2);
		if (ret)
		{
			printk(KERN_ERR "vreg pa enable failed");
			return -EIO;
		}
#endif
	}
	return 0;
}


static void __init qsd8x50_init(void)
{
#if HUAWEI_HWID(S70)
    if (gpio_request(GPIO_GPS_LNA_EN, "gps_lna_en")) {
	    pr_err("gpio_request failed on pin %d\n", GPIO_GPS_LNA_EN);
	} else
		gpio_direction_output(GPIO_GPS_LNA_EN, 1);
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
#if HWVERID_HIGHER(S70, T1)
	bcm4325_pm_init();
#endif
#endif

	if (socinfo_init() < 0)
		printk(KERN_ERR "%s: socinfo_init() failed!\n",
		       __func__);
	msm_clock_init(msm_clocks_8x50, msm_num_clocks_8x50);
	platform_add_devices(early_devices, ARRAY_SIZE(early_devices));
#if defined(CONFIG_SMC91X) || defined(CONFIG_SMC91X_MODULE)
	qsd8x50_cfg_smc91x();
#endif

	msm_acpu_clock_init(&qsd8x50_clock_data);

	msm_hsusb_pdata.swfi_latency =
		msm_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_hsusb_peripheral.dev.platform_data = &msm_hsusb_pdata;

	msm_gadget_pdata.swfi_latency =
		msm_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_otg.dev.platform_data = &msm_otg_pdata;
	msm_device_gadget_peripheral.dev.platform_data = &msm_gadget_pdata;

#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)
	msm_device_tsif.dev.platform_data = &tsif_platform_data;
#endif

#if HUAWEI_HWID(S70)
    msm_add_tscc();
#endif

	platform_add_devices(devices, ARRAY_SIZE(devices));
	msm_fb_add_devices();
#ifdef CONFIG_MSM_CAMERA
       /*lijuan rm*/
	/*config_camera_off_gpios();*/ /* might not be necessary */
#endif
	qsd8x50_init_usb();
	qsd8x50_init_mmc();
#if defined(CONFIG_MOUSE_OFN_HID) || defined(CONFIG_MOUSE_OFN_HID_MODULE)
	ofn_power_init();
#endif

	bt_power_init();
	audio_gpio_init();

#if defined(CONFIG_CYPRESS120) || defined(CONFIG_CYPRESS120_MODULE)
    ctp_gpio_init();
#endif
	msm_device_i2c_init();
#if defined(CONFIG_GPIO_I2C_ADPT)
	msm_device_gpio_i2c_adpt_init();
#endif
	bs300_power_init(1);


#if defined(CONFIG_SPI_QSD) || defined(CONFIG_SPI_QSD_MODULE)
	msm_qsd_spi_init();
#endif

	i2c_register_board_info(0, msm_i2c_board_info,
				ARRAY_SIZE(msm_i2c_board_info));

	i2c_register_board_info(2, gpio_i2c_board_info,
				ARRAY_SIZE(gpio_i2c_board_info));
#if defined(CONFIG_SPI_QSD) || defined(CONFIG_SPI_QSD_MODULE)
	spi_register_board_info(msm_spi_board_info,
				ARRAY_SIZE(msm_spi_board_info));
#endif

	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));
	kgsl_phys_memory_init();

#ifdef CONFIG_SURF_FFA_GPIO_KEYPAD
	if (machine_is_qsd8x50_ffa() || machine_is_qsd8x50a_ffa())
		platform_device_register(&keypad_device_8k_ffa);
	else
		platform_device_register(&keypad_device_surf);
#endif

#ifdef CONFIG_HID_GPIO_KEYPAD
	platform_device_register(&hid_keypad_device);
#endif

#if HUAWEI_HWID(S70)
	{
	    struct vreg *vreg_ruim;

		vreg_ruim = vreg_get(0, "ruim");
		if (!vreg_ruim) {
			printk("ZRF:error1\n\r ") ;
		}

		if (vreg_set_level(vreg_ruim, 1800)) {
			printk("ZRF:error2\n\r ");
		}

		if (vreg_enable(vreg_ruim)) {
			printk("ZRF:error3\n\r ") ;
		}
    }
#endif

#if defined(CONFIG_HUAWEI_INCIDENT_LED)
	init_incident_led_device();
#endif

#ifdef CONFIG_HUAWEI_MSM_VIBRATOR
	init_vibrator_device();
#endif

#ifdef CONFIG_DOCK_DET
    init_dock_detect();
#endif
#ifdef CONFIG_HUAWEI_CDMA_EVDO
	gpio_tlmm_config(GPIO_CFG(26, 0, GPIO_OUTPUT, 
					GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
	gpio_set_value(26, 0);

	gpio_tlmm_config(GPIO_CFG(29, 0, GPIO_OUTPUT, 
					GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
	gpio_set_value(29, 0);
 #endif

}

static void __init qsd8x50_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;

	size = pmem_kernel_ebi1_size;
	if (size) {
		addr = alloc_bootmem_aligned(size, 0x100000);
		android_pmem_kernel_ebi1_pdata.start = __pa(addr);
		android_pmem_kernel_ebi1_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for kernel"
			" ebi1 pmem arena\n", size, addr, __pa(addr));
	}

#ifdef CONFIG_KERNEL_PMEM_SMI_REGION
	size = pmem_kernel_smi_size;
	if (size > MSM_PMEM_SMIPOOL_SIZE) {
		printk(KERN_ERR "pmem kernel smi arena size %lu is too big\n",
			size);

		size = MSM_PMEM_SMIPOOL_SIZE;
	}

	android_pmem_kernel_smi_pdata.start = MSM_PMEM_SMIPOOL_BASE;
	android_pmem_kernel_smi_pdata.size = size;

	pr_info("allocating %lu bytes at %lx (%lx physical)"
		"for pmem kernel smi arena\n", size,
		(long unsigned int) MSM_PMEM_SMIPOOL_BASE,
		__pa(MSM_PMEM_SMIPOOL_BASE));
#endif

	size = pmem_sf_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_pdata.start = __pa(addr);
		android_pmem_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for sf "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_adsp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_adsp_pdata.start = __pa(addr);
		android_pmem_adsp_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for adsp "
			"pmem arena\n", size, addr, __pa(addr));
	}


	size = MSM_FB_SIZE;
	addr = (void *)MSM_FB_BASE;
	msm_fb_resources[0].start = (unsigned long)addr;
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("using %lu bytes of SMI at %lx physical for fb\n",
	       size, (unsigned long)addr);

	size = audio_size ? : MSM_AUDIO_SIZE;
	addr = alloc_bootmem(size);
	msm_audio_resources[0].start = __pa(addr);
	msm_audio_resources[0].end = msm_audio_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for audio\n",
		size, addr, __pa(addr));
}

static void __init qsd8x50_map_io(void)
{
	msm_shared_ram_phys = MSM_SHARED_RAM_PHYS;
	msm_map_qsd8x50_io();
	qsd8x50_allocate_memory_regions();
}

MACHINE_START(QSD8X50_SURF, "QCT QSD8X50 SURF")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = qsd8x50_map_io,
	.init_irq = qsd8x50_init_irq,
	.init_machine = qsd8x50_init,
	.timer = &msm_timer,
MACHINE_END

MACHINE_START(QSD8X50_FFA, "QCT QSD8X50 FFA")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = qsd8x50_map_io,
	.init_irq = qsd8x50_init_irq,
	.init_machine = qsd8x50_init,
	.timer = &msm_timer,
MACHINE_END

MACHINE_START(QSD8X50A_SURF, "QCT QSD8X50A SURF")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = qsd8x50_map_io,
	.init_irq = qsd8x50_init_irq,
	.init_machine = qsd8x50_init,
	.timer = &msm_timer,
MACHINE_END

MACHINE_START(QSD8X50A_FFA, "QCT QSD8X50A FFA")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.boot_params = PHYS_OFFSET + 0x100,
	.map_io = qsd8x50_map_io,
	.init_irq = qsd8x50_init_irq,
	.init_machine = qsd8x50_init,
	.timer = &msm_timer,
MACHINE_END
