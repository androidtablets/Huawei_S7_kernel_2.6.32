/* Driver for dock device 

 *  Copyright 2010 HUAWEI Tech. Co., Ltd.

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
/*
 *  Dock detect driver.
 *
 */

#include <linux/interrupt.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <linux/input/dock_det.h>
#include <mach/msm_rpcrouter.h>
#include <linux/err.h>

#define HARDWARE_VERSION_RPC_PROG	0x30000089
#define HARDWARE_VERSION_RPC_VERS	0x00010001
#define ONCRPC_HARDWARE_VERSION_READ_PROC  24
#define HARDWARE_VERSION_RPC_TIMEOUT    1000	/* 10 sec 10000*/
#define DONT_OPERATE_65023_LDO2_HD_VER_NUM   10

#define DOCK_PLUGIN		0
#define DOCK_PLUGOUT	1

static char g_hardware_version_num[33];
static char WCDMA_1_ptr[15] = {'V', '0', '0', '0', '1', '_', '0', '0', '0', '1', '_', '0', '0', '1', '4'};
static char WCDMA_2_ptr[15] = {'V', '0', '0', '0', '1', '_', '0', '0', '1', '4', '_', '0', '0', '1', '4'};   
static char WCDMA_3_ptr[15] = {'V', '0', '0', '0', '1', '_', '0', '0', '0', '1', '_', '0', '0', '0', '1'};   
static char WCDMA_4_ptr[15] = {'V', '0', '0', '0', '1', '_', '0', '0', '0', '1', '_', '0', '0', '0', '5'};   
static char EVDO_ptr[15] = {'V', '0', '0', '0', '5', '_', '0', '0', '0', '1', '_', '0', '0', '0', '1'};  

struct dock_detect_data
{	
	u32	irq;
	u32	det_gpio;
};

static struct dock_detect_data *data = NULL;
static int p_state = DOCK_PLUGOUT; 
 
static int get_hardware_version_info(void)
{
	struct msm_rpc_endpoint *hardware_version_ep;
	struct rpc_request_hdr request_hardware_ver;
	struct rpc_rep_hardware_ver{
		struct rpc_reply_hdr hdr;
		char hardware_num[33];
	} reply_hardware_ver;

	int rc = 0;
	int i=0;

	hardware_version_ep =
	    msm_rpc_connect_compatible(HARDWARE_VERSION_RPC_PROG, HARDWARE_VERSION_RPC_VERS, 0);

	if (hardware_version_ep == NULL)
	{
		printk(KERN_ERR "%s: rpc connect failed .\n", __func__);
		return -ENODEV;
	}
	else if (IS_ERR(hardware_version_ep)) 
	{
		rc = PTR_ERR(hardware_version_ep);
		printk(KERN_ERR
		       "%s: rpc connect failed ."
		       " rc = %d\n ", __func__, rc);
		hardware_version_ep = NULL;
		return rc;
	}
	
	rc = msm_rpc_call_reply(hardware_version_ep,
				ONCRPC_HARDWARE_VERSION_READ_PROC,
				&request_hardware_ver, sizeof(request_hardware_ver),
				&reply_hardware_ver, sizeof(reply_hardware_ver),
				msecs_to_jiffies(HARDWARE_VERSION_RPC_TIMEOUT));
	if (rc < 0) {
		printk(KERN_ERR "%s(): msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_HARDWARE_VERSION_READ_PROC, rc);
		return rc;
	} 

	for(i=0; i<33; i++){
		g_hardware_version_num[i] = reply_hardware_ver.hardware_num[i];
	}

	printk(KERN_ERR "%s(): msm_rpc_call_reply succeed! proc=%d, hardware_num=%s, g_hardware_version_num = %s\n",
		       __func__, ONCRPC_HARDWARE_VERSION_READ_PROC, reply_hardware_ver.hardware_num, g_hardware_version_num);
	
	return rc;
}


static int memcmp__buffer(char *src, char *dest, int len)
{
	int rc = 0;
	int i = 0;
	for( i=0; i<len; i++ ) {
		if(src[i] != dest[i]) {
			rc = 1;
			break;
		}
	}

	return rc;
}

static ssize_t dock_attr_show(struct device_driver *driver, char *buf)
{
	if (p_state == DOCK_PLUGOUT) {
        return sprintf(buf, "%s", "plugout\n");
	} else if (p_state == DOCK_PLUGIN) {
		return sprintf(buf, "%s", "plugin\n");
    } else {
    	return -1;
    }
}

static DRIVER_ATTR(state, S_IRUGO, dock_attr_show, NULL);

static irqreturn_t dock_det_interrupt(int irq, void *dev_id)
{
		p_state = gpio_get_value(data->det_gpio);
   		return IRQ_HANDLED;
		
}

static int dock_detect_probe(struct platform_device *pdev)
{
	int rc; 
	struct dock_det_platform_data *pdata;

    data = kzalloc(sizeof(struct dock_detect_data), GFP_KERNEL);
    if (!data) {
    	return -ENOMEM;
    }

    pdata = pdev->dev.platform_data;
	data->det_gpio = pdata->det_gpio;
    data->irq = pdata->det_irq_gpio;
        
	rc = request_irq(data->irq,
		       dock_det_interrupt,
		       IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
		       "dock_detect",
		       NULL);  

	if (rc < 0) {       
		printk(KERN_ERR "request DOCK_DET_IRQ error. rc=%d\n", rc);      
        kfree(data);
        return rc;
	} 

	p_state = gpio_get_value(data->det_gpio);

    return 0;
}

static int dock_detect_remove(struct platform_device *pdev)
{
    kfree(data);
    return 0;
}


static struct platform_driver dock_detect_driver =
{
    .probe     = dock_detect_probe,       
    .remove    = dock_detect_remove,
    .driver    = {
        .name  = "dock_detect",
        .owner = THIS_MODULE,
    },
};


static int __init dock_det_init(void)
{
	int rc = 0;	
    int err = 0;
	
	err = get_hardware_version_info();
	if (err) {
		printk(KERN_ERR "%s : get hardware version err = %d\n",__func__, err);
	}

	if((0 != memcmp__buffer(EVDO_ptr, g_hardware_version_num, 15))
		&& (0 != memcmp__buffer(WCDMA_1_ptr, g_hardware_version_num, 15))
		&& (0 != memcmp__buffer(WCDMA_2_ptr, g_hardware_version_num, 15)) 
		&& (0 != memcmp__buffer(WCDMA_3_ptr, g_hardware_version_num, 15)) 
		&& (0 != memcmp__buffer(WCDMA_4_ptr, g_hardware_version_num, 15))) {

		rc = platform_driver_register(&dock_detect_driver); 
	    if (rc < 0) {
	        printk(KERN_ERR "register dock detect driver error: %d\n", rc);
	    }	 

		if ((rc = driver_create_file(&dock_detect_driver.driver, &driver_attr_state))){
				printk("failed to create sysfs entry(state): %d\n", rc);
		}	    
	    
	}
	
	
    return rc;
}

static void __exit dock_det_exit(void)
{
    return platform_driver_unregister(&dock_detect_driver);

}

module_init(dock_det_init);
module_exit(dock_det_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Dock Detect Driver");
