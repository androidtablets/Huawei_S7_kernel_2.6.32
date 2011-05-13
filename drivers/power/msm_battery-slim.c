/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <linux/earlysuspend.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <asm/atomic.h>

#include <mach/msm_rpcrouter.h>
#include <mach/msm_battery.h>
#if defined(CONFIG_BATTERY_BQ275X0) ||defined(CONFIG_BATTERY_BQ275X0_MODULE)
#include <linux/i2c/bq275x0_battery.h>
#endif

#include <linux/wakelock.h>

#define BAT_MSG_FATAL(format,arg...)  do { printk(KERN_CRIT format, ## arg); } while (0)
#define BAT_MSG_ERR(format,arg...)    do { printk(KERN_ERR format, ## arg);  } while (0)

#define BAT_MSG_DBG_OUT  0
#if  BAT_MSG_DBG_OUT
#define BAT_MSG_DBG(format,arg...)    do { printk(KERN_ALERT format, ## arg);  } while (0)
#else
#define BAT_MSG_DBG(format,arg...)    do { (void)(format); } while (0)
#endif

#define BAT_VOLT_GAIN_BY_ADC  1
#define BAT_VOLT_GAIN_BY_CMP  2
#define BAT_VOLT_GAIN_MODE  BAT_VOLT_GAIN_BY_ADC

#undef CONFIG_HAS_EARLYSUSPEND
#undef KERN_INFO
#define KERN_INFO  "<7>"


#define BATTERY_RPC_PROG	0x30000089
#define BATTERY_RPC_VERS	0x00010001

#define BATTERY_RPC_CB_PROG	0x31000089
#define BATTERY_RPC_CB_VERS	0x00010001

#define CHG_RPC_PROG		0x3000001a
#define CHG_RPC_VERS		0x00010001


#define BATTERY_REGISTER_PROC                          	2
#define BATTERY_GET_CLIENT_INFO_PROC                   	3
#define BATTERY_MODIFY_CLIENT_PROC                     	4
#define BATTERY_DEREGISTER_CLIENT_PROC			5
#define BATTERY_SERVICE_TABLES_PROC                    	6
#define BATTERY_IS_SERVICING_TABLES_ENABLED_PROC       	7
#define BATTERY_ENABLE_TABLE_SERVICING_PROC            	8
#define BATTERY_DISABLE_TABLE_SERVICING_PROC           	9
#define BATTERY_READ_PROC                              	10
#define BATTERY_MIMIC_LEGACY_VBATT_READ_PROC           	11
#define BATTERY_READ_MV_PROC 				12
#define BATTERY_ENABLE_DISABLE_FILTER_PROC 		14

#define BATTERY_EMPTY_DISP_CAPACITY			1
#define VBATT_FILTER			2

#define BATTERY_CB_TYPE_PROC 		1
#define BATTERY_CB_ID_ALL_ACTIV       	1
#define BATTERY_CB_ID_LOW_VOL		2

#define BATTERY_LOW            	2800
#define BATTERY_HIGH           	4300

#define BATTERY_LOW_CORECTION   	100

#define ONCRPC_CHG_IS_CHARGING_PROC 		2
#define ONCRPC_CHG_IS_CHARGER_VALID_PROC 	3
#define ONCRPC_CHG_IS_BATTERY_VALID_PROC 	4
#define ONCRPC_CHG_UI_EVENT_READ_PROC 		5
#define ONCRPC_CHG_GET_GENERAL_STATUS_PROC 	12

//#define ONCRPC_CHG_GET_MODEM_TIME_PROC 	13

#define ONCRPC_CHG_SET_GAUGE_STATUS_PROC 13

#define ONCRPC_CHARGER_API_VERSIONS_PROC 	0xffffffff

#define CHARGER_API_VERSION  			0x00010003

#define DEFAULT_CHARGER_API_VERSION		0x00010001


#define BATT_RPC_TIMEOUT    10000	/* 10 sec */

#define INVALID_BATT_HANDLE    -1

#define RPC_TYPE_REQ     0
#define RPC_TYPE_REPLY   1
#define RPC_REQ_REPLY_COMMON_HEADER_SIZE   (3 * sizeof(uint32_t))


#define SUSPEND_EVENT		(1UL << 0)
#define RESUME_EVENT		(1UL << 1)
#define CLEANUP_EVENT		(1UL << 2)


#define POWER_SUPPLY_UNHNOW    0
#define POWER_SUPPLY_BATTERY   1
#define POWER_SUPPLY_AC		   2
#define POWER_SUPPLY_USB	   3

#define AP_FLAG_ON	1
#define AP_FLAG_OFF	0
#define MODEM_FLAG_ON	1
#define MODEM_FLAG_OFF	0
#define VOLTAGE_ADJUST_LEVEL1 3800
#define VOLTAGE_ADJUST_LEVEL2 3600

typedef enum 
{
  CHG_IDLE_ST,                  /* Charger state machine entry point.       */
  CHG_WALL_IDLE_ST,             /* Wall charger state machine entry point.  */
  CHG_WALL_TRICKLE_ST,          /* Wall charger low batt charging state.    */
  CHG_WALL_NO_TX_FAST_ST,       /* Wall charger high I charging state.      */
  CHG_WALL_FAST_ST,             /* Wall charger high I charging state.      */
  CHG_WALL_TOPOFF_ST,           /* Wall charger top off charging state.     */
  CHG_WALL_MAINT_ST,            /* Wall charger maintance charging state.   */
  CHG_WALL_TX_WAIT_ST,          /* Wall charger TX WAIT charging state.     */
  CHG_WALL_ERR_WK_BAT_WK_CHG_ST,/* Wall CHG ERR: weak batt and weak charger.*/
  CHG_WALL_ERR_WK_BAT_BD_CHG_ST,/* Wall CHG ERR: weak batt and bad charger. */
  CHG_WALL_ERR_GD_BAT_BD_CHG_ST,/* Wall CHG ERR: good batt and bad charger. */
  CHG_WALL_ERR_GD_BAT_WK_CHG_ST,/* Wall CHG ERR: good batt and weak charger.*/
  CHG_WALL_ERR_BD_BAT_GD_CHG_ST,/* Wall CHG ERR: Bad batt and good charger. */
  CHG_WALL_ERR_BD_BAT_WK_CHG_ST,/* Wall CHG ERR: Bad batt and weak charger. */
  CHG_WALL_ERR_BD_BAT_BD_CHG_ST,/* Wall CHG ERR: Bad batt and bad charger.  */
  CHG_WALL_ERR_GD_BAT_BD_BTEMP_CHG_ST,/* Wall CHG ERR: GD batt and BD batt temp */
  CHG_WALL_ERR_WK_BAT_BD_BTEMP_CHG_ST,/* Wall CHG ERR: WK batt and BD batt temp */
  CHG_WALL_BATT_PLUGIN_ST,/* Wall CHG ERR: BATT plug in */
  CHG_WALL_CHARGER_PLUGIN_ST,/* Wall CHG ERR: BATT plug in */
  CHG_USB_IDLE_ST,              /* USB charger state machine entry point.   */
  CHG_USB_TRICKLE_ST,           /* USB charger low batt charging state.     */
  CHG_USB_NO_TX_FAST_ST,        /* USB charger high I charging state.       */ 
  CHG_USB_FAST_ST,              /* USB charger high I charging state.       */ 
  CHG_USB_TOPOFF_ST,            /* USB charger top off state charging state.*/ 
  CHG_USB_DONE_ST,              /* USB charger Done charging state.         */ 
  CHG_USB_ERR_WK_BAT_WK_CHG_ST, /* USB CHG ERR: weak batt and weak charger. */
  CHG_USB_ERR_WK_BAT_BD_CHG_ST, /* USB CHG ERR: weak batt and bad charger.  */
  CHG_USB_ERR_GD_BAT_BD_CHG_ST, /* USB CHG ERR: good batt and bad charger.  */
  CHG_USB_ERR_GD_BAT_WK_CHG_ST, /* USB CHG ERR: good batt and weak charger. */
  CHG_USB_ERR_BD_BAT_GD_CHG_ST, /* USB CHG ERR: Bad batt and good charger.  */
  CHG_USB_ERR_BD_BAT_WK_CHG_ST, /* USB CHG ERR: Bad batt and weak charger.  */
  CHG_USB_ERR_BD_BAT_BD_CHG_ST, /* USB CHG ERR: Bad batt and bad charger.   */
  CHG_USB_ERR_GD_BAT_BD_BTEMP_CHG_ST,/* USB CHG ERR: GD batt and BD batt temp */
  CHG_USB_ERR_WK_BAT_BD_BTEMP_CHG_ST,/* USB CHG ERR: WK batt and BD batt temp */
  CHG_VBATDET_CAL_ST,           /* VBATDET calibration state*/
  CHG_INVALID_ST
} chg_state_type;

enum {
	BATTERY_REGISTRATION_SUCCESSFUL = 0,
	BATTERY_DEREGISTRATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_MODIFICATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_INTERROGATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_CLIENT_TABLE_FULL = 1,
	BATTERY_REG_PARAMS_WRONG = 2,
	BATTERY_DEREGISTRATION_FAILED = 4,
	BATTERY_MODIFICATION_FAILED = 8,
	BATTERY_INTERROGATION_FAILED = 16,
	/* Client's filter could not be set because perhaps it does not exist */
	BATTERY_SET_FILTER_FAILED         = 32,
	/* Client's could not be found for enabling or disabling the individual
	 * client */
	BATTERY_ENABLE_DISABLE_INDIVIDUAL_CLIENT_FAILED  = 64,
	BATTERY_LAST_ERROR = 128,
};

/*
 * This enum contains defintions of the charger hardware status
 */
enum chg_charger_status_type {
    /* The charger is good      */
    CHARGER_STATUS_GOOD,
    /* The charger is bad       */
    CHARGER_STATUS_BAD,
    /* The charger is weak      */
    CHARGER_STATUS_WEAK,
    /* Invalid charger status.  */
    CHARGER_STATUS_INVALID
};
/*
static char *charger_status[] = {
	"good", "bad", "weak", "invalid"
};
*/
enum {
	BATTERY_VOLTAGE_UP = 0,
	BATTERY_VOLTAGE_DOWN,
	BATTERY_VOLTAGE_ABOVE_THIS_LEVEL,
	BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
	BATTERY_VOLTAGE_LEVEL,
	BATTERY_ALL_ACTIVITY,
	VBATT_CHG_EVENTS,
	BATTERY_VOLTAGE_UNKNOWN,
};


typedef enum
{
    CHG_UI_EVENT_IDLE,           /* Starting point, no charger.        */
    CHG_UI_EVENT_NO_POWER,       /* No/Weak Battery + Weak Charger.    */
    CHG_UI_EVENT_VERY_LOW_POWER, /* No/Weak Battery + Strong Charger.  */
    CHG_UI_EVENT_LOW_POWER,      /* Low Battery     + Strong Charger.  */
    CHG_UI_EVENT_NORMAL_POWER,   /* Enough Power for most applications.*/
    CHG_UI_EVENT_DONE,           /* Done charging, batt full.          */
    CHG_UI_EVENT_INVALID,
#ifdef FEATURE_L4LINUX
    CHG_UI_EVENT_MAX32 = 0x7fffffff  /* Pad enum to 32-bit int */
#endif
}chg_ui_event_type;

/*
 *This enum contains defintions of the charger hardware type
 */
enum chg_charger_hardware_type {
    /* The charger is removed                 */
    CHARGER_TYPE_NONE,
    /* The charger is a regular wall charger   */
    CHARGER_TYPE_WALL,
    /* The charger is a PC USB                 */
    CHARGER_TYPE_USB_PC,
    /* The charger is a wall USB charger       */
    CHARGER_TYPE_USB_WALL,
    /* The charger is a USB carkit             */
    CHARGER_TYPE_USB_CARKIT,
    /* Invalid charger hardware status.        */
    CHARGER_TYPE_INVALID
};
#if 0
static char *charger_type[] = {
	"No charger", "wall", "USB PC", "USB wall", "USB car kit",
	"invalid charger"
};
#endif

/*
 *  This enum contains defintions of the battery status
 */
enum chg_battery_status_type {
    BATTERY_STATUS_UNKNOWN = 0,
	BATTERY_STATUS_CHARGING,
	BATTERY_STATUS_DISCHARGING,
	BATTERY_STATUS_NOT_CHARGING,
	BATTERY_STATUS_FULL,
};

enum chg_battery_health_type {
    BATTERY_HEALTH_UNKNOWN = 0,
    BATTERY_HEALTH_GOOD,    /*! The battery is good        */
    BATTERY_HEALTH_BAD_TEMP,   /*! The battery is cold/hot    */  
    BATTERY_HEALTH_BAD,   /*! The battery is bad         */
    BATTERY_HEALTH_OVERVOLTAGE,
    BATTERY_HEALTH_INVALID,  /*! Invalid battery status.    */
	BATTERY_HEALTH_COLD
};
#if 0
static char *battery_status[] = {
	"unknown", "bat charging ", "bat discharging", "bat not charging", "bat full"
};

static char *battery_health[] = {
	"unknown","good ", "bad temperature", "bad", "over voltage","invalid","cold"
};
#endif

/*
 *This enum contains defintions of the battery voltage level
 */
enum chg_battery_level_type {
    /* The battery voltage is dead/very low (less than 3.2V)        */
    BATTERY_LEVEL_DEAD,
    /* The battery voltage is weak/low (between 3.2V and 3.4V)      */
    BATTERY_LEVEL_WEAK,
    /* The battery voltage is good/normal(between 3.4V and 4.2V)  */
    BATTERY_LEVEL_GOOD,
    /* The battery voltage is up to full (close to 4.2V)            */
    BATTERY_LEVEL_FULL,
    BATTERY_LEVEL_OVERVOLTAGE,
    /* Invalid battery voltage level.                               */
    BATTERY_LEVEL_INVALID
};
#if 0
static char *battery_level[] = {
	"dead", "weak", "good", "full", "over valtage", "invalid"
};
#endif

/* Generic type definition used to enable/disable charger functions */
enum {
	CHG_CMD_DISABLE,
	CHG_CMD_ENABLE,
	CHG_CMD_INVALID,
	CHG_CMD_MAX32 = 0x7fffffff
};

static u32 msm_batt_capacity (u32 voltage);
static void msm_batt_change_power_supply(unsigned int supply_type);
static unsigned int msm_batt_get_power_supply_type(unsigned int pre_charger_type, 
													unsigned int pre_charger_status, 
													unsigned int cur_charger_type, 
													unsigned int cur_charger_status);

struct batt_client_registration_req {

	struct rpc_request_hdr hdr;

	/* The voltage at which callback (CB) should be called. */
	u32 desired_batt_voltage;

	/* The direction when the CB should be called. */
	u32 voltage_direction;

	/* The registered callback to be called when voltage and
	 * direction specs are met. */
	u32 batt_cb_id;

	/* The call back data */
	u32 cb_data;
	u32 more_data;
	u32 batt_error;
};

struct batt_client_registration_rep {
	struct rpc_reply_hdr hdr;
	u32 batt_clnt_handle;
};

struct rpc_reply_batt_chg {
	struct rpc_reply_hdr hdr;
	u32 	more_data;

	u32	charger_status;
	u32	charger_type;
	u32	battery_status;
	u32	battery_level;
	u32 battery_voltage;
	u32	battery_temp;

	u32	battery_capacity;
	u32	battery_health;
	u32	battery_valid;

	u32	charging_machine_state;
	u32	charging_current;
	
	s32 is_poweroff_charging ;

	
};

static struct rpc_reply_batt_chg rep_batt_chg;

struct msm_battery_info {

	u32 voltage_max_design;
	u32 voltage_min_design;
	u32 chg_api_version;
	u32 batt_technology;

	u32 avail_chg_sources;
	u32 current_chg_source;

	u32 batt_status;
	u32 batt_status_prev ; 
	u32 batt_health;
	u32 charger_valid;
	u32 batt_valid;
	u32 batt_capacity; /* in percentage */

	s32 charger_status;
	s32 charger_type;
	s32 battery_status;
	s32 battery_level;
	s32 battery_voltage; /* in millie volts */
	s32 battery_temp;  /* in celsius */
	s32 pwrdwn_chg_flag ;
	s32 voltage_delta ;
	u32 batt_capacity_last; /* in percentage */
	u32 batt_voltage_last; 

	u32	charging_machine_state;
	u32	charging_current;
	
	u32(*calculate_capacity) (u32 voltage);

	s32 batt_handle;

	spinlock_t lock;

	struct power_supply *msm_psy_ac;
	struct power_supply *msm_psy_usb;
	struct power_supply *msm_psy_batt;

	struct msm_rpc_endpoint *batt_ep;
	struct msm_rpc_endpoint *chg_ep;

	struct workqueue_struct *msm_batt_wq;

	struct task_struct *cb_thread;

	wait_queue_head_t wait_q;

	struct early_suspend early_suspend;

    /* Begin: added by w00163571 2010-6-18 for while charging ap not sleep */
	struct wake_lock wlock;
    /* End: added by w00163571 2010-6-18 for while charging ap not sleep  */
	
	atomic_t handle_event;
	atomic_t event_handled;

	u32 type_of_event;
	uint32_t vbatt_modify_rpc_req_xid;
	uint32_t vbatt_volt_rpc_req_xid;
};

static void msm_batt_wait_for_batt_chg_event(struct work_struct *work);

static DECLARE_WORK(msm_batt_cb_work, msm_batt_wait_for_batt_chg_event);

static int msm_batt_cleanup(void);

#define BATT_INITIAL_CAP 200

static struct msm_battery_info msm_batt_info = {
	.batt_handle = INVALID_BATT_HANDLE,
	.charger_status = CHARGER_STATUS_INVALID,
	.charger_type = CHARGER_TYPE_INVALID,
	.battery_status = BATTERY_STATUS_UNKNOWN,
	.battery_level = BATTERY_LEVEL_INVALID,
	.battery_voltage = BATTERY_HIGH,
	.batt_capacity = 0,//BATT_INITIAL_CAP, /*mod by zrf*/
	.batt_status = POWER_SUPPLY_STATUS_DISCHARGING,
	.batt_status_prev = POWER_SUPPLY_STATUS_UNKNOWN, /*add by zrf*/
	.batt_health = POWER_SUPPLY_HEALTH_GOOD,
	.batt_valid  = 1,
	.battery_temp = 23,
	.pwrdwn_chg_flag = 0,
	.voltage_delta = 0 ,//add by zrf for battery compensation.
};

typedef struct
{
#if 0
    	u32    battery_voltage;
    	s32     battery_temp;
#endif        
	u32    charging_curr;
}chg_gauge_status_type;

static chg_gauge_status_type chg_gauge_status;


static enum power_supply_property msm_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *msm_power_supplied_to[] = {
	"battery",
};

#define   BSP_BATTERY_CAPACITY            79200       /*2200mA*3600s/100*/
#define   BSP_BATTERY_CURRENT             (1200+200)  /*1200mA最大充电电流+200mA补偿*/
#define   BSP_BATTER_CAPACITY_MAX         10          /*定义两种算法的最大差值*/
#define   BSP_BATTERY_STATUS_SWITH_MIN    (600*HZ)   /*定义切换最小时间*/
#define   BSP_BATTERY_CURRENT_DALT		  (200)
/*功能描述：LED状态枚举*/
typedef enum tagBSP_CHARGE_E_TYPE
{
    BSP_CHARGE_TYPE_E_BATTERY = 0,
    BSP_CHARGE_TYPE_E_AC,
    BSP_CHARGE_TYPE_E_USB,
}BSP_CHARGE_E_TYPE;

typedef enum tagBSP_CHARGE_E_STATUS
{
    BSP_CHARGE_STATUS_E_BATTERY = 0,
    BSP_CHARGE_STATUS_E_AC,
    BSP_CHARGE_STATUS_E_USB,
    BSP_CHARGE_STATUS_BATTERY_TO_AC,
    BSP_CHARGE_STATUS_AC_TO_BATTERY,
    BSP_CHARGE_STATUS_USB_TO_BATTERY,
    BSP_CHARGE_STATUS_BATTERY_TO_USB,
    BSP_CHARGE_STATUS_USB_TO_AC,
    BSP_CHARGE_STATUS_AC_TO_USB,
}BSP_CHARGE_E_STATUS;

typedef struct tagBacklight_Current_Rectify
{
    unsigned int ulLevel;
    unsigned int ulCurrent;    
}Backlight_Current_Rectify;


static unsigned int gBspChargeStatus;
static unsigned int gsupply_type = BSP_CHARGE_TYPE_E_BATTERY; 
static unsigned int gpre_supply_type = 0xffffffff;
static unsigned int gBspTimers1 = 0;
static unsigned int g_preBatterValid = 1;
static unsigned int g_BatterValid = 1;



static bool g_sleep_cal_capacity_flag = false;
void msm_battery_set_delta(int current_mv)
{
	msm_batt_info.voltage_delta = current_mv / 10 ;   //0.1 Ohm.
}
EXPORT_SYMBOL(msm_battery_set_delta);


static void msm_batt_suspend_locks_init(struct msm_battery_info* pbattery_info, int init)
{
	if (init) {
		wake_lock_init(&pbattery_info->wlock, WAKE_LOCK_SUSPEND,
				"batt_charging_active");
	
	} else {
		wake_lock_destroy(&pbattery_info->wlock);
	
	}
}



static int msm_power_get_property(struct power_supply *psy,
				  enum power_supply_property psp,
				  union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:

		if (psy->type == POWER_SUPPLY_TYPE_MAINS) {

			val->intval = msm_batt_info.current_chg_source & AC_CHG
			    ? 1 : 0;
			BAT_MSG_DBG("%s(): power supply = %s online = %d\n"
					, __func__, psy->name, val->intval);

		}

		if (psy->type == POWER_SUPPLY_TYPE_USB) {

			val->intval = msm_batt_info.current_chg_source & USB_CHG
			    ? 1 : 0;

			BAT_MSG_DBG("%s(): power supply = %s online = %d\n"
					, __func__, psy->name, val->intval);
		}

		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct power_supply msm_psy_ac = {
	.name = "ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.supplied_to = msm_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
	.properties = msm_power_props,
	.num_properties = ARRAY_SIZE(msm_power_props),
	.get_property = msm_power_get_property,
};

static struct power_supply msm_psy_usb = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.supplied_to = msm_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
	.properties = msm_power_props,
	.num_properties = ARRAY_SIZE(msm_power_props),
	.get_property = msm_power_get_property,
};

static enum power_supply_property msm_batt_power_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,

	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_PWRDWN_CHG ,
	POWER_SUPPLY_PROP_VOLTAGE_DELTA ,
	POWER_SUPPLY_PROP_CURRENT_NOW
};

static enum power_supply_property gauge_batt_power_props[] = {
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CAPACITY
};


extern u32 bsp_lcdc_panel_get_backlight(void);


static void msm_batt_update_psy_status_v0(void);
static void msm_batt_update_psy_status_v1(void);

static void msm_batt_external_power_changed(struct power_supply *psy)
{
	BAT_MSG_DBG("%s() : external power supply changed for %s\n",
			__func__, psy->name);
	power_supply_changed(psy);
}

static int msm_batt_power_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	switch (psp) {

	case POWER_SUPPLY_PROP_STATUS:
		val->intval = msm_batt_info.batt_status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = msm_batt_info.batt_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = msm_batt_info.batt_valid;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = msm_batt_info.batt_technology;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = msm_batt_info.voltage_max_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = msm_batt_info.voltage_min_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = msm_batt_info.battery_voltage;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		//if( msm_batt_info.batt_capacity == BATT_INITIAL_CAP )
		if( msm_batt_info.batt_capacity > 100 )
		{
			val->intval = 100 ;
		}
		else
		{
			val->intval = msm_batt_info.batt_capacity;
		}
		break;

	case POWER_SUPPLY_PROP_TEMP:
		val->intval = msm_batt_info.battery_temp;
		break;
	case POWER_SUPPLY_PROP_PWRDWN_CHG :
		val->intval = msm_batt_info.pwrdwn_chg_flag ;
		break ;
	case POWER_SUPPLY_PROP_VOLTAGE_DELTA:
		val->intval = msm_batt_info.voltage_delta ;
		break ;		
    case POWER_SUPPLY_PROP_CURRENT_NOW:
        val->intval = msm_batt_info.charging_current;
        break;
	default:
		return -EINVAL;
	}

	BAT_MSG_DBG( "%s(): psp = %d, val->intval = %d \n",
			__func__, psp, val->intval);
	return 0;
}

static struct power_supply msm_psy_batt = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = msm_batt_power_props,
	.num_properties = ARRAY_SIZE(msm_batt_power_props),
	.get_property = msm_batt_power_get_property,
	.external_power_changed = msm_batt_external_power_changed,
};

static int msm_batt_get_batt_chg_status_v1(void)
{
	int rc ;
	struct rpc_req_batt_chg {
		struct rpc_request_hdr hdr;
		u32 more_data;
	} req_batt_chg;

	req_batt_chg.more_data = cpu_to_be32(1);

	memset(&rep_batt_chg, 0, sizeof(rep_batt_chg));

	BAT_MSG_DBG("---------------------------------------------\n enter %s() func!\n",__func__);
	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_GET_GENERAL_STATUS_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_batt_chg, sizeof(rep_batt_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		BAT_MSG_ERR("%s(): msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_GET_GENERAL_STATUS_PROC, rc);
		return rc;
	} 
	else if (be32_to_cpu(rep_batt_chg.more_data)) 
	{
		rep_batt_chg.charger_status = be32_to_cpu(rep_batt_chg.charger_status);
		rep_batt_chg.charger_type = be32_to_cpu(rep_batt_chg.charger_type);
		rep_batt_chg.battery_status = be32_to_cpu(rep_batt_chg.battery_status);
		rep_batt_chg.battery_level = be32_to_cpu(rep_batt_chg.battery_level);
		rep_batt_chg.battery_voltage = be32_to_cpu(rep_batt_chg.battery_voltage);
		rep_batt_chg.battery_temp = be32_to_cpu(rep_batt_chg.battery_temp);	
		rep_batt_chg.battery_capacity = be32_to_cpu(rep_batt_chg.battery_capacity);
		rep_batt_chg.battery_health = be32_to_cpu(rep_batt_chg.battery_health);
		rep_batt_chg.battery_valid = be32_to_cpu(rep_batt_chg.battery_valid);

		rep_batt_chg.charging_machine_state = be32_to_cpu(rep_batt_chg.charging_machine_state);
		rep_batt_chg.charging_current = be32_to_cpu(rep_batt_chg.charging_current);
		rep_batt_chg.is_poweroff_charging = be32_to_cpu(rep_batt_chg.is_poweroff_charging);
		
		BAT_MSG_DBG(
		       "%s: rep_batt_chg.is_poweroff_charging=%d\n",
		       __func__, rep_batt_chg.is_poweroff_charging);

		
	} 
	else
	{
		BAT_MSG_ERR("%s():No more data in batt_chg rpc reply\n",__func__);
		return -EIO;
	}

	BAT_MSG_DBG("batt_health = %s, charger_type = %s, "
				"batt_valid = %d, batt_status = %s, batt_level = %s\n"
				"batt_capacity = %d%%, batt_volt = %u, batt_temp = %d, \n"
				"charger_status = %s, is_powerdown_charging = %d\n"
				"charger_machine_state = %d, charging_current = %d\n",
				battery_health[rep_batt_chg.battery_health],
				charger_type[rep_batt_chg.charger_type],
				rep_batt_chg.battery_valid,
				battery_status[rep_batt_chg.battery_status],
				battery_level[rep_batt_chg.battery_level],
				rep_batt_chg.battery_capacity,
				rep_batt_chg.battery_voltage,
				rep_batt_chg.battery_temp,
				charger_status[rep_batt_chg.charger_status],
				rep_batt_chg.is_poweroff_charging,
				rep_batt_chg.charging_machine_state,
				rep_batt_chg.charging_current);
	return 0;
}

static int msm_batt_get_batt_chg_status_v0(u32 *batt_charging,
					u32 *charger_valid,
					u32 *chg_batt_event)
{
	struct rpc_request_hdr req_batt_chg;

	struct rpc_reply_batt_volt {
		struct rpc_reply_hdr hdr;
		u32 voltage;
	} rep_volt;

	struct rpc_reply_chg_reply {
		struct rpc_reply_hdr hdr;
		u32 chg_batt_data;
	} rep_chg;

	int rc;
	*batt_charging = 0;
	*chg_batt_event = CHG_UI_EVENT_INVALID;
	*charger_valid = 0;

	rc = msm_rpc_call_reply(msm_batt_info.batt_ep,
				BATTERY_READ_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_volt, sizeof(rep_volt),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		BAT_MSG_ERR(
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, BATTERY_READ_PROC, rc);

		return rc;
	}
	msm_batt_info.battery_voltage = be32_to_cpu(rep_volt.voltage);

	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_IS_CHARGING_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_chg, sizeof(rep_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		BAT_MSG_ERR(
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_IS_CHARGING_PROC, rc);
		return rc;
	}
	*batt_charging = be32_to_cpu(rep_chg.chg_batt_data);

	msm_batt_info.pwrdwn_chg_flag = (2 == *batt_charging ) ? 1 : 0 ;

	BAT_MSG_DBG( "%s msm_batt_info.pwrdwn_chg_flag = %d\n\r" , __func__ , msm_batt_info.pwrdwn_chg_flag ) ;

	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_IS_CHARGER_VALID_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_chg, sizeof(rep_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		BAT_MSG_ERR(
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_IS_CHARGER_VALID_PROC, rc);
		return rc;
	}
	*charger_valid = be32_to_cpu(rep_chg.chg_batt_data);

	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_IS_BATTERY_VALID_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_chg, sizeof(rep_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		BAT_MSG_ERR(
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_IS_BATTERY_VALID_PROC, rc);
		return rc;
	}
	msm_batt_info.batt_valid = be32_to_cpu(rep_chg.chg_batt_data);

	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_UI_EVENT_READ_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_chg, sizeof(rep_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		BAT_MSG_ERR(
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_UI_EVENT_READ_PROC, rc);
		return rc;
	}
	*chg_batt_event = be32_to_cpu(rep_chg.chg_batt_data);

	return 0;
}

static void msm_batt_update_psy_status_v0(void)
{
	u32 batt_charging = 0;
	u32 chg_batt_event = CHG_UI_EVENT_INVALID;
	u32 charger_valid = 0;

	msm_batt_get_batt_chg_status_v0(&batt_charging, &charger_valid,
				     &chg_batt_event);

	BAT_MSG_DBG( "%s: batt_charging = %u  batt_valid = %u "
			" batt_volt = %u\n charger_valid = %u "
			" chg_batt_event = %u\n",__func__,
			batt_charging, msm_batt_info.batt_valid,
			msm_batt_info.battery_voltage,
			charger_valid, chg_batt_event);

	BAT_MSG_DBG( "%s: msm_batt_update_psy_status_v0: Previous charger valid status = %u"
			"  current charger valid status = %u\n", __func__,
			msm_batt_info.charger_valid, charger_valid);

	if (msm_batt_info.charger_valid != charger_valid) {

		BAT_MSG_DBG( "%s: msm_batt_info.charger_valid = %d, charger_valid = %d \n",__func__,msm_batt_info.charger_valid,charger_valid);
		msm_batt_info.charger_valid = charger_valid;
		if (msm_batt_info.charger_valid)
			msm_batt_info.current_chg_source |= USB_CHG;
		else
			msm_batt_info.current_chg_source &= ~USB_CHG;
		power_supply_changed(&msm_psy_usb);
	}

	if (msm_batt_info.batt_valid) {

		if (msm_batt_info.battery_voltage >
		    msm_batt_info.voltage_max_design)
			msm_batt_info.batt_health =
			    POWER_SUPPLY_HEALTH_OVERVOLTAGE;

		else if (msm_batt_info.battery_voltage
			 < msm_batt_info.voltage_min_design)
			msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_DEAD;
		else
			msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;

		if (batt_charging && msm_batt_info.charger_valid)
			msm_batt_info.batt_status =
			    POWER_SUPPLY_STATUS_CHARGING;
		else if (!batt_charging)
			msm_batt_info.batt_status =
			    POWER_SUPPLY_STATUS_DISCHARGING;

		if (chg_batt_event == CHG_UI_EVENT_DONE)
			msm_batt_info.batt_status = POWER_SUPPLY_STATUS_FULL;

		msm_batt_info.batt_capacity =
		    msm_batt_info.calculate_capacity(
				    msm_batt_info.battery_voltage);

	} else {
		msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_UNKNOWN;
		msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
		msm_batt_info.batt_capacity = 0;
		msm_batt_info.batt_capacity_last = 0;
		msm_batt_info.pwrdwn_chg_flag = 0 ;
	}

	power_supply_changed(&msm_psy_batt);
}

static int msm_batt_register(u32 desired_batt_voltage,
			     u32 voltage_direction, u32 batt_cb_id, u32 cb_data)
{
	struct batt_client_registration_req req;
	struct batt_client_registration_rep rep;
	int rc;

	req.desired_batt_voltage = cpu_to_be32(desired_batt_voltage);
	req.voltage_direction = cpu_to_be32(voltage_direction);
	req.batt_cb_id = cpu_to_be32(batt_cb_id);
	req.cb_data = cpu_to_be32(cb_data);
	req.more_data = cpu_to_be32(1);
	req.batt_error = cpu_to_be32(0);

	rc = msm_rpc_call_reply(msm_batt_info.batt_ep,
				BATTERY_REGISTER_PROC, &req,
				sizeof(req), &rep, sizeof(rep),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		BAT_MSG_ERR(
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, BATTERY_REGISTER_PROC, rc);
		return rc;
	} else {
		rc = be32_to_cpu(rep.batt_clnt_handle);
		BAT_MSG_DBG( "batt_clnt_handle = %d\n", rc);
		return rc;
	}
}

static int msm_batt_modify_client(u32 client_handle, u32 desired_batt_voltage,
	     u32 voltage_direction, u32 batt_cb_id, u32 cb_data)
{
	int rc;

	struct batt_modify_client_req {
		struct rpc_request_hdr hdr;

		u32 client_handle;

		/* The voltage at which callback (CB) should be called. */
		u32 desired_batt_voltage;

		/* The direction when the CB should be called. */
		u32 voltage_direction;

		/* The registered callback to be called when voltage and
		 * direction specs are met. */
		u32 batt_cb_id;

		/* The call back data */
		u32 cb_data;
	} req;

	req.client_handle = cpu_to_be32(client_handle);
	req.desired_batt_voltage = cpu_to_be32(desired_batt_voltage);
	req.voltage_direction = cpu_to_be32(voltage_direction);
	req.batt_cb_id = cpu_to_be32(batt_cb_id);
	req.cb_data = cpu_to_be32(cb_data);

	msm_rpc_setup_req(&req.hdr, BATTERY_RPC_PROG, BATTERY_RPC_VERS,
			 BATTERY_MODIFY_CLIENT_PROC);

	msm_batt_info.vbatt_modify_rpc_req_xid = req.hdr.xid;

	rc = msm_rpc_write(msm_batt_info.batt_ep, &req, sizeof(req));

	if (rc < 0) {
		BAT_MSG_ERR(
		       "%s(): msm_rpc_write failed.  proc = 0x%08x rc = %d\n",
		       __func__, BATTERY_MODIFY_CLIENT_PROC, rc);
		return rc;
	}

	return 0;
}

static int msm_batt_deregister(u32 handle)
{
	int rc;
	struct batt_client_deregister_req {
		struct rpc_request_hdr req;
		s32 handle;
	} batt_deregister_rpc_req;

	struct batt_client_deregister_reply {
		struct rpc_reply_hdr hdr;
		u32 batt_error;
	} batt_deregister_rpc_reply;

	batt_deregister_rpc_req.handle = cpu_to_be32(handle);
	batt_deregister_rpc_reply.batt_error = cpu_to_be32(BATTERY_LAST_ERROR);

	rc = msm_rpc_call_reply(msm_batt_info.batt_ep,
				BATTERY_DEREGISTER_CLIENT_PROC,
				&batt_deregister_rpc_req,
				sizeof(batt_deregister_rpc_req),
				&batt_deregister_rpc_reply,
				sizeof(batt_deregister_rpc_reply),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		BAT_MSG_ERR(
		       "%s: msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, BATTERY_DEREGISTER_CLIENT_PROC, rc);
		return rc;
	}

	if (be32_to_cpu(batt_deregister_rpc_reply.batt_error) !=
			BATTERY_DEREGISTRATION_SUCCESSFUL) {

		BAT_MSG_DBG( "%s: vBatt deregistration Failed "
		       "  proce_num = %d,"
		       " batt_clnt_handle = %d\n",
		       __func__, BATTERY_DEREGISTER_CLIENT_PROC, handle);
		return -EIO;
	}
	return 0;
}

static int  msm_batt_handle_suspend(void)
{
	int rc;

	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {

		rc = msm_batt_modify_client(msm_batt_info.batt_handle,
				BATTERY_LOW, BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
				BATTERY_CB_ID_LOW_VOL, BATTERY_LOW);

		if (rc < 0) {
			BAT_MSG_ERR(
			       "%s(): failed to modify client for registering"
			       " call back when  voltage goes below %u\n",
			       __func__, BATTERY_LOW);

			return rc;
		}
	}

	return 0;
}

static int  msm_batt_handle_resume(void)
{
	int rc;

	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {

		rc = msm_batt_modify_client(msm_batt_info.batt_handle,
				BATTERY_LOW, BATTERY_ALL_ACTIVITY,
			       BATTERY_CB_ID_ALL_ACTIV, BATTERY_ALL_ACTIVITY);
		if (rc < 0) {
			BAT_MSG_ERR(
			       "%s(): failed to modify client for registering"
			       " call back for ALL activity \n", __func__);
			return rc;
		}
	}
	return 0;
}


static int  msm_batt_handle_event(void)
{
	int rc;

	if (!atomic_read(&msm_batt_info.handle_event)) {

		BAT_MSG_ERR("%s(): batt call back thread while in "
			"msm_rpc_read got signal. Signal is not from "
			"early suspend or  from late resume or from Clean up "
			"thread.\n", __func__);
		return 0;
	}

	BAT_MSG_DBG( "%s(): batt call back thread while in msm_rpc_read "
			"got signal\n", __func__);

	if (msm_batt_info.type_of_event & SUSPEND_EVENT) {

		BAT_MSG_DBG( "%s(): Handle Suspend event. event = %08x\n",
				__func__, msm_batt_info.type_of_event);

		rc = msm_batt_handle_suspend();

		return rc;

	} else if (msm_batt_info.type_of_event & RESUME_EVENT) {

		BAT_MSG_DBG( "%s(): Handle Resume event. event = %08x\n",
				__func__, msm_batt_info.type_of_event);

		rc = msm_batt_handle_resume();

		return rc;

	} else if (msm_batt_info.type_of_event & CLEANUP_EVENT) {

		BAT_MSG_DBG( "%s(): Cleanup event occured. event = %08x\n",
				__func__, msm_batt_info.type_of_event);

		return 0;

	} else  {

		BAT_MSG_DBG( "%s(): Unknown event occured. event = %08x\n",
				__func__, msm_batt_info.type_of_event);
		return 0;
	}
}


static void msm_batt_handle_vbatt_rpc_reply(struct rpc_reply_hdr *reply)
{

	struct rpc_reply_vbatt_modify_client {
		struct rpc_reply_hdr hdr;
		u32 modify_client_result;
	} *rep_vbatt_modify_client;

	struct rpc_reply_vbatt_volt {
		struct rpc_reply_hdr hdr;
		u32 volt;;
	} *rep_vbatt_volt;

	u32 modify_client_result;

	if (msm_batt_info.type_of_event & SUSPEND_EVENT) {
		BAT_MSG_DBG( "%s(): Suspend event. Got RPC REPLY for vbatt"
			" modify client RPC req. \n", __func__);
	} else if (msm_batt_info.type_of_event & RESUME_EVENT) {
		BAT_MSG_DBG( "%s(): Resume event. Got RPC REPLY for vbatt"
			" modify client RPC req. \n", __func__);
	}

	/* If an earlier call timed out, we could get the (no longer wanted)
	 * reply for it. Ignore replies that  we don't expect.
	 */
	if (reply->xid != msm_batt_info.vbatt_modify_rpc_req_xid &&
		reply->xid != msm_batt_info.vbatt_volt_rpc_req_xid) {

		BAT_MSG_ERR("%s(): returned RPC REPLY XID is not"
				" equall to VBATT RPC REQ XID \n", __func__);

		return;
	}
	if (reply->reply_stat != RPCMSG_REPLYSTAT_ACCEPTED) {

		BAT_MSG_ERR("%s(): reply_stat != "
			" RPCMSG_REPLYSTAT_ACCEPTED \n", __func__);

		return;
	}

	if (reply->data.acc_hdr.accept_stat != RPC_ACCEPTSTAT_SUCCESS) {

		BAT_MSG_ERR("%s(): reply->data.acc_hdr.accept_stat "
				" != RPCMSG_REPLYSTAT_ACCEPTED \n", __func__);

		return;
	}

	if (reply->xid == msm_batt_info.vbatt_modify_rpc_req_xid) {

		rep_vbatt_modify_client =
			(struct rpc_reply_vbatt_modify_client *) reply;

		modify_client_result =
		be32_to_cpu(rep_vbatt_modify_client->modify_client_result);

		if (modify_client_result != BATTERY_MODIFICATION_SUCCESSFUL) {

			BAT_MSG_ERR("%s() :  modify client failed."
				"modify_client_result  = %u\n", __func__,
				modify_client_result);
		} else {
			BAT_MSG_DBG( "%s() : modify client successful.\n",
				__func__);
		}

	} else if (reply->xid == msm_batt_info.vbatt_volt_rpc_req_xid) {

		rep_vbatt_volt = (struct rpc_reply_vbatt_volt *) reply;

		rep_vbatt_volt->volt =
			be32_to_cpu(rep_vbatt_volt->volt);

		if (msm_batt_info.battery_voltage == rep_vbatt_volt->volt) {

			BAT_MSG_DBG( " No charger. Batt Volt = %u."
					" No change in voltage.\n",
					msm_batt_info.battery_voltage);

			return;
		}

		msm_batt_info.battery_voltage = rep_vbatt_volt->volt;

		msm_batt_info.batt_capacity = msm_batt_info.calculate_capacity(
					msm_batt_info.battery_voltage);

		msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
		msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
		msm_batt_info.batt_valid  = 1 ;
		msm_batt_info.battery_temp = 23;

		BAT_MSG_DBG( "%s() : No charger. Batt Volt = %u.\n",
				 __func__, msm_batt_info.battery_voltage);

		power_supply_changed(&msm_psy_batt);

	} else
		BAT_MSG_DBG( "%s(): returned RPC REPLY XID is not"
				" equall to VBATT RPC REQ XID \n", __func__);
}

static void msm_batt_wake_up_waiting_thread(u32 event)
{
	msm_batt_info.type_of_event &= ~event;

	atomic_set(&msm_batt_info.handle_event, 0);
	atomic_set(&msm_batt_info.event_handled, 1);
	wake_up(&msm_batt_info.wait_q);
}
static enum battery_measurement_type g_batt_measurement_type = BATT_MEASURE_UNKNOW;

enum battery_measurement_type power_get_batt_measurement_type(void)
{
	return g_batt_measurement_type;
}

void power_set_batt_measurement_type(enum battery_measurement_type type)
{
	if(type >= BATT_MEASURE_UNKNOW)
		return;
	g_batt_measurement_type = type;
}

static int msm_batt_property_filter(enum power_supply_property* pspArray,
										unsigned int array_len, 
										enum power_supply_property psp)
{
	unsigned int icount = 0;
	while(icount < array_len)
	{
		if(psp == pspArray[icount])
		{			
			return 1;
		}
		icount++;
	}
	return 0;
}

static int msm_batt_get_modem_batt_chg_status(void)
{
	int rc ;
	struct rpc_req_batt_chg {
		struct rpc_request_hdr hdr;
		u32 more_data;
	} req_batt_chg;

	req_batt_chg.more_data = cpu_to_be32(1);

	memset(&rep_batt_chg, 0, sizeof(rep_batt_chg));

	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_GET_GENERAL_STATUS_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_batt_chg, sizeof(rep_batt_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		BAT_MSG_ERR("%s(): msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_GET_GENERAL_STATUS_PROC, rc);
		return rc;
	} 
	else if (be32_to_cpu(rep_batt_chg.more_data)) 
	{
		rep_batt_chg.charger_status = be32_to_cpu(rep_batt_chg.charger_status);
		rep_batt_chg.charger_type = be32_to_cpu(rep_batt_chg.charger_type);
		rep_batt_chg.battery_status = be32_to_cpu(rep_batt_chg.battery_status);
		rep_batt_chg.battery_level = be32_to_cpu(rep_batt_chg.battery_level);
		rep_batt_chg.battery_voltage = be32_to_cpu(rep_batt_chg.battery_voltage);
		rep_batt_chg.battery_temp = be32_to_cpu(rep_batt_chg.battery_temp);
		rep_batt_chg.battery_capacity = be32_to_cpu(rep_batt_chg.battery_capacity);
		rep_batt_chg.battery_health = be32_to_cpu(rep_batt_chg.battery_health);     
		rep_batt_chg.battery_valid = be32_to_cpu(rep_batt_chg.battery_valid);
		rep_batt_chg.charging_machine_state = be32_to_cpu(rep_batt_chg.charging_machine_state);
		rep_batt_chg.charging_current = be32_to_cpu(rep_batt_chg.charging_current);
		rep_batt_chg.is_poweroff_charging = be32_to_cpu(rep_batt_chg.is_poweroff_charging);
		
		BAT_MSG_DBG(
		       "%s: rep_batt_chg.is_poweroff_charging=%d\n",
		       __func__, rep_batt_chg.is_poweroff_charging);
	} 
	else
	{
		BAT_MSG_ERR("%s():No more data in batt_chg rpc reply\n",__func__);
		return -EIO;
	}

	BAT_MSG_DBG("batt_health = %s, charger_type = %s, "
				"batt_valid = %d, batt_status = %s, batt_level = %s\n"
				"batt_capacity = %d%%, batt_volt = %u, batt_temp = %d \n"
				"charger_status = %s, is_powerdown_charging = %d\n"
				"charger_machine_state = %d, charging_current = %d\n",
				battery_health[rep_batt_chg.battery_health],
				charger_type[rep_batt_chg.charger_type],
				rep_batt_chg.battery_valid,
				battery_status[rep_batt_chg.battery_status],
				battery_level[rep_batt_chg.battery_level],
				rep_batt_chg.battery_capacity,
				rep_batt_chg.battery_voltage,
				rep_batt_chg.battery_temp,
				charger_status[rep_batt_chg.charger_status],
				rep_batt_chg.is_poweroff_charging,
				rep_batt_chg.charging_machine_state,
				rep_batt_chg.charging_current);
	return 0;
}


static int msm_batt_set_modem_batt_chg_status(chg_gauge_status_type *pchg_gauge_status)
{
	int rc=0;
	struct rpc_send_gauge_chg {
		struct rpc_request_hdr hdr;
		chg_gauge_status_type gauge_status;
    	}send_gauge_chg;
    
	if(pchg_gauge_status == NULL) {
		BAT_MSG_FATAL("%s(): bad pointer\n", __func__);
		return -1;
    	}
#if 0
	send_gauge_chg.gauge_status.battery_voltage = cpu_to_be32(pchg_gauge_status->battery_voltage);
	send_gauge_chg.gauge_status.battery_temp = cpu_to_be32(pchg_gauge_status->battery_temp);
#endif
    send_gauge_chg.gauge_status.charging_curr = cpu_to_be32(pchg_gauge_status->charging_curr);

	rc = msm_rpc_call(msm_batt_info.chg_ep
	        				,ONCRPC_CHG_SET_GAUGE_STATUS_PROC
	        				,&send_gauge_chg,sizeof(send_gauge_chg)
	        				,msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		BAT_MSG_ERR("%s(): msm_rpc_call failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_SET_GAUGE_STATUS_PROC, rc);
		return rc;
	}

	return 0;
}

#if defined(CONFIG_BATTERY_BQ275X0) ||defined(CONFIG_BATTERY_BQ275X0_MODULE)
extern struct i2c_client* g_battery_measure_by_bq275x0_i2c_client;

static int msm_batt_get_gauge_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct bq275x0_device_info di;
	if(g_battery_measure_by_bq275x0_i2c_client)
	{
		di.client = g_battery_measure_by_bq275x0_i2c_client;
	}
	else
	{
		return -EINVAL;
	}
	
	switch (psp){

	case POWER_SUPPLY_PROP_STATUS:
		val->intval = msm_batt_info.batt_status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = msm_batt_info.batt_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = msm_batt_info.batt_valid;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = msm_batt_info.batt_technology;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = msm_batt_info.voltage_max_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = msm_batt_info.voltage_min_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = bq275x0_battery_voltage(&di);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = bq275x0_battery_capacity(&di);
		if(val->intval < 0)
		{
			val->intval = 0;
		}
        if((msm_batt_info.batt_status == BATTERY_STATUS_FULL) 
					|| (msm_batt_info.battery_level == BATTERY_LEVEL_FULL))
        {
            val->intval = 100;
        }
		break;
	case POWER_SUPPLY_PROP_TEMP:
		//val->intval = bq275x0_battery_temperature(&di);
		val->intval = msm_batt_info.battery_temp;
		break;
	case POWER_SUPPLY_PROP_PWRDWN_CHG :
		val->intval = msm_batt_info.pwrdwn_chg_flag ;
		break ;
	case POWER_SUPPLY_PROP_VOLTAGE_DELTA:
		val->intval = msm_batt_info.voltage_delta;
		break ;		
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = bq275x0_battery_current(&di);
		break;
	default:
		return -EINVAL;
	}
	BAT_MSG_DBG( "%s(): psp = %d, val->intval = %d \n",
			__func__, psp, val->intval);
	return 0;
}
static struct power_supply msm_psy_batt_gauge = {
	.name = "battery_gauge",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = msm_batt_power_props,
	.num_properties = ARRAY_SIZE(msm_batt_power_props),
	.get_property = msm_batt_get_gauge_property,
	.external_power_changed = msm_batt_external_power_changed,
};
#endif

static int msm_batt_get_gauge_ap_property(struct power_supply *psy, enum power_supply_property psp)
{
	signed int ret = 0;
	union power_supply_propval val;

	ret = psy->get_property(psy,psp,&val);
	if(ret)
		return ret;

	switch (psp) 
	{
		case POWER_SUPPLY_PROP_STATUS:
			msm_batt_info.batt_status = val.intval;
			break;
		case POWER_SUPPLY_PROP_HEALTH:
			msm_batt_info.batt_health = val.intval;
			break;
		case POWER_SUPPLY_PROP_PRESENT:
			msm_batt_info.batt_valid = val.intval;
			break;
		case POWER_SUPPLY_PROP_PWRDWN_CHG :
			msm_batt_info.pwrdwn_chg_flag = val.intval;
			break;
		case POWER_SUPPLY_PROP_TEMP: 
			msm_batt_info.battery_temp = val.intval;
			break;
		case POWER_SUPPLY_PROP_TECHNOLOGY: 
			msm_batt_info.batt_technology = val.intval;
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
			msm_batt_info.voltage_max_design = val.intval;
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
			msm_batt_info.voltage_min_design = val.intval;
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:     
			msm_batt_info.battery_voltage = val.intval;
			break;
		case POWER_SUPPLY_PROP_CAPACITY:
			msm_batt_info.batt_capacity = val.intval;
			if(msm_batt_info.batt_capacity > 100)
				msm_batt_info.batt_capacity = 100;
			break;
	    case POWER_SUPPLY_PROP_CURRENT_NOW:
			msm_batt_info.charging_current = val.intval;
			break;			
		case POWER_SUPPLY_PROP_VOLTAGE_DELTA:
			msm_batt_info.voltage_delta = msm_batt_info.battery_voltage - msm_batt_info.batt_voltage_last;
			break;		
		default:
			break;
	}
	#if 0
	chg_gauge_status. battery_voltage = msm_batt_info.battery_voltage;
	chg_gauge_status. battery_temp = msm_batt_info.battery_temp;  /*this require gauge measure temp*/  
       #endif
    	chg_gauge_status.charging_curr = msm_batt_info.charging_current;
       msm_batt_set_modem_batt_chg_status(&chg_gauge_status);//(pchg_gauge_status);
	return 0;
}

int msm_batt_get_modem_property(enum power_supply_property psp)
{	
	int val = 0;
	switch (psp) 
	{
		case POWER_SUPPLY_PROP_STATUS:
			msm_batt_info.batt_status = rep_batt_chg.battery_status;
			val = rep_batt_chg.battery_status;
			break;
		case POWER_SUPPLY_PROP_HEALTH:
			msm_batt_info.batt_health = rep_batt_chg.battery_health;
			val = rep_batt_chg.battery_health;
			break;
		case POWER_SUPPLY_PROP_PRESENT:
			msm_batt_info.batt_valid = rep_batt_chg.battery_valid;
			val = rep_batt_chg.battery_valid;
			break;
		case POWER_SUPPLY_PROP_PWRDWN_CHG : 
			msm_batt_info.pwrdwn_chg_flag = rep_batt_chg.is_poweroff_charging;
			val = rep_batt_chg.is_poweroff_charging;
			break;
		case POWER_SUPPLY_PROP_TEMP:  
			msm_batt_info.battery_temp = rep_batt_chg.battery_temp;
			val = rep_batt_chg.battery_temp;
			break;
		
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:     
			msm_batt_info.battery_voltage = rep_batt_chg.battery_voltage;
			val = rep_batt_chg.battery_voltage;
			break;
		
	    case POWER_SUPPLY_PROP_CURRENT_NOW:
			msm_batt_info.charging_current = rep_batt_chg.charging_current;
			val = rep_batt_chg.charging_current;
			break;
		default:
			break;
	}

	msm_batt_info.charger_status 	= 	rep_batt_chg.charger_status;
	msm_batt_info.charger_type 	= 	rep_batt_chg.charger_type;
	msm_batt_info.battery_level 	=  	rep_batt_chg.battery_level;
	msm_batt_info.batt_status_prev = msm_batt_info.batt_status ;
	msm_batt_info.charging_machine_state  = rep_batt_chg.charging_machine_state;

	return val;

}

static void msm_batt_update_gauge_property(struct power_supply *psy)
{
	int icount = 0;
    	unsigned int array_len = sizeof(gauge_batt_power_props)/sizeof(enum power_supply_property);

	if(BATT_MEASURE_BY_BQ275x0 == power_get_batt_measurement_type())
	{
        	while(icount < array_len) {
			msm_batt_get_gauge_ap_property(psy,gauge_batt_power_props[icount]);
			icount++;
		}
	}
}

void msm_batt_update_modem_property(void)
{
	unsigned int total_property_array_len = 0, gauge_property_ap_array_len = 0, icount = 0;
	enum power_supply_property* gauge_property_ap_array = NULL;

	msm_batt_get_modem_batt_chg_status();
	
	if(BATT_MEASURE_BY_BQ275x0 == power_get_batt_measurement_type())
	{
		gauge_property_ap_array = gauge_batt_power_props;
		gauge_property_ap_array_len = \
			sizeof(gauge_batt_power_props)/sizeof(enum power_supply_property);
	}
       
	total_property_array_len = sizeof(msm_batt_power_props)/sizeof(enum power_supply_property);
	if(gauge_property_ap_array)
	{
		while(icount < total_property_array_len)
		{
			if(0 == msm_batt_property_filter(gauge_property_ap_array, \
											gauge_property_ap_array_len, \
											msm_batt_power_props[icount]))
			{
				msm_batt_get_modem_property(msm_batt_power_props[icount]);
			}
			icount++;
		}
	}
}
static void msm_batt_gauge_charging_status_switch(void)
{
	if(gpre_supply_type != gsupply_type)
    {		
        switch(gsupply_type)
        {
            case BSP_CHARGE_TYPE_E_AC:
                if(BSP_CHARGE_TYPE_E_BATTERY == gpre_supply_type)
                {        
                    gBspChargeStatus = BSP_CHARGE_STATUS_BATTERY_TO_AC;                    
                }
				else if(BSP_CHARGE_TYPE_E_USB == gpre_supply_type)
				{
					gBspChargeStatus = BSP_CHARGE_STATUS_USB_TO_AC;
				}
                break;
            case BSP_CHARGE_TYPE_E_BATTERY:
                if(BSP_CHARGE_TYPE_E_AC == gpre_supply_type)
                {    
                    gBspChargeStatus = BSP_CHARGE_STATUS_AC_TO_BATTERY; 
                }
                else if(BSP_CHARGE_TYPE_E_USB == gpre_supply_type)
                {
                    gBspChargeStatus = BSP_CHARGE_STATUS_USB_TO_BATTERY;
                }
                break;
			case BSP_CHARGE_TYPE_E_USB:
				if(BSP_CHARGE_TYPE_E_AC == gpre_supply_type)
                {    
                    gBspChargeStatus = BSP_CHARGE_STATUS_AC_TO_USB; 
                }
                else if(BSP_CHARGE_TYPE_E_BATTERY == gpre_supply_type)
                {
                    gBspChargeStatus = BSP_CHARGE_STATUS_BATTERY_TO_USB;
                }
                break;
            default:
                break;
        }
		gpre_supply_type = gsupply_type;
    }
    else
    {
		switch(gsupply_type)
        {
            case BSP_CHARGE_TYPE_E_AC:
                gBspChargeStatus = BSP_CHARGE_STATUS_E_AC;
                break;
            case BSP_CHARGE_TYPE_E_BATTERY:                 
                gBspChargeStatus = BSP_CHARGE_STATUS_E_BATTERY; 
                break;
			case BSP_CHARGE_TYPE_E_USB:  
                gBspChargeStatus = BSP_CHARGE_STATUS_E_USB;  
                break;
            default:
                break;
        }
    }

   	switch(gBspChargeStatus)
    {
		case BSP_CHARGE_STATUS_E_AC:           
			wake_lock_timeout(&msm_batt_info.wlock, HZ * 120);
            break;
        case BSP_CHARGE_STATUS_BATTERY_TO_AC:             
			wake_lock_timeout(&msm_batt_info.wlock, HZ * 120);
            break;
        case BSP_CHARGE_STATUS_AC_TO_BATTERY:
            wake_unlock(&msm_batt_info.wlock);
            break;
        case BSP_CHARGE_STATUS_E_BATTERY:
            break;
       	case BSP_CHARGE_STATUS_E_USB:
            wake_lock_timeout(&msm_batt_info.wlock, HZ * 120);
            break;
    	case BSP_CHARGE_STATUS_USB_TO_BATTERY:
            wake_unlock(&msm_batt_info.wlock);
            break;
    	case BSP_CHARGE_STATUS_BATTERY_TO_USB:
            wake_lock_timeout(&msm_batt_info.wlock, HZ * 120);
            break;
    	case BSP_CHARGE_STATUS_USB_TO_AC:
            wake_lock_timeout(&msm_batt_info.wlock, HZ * 120);
            break;
    	case BSP_CHARGE_STATUS_AC_TO_USB:
            wake_lock_timeout(&msm_batt_info.wlock, HZ * 120);
			break;
        default:
            break;
    }
}

static void msm_batt_update_psy_status_gauge(int update_modem_flag, int update_ap_flag)
{
	unsigned int supply_type = POWER_SUPPLY_UNHNOW;   // 0:unknow; 1: bat; 2: ac;  3: usb 

	if(update_modem_flag ){
		msm_batt_update_modem_property();      
       }

	if(update_ap_flag )
	{
		if(BATT_MEASURE_BY_QC_SOFTWARE != power_get_batt_measurement_type())
		{
			msm_batt_update_gauge_property(msm_batt_info.msm_psy_batt);
		}
	}

       supply_type = msm_batt_get_power_supply_type(msm_batt_info.charger_type
                                                            ,msm_batt_info.charger_status
                                                            ,rep_batt_chg.charger_type
                                                            ,rep_batt_chg.charger_status);
   
    msm_batt_gauge_charging_status_switch();
    if(supply_type != POWER_SUPPLY_USB){
	msm_batt_change_power_supply(supply_type);
    }
}

static void msm_batt_gauge_measure(struct work_struct *work)
    {
        void *rpc_packet;
        unsigned int rpc_wait_timeout = msecs_to_jiffies(10*1000);  //10 sec
        struct rpc_request_hdr *req;
        int rpc_packet_type;
        struct rpc_reply_hdr rpc_reply;
        int len;
        unsigned long flags;
        int rc;
    
        spin_lock_irqsave(&msm_batt_info.lock, flags);
        msm_batt_info.cb_thread = current;
        spin_unlock_irqrestore(&msm_batt_info.lock, flags);
    
        allow_signal(SIGCONT);
    
        msm_batt_update_psy_status_gauge(MODEM_FLAG_ON,AP_FLAG_ON);
    
        while (1) 
        {
            rpc_packet = NULL;
            
            // rpc_ep flag is 0, timeout is 10sec ; wait while rpc interrupt event is valid or timeout
            len = msm_rpc_read(msm_batt_info.batt_ep, &rpc_packet, -1, rpc_wait_timeout);  
            
            BAT_MSG_DBG( "%s: msm_rpc_read return %d \n", __func__, len);
    
            if(len == -ETIMEDOUT)  //time out
            {
                BAT_MSG_DBG( "%s: Get packets from modem Vbatt server TIME OUT\n", __func__);
                goto read_batt_info_from_coulomb_ic;
            }
            else if (len == -ERESTARTSYS)
            {
                flush_signals(current);
                rc = msm_batt_handle_event();
                if (msm_batt_info.type_of_event & CLEANUP_EVENT) 
                {
                    msm_batt_wake_up_waiting_thread(CLEANUP_EVENT);
                    break;
                } 
                else if (msm_batt_info.type_of_event & (SUSPEND_EVENT | RESUME_EVENT)) 
                {
                    if (rc < 0) 
                    {
                        /*Could not sent VBATT modify rpc req */
                        msm_batt_wake_up_waiting_thread(SUSPEND_EVENT | RESUME_EVENT);
                    }
                    /* Wait for RPC reply  for vbatt modify
                     * client RPC call. Then wake up suspend and
                     * resume threads.
                     */
                    continue;
                }
            }
                
    
            BAT_MSG_DBG( "%s: Got some packet from modem Vbatt server\n", __func__);
    
            if (len < 0) 
            {
                BAT_MSG_DBG( "%s: msm_rpc_read failed while "
                       "waiting for call back packet. rc = %d\n",__func__, len);
                continue;
            }
    
            if (len < RPC_REQ_REPLY_COMMON_HEADER_SIZE) 
            {
                BAT_MSG_DBG( "%s: The pkt is neither req nor reply."
                       " len of pkt = %d\n", __func__, len);
                kfree(rpc_packet);
                continue;
            }
    
            req = (struct rpc_request_hdr *)rpc_packet;
    
            rpc_packet_type = be32_to_cpu(req->type);
    
            if (rpc_packet_type == RPC_TYPE_REPLY) 
            {
                msm_batt_handle_vbatt_rpc_reply(rpc_packet);
                kfree(rpc_packet);
    
                if (msm_batt_info.type_of_event &
                    (SUSPEND_EVENT | RESUME_EVENT)) 
                {
                    msm_batt_wake_up_waiting_thread(
                            SUSPEND_EVENT | RESUME_EVENT);
                }
                continue;
            }
            if (rpc_packet_type != RPC_TYPE_REQ) 
            {
                BAT_MSG_DBG( "%s: Type_of_packet is neither req or"
                       " reply. Type_of_packet = %d\n",__func__, rpc_packet_type);
                kfree(rpc_packet);
                continue;
            }
    
            req->type = be32_to_cpu(req->type);
            req->xid = be32_to_cpu(req->xid);
            req->rpc_vers = be32_to_cpu(req->rpc_vers);
    
            if (req->rpc_vers != 2) 
            {
                BAT_MSG_ERR("%s: incorrect rpc version = %d\n",__func__, req->rpc_vers);
                kfree(rpc_packet);
                continue;
            }
    
            req->prog = be32_to_cpu(req->prog);
            if (req->prog != BATTERY_RPC_CB_PROG) 
            {
                BAT_MSG_ERR("%s: Invalid Prog number for rpc call"
                       " back req. prog number = %d\n",__func__, req->prog);
                kfree(rpc_packet);
                continue;
            }
    
            req->procedure = be32_to_cpu(req->procedure);
    
            if (req->procedure != BATTERY_CB_TYPE_PROC) 
            {
                BAT_MSG_ERR("%s: Invalid procedure num  rpc call"
                       " back req. req->procedure = %d\n",__func__, req->procedure);
                kfree(rpc_packet);
                continue;
            }
    
            rpc_reply.xid = cpu_to_be32(req->xid);
            rpc_reply.type = cpu_to_be32(RPC_TYPE_REPLY);
            rpc_reply.reply_stat = cpu_to_be32(RPCMSG_REPLYSTAT_ACCEPTED);
            rpc_reply.data.acc_hdr.accept_stat =
                cpu_to_be32(RPC_ACCEPTSTAT_SUCCESS);
            rpc_reply.data.acc_hdr.verf_flavor = 0;
            rpc_reply.data.acc_hdr.verf_length = 0;
    
            len = msm_rpc_write(msm_batt_info.batt_ep,
                        &rpc_reply, sizeof(rpc_reply));
            if (len < 0)
                BAT_MSG_ERR("%s: could not send rpc reply for"
                       " call back from  batt server."
                       " reply  write response %d\n", __func__, len);
    
            kfree(rpc_packet);
    
    
            BAT_MSG_DBG( "%s: Get info from modem measure_by_gauge\n", __func__);
    
            msm_batt_update_modem_property();
    
    read_batt_info_from_coulomb_ic:
            BAT_MSG_DBG( "%s: Get info from ap measure_by_gauge\n", __func__);
    
            msm_batt_update_psy_status_gauge(MODEM_FLAG_OFF,AP_FLAG_ON);
        }
    
        BAT_MSG_DBG( "%s: Batt RPC call back thread stopped.\n", __func__);
    }

static void msm_batt_software_measure(struct work_struct *work)
{
	void *rpc_packet;
	struct rpc_request_hdr *req;
	int rpc_packet_type;
	struct rpc_reply_hdr rpc_reply;
	int len;
	unsigned long flags;
	int rc;

	spin_lock_irqsave(&msm_batt_info.lock, flags);
	msm_batt_info.cb_thread = current;
	spin_unlock_irqrestore(&msm_batt_info.lock, flags);

	BAT_MSG_DBG( "%s: Batt RPC call back thread started. chg_api_version = 0x%08x\n", __func__,msm_batt_info.chg_api_version);

	allow_signal(SIGCONT);

	BAT_MSG_DBG( "%s: First time Update Batt status without waiting for"
			" call back event from modem . chg_api_version = 0x%08x\n", __func__,msm_batt_info.chg_api_version);

	if (msm_batt_info.chg_api_version >= CHARGER_API_VERSION)
		msm_batt_update_psy_status_v1();
	else
		msm_batt_update_psy_status_v0();

	while (1) {

		rpc_packet = NULL;

		len = msm_rpc_read(msm_batt_info.batt_ep, &rpc_packet, -1, -1);

		if (len == -ERESTARTSYS) {

			flush_signals(current);

			rc = msm_batt_handle_event();

			if (msm_batt_info.type_of_event & CLEANUP_EVENT) {

				msm_batt_wake_up_waiting_thread(CLEANUP_EVENT);
				break;

			} else if (msm_batt_info.type_of_event &
					(SUSPEND_EVENT | RESUME_EVENT)) {

				if (rc < 0) {
					/*Could not sent VBATT modify rpc req */
					msm_batt_wake_up_waiting_thread(
						SUSPEND_EVENT | RESUME_EVENT);
				}
				/* Wait for RPC reply  for vbatt modify
				 * client RPC call. Then wake up suspend and
				 * resume threads.
				 */
				continue;
			}
		}

		BAT_MSG_DBG( "%s: Got some packet from modem Vbatt server\n"
				, __func__);

		if (len < 0) {
			BAT_MSG_DBG( "%s: msm_rpc_read failed while "
			       "waiting for call back packet. rc = %d\n",
			       __func__, len);
			continue;
		}

		if (len < RPC_REQ_REPLY_COMMON_HEADER_SIZE) {
			BAT_MSG_DBG( "%s: The pkt is neither req nor reply."
			       " len of pkt = %d\n", __func__, len);
			kfree(rpc_packet);
			continue;
		}

		req = (struct rpc_request_hdr *)rpc_packet;

		rpc_packet_type = be32_to_cpu(req->type);

		if (rpc_packet_type == RPC_TYPE_REPLY) {

			msm_batt_handle_vbatt_rpc_reply(rpc_packet);
			kfree(rpc_packet);

			if (msm_batt_info.type_of_event &
				(SUSPEND_EVENT | RESUME_EVENT)) {

				msm_batt_wake_up_waiting_thread(
						SUSPEND_EVENT | RESUME_EVENT);
			}
			continue;
		}
		if (rpc_packet_type != RPC_TYPE_REQ) {
			BAT_MSG_DBG( "%s: Type_of_packet is neither req or"
			       " reply. Type_of_packet = %d\n",
			       __func__, rpc_packet_type);
			kfree(rpc_packet);
			continue;
		}

		req->type = be32_to_cpu(req->type);
		req->xid = be32_to_cpu(req->xid);
		req->rpc_vers = be32_to_cpu(req->rpc_vers);

		if (req->rpc_vers != 2) {
			BAT_MSG_ERR("%s: incorrect rpc version = %d\n",
			       __func__, req->rpc_vers);
			kfree(rpc_packet);
			continue;
		}

		req->prog = be32_to_cpu(req->prog);
		if (req->prog != BATTERY_RPC_CB_PROG) {
			BAT_MSG_ERR("%s: Invalid Prog number for rpc call"
			       " back req. prog number = %d\n",
			       __func__, req->prog);
			kfree(rpc_packet);
			continue;
		}

		req->procedure = be32_to_cpu(req->procedure);

		if (req->procedure != BATTERY_CB_TYPE_PROC) {
			BAT_MSG_ERR("%s: Invalid procedure num  rpc call"
			       " back req. req->procedure = %d\n",
			       __func__, req->procedure);
			kfree(rpc_packet);
			continue;
		}

		rpc_reply.xid = cpu_to_be32(req->xid);
		rpc_reply.type = cpu_to_be32(RPC_TYPE_REPLY);
		rpc_reply.reply_stat = cpu_to_be32(RPCMSG_REPLYSTAT_ACCEPTED);
		rpc_reply.data.acc_hdr.accept_stat =
		    cpu_to_be32(RPC_ACCEPTSTAT_SUCCESS);
		rpc_reply.data.acc_hdr.verf_flavor = 0;
		rpc_reply.data.acc_hdr.verf_length = 0;

		len = msm_rpc_write(msm_batt_info.batt_ep,
				    &rpc_reply, sizeof(rpc_reply));
		if (len < 0)
			BAT_MSG_ERR("%s: could not send rpc reply for"
			       " call back from  batt server."
			       " reply  write response %d\n", __func__, len);

		kfree(rpc_packet);

		BAT_MSG_DBG( "In %s() func, begin to call update_psy_status, chg_api_version = 0x%08x \n",
			        __func__, msm_batt_info.chg_api_version);
		
		if (msm_batt_info.chg_api_version >= CHARGER_API_VERSION)
			msm_batt_update_psy_status_v1();
		else
			msm_batt_update_psy_status_v0();
	}

	BAT_MSG_DBG( "%s: Batt RPC call back thread stopped.\n", __func__);	
}

static void msm_batt_wait_for_batt_chg_event(struct work_struct *work)
{
	enum battery_measurement_type measure_type= power_get_batt_measurement_type();

	switch(measure_type) {
		case BATT_MEASURE_BY_QC_SOFTWARE:
			msm_batt_software_measure(work);
			break;
		case BATT_MEASURE_BY_BQ275x0:
			msm_batt_gauge_measure(work);
			break;
		default:		/*BATT_MEASURE_UNKNOW*/
			msm_batt_software_measure(work);
			break;
	}
}


static int msm_batt_send_event(u32 type_of_event)
{
	int rc;
	unsigned long flags;

	rc = 0;

	spin_lock_irqsave(&msm_batt_info.lock, flags);


	if (type_of_event & SUSPEND_EVENT) {
		BAT_MSG_DBG( "%s() : Suspend event ocurred."
				"events = %08x\n", __func__, type_of_event);
	}
	else if (type_of_event & RESUME_EVENT) {
		BAT_MSG_DBG( "%s() : Resume event ocurred."
				"events = %08x\n", __func__, type_of_event);
	}
	else if (type_of_event & CLEANUP_EVENT) {
		BAT_MSG_DBG( "%s() : Cleanup event ocurred."
				"events = %08x\n", __func__, type_of_event);
	}else {
		BAT_MSG_DBG( "%s() : Unknown event ocurred."
				"events = %08x\n", __func__, type_of_event);

		spin_unlock_irqrestore(&msm_batt_info.lock, flags);
		return -EIO;
	}

	msm_batt_info.type_of_event |=  type_of_event;

	if (msm_batt_info.cb_thread) {
		atomic_set(&msm_batt_info.handle_event, 1);
		send_sig(SIGCONT, msm_batt_info.cb_thread, 0);
		spin_unlock_irqrestore(&msm_batt_info.lock, flags);

		rc = wait_event_interruptible(msm_batt_info.wait_q,
			atomic_read(&msm_batt_info.event_handled));

		if (rc == -ERESTARTSYS) {

			BAT_MSG_ERR("%s(): Suspend/Resume/cleanup thread "
				"got a signal while waiting for batt call back"
				" thread to finish\n", __func__);

		} else if (rc < 0) {

			BAT_MSG_ERR("%s(): Suspend/Resume/cleanup thread "
				"wait returned error while waiting for batt "
				"call back thread to finish. rc = %d\n",
				__func__, rc);
		} else
			BAT_MSG_DBG( "%s(): Suspend/Resume/cleanup thread "
				"wait returned rc = %d\n", __func__, rc);

		atomic_set(&msm_batt_info.event_handled, 0);
	} else {
		BAT_MSG_DBG( "%s(): Battery call Back thread not Started.\n",
				__func__);

		atomic_set(&msm_batt_info.handle_event, 1);
		spin_unlock_irqrestore(&msm_batt_info.lock, flags);
	}

	return rc;
}

static void msm_batt_start_cb_thread(void)
{
	atomic_set(&msm_batt_info.handle_event, 0);
	atomic_set(&msm_batt_info.event_handled, 0);
	queue_work(msm_batt_info.msm_batt_wq, &msm_batt_cb_work);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void msm_batt_early_suspend(struct early_suspend *h);
#endif

static int msm_batt_cleanup(void)
{
	int rc = 0;
	int rc_local;

	if (msm_batt_info.msm_batt_wq) {
		msm_batt_send_event(CLEANUP_EVENT);
		destroy_workqueue(msm_batt_info.msm_batt_wq);
	}

	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {

		rc = msm_batt_deregister(msm_batt_info.batt_handle);
		if (rc < 0)
			BAT_MSG_ERR("%s: msm_batt_deregister failed rc=%d\n",
			       __func__, rc);
	}
	msm_batt_info.batt_handle = INVALID_BATT_HANDLE;

	if (msm_batt_info.msm_psy_ac)
		power_supply_unregister(msm_batt_info.msm_psy_ac);

	if (msm_batt_info.msm_psy_usb)
		power_supply_unregister(msm_batt_info.msm_psy_usb);
	if (msm_batt_info.msm_psy_batt)
		power_supply_unregister(msm_batt_info.msm_psy_batt);

	if (msm_batt_info.batt_ep) {
		rc_local = msm_rpc_close(msm_batt_info.batt_ep);
		if (rc_local < 0) {
			BAT_MSG_ERR("%s: msm_rpc_close failed for batt_ep rc=%d\n",
			       __func__, rc_local);
			if (!rc)
				rc = rc_local;
		}
	}

	if (msm_batt_info.chg_ep) {
		rc_local = msm_rpc_close(msm_batt_info.chg_ep);
		if (rc_local < 0) {
			BAT_MSG_ERR("%s: msm_rpc_close failed for chg_ep rc=%d\n",
			       __func__, rc_local);
			if (!rc)
				rc = rc_local;
		}
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	if (msm_batt_info.early_suspend.suspend == msm_batt_early_suspend)
		unregister_early_suspend(&msm_batt_info.early_suspend);
#endif
	return rc;
}



#ifdef CONFIG_HAS_EARLYSUSPEND

void msm_batt_early_suspend(struct early_suspend *h)
{
	int rc;
    
	rc = msm_batt_send_event(SUSPEND_EVENT);

	BAT_MSG_DBG( "%s(): Handled early suspend event."
	       " rc = %d\n", __func__, rc);
}

void msm_batt_late_resume(struct early_suspend *h)
{
	int rc;
	
    
	
	rc = msm_batt_send_event(RESUME_EVENT);

	BAT_MSG_DBG( "%s(): Handled Late resume event."
	       " rc = %d\n", __func__, rc);
}
#endif


static int msm_batt_enable_filter(u32 vbatt_filter)
{
	int rc;
	struct rpc_req_vbatt_filter {
		struct rpc_request_hdr hdr;
		u32 batt_handle;
		u32 enable_filter;
		u32 vbatt_filter;
	} req_vbatt_filter;

	struct rpc_rep_vbatt_filter {
		struct rpc_reply_hdr hdr;
		u32 filter_enable_result;
	} rep_vbatt_filter;

	req_vbatt_filter.batt_handle = cpu_to_be32(msm_batt_info.batt_handle);
	req_vbatt_filter.enable_filter = cpu_to_be32(1);
	req_vbatt_filter.vbatt_filter = cpu_to_be32(vbatt_filter);

	rc = msm_rpc_call_reply(msm_batt_info.batt_ep,
				BATTERY_ENABLE_DISABLE_FILTER_PROC,
				&req_vbatt_filter, sizeof(req_vbatt_filter),
				&rep_vbatt_filter, sizeof(rep_vbatt_filter),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		BAT_MSG_ERR("%s: msm_rpc_call_reply failed! proc = %d rc = %d\n",
			__func__, BATTERY_ENABLE_DISABLE_FILTER_PROC, rc);
		return rc;
	} else {
		rc =  be32_to_cpu(rep_vbatt_filter.filter_enable_result);

		if (rc != BATTERY_DEREGISTRATION_SUCCESSFUL) {
			BAT_MSG_ERR("%s: vbatt Filter enable failed."
				" proc = %d  filter enable result = %d"
				" filter number = %d\n", __func__,
				BATTERY_ENABLE_DISABLE_FILTER_PROC, rc,
				vbatt_filter);
			return -EIO;
		}

		BAT_MSG_DBG( "%s: vbatt Filter enabled successfully.\n",
				__func__);
		return 0;
	}
}


static int __devinit msm_batt_probe(struct platform_device *pdev)
{
	int rc;
	struct msm_psy_batt_pdata *pdata = pdev->dev.platform_data;

	if (pdev->id != -1) {
		dev_err(&pdev->dev,
			"%s: MSM chipsets Can only support one"
			" battery ", __func__);
		return -EINVAL;
	}

	if (pdata->avail_chg_sources & AC_CHG) {
		rc = power_supply_register(&pdev->dev, &msm_psy_ac);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"%s: power_supply_register failed"
				" rc = %d\n", __func__, rc);
			msm_batt_cleanup();
			return rc;
		}
		msm_batt_info.msm_psy_ac = &msm_psy_ac;
		msm_batt_info.avail_chg_sources |= AC_CHG;
	}

	if (pdata->avail_chg_sources & USB_CHG) {
		rc = power_supply_register(&pdev->dev, &msm_psy_usb);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"%s: power_supply_register failed"
				" rc = %d\n", __func__, rc);
			msm_batt_cleanup();
			return rc;
		}
		msm_batt_info.msm_psy_usb = &msm_psy_usb;
		msm_batt_info.avail_chg_sources |= USB_CHG;
	}

	if (!msm_batt_info.msm_psy_ac && !msm_batt_info.msm_psy_usb) {

		dev_err(&pdev->dev,
			"%s: No external Power supply(AC or USB)"
			"is avilable\n", __func__);
		msm_batt_cleanup();
		return -ENODEV;
	}

	msm_batt_info.voltage_max_design = pdata->voltage_max_design;
	msm_batt_info.voltage_min_design = pdata->voltage_min_design;
	msm_batt_info.batt_technology = pdata->batt_technology;
	msm_batt_info.calculate_capacity = pdata->calculate_capacity;

	if (!msm_batt_info.voltage_min_design)
		msm_batt_info.voltage_min_design = BATTERY_LOW;
	if (!msm_batt_info.voltage_max_design)
		msm_batt_info.voltage_max_design = BATTERY_HIGH;

	if (msm_batt_info.batt_technology == POWER_SUPPLY_TECHNOLOGY_UNKNOWN)
		msm_batt_info.batt_technology = POWER_SUPPLY_TECHNOLOGY_LION;
	if(BATT_MEASURE_UNKNOW == power_get_batt_measurement_type()) {
		power_set_batt_measurement_type(BATT_MEASURE_BY_QC_SOFTWARE);
	}

	if(BATT_MEASURE_BY_QC_SOFTWARE == power_get_batt_measurement_type()) {
		if (!msm_batt_info.calculate_capacity) {
			msm_batt_info.calculate_capacity = msm_batt_capacity;
             }
	}
    	else {
		msm_batt_info.calculate_capacity = NULL;
       }    	

	switch(power_get_batt_measurement_type()) {

		case BATT_MEASURE_BY_QC_SOFTWARE:
			rc = power_supply_register(&pdev->dev, &msm_psy_batt);
			if (rc < 0) {
				dev_err(&pdev->dev, "%s: power_supply_register failed"
					" rc=%d\n", __func__, rc);
				msm_batt_cleanup();
				return rc;
			}
			msm_batt_info.msm_psy_batt = &msm_psy_batt;
			break;
#if defined(CONFIG_BATTERY_BQ275X0) ||defined(CONFIG_BATTERY_BQ275X0_MODULE)            
       	case BATT_MEASURE_BY_BQ275x0:
			rc = power_supply_register(&pdev->dev, &msm_psy_batt_gauge);
			if (rc < 0) 
			{
				BAT_MSG_FATAL("failed to register bq275x0 battery\n");
				msm_batt_cleanup();
				return rc;
			}
			msm_batt_info.msm_psy_batt = &msm_psy_batt_gauge;
            		break;
#endif                    
		default:
			break;	
      	}
	rc = msm_batt_register(BATTERY_LOW, BATTERY_ALL_ACTIVITY,
			       BATTERY_CB_ID_ALL_ACTIV, BATTERY_ALL_ACTIVITY);
	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_register failed rc=%d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}
	msm_batt_info.batt_handle = rc;

	rc =  msm_batt_enable_filter(VBATT_FILTER);

#ifdef CONFIG_HAS_EARLYSUSPEND
	msm_batt_info.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	msm_batt_info.early_suspend.suspend = msm_batt_early_suspend;
	msm_batt_info.early_suspend.resume = msm_batt_late_resume;
	register_early_suspend(&msm_batt_info.early_suspend);
#endif
	msm_batt_start_cb_thread();

	return 0;
}


int msm_batt_get_charger_api_version(void)
{
	int rc ;
	struct rpc_reply_hdr *reply;

	struct rpc_req_chg_api_ver {
		struct rpc_request_hdr hdr;
		u32 more_data;
	} req_chg_api_ver;

	struct rpc_rep_chg_api_ver {
		struct rpc_reply_hdr hdr;
		u32 num_of_chg_api_versions;
		u32 *chg_api_versions;
	};

	u32 num_of_versions;

	struct rpc_rep_chg_api_ver *rep_chg_api_ver;


	req_chg_api_ver.more_data = cpu_to_be32(1);

	msm_rpc_setup_req(&req_chg_api_ver.hdr, CHG_RPC_PROG, CHG_RPC_VERS,
			 ONCRPC_CHARGER_API_VERSIONS_PROC);

	rc = msm_rpc_write(msm_batt_info.chg_ep, &req_chg_api_ver,
			sizeof(req_chg_api_ver));
	if (rc < 0) {
		BAT_MSG_ERR(
		       "%s(): msm_rpc_write failed.  proc = 0x%08x rc = %d\n",
		       __func__, ONCRPC_CHARGER_API_VERSIONS_PROC, rc);
		return rc;
	}

	for (;;) {
		rc = msm_rpc_read(msm_batt_info.chg_ep, (void *) &reply, -1,
				BATT_RPC_TIMEOUT);
		if (rc < 0)
			return rc;
		if (rc < RPC_REQ_REPLY_COMMON_HEADER_SIZE) {
			BAT_MSG_ERR("%s(): msm_rpc_read failed. read"
					" returned invalid packet which is"
					" neither rpc req nor rpc reply. "
					"legnth of packet = %d\n",
					__func__, rc);

			rc = -EIO;
			break;
		}
		/* we should not get RPC REQ or call packets -- ignore them */
		if (reply->type == RPC_TYPE_REQ) {

			BAT_MSG_ERR("%s(): returned RPC REQ packets while"
				" waiting for RPC REPLY replay read \n",
				__func__);
			kfree(reply);
			continue;
		}

		/* If an earlier call timed out, we could get the (no
		 * longer wanted) reply for it.	 Ignore replies that
		 * we don't expect
		 */
		if (reply->xid != req_chg_api_ver.hdr.xid) {

			BAT_MSG_ERR("%s(): returned RPC REPLY XID is not"
					" equall to RPC REQ XID \n", __func__);
			kfree(reply);
			continue;
		}
		if (reply->reply_stat != RPCMSG_REPLYSTAT_ACCEPTED) {
			rc = -EPERM;
			break;
		}
		if (reply->data.acc_hdr.accept_stat !=
				RPC_ACCEPTSTAT_SUCCESS) {
			rc = -EINVAL;
			break;
		}

		rep_chg_api_ver = (struct rpc_rep_chg_api_ver *)reply;

		num_of_versions =
			be32_to_cpu(rep_chg_api_ver->num_of_chg_api_versions);

		rep_chg_api_ver->chg_api_versions =  (u32 *)
			((u8 *) reply + sizeof(struct rpc_reply_hdr) +
			sizeof(rep_chg_api_ver->num_of_chg_api_versions));

		rc = be32_to_cpu(
			rep_chg_api_ver->chg_api_versions[num_of_versions - 1]);

		BAT_MSG_DBG( "%s(): num_of_chg_api_versions = %u"
			"  The chg api version = 0x%08x\n", __func__,
			num_of_versions, rc);
		break;
	}
	kfree(reply);
	return rc;
}

typedef struct
{
   unsigned int   volt;
   unsigned int   cap;
} volt2cap_strt;

typedef struct
{
   unsigned int   curr;
   unsigned int   cap;
} curr2cap_strt;

volt2cap_strt batt_power_supply_array[] = 
{
#if 0
	{4000,100},
	{3964,95},
	{3928,90},
	{3892,85},
	{3856,80},
	{3820,75},
	{3800,70},
	{3780,65},
	{3760,60},
	{3740,55},
	{3720,50},
	{3700,45},
	{3680,40},
	{3660,35},
	{3640,30},
	{3620,25},
	{3600,20}, 
	{3580,15},
	{3550,10}, 
	{3520,5},
	{3400,3},
	{3350,1}
#else
	{4070,100},
	{4040,95},
	{4000,90},
	{3970,85},
	{3940,80},
	{3920,75},
	{3900,70},
	{3880,65},
	{3860,60},
	{3840,55},
	{3820,50},
	{3800,45},
	{3780,40},
	{3760,35},
	{3740,30},
	{3710,25},
	{3670,20}, 
	{3620,15},
	{3560,10}, 
	{3520,5},
	{3400,3},
	{3350,1}
#endif
};

volt2cap_strt batt_idle_array[] = 
{
	{4200,100},
	{4160,95},
	{4120,90},
	{4060,85},
	{4020,80},
	{3990,75},
	{3970,70},
	{3950,65},
	{3930,60},
	{3910,55},
	{3880,50},
	{3860,45},
	{3840,40},
	{3820,35},
	{3800,30},
	{3780,25},
	{3750,20}, 
	{3700,15},
	{3650,10}, 
	{3580,5},
	{3480,3},
	{3350,1}
};

volt2cap_strt batt_plugin_array[] = 
{
	{4140,100},
	{4118,95},
	{4098,90},
	{4050,85},
	{4020,80},
	{3990,75},
	{3970,70},
	{3950,65},
	{3930,60},
	{3910,55},
	{3880,50},
	{3860,45},
	{3840,40},
	{3820,35},
	{3800,30},
	{3780,25},
	{3750,20}, 
	{3700,15},
	{3650,10}, 
	{3580,5},
	{3480,3},
	{3350,1}
};

volt2cap_strt wall_power_supply_array[] = 
{
	{4222,60},
	{4200,55},
	{4178,50},
	{4156,45},
	{4134,40},
	{4112,35},
	{4090,30},
	{4078,25},
	{4056,20},
	{4034,15},
	{4000,10},
	{3800,5},
	{3400,1}
};

#if 0
curr2cap_strt wall_power_curr_cap_array[] = 
{
	{225,99},
	{300,97},
	{500,94},
	{700,90},
	{900,86},
	{1100,81},
	{1300,75},
	{1500,68},
	{1700,60}
};
#else
curr2cap_strt wall_power_curr_cap_array[] = 
{
	{250,99},
	{300,98},
	{400,95},
	{500,94},
	{700,92},
	{800,90},
	{900,88},
	{1100,83},
	{1300,75},
	{1500,68},
	{1700,60}
};
#endif
static unsigned int get_capacity_by_volt(volt2cap_strt* volt_cap_array, int array_size, int voltage)
{
	int icount = 0, volt_step = 0, dalt_volt = 0, cap_step = 0, capacity = 0;
	if(voltage >= volt_cap_array[0].volt)
		capacity = volt_cap_array[0].cap;
	else if(voltage <= volt_cap_array[array_size - 1].volt)
		capacity = volt_cap_array[array_size - 1].cap;
	else
	{
		for(icount = 0; icount < array_size; icount++)
		{
			if((voltage < volt_cap_array[icount].volt) 
				&& (voltage >= volt_cap_array[icount+1].volt))
			{
				dalt_volt = voltage - volt_cap_array[icount+1].volt;
				volt_step = volt_cap_array[icount].volt - volt_cap_array[icount+1].volt;
				cap_step = volt_cap_array[icount].cap - volt_cap_array[icount+1].cap;
				capacity = volt_cap_array[icount+1].cap + (dalt_volt * cap_step)/volt_step;
				break;
			}
		}
	}
	return capacity;
}
#if 0
static unsigned int get_capacity_by_curr_charging(curr2cap_strt* curr_cap_array, unsigned int array_size, unsigned int charing_current)
{
	int icount = 0, curr_step = 0, dalt_curr = 0, cap_step = 0, capacity = 0;

	if(charing_current <= curr_cap_array[0].curr)
		capacity = curr_cap_array[0].cap;
	else if(charing_current >= curr_cap_array[array_size - 1].curr)
		capacity = curr_cap_array[array_size - 1].cap;
	else
	{
		for(icount = 0; icount < array_size; icount++)
		{
			if((charing_current > curr_cap_array[icount].curr) 
				&& (charing_current <= curr_cap_array[icount+1].curr))
			{
				dalt_curr = curr_cap_array[icount+1].curr - charing_current;
				curr_step = curr_cap_array[icount+1].curr - curr_cap_array[icount].curr;
				cap_step = curr_cap_array[icount].cap - curr_cap_array[icount+1].cap;
				capacity = curr_cap_array[icount+1].cap + (dalt_curr * cap_step)/curr_step;
				break;
			}
		}
	}
	return capacity;
}
#endif
static unsigned int huawei_batt_capacity(unsigned int power_supply_type, int voltage)
{
	int capacity = 0, array_size = 0;
	BAT_MSG_DBG( "%s: battery_volt = %d, power_supply = %d, gBspTimers1 = %d \n", __func__,voltage,power_supply_type,gBspTimers1);

	if((power_supply_type != POWER_SUPPLY_AC) &&
		(power_supply_type != POWER_SUPPLY_BATTERY))
		return 0;
	
	if(power_supply_type == POWER_SUPPLY_BATTERY)
	{
		if(gBspTimers1 == 0) //开机第一次读电量
		{

		//	array_size = sizeof(batt_idle_array)/sizeof(batt_idle_array[0]);
		//	capacity = get_capacity_by_volt( batt_idle_array,array_size,voltage);
			array_size = sizeof(batt_power_supply_array)/sizeof(batt_power_supply_array[0]);
			capacity = get_capacity_by_volt( batt_power_supply_array,array_size,voltage);
			BAT_MSG_DBG( "%s: GET POWER ON FIRST CAP: battery_volt = %d, power_supply = %d capacity = %d\n", __func__,voltage,power_supply_type,capacity);

		}
		else
		{
			array_size = sizeof(batt_power_supply_array)/sizeof(batt_power_supply_array[0]);
			capacity = get_capacity_by_volt( batt_power_supply_array,array_size,voltage);
		}
	}
	else if(power_supply_type == POWER_SUPPLY_AC)
	{
		if(gBspTimers1 == 0)  //开机第一次读电量
		{
			array_size = sizeof(batt_plugin_array)/sizeof(batt_plugin_array[0]);
			capacity = get_capacity_by_volt( batt_plugin_array,array_size,voltage);
		//	array_size = sizeof(batt_power_supply_array)/sizeof(batt_power_supply_array[0]);
		//	capacity = get_capacity_by_volt( batt_power_supply_array,array_size,voltage);
			
			if(capacity >= 100)
			{
				if((msm_batt_info.batt_status == BATTERY_STATUS_FULL) 
					|| (msm_batt_info.battery_level == BATTERY_LEVEL_FULL))
					capacity = 100;
				else
					capacity = 99;
			}
			BAT_MSG_DBG( "%s: GET POWER ON FIRST CAP: battery_volt = %d, power_supply = %d capacity = %d\n", __func__,voltage,power_supply_type,capacity);

		}
		else
		{
			array_size = sizeof(batt_idle_array)/sizeof(batt_idle_array[0]);
			capacity = get_capacity_by_volt( batt_idle_array,array_size,voltage);
			BAT_MSG_DBG( "%s: AC MODEM battery_volt = %d, power_supply = %d capacity = %d\n", __func__,voltage,power_supply_type,capacity);
		}
		#if 0
		else
		{
			if((msm_batt_info.batt_status == BATTERY_STATUS_FULL) || (msm_batt_info.battery_level == BATTERY_LEVEL_FULL))
			{
				capacity = 100;
			}
			else if(msm_batt_info.charging_machine_state == CHG_WALL_TOPOFF_ST) //恒压充电阶段
			{
				array_size = sizeof(wall_power_curr_cap_array)/sizeof(wall_power_curr_cap_array[0]);
				capacity = get_capacity_by_curr_charging( (curr2cap_strt *)wall_power_curr_cap_array,array_size,msm_batt_info.charging_current);
			}
			else if((msm_batt_info.charging_machine_state == CHG_WALL_BATT_PLUGIN_ST) || 
				(msm_batt_info.charging_machine_state == CHG_WALL_CHARGER_PLUGIN_ST))
			{
				array_size = sizeof(batt_plugin_array)/sizeof(batt_plugin_array[0]);
				capacity = get_capacity_by_volt( batt_plugin_array,array_size,voltage);
			}
			else if(msm_batt_info.charging_machine_state == CHG_WALL_MAINT_ST)
			{
				capacity = 100;
			}
			else
			{
				array_size = sizeof(wall_power_supply_array)/sizeof(wall_power_supply_array[0]);
				capacity = get_capacity_by_volt( wall_power_supply_array,array_size,voltage);
			}
			BAT_MSG_DBG( "%s: ***************************: battery_volt = %d, power_supply = %d capacity = %d\n", __func__,voltage,power_supply_type,capacity);

		}
		#endif
	}
	
	BAT_MSG_DBG( "%s: battery_volt = %d, power_supply = %d, capacity = %d \n", __func__,voltage,power_supply_type,capacity);
	return capacity;
}

#define   BSP_BATTERY_CAPACITY_PERCENTAGE_MA_SEC           79200       /*2200mA*3600s/100*/
static unsigned long g_charging_begin_time = 0;
static unsigned int g_charging_cmp_cap_time = 0;
static int charging_coulometer_by_current = 0;

static void msm_batt_update_psy_status_v1(void)
{
	static u32 unnecessary_event_count;
	u32 supply_type = POWER_SUPPLY_UNHNOW;   // 0:unknow; 1: bat; 2: ac;  3: usb 
    u32 ulBatt_capacity;
	u32 dalt_capacity_by_current = 0;
	signed int  dalt_capacity = 0;
	
	static int g_two_min_flag = 0;
	//static int capacity_increase_total = 0;
	static signed int charging_cap_gap = 0;
	//int capacity_increase_max = 0;
	unsigned int lcd_light_level = 0;
	static unsigned int charging_time_by_sec = 0;
	unsigned long temp_time = 0;
   	const unsigned long jifies_max = 0xffffffff;
	BAT_MSG_DBG("---------------------------\n enter %s func!\n",__func__);

	if(g_sleep_cal_capacity_flag)
	{
		BAT_MSG_DBG("%s: resume first call exit!\n",__func__);
		return ;
	}
	lcd_light_level = bsp_lcdc_panel_get_backlight();
	msm_batt_get_batt_chg_status_v1();

	unnecessary_event_count = 0;

	supply_type = msm_batt_get_power_supply_type(msm_batt_info.charger_type,msm_batt_info.charger_status,rep_batt_chg.charger_type,rep_batt_chg.charger_status);
	

	msm_batt_info.battery_voltage  	= 	rep_batt_chg.battery_voltage;
	msm_batt_info.charger_status 	= 	rep_batt_chg.charger_status;
	msm_batt_info.charger_type 	= 	rep_batt_chg.charger_type;
	msm_batt_info.batt_status 	=  	rep_batt_chg.battery_status;
	msm_batt_info.battery_level 	=  	rep_batt_chg.battery_level;
	msm_batt_info.battery_temp 	=	rep_batt_chg.battery_temp;
	msm_batt_info.pwrdwn_chg_flag =    rep_batt_chg.is_poweroff_charging;
	msm_batt_info.batt_status_prev = msm_batt_info.batt_status ;
	msm_batt_info.batt_valid  = rep_batt_chg.battery_valid;
	msm_batt_info.batt_health  = rep_batt_chg.battery_health;
	msm_batt_info.charging_machine_state  = rep_batt_chg.charging_machine_state;
	msm_batt_info.charging_current  = rep_batt_chg.charging_current;

	if(rep_batt_chg.battery_valid == 0)
	{
		msm_batt_info.batt_capacity = 0;
		msm_batt_info.battery_voltage = 2800;
		g_preBatterValid = 0;
		goto END_UPDATE_BATT_STAUS;
	}

	g_BatterValid = rep_batt_chg.battery_valid;
	ulBatt_capacity = msm_batt_info.calculate_capacity(rep_batt_chg.battery_voltage);
	
    BAT_MSG_DBG("%s: =============calculate_capacity = %d=============\n",__func__,ulBatt_capacity); 
	/*第一次执行，时从MODEM那里取一个初始值*/
    if(((0 == gBspTimers1) && (rep_batt_chg.battery_valid == 1) && 
		(rep_batt_chg.battery_status != BATTERY_STATUS_UNKNOWN)) 
		|| (((rep_batt_chg.battery_status == BATTERY_STATUS_DISCHARGING) ||
		(rep_batt_chg.battery_status == BATTERY_STATUS_NOT_CHARGING)) &&
		(g_BatterValid == 1) && (g_preBatterValid == 0)))
    {
    	gBspTimers1 = 1;
        msm_batt_info.batt_capacity  = ulBatt_capacity;
        msm_batt_info.batt_capacity_last = msm_batt_info.batt_capacity;
		msm_batt_info.batt_voltage_last = rep_batt_chg.battery_voltage;

		g_preBatterValid = rep_batt_chg.battery_valid;
		charging_cap_gap = 0;
		g_charging_begin_time = jiffies;
		charging_coulometer_by_current = 0;
		BAT_MSG_DBG("%s: ****************************battery_valid = %d,capacity = %d, voltage = %d, status = %s,level = %s, health = %s\n",
			__func__,rep_batt_chg.battery_valid,msm_batt_info.batt_capacity,rep_batt_chg.battery_voltage,
			battery_status[rep_batt_chg.battery_status],battery_level[rep_batt_chg.battery_level],battery_health[rep_batt_chg.battery_health]); 
			
		goto END_UPDATE_BATT_STAUS;
    }

	g_preBatterValid = rep_batt_chg.battery_valid;
	
	if(msm_batt_info.charging_machine_state == CHG_WALL_BATT_PLUGIN_ST)
	{
		msm_batt_info.batt_capacity  = ulBatt_capacity;
        msm_batt_info.batt_capacity_last = msm_batt_info.batt_capacity;
		msm_batt_info.batt_voltage_last = rep_batt_chg.battery_voltage;

		charging_cap_gap = 0;
		g_charging_begin_time = jiffies;
		charging_coulometer_by_current = 0;
		goto END_UPDATE_BATT_STAUS;
	}
				
	if(gpre_supply_type != gsupply_type)
	{
		BAT_MSG_DBG("%s: --------------POWER CHANGED------------------\n",__func__);
        /*插上外接电源*/
		msm_batt_info.batt_capacity_last = msm_batt_info.batt_capacity;
		msm_batt_info.batt_voltage_last = rep_batt_chg.battery_voltage;
        switch(gsupply_type)
        {
            case BSP_CHARGE_TYPE_E_AC:
                if(BSP_CHARGE_TYPE_E_BATTERY == gpre_supply_type)
                {        
                    gBspChargeStatus = BSP_CHARGE_STATUS_BATTERY_TO_AC;
                    BAT_MSG_DBG("%s: @@@@@@@@BSP_CHARGE_STATUS_BATTERY_TO_AC\n",__func__);
                }
				else if(BSP_CHARGE_TYPE_E_USB == gpre_supply_type)
				{
					gBspChargeStatus = BSP_CHARGE_STATUS_USB_TO_AC;
                    BAT_MSG_DBG("%s: @@@@@@@@BSP_CHARGE_STATUS_USB_TO_AC\n",__func__);
				}
                break;
            case BSP_CHARGE_TYPE_E_BATTERY:
                if(BSP_CHARGE_TYPE_E_AC == gpre_supply_type)
                {    
                    gBspChargeStatus = BSP_CHARGE_STATUS_AC_TO_BATTERY; 
                    BAT_MSG_DBG("%s: @@@@@@@@@@@BSP_CHARGE_STATUS_AC_TO_BATTERY\n",__func__);
                }
                else if(BSP_CHARGE_TYPE_E_USB == gpre_supply_type)
                {
                    gBspChargeStatus = BSP_CHARGE_STATUS_USB_TO_BATTERY;
                    BAT_MSG_DBG("%s: @@@@@@@@@@BSP_CHARGE_STATUS_USB_TO_BATTERY\n",__func__);
                }
                break;
			case BSP_CHARGE_TYPE_E_USB:
				if(BSP_CHARGE_TYPE_E_AC == gpre_supply_type)
                {    
                    gBspChargeStatus = BSP_CHARGE_STATUS_AC_TO_USB; 
                    BAT_MSG_DBG("%s: @@@@@@@@@@@BSP_CHARGE_STATUS_AC_TO_USB\n",__func__);
                }
                else if(BSP_CHARGE_TYPE_E_BATTERY == gpre_supply_type)
                {
                    gBspChargeStatus = BSP_CHARGE_STATUS_BATTERY_TO_USB;
                    BAT_MSG_DBG("%s: @@@@@@@@@@BSP_CHARGE_STATUS_BATTERY_TO_USB\n",__func__);
                }
                break;
            default:
                break;
        }
       
		gpre_supply_type = gsupply_type;
	}
    else
    {
    	switch(gsupply_type)
        {
            case BSP_CHARGE_TYPE_E_AC:
                gBspChargeStatus = BSP_CHARGE_STATUS_E_AC;
                BAT_MSG_DBG("%s: @@@@@@@@BSP_CHARGE_STATUS_EAC\n",__func__);
                break;
            case BSP_CHARGE_TYPE_E_BATTERY:
                  
                gBspChargeStatus = BSP_CHARGE_STATUS_E_BATTERY; 
                BAT_MSG_DBG("%s: @@@@@@@@@@@BSP_CHARGE_STATUS_E_BATTERY\n",__func__);
                break;
			case BSP_CHARGE_TYPE_E_USB:  
                gBspChargeStatus = BSP_CHARGE_STATUS_E_USB; 
                BAT_MSG_DBG("%s: @@@@@@@@@@@BSP_STATUS_E_USB\n",__func__);  
                break;
            default:
                break;
        }

    }
	
	switch(gBspChargeStatus)
    {
		case BSP_CHARGE_STATUS_E_AC:
			wake_lock_timeout(&msm_batt_info.wlock, HZ * 120);
			BAT_MSG_DBG("%s: AC CHARGING charging_machine_state = %d, batt_level = %d\n" 
				"charging_status = %d, batt_health = %d, lcd_level = %d, last_capacity = %d \n",
				__func__,msm_batt_info.charging_machine_state,msm_batt_info.battery_level,msm_batt_info.batt_status,
				msm_batt_info.batt_health,lcd_light_level,msm_batt_info.batt_capacity_last);
			if((msm_batt_info.battery_level == BATTERY_LEVEL_FULL) ||
			//	(msm_batt_info.battery_level == BATTERY_LEVEL_OVERVOLTAGE)||
				(msm_batt_info.batt_status == BATTERY_STATUS_FULL))
			{
				msm_batt_info.batt_voltage_last = msm_batt_info.battery_voltage;
				dalt_capacity = 100 - msm_batt_info.batt_capacity_last;
				charging_time_by_sec = 0;
			}
			else
			{
				/*判断jiffies是否发生回转*/    
				if(jiffies > g_charging_begin_time)    
				{        
					temp_time = jiffies - g_charging_begin_time;                        
				}    
				else    
				{        
					//temp_time = 0xffffffffffffffff - g_charging_begin_time;
					temp_time = jifies_max - g_charging_begin_time;        
					temp_time = jiffies + temp_time + 1;    
				}
				charging_time_by_sec += temp_time/HZ;

				BAT_MSG_DBG("%s: AC---------- g_charging_begin_time = %lu, jiffies = %lu, temp_time = %lu, charging_time_by_sec = %d S\n",
					__func__,g_charging_begin_time,jiffies,temp_time,charging_time_by_sec);

				if(msm_batt_info.charging_current == 0)
				{
					charging_time_by_sec = 0;
					//return;
					goto END_UPDATE_BATT_STAUS;
				}
				if(charging_time_by_sec < 40)
				{	
					//return;
					goto END_UPDATE_BATT_STAUS;
				}
				
				g_charging_cmp_cap_time += charging_time_by_sec;
				if(g_charging_cmp_cap_time >= 5*60)
				{
					g_charging_cmp_cap_time = 0;
					charging_cap_gap = ulBatt_capacity - msm_batt_info.batt_capacity_last;
				}
				g_charging_begin_time = jiffies;
				charging_coulometer_by_current += (msm_batt_info.charging_current)*charging_time_by_sec;
				dalt_capacity_by_current = charging_coulometer_by_current/BSP_BATTERY_CAPACITY_PERCENTAGE_MA_SEC;

				BAT_MSG_DBG("%s: AC---------- charging_cap_gap = %d charging_curr = %dMA charging_coulometer_by_current = %dMA/Sec dalt_capacity_by_current = %d \n",
					__func__,charging_cap_gap,msm_batt_info.charging_current,charging_coulometer_by_current,dalt_capacity_by_current);
				if(charging_cap_gap > 0)
				{
					if(charging_cap_gap > 5)
					{
						if(dalt_capacity_by_current > 1)
							dalt_capacity = dalt_capacity_by_current;
						else
							dalt_capacity = 1;

						if(charging_cap_gap < dalt_capacity)
							charging_cap_gap -= dalt_capacity;
						else
							charging_cap_gap = 0;
						
						if(charging_coulometer_by_current >= BSP_BATTERY_CAPACITY_PERCENTAGE_MA_SEC * dalt_capacity)
							charging_coulometer_by_current -= BSP_BATTERY_CAPACITY_PERCENTAGE_MA_SEC * dalt_capacity;
						else
							charging_coulometer_by_current = 0;
					}
					else
					{
						if(dalt_capacity_by_current > 0)
						{
							charging_coulometer_by_current -= BSP_BATTERY_CAPACITY_PERCENTAGE_MA_SEC * dalt_capacity_by_current;
							dalt_capacity = dalt_capacity_by_current;
						}
						else
						{				
							g_two_min_flag = (g_two_min_flag + 1)%2;
							if(g_two_min_flag == 0)
							{
								dalt_capacity = 1;
								charging_cap_gap -= 1;
							}
							else
								dalt_capacity = 0;
							 
						}
					}
				}
				else if(charging_cap_gap < 0)
				{					
					#if 0
					if(dalt_capacity_by_current > 0)
					{
						if(abs(charging_cap_gap) > 15)
							g_two_min_flag = (g_two_min_flag + 1)%2;
						if((abs(charging_cap_gap) > 5) && (abs(charging_cap_gap) <= 15))
							g_two_min_flag = (g_two_min_flag + 1)%3;
						else
							g_two_min_flag = (g_two_min_flag + 1)%5;
						
						if(g_two_min_flag == 0)
						{
							dalt_capacity = dalt_capacity_by_current - 1;
							charging_cap_gap += 1;
						}
						else
							dalt_capacity = dalt_capacity_by_current;
						charging_coulometer_by_current -= BSP_BATTERY_CAPACITY_PERCENTAGE_MA_SEC * dalt_capacity_by_current;
					}
					#else
					charging_coulometer_by_current = 0;
					dalt_capacity = 0;
					#endif
					
				}
				else //charging_cap_gap == 0
				{
					if(dalt_capacity_by_current > 0)
					{
						charging_coulometer_by_current -= BSP_BATTERY_CAPACITY_PERCENTAGE_MA_SEC * dalt_capacity_by_current;
						dalt_capacity = dalt_capacity_by_current;
					}
				}
				BAT_MSG_DBG("%s: AC CHARGING charging_time_by_sec = %d, dalt_capacity_by_current = %d \n" 
				"dalt_capacity = %d, charging_cap_gap = %d, g_two_min_flag = %d \n",
				__func__,charging_time_by_sec,
				dalt_capacity_by_current,dalt_capacity,charging_cap_gap,g_two_min_flag);

				charging_time_by_sec = 0;
			}
			break;
        case BSP_CHARGE_STATUS_BATTERY_TO_AC: 
			BAT_MSG_DBG("%s: ++++++++++++++ BSP_CHARGE_STATUS_BATTERY_TO_AC +++++++++++++\n", __func__);

			wake_lock_timeout(&msm_batt_info.wlock, HZ * 120);
			msm_batt_info.batt_capacity_last = msm_batt_info.batt_capacity;
			msm_batt_info.batt_voltage_last = rep_batt_chg.battery_voltage;	
			dalt_capacity = 0;
			charging_cap_gap = ulBatt_capacity - msm_batt_info.batt_capacity_last;
			g_charging_begin_time = jiffies;
			charging_coulometer_by_current = 0;
			g_charging_cmp_cap_time = 0;
			charging_time_by_sec = 0;
			BAT_MSG_DBG("%s: BATT TO AC---------- charging_cap_gap = %d, g_charging_begin_time = %lu \n",__func__,charging_cap_gap,g_charging_begin_time);
            break;
		case BSP_CHARGE_STATUS_AC_TO_BATTERY:
			BAT_MSG_DBG("%s: ++++++++++++++ BSP_CHARGE_STATUS_AC_TO_BATTERY ++++++++++++++\n", __func__);
			msm_batt_info.batt_capacity_last = msm_batt_info.batt_capacity;
			msm_batt_info.batt_voltage_last = rep_batt_chg.battery_voltage;
			wake_unlock(&msm_batt_info.wlock);
			dalt_capacity = 0;
			charging_cap_gap = 0;
            break;
			
		case BSP_CHARGE_STATUS_E_BATTERY:
			if(ulBatt_capacity >= msm_batt_info.batt_capacity_last)
			{
				dalt_capacity = 0;
				msm_batt_info.batt_voltage_last = rep_batt_chg.battery_voltage;
				BAT_MSG_DBG("%s: IN BATT Mode, disp capacity <= true capacity by volt, disp_capacity = %d, true_capacity = %d \n",__func__,msm_batt_info.batt_capacity_last,ulBatt_capacity);
			}
			else
			{
				if(msm_batt_info.battery_voltage > VOLTAGE_ADJUST_LEVEL1)
				{
					if(msm_batt_info.batt_capacity_last - ulBatt_capacity >= 10)
						dalt_capacity = -1;
					else
					{
						g_two_min_flag = (g_two_min_flag + 1)%2;
						if(g_two_min_flag == 1)
							dalt_capacity = -1;
						else
							dalt_capacity = 0;
					}
				}
				else if((msm_batt_info.battery_voltage > VOLTAGE_ADJUST_LEVEL2) && (msm_batt_info.battery_voltage <= VOLTAGE_ADJUST_LEVEL1 ))
				{
					if(msm_batt_info.batt_capacity_last - ulBatt_capacity >= 10)
					{
						if(abs(msm_batt_info.batt_capacity_last - ulBatt_capacity)/2 >= 1)
							dalt_capacity = 0 - (msm_batt_info.batt_capacity_last - ulBatt_capacity)/2;
						else
							dalt_capacity = -1;
					}
					else if((msm_batt_info.batt_capacity_last - ulBatt_capacity >= 3) && (msm_batt_info.batt_capacity_last - ulBatt_capacity < 10))
						dalt_capacity = -1;
					else
					{
						g_two_min_flag = (g_two_min_flag + 1)%2;
						if(g_two_min_flag == 1)
							dalt_capacity = -1;
						else
							dalt_capacity = 0;
					}
				}
				else
				{
				/*	if(msm_batt_info.batt_capacity_last > 15)
					{
						if(abs(msm_batt_info.batt_capacity_last - ulBatt_capacity)/2 >= 1)
							dalt_capacity = 0 - (msm_batt_info.batt_capacity_last - ulBatt_capacity)/2;
						else
							dalt_capacity = -1;
					}
					else */
						dalt_capacity = ulBatt_capacity - msm_batt_info.batt_capacity_last;
				}
			}
			break;
    	case BSP_CHARGE_STATUS_E_USB:
    	case BSP_CHARGE_STATUS_USB_TO_BATTERY:
    	case BSP_CHARGE_STATUS_BATTERY_TO_USB:
    	case BSP_CHARGE_STATUS_USB_TO_AC:
    	case BSP_CHARGE_STATUS_AC_TO_USB:
			break;
        default:
            break;
            
    }

	if(dalt_capacity < 0)
	{
	//	if(msm_batt_info.batt_capacity_last +  dalt_capacity >= BATTERY_EMPTY_DISP_CAPACITY)
		if(msm_batt_info.batt_capacity_last > abs(dalt_capacity) + BATTERY_EMPTY_DISP_CAPACITY)
			msm_batt_info.batt_capacity = msm_batt_info.batt_capacity_last +  dalt_capacity;
		else
			msm_batt_info.batt_capacity = BATTERY_EMPTY_DISP_CAPACITY;

	}
	else
	{
		if(msm_batt_info.batt_health == BATTERY_HEALTH_GOOD)
			msm_batt_info.batt_capacity = msm_batt_info.batt_capacity_last +  dalt_capacity;
	}

    if(msm_batt_info.batt_capacity >= 100) 
    {
		if(rep_batt_chg.battery_status == BATTERY_STATUS_CHARGING)
			msm_batt_info.batt_capacity = 99;
    }

	if((msm_batt_info.battery_level == BATTERY_LEVEL_FULL) ||
	//	(msm_batt_info.battery_level == BATTERY_LEVEL_OVERVOLTAGE)||
		(msm_batt_info.batt_status == BATTERY_STATUS_FULL))
	{
		msm_batt_info.batt_capacity = 100;
	}
	
	BAT_MSG_DBG(
		"dalt_capacity = %d , pre_capacity = %d, cur_capacity = %d, batt_status=%s,batt_level = %s\n "
		"last_voltage = %d, cur_voltage = %d \n",
		dalt_capacity,msm_batt_info.batt_capacity_last,msm_batt_info.batt_capacity,
		battery_status[msm_batt_info.batt_status],battery_level[msm_batt_info.battery_level],
		msm_batt_info.batt_voltage_last,rep_batt_chg.battery_voltage);

	msm_batt_info.batt_capacity_last = msm_batt_info.batt_capacity;
	
END_UPDATE_BATT_STAUS:
	
    BAT_MSG_DBG("%s() : ***********************************\n",__func__);
	BAT_MSG_DBG("batt_health = %s, charger_type = %s, "
				"batt_valid = %d, batt_status = %s, batt_level = %s\n"
				"batt_capacity = %d%%, batt_volt = %u, batt_temp = %d, charger_status = %s,\n" 
				"ispowerdown_charging = %d\n\n",
				battery_health[msm_batt_info.batt_health],
				charger_type[msm_batt_info.charger_type],
				msm_batt_info.batt_valid,
				battery_status[msm_batt_info.batt_status],
				battery_level[msm_batt_info.battery_level],
				msm_batt_info.batt_capacity,
				msm_batt_info.battery_voltage,
				msm_batt_info.battery_temp,
				charger_status[msm_batt_info.charger_status],
				msm_batt_info.pwrdwn_chg_flag);

	msm_batt_change_power_supply(supply_type);

}

static unsigned int  msm_batt_capacity (u32 batt_voltage)
{
	unsigned int power_supply_type = POWER_SUPPLY_UNHNOW;
	//unsigned int lcd_level = 0;
	unsigned int batt_capacity = 1;

	BAT_MSG_DBG( "%s: battery_level = %s, battery_status = %s, charger_status = %s,battery_voltage = %d \n", 
		__func__,battery_level[msm_batt_info.battery_level],battery_status[msm_batt_info.batt_status],
		charger_status[msm_batt_info.charger_status],batt_voltage);
	if((msm_batt_info.battery_level == BATTERY_LEVEL_FULL) ||
		(msm_batt_info.battery_level == BATTERY_LEVEL_OVERVOLTAGE)||
		(msm_batt_info.batt_status == BATTERY_STATUS_FULL))
	{
		batt_capacity = 100;
		return batt_capacity;
	}
	else if(msm_batt_info.battery_level == BATTERY_LEVEL_DEAD)
	{
		batt_capacity = 0;
		return batt_capacity;
	}


	if(	(msm_batt_info.batt_status == BATTERY_STATUS_CHARGING)&&
		(msm_batt_info.charger_type== CHARGER_TYPE_WALL)&&
		((msm_batt_info.charger_status == CHARGER_STATUS_GOOD)||
		(msm_batt_info.charger_status == CHARGER_STATUS_WEAK)))
	{
		power_supply_type = POWER_SUPPLY_AC;
	}
	else
		power_supply_type = POWER_SUPPLY_BATTERY;

	batt_capacity = huawei_batt_capacity(power_supply_type,batt_voltage);

	if(batt_capacity > 100)
		batt_capacity = 100;
	BAT_MSG_DBG( "%s: input battery_volt = %d, power_supply = %d, capacity = %d, lcd_level = %d \n", __func__,batt_voltage,power_supply_type,batt_capacity,lcd_level);
	return batt_capacity;
}



static unsigned int msm_batt_get_power_supply_type(unsigned int pre_charger_type, 
													unsigned int pre_charger_status, 
													unsigned int cur_charger_type, 
													unsigned int cur_charger_status)
{
	unsigned int supply_type = POWER_SUPPLY_UNHNOW;   // 0:unknow; 1: bat; 2: ac;  3: usb 
	if(pre_charger_type != cur_charger_type)
	{
		if(cur_charger_type == CHARGER_TYPE_WALL)
		{
			supply_type = POWER_SUPPLY_AC; 
			gsupply_type = BSP_CHARGE_TYPE_E_AC;
		}
		else if((cur_charger_type == CHARGER_TYPE_USB_WALL) ||
			(cur_charger_type == CHARGER_TYPE_USB_PC) ||
			(cur_charger_type == CHARGER_TYPE_USB_CARKIT))
		{
			supply_type = POWER_SUPPLY_USB;
			gsupply_type = BSP_CHARGE_TYPE_E_USB;
		}
		else
		{
			supply_type = POWER_SUPPLY_BATTERY;
			gsupply_type = BSP_CHARGE_TYPE_E_BATTERY;
		}
	}
	else
	{
		if(pre_charger_status != cur_charger_status)
		{
			if(((pre_charger_status == CHARGER_STATUS_GOOD) || 
				(pre_charger_status == CHARGER_STATUS_WEAK))&&
				((cur_charger_status == CHARGER_STATUS_BAD) ||
				(cur_charger_status == CHARGER_STATUS_INVALID)))
			{
				supply_type = POWER_SUPPLY_BATTERY;
				gsupply_type = BSP_CHARGE_TYPE_E_BATTERY;
			}
			else if(((cur_charger_status == CHARGER_STATUS_GOOD) || 
				(cur_charger_status == CHARGER_STATUS_WEAK))&&
				((pre_charger_status == CHARGER_STATUS_BAD) ||
				(pre_charger_status == CHARGER_STATUS_INVALID)))
			{
				if(cur_charger_type == CHARGER_TYPE_WALL)
				{
					supply_type = POWER_SUPPLY_AC; 
					gsupply_type = BSP_CHARGE_TYPE_E_AC;
				}
				else if((cur_charger_type == CHARGER_TYPE_USB_WALL) ||
					(cur_charger_type == CHARGER_TYPE_USB_PC) ||
					(cur_charger_type == CHARGER_TYPE_USB_CARKIT))
				{
					supply_type = POWER_SUPPLY_USB;
					gsupply_type = BSP_CHARGE_TYPE_E_USB;
				}
				else
				{
					supply_type = POWER_SUPPLY_BATTERY;
					gsupply_type = BSP_CHARGE_TYPE_E_BATTERY;
				}
			}
		}
		else
		{
			if(cur_charger_type == CHARGER_TYPE_WALL)
			{
				supply_type = POWER_SUPPLY_AC; 
				gsupply_type = BSP_CHARGE_TYPE_E_AC;
			}
			else if((cur_charger_type == CHARGER_TYPE_USB_WALL) ||
				(cur_charger_type == CHARGER_TYPE_USB_PC) ||
				(cur_charger_type == CHARGER_TYPE_USB_CARKIT))
			{
				supply_type = POWER_SUPPLY_USB;
				gsupply_type = BSP_CHARGE_TYPE_E_USB;
			}
			else
			{
				supply_type = POWER_SUPPLY_BATTERY;
				gsupply_type = BSP_CHARGE_TYPE_E_BATTERY;
			}
		}
			
	}
	return supply_type;
}

static void msm_batt_change_power_supply(unsigned int supply_type)
{
	if(supply_type == POWER_SUPPLY_BATTERY) //battery
	{
		msm_batt_info.current_chg_source &= ~USB_CHG;
		msm_batt_info.current_chg_source &= ~AC_CHG;
		BAT_MSG_DBG("%s() : charger_type = BATT\n\n\n\n\n\n",
				__func__);
		power_supply_changed(msm_batt_info.msm_psy_batt);
	}
	else if(supply_type == POWER_SUPPLY_AC)
	{
		msm_batt_info.current_chg_source &= ~USB_CHG;
		msm_batt_info.current_chg_source |= AC_CHG;
		BAT_MSG_DBG("%s() : charger_type = WALL\n\n\n\n\n\n",
				__func__);
		power_supply_changed(&msm_psy_ac);
	}
	else if(supply_type == POWER_SUPPLY_USB)
	{
		msm_batt_info.current_chg_source &= ~AC_CHG;
		msm_batt_info.current_chg_source |= USB_CHG;
		BAT_MSG_DBG("%s() : charger_type = USB\n\n\n\n\n\n",__func__);		
		power_supply_changed(&msm_psy_usb);
	}
}

static struct platform_driver msm_batt_driver;
static int __devinit msm_batt_init_rpc(void)
{
	int rc;

	spin_lock_init(&msm_batt_info.lock);

	msm_batt_info.msm_batt_wq =
	    create_singlethread_workqueue("msm_battery");

	if (!msm_batt_info.msm_batt_wq) {
		BAT_MSG_ERR("%s: create workque failed \n", __func__);
		return -ENOMEM;
	}

	msm_batt_info.batt_ep =
	    msm_rpc_connect_compatible(BATTERY_RPC_PROG, BATTERY_RPC_VERS, 0);

	if (msm_batt_info.batt_ep == NULL) {
		return -ENODEV;
	} else if (IS_ERR(msm_batt_info.batt_ep)) {
		int rc = PTR_ERR(msm_batt_info.batt_ep);
		BAT_MSG_ERR("%s: rpc connect failed for BATTERY_RPC_PROG."
		       " rc = %d\n ", __func__, rc);
		msm_batt_info.batt_ep = NULL;
		return rc;
	}
	msm_batt_info.chg_ep =
	    msm_rpc_connect_compatible(CHG_RPC_PROG, CHG_RPC_VERS, 0);

	if (msm_batt_info.chg_ep == NULL) {
		return -ENODEV;
	} else if (IS_ERR(msm_batt_info.chg_ep)) {
		int rc = PTR_ERR(msm_batt_info.chg_ep);
		BAT_MSG_ERR("%s: rpc RPC_PROG. rc = %d\n",
		       __func__, rc);
		msm_batt_info.chg_ep = NULL;
		return rc;
	}

	msm_batt_info.chg_api_version =  msm_batt_get_charger_api_version();

	if (msm_batt_info.chg_api_version < 0)
		msm_batt_info.chg_api_version = DEFAULT_CHARGER_API_VERSION;

	rc = platform_driver_register(&msm_batt_driver);

	if (rc < 0) {
		BAT_MSG_ERR("%s: platform_driver_register failed for "
			"batt driver. rc = %d\n", __func__, rc);
		return rc;
	}

	init_waitqueue_head(&msm_batt_info.wait_q);

	return 0;
}

static int __devexit msm_batt_remove(struct platform_device *pdev)
{
	int rc;
	rc = msm_batt_cleanup();

	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_cleanup  failed rc=%d\n", __func__, rc);
		return rc;
	}
	return 0;
}

static void msm_batt_suspend_work(void)
{
	return;
}

static void msm_batt_resume_work(void)
{
	unsigned int supply_type = POWER_SUPPLY_UNHNOW;   // 0:unknow; 1: bat; 2: ac;  3: usb 
	unsigned int volt_capacity = 0;
	msm_batt_get_batt_chg_status_v1();
	
	volt_capacity = msm_batt_info.calculate_capacity(rep_batt_chg.battery_voltage);

	supply_type = msm_batt_get_power_supply_type(msm_batt_info.charger_type,msm_batt_info.charger_status,rep_batt_chg.charger_type,rep_batt_chg.charger_status);
	
	msm_batt_info.battery_voltage  	= 	rep_batt_chg.battery_voltage;
	msm_batt_info.charger_status 	= 	rep_batt_chg.charger_status;
	msm_batt_info.charger_type 	= 	rep_batt_chg.charger_type;
	msm_batt_info.batt_status 	=  	rep_batt_chg.battery_status;
	msm_batt_info.battery_level 	=  	rep_batt_chg.battery_level;
	msm_batt_info.battery_temp 	=	rep_batt_chg.battery_temp;
	msm_batt_info.pwrdwn_chg_flag =    rep_batt_chg.is_poweroff_charging;
	msm_batt_info.batt_status_prev = msm_batt_info.batt_status ;
	msm_batt_info.batt_valid  = rep_batt_chg.battery_valid;
	msm_batt_info.batt_health  = rep_batt_chg.battery_health;

	msm_batt_info.batt_voltage_last = msm_batt_info.battery_voltage;
	if((msm_batt_info.charger_status == BATTERY_STATUS_DISCHARGING) 
		|| (msm_batt_info.charger_status == BATTERY_STATUS_NOT_CHARGING))
	{
		if(volt_capacity < msm_batt_info.batt_capacity_last)
		{
			msm_batt_info.batt_capacity = volt_capacity;
			msm_batt_info.batt_capacity_last = msm_batt_info.batt_capacity;
		}
		else
			msm_batt_info.batt_capacity = msm_batt_info.batt_capacity_last;
	}
	else if(msm_batt_info.charger_status == BATTERY_STATUS_CHARGING)
	{
		if(volt_capacity > msm_batt_info.batt_capacity_last)
		{
			msm_batt_info.batt_capacity = volt_capacity;
			msm_batt_info.batt_capacity_last = msm_batt_info.batt_capacity;
		}
		else
			msm_batt_info.batt_capacity = msm_batt_info.batt_capacity_last;
	}
	else if(msm_batt_info.charger_status == BATTERY_STATUS_FULL)
	{
		msm_batt_info.batt_capacity = 100;
		msm_batt_info.batt_capacity_last = msm_batt_info.batt_capacity;
	}

	if(gpre_supply_type != gsupply_type)
	{
		BAT_MSG_DBG("%s: --------------POWER CHANGED------------------\n",__func__);
        /*插上外接电源*/
        switch(gsupply_type)
        {
            case BSP_CHARGE_TYPE_E_AC:
                if(BSP_CHARGE_TYPE_E_BATTERY == gpre_supply_type)
                {        
                    gBspChargeStatus = BSP_CHARGE_STATUS_BATTERY_TO_AC;
                    BAT_MSG_DBG("%s: @@@@@@@@BSP_CHARGE_STATUS_BATTERY_TO_AC\n",__func__);
                }
				else if(BSP_CHARGE_TYPE_E_USB == gpre_supply_type)
				{
					gBspChargeStatus = BSP_CHARGE_STATUS_USB_TO_AC;
                    BAT_MSG_DBG("%s: @@@@@@@@BSP_CHARGE_STATUS_USB_TO_AC\n",__func__);
				}
				wake_lock_timeout(&msm_batt_info.wlock, HZ * 120);
                break;
            case BSP_CHARGE_TYPE_E_BATTERY:
                if(BSP_CHARGE_TYPE_E_AC == gpre_supply_type)
                {    
                    gBspChargeStatus = BSP_CHARGE_STATUS_AC_TO_BATTERY; 
				//	wake_unlock(&msm_batt_info.wlock);
                    BAT_MSG_DBG("%s: @@@@@@@@@@@BSP_CHARGE_STATUS_AC_TO_BATTERY\n",__func__);
                }
                else if(BSP_CHARGE_TYPE_E_USB == gpre_supply_type)
                {
                    gBspChargeStatus = BSP_CHARGE_STATUS_USB_TO_BATTERY;
                    BAT_MSG_DBG("%s: @@@@@@@@@@BSP_CHARGE_STATUS_USB_TO_BATTERY\n",__func__);
                }
                break;
			case BSP_CHARGE_TYPE_E_USB:
				if(BSP_CHARGE_TYPE_E_AC == gpre_supply_type)
                {    
                    gBspChargeStatus = BSP_CHARGE_STATUS_AC_TO_USB; 
                    BAT_MSG_DBG("%s: @@@@@@@@@@@BSP_CHARGE_STATUS_AC_TO_USB\n",__func__);
                }
                else if(BSP_CHARGE_TYPE_E_BATTERY == gpre_supply_type)
                {
                    gBspChargeStatus = BSP_CHARGE_STATUS_BATTERY_TO_USB;
                    BAT_MSG_DBG("%s: @@@@@@@@@@BSP_CHARGE_STATUS_BATTERY_TO_USB\n",__func__);
                }
                break;
            default:
                break;
        }
       
		gpre_supply_type = gsupply_type;
	}
    else
    {
    	switch(gsupply_type)
        {
            case BSP_CHARGE_TYPE_E_AC:
                gBspChargeStatus = BSP_CHARGE_STATUS_E_AC;
				wake_lock_timeout(&msm_batt_info.wlock, HZ * 120);
                BAT_MSG_DBG("%s: @@@@@@@@BSP_CHARGE_STATUS_EAC\n",__func__);
                break;
            case BSP_CHARGE_TYPE_E_BATTERY:
                  
                gBspChargeStatus = BSP_CHARGE_STATUS_E_BATTERY; 
                BAT_MSG_DBG("%s: @@@@@@@@@@@BSP_CHARGE_STATUS_E_BATTERY\n",__func__);
                break;
			case BSP_CHARGE_TYPE_E_USB:  
                gBspChargeStatus = BSP_CHARGE_STATUS_E_USB; 
                BAT_MSG_DBG("%s: @@@@@@@@@@@BSP_STATUS_E_USB\n",__func__);  
                break;
            default:
                break;
        }

    }

	BAT_MSG_DBG("%s() : ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n",__func__);
	BAT_MSG_DBG("batt_health = %s, charger_type = %s, "
				"batt_valid = %d, batt_status = %s, batt_level = %s\n"
				"batt_capacity = %d%%, batt_volt = %u, batt_temp = %d, charger_status = %s,\n" 
				"ispowerdown_charging = %d\n",
				battery_health[msm_batt_info.batt_health],
				charger_type[msm_batt_info.charger_type],
				msm_batt_info.batt_valid,
				battery_status[msm_batt_info.batt_status],
				battery_level[msm_batt_info.battery_level],
				msm_batt_info.batt_capacity,
				msm_batt_info.battery_voltage,
				msm_batt_info.battery_temp,
				charger_status[msm_batt_info.charger_status],
				msm_batt_info.pwrdwn_chg_flag);
	msm_batt_change_power_supply(supply_type);
}

int msm_batt_suspend(struct platform_device *dev_id, pm_message_t state)
{
	BAT_MSG_DBG( "----------------%s()-----------\n", __func__);	
    if(BATT_MEASURE_BY_QC_SOFTWARE == power_get_batt_measurement_type())
	{
		g_sleep_cal_capacity_flag = true;
		msm_batt_suspend_work();
    }
	return 0;
}

int msm_batt_resume(struct platform_device *dev_id)
{
	BAT_MSG_DBG( "----------------%s()-----------\n", __func__);	
    if(BATT_MEASURE_BY_QC_SOFTWARE == power_get_batt_measurement_type())
	{
		msm_batt_resume_work();
		g_sleep_cal_capacity_flag = false;
    }
	return 0;
	
}

static struct platform_driver msm_batt_driver = {
	.probe = msm_batt_probe,
	.remove = __devexit_p(msm_batt_remove),
	.suspend = msm_batt_suspend,
	.resume = msm_batt_resume,
	.driver = {
		   .name = "msm-battery",
		   .owner = THIS_MODULE,
		   },
};

static int __init msm_batt_init(void)
{
	int rc;

	rc = msm_batt_init_rpc();

	if (rc < 0) {
		BAT_MSG_ERR("%s: msm_batt_init_rpc Failed  rc=%d\n",
		       __func__, rc);
		msm_batt_cleanup();
		return rc;
	}

	msm_batt_suspend_locks_init(&msm_batt_info,1);
	return 0;
}

static void __exit msm_batt_exit(void)
{
	platform_driver_unregister(&msm_batt_driver);
	msm_batt_suspend_locks_init(&msm_batt_info,0);
}

//module_init(msm_batt_init);
late_initcall(msm_batt_init);	/*init msm_battery driver after detect coloumbic*/
module_exit(msm_batt_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Kiran Kandi, Qualcomm Innovation Center, Inc.");
MODULE_DESCRIPTION("Battery driver for Qualcomm MSM chipsets.");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:msm_battery");
