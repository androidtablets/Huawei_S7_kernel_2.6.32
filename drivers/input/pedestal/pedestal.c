/* Driver for Pedestal device 

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
 *  Pedestal driver.
 *
 */


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>

#include <asm/irq.h>

#include <mach/hardware.h>
#include <mach/io.h>
#include <mach/system.h>
#include <mach/gpio.h>

#include <mach/pedestal.h>
#include "irda_rmtctrl.h"
#include "sound_box.h"

struct pedestal_gpio_state g_gpio_irq_state;
struct pedestal_driver_data* g_pdriverdata = NULL;

irqreturn_t pedestal_dect_interrupt(int irq, void *dev_id);

static int pedestal_unknow_init(void *dev_id)
{
	return 0;
}

static  void pedestal_unknow_exit(void *dev_id)
{
}

static irqreturn_t pedestal_unknow_interrupt(int irq, void *dev_id)
{
	return 0;
}


#ifdef CONFIG_PM
int pedestal_unknow_suspend(void *dev_id, pm_message_t state)
{
	return 0;
}

int pedestal_unknow_resume(void *dev_id)
{
	return 0;
}
#endif

static struct pedestal_func pedestal_driver_func[] = {
	{
		.driver_init 		= pedestal_interface_init,
		.driver_exit 		= pedestal_interface_exit,
		.driver_interrupt 	= pedestal_interface_interrupt,
	#ifdef CONFIG_PM
		.driver_suspend 	= pedestal_interface_suspend,
		.driver_resume 		= pedestal_interface_resume,
	#endif
	},

	{
		.driver_init 		= pedestal_soundbox_init,
		.driver_exit 		= pedestal_soundbox_exit,
		.driver_interrupt 	= pedestal_soundbox_interrupt,
	#ifdef CONFIG_PM
		.driver_suspend 	= pedestal_soundbox_suspend,
		.driver_resume 		= pedestal_soundbox_resume,
	#endif
	},

	{
		.driver_init 		= pedestal_unknow_init,
		.driver_exit 		= pedestal_unknow_exit,
		.driver_interrupt 	= pedestal_unknow_interrupt,
	#ifdef CONFIG_PM
		.driver_suspend 	= pedestal_unknow_suspend,
		.driver_resume 		= pedestal_unknow_resume,
	#endif
	},	
};


int pedestal_i2c_write_byte_data(struct i2c_client *client, unsigned char command, unsigned char value)
{
	int rc = 0;
	int icount = 0;
	
	while(((rc = i2c_smbus_write_byte_data(client,command,value)) < 0) && (icount < 3))
	{
		icount--;
		mdelay(5);
	}

	if(rc < 0)
		PEDESTAL_ERR("pedestal_i2c_write_byte_data write reg:0x%02x  failed return err code = %d, retry count = %d!\n",command,rc,icount);

	return rc;
}

int pedestal_i2c_read_byte_data(struct i2c_client *client, unsigned char command)
{
	int rc = 0;
	int icount = 0;
	while(((rc = i2c_smbus_read_byte_data(client,command)) < 0) && (icount < 3))
	{
		icount++;
		mdelay(5);
	}

	if(rc < 0)
		PEDESTAL_ERR("pedestal_i2c_read_byte_data read reg:0x%02x  failed return err code = %d, retry count = %d!\n",command,rc,icount);

	return rc;
}
	
static struct msm_gpio pedestal_gpio_cfg[] = {
	#if(IRDA_GPIO_INVERTED)
	{ GPIO_CFG(PEDESTAL_INT_GPIO, 0, GPIO_INPUT, GPIO_PULL_DOWN, GPIO_2MA),"pedestal_irda_irq" },  //interface pedestal irda remote control gpio
	#else
	{ GPIO_CFG(PEDESTAL_INT_GPIO, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA),"pedestal_irda_irq" },
	#endif

	{ GPIO_CFG(PEDESTAL_INT_GPIO, 0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA),"pedestal_i2c_irq" },   //sound box pedestal i2c irq gpio
};

int pedestal_gpio_setup(unsigned int pedestal_type)
{
	int32_t rc = -ENODEV;

	if(pedestal_type >= PEDESTAL_TYPE_UNDEF)
		return rc;

	spin_lock_irq(&g_pdriverdata->pedestal_lock); 
	if(g_gpio_irq_state.pedestal_gpio_req_state[pedestal_type] > 0)
	{
		PEDESTAL_ERR("pedestal_gpio_setup: request gpio again!\n");
	}
	else
	{
		rc = msm_gpios_request_enable(&pedestal_gpio_cfg[pedestal_type],1);
		if(!rc)
			g_gpio_irq_state.pedestal_gpio_req_state[pedestal_type]++;
	}
	spin_unlock_irq(&g_pdriverdata->pedestal_lock);
	return rc;
}

void pedestal_gpio_release(unsigned int pedestal_type)
{
	if(pedestal_type < PEDESTAL_TYPE_UNDEF)
	{
		spin_lock_irq(&g_pdriverdata->pedestal_lock); 
		if(g_gpio_irq_state.pedestal_gpio_req_state[pedestal_type] > 0)
		{
			msm_gpios_disable_free(&pedestal_gpio_cfg[pedestal_type],1);
			g_gpio_irq_state.pedestal_gpio_req_state[pedestal_type]--;
		}
		else
			PEDESTAL_ERR("pedestal_gpio_setup: request gpio again!\n");
		spin_unlock_irq(&g_pdriverdata->pedestal_lock);
	}
}

int pedestal_dect_irq_request(unsigned int irq, irq_handler_t handler,
		unsigned long irqflags, const char *devname, void *dev_id)
{
	int rc = 1;
	struct pedestal_driver_data *dd = dev_id;
	spin_lock_irq(&dd->pedestal_lock); 
	if(g_gpio_irq_state.irq_dect_req_state > 0)
	{
		rc = 1;
		PEDESTAL_ERR("pedestal_dect_irq_request: request dect irq again!\n");
		goto dect_irq_request_exit;
	}
	
	if(can_request_irq(irq,irqflags))
	{
		rc = request_irq(irq,handler,irqflags,devname,dev_id);
		if (rc) {
			PEDESTAL_ERR("pedestal_dect_irq_request: request dect irq failed, err code = %d\n",rc);
			goto dect_irq_request_exit;
		}
		else
		{
			g_gpio_irq_state.irq_dect_req_state++;
		}
	}
	else
	{
		PEDESTAL_ERR("pedestal_dect_irq_request: can_request_irq failed !\n");
		rc = 2;
	}
	
dect_irq_request_exit:
	spin_unlock_irq(&dd->pedestal_lock);
	return rc;
}

void pedestal_dect_irq_free(unsigned int irq, void *dev_id)
{
	struct pedestal_driver_data *dd = dev_id;
	spin_lock_irq(&dd->pedestal_lock); 
	if(g_gpio_irq_state.irq_dect_req_state > 0)
	{
		g_gpio_irq_state.irq_dect_req_state--;
		free_irq(irq, dev_id);
		PEDESTAL_INFO("pedestal_dect_irq_free: free detect irq succeed !\n");
	}
	else
	{
		PEDESTAL_INFO("pedestal_dect_irq_free: free detect irq again !\n");
	}
	spin_unlock_irq(&dd->pedestal_lock);
}

static int pedestal_dev_open(struct input_dev *dev)
{
	struct pedestal_driver_data *dd = input_get_drvdata(dev);
	int rc;

	if (!dd->pedestal_client) {
		PEDESTAL_ERR("pedestal_dev_open: no i2c adapter present\n");
		rc = -ENODEV;
		goto fail_on_open;
	}
	
	//request gpio irq
	rc = pedestal_dect_irq_request(dd->pedestal_dect_irq,
		       &pedestal_dect_interrupt,
		       IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		       PEDESTAL_DECT_NAME,
		       dd);
	if (rc) {
			PEDESTAL_ERR("pedestal_dev_open: request irq failed, err code = %d\n",rc);
			goto fail_on_open;
	}

	
	return 0;
fail_on_open:
	return rc;
}

static void pedestal_dev_close(struct input_dev *dev)
{
	struct pedestal_driver_data *dd = input_get_drvdata(dev);

	pedestal_dect_irq_free(dd->pedestal_dect_irq, dd);
}

#define PEDESTAL_I2C_RETRAY_MAX   5
static void pedestal_plugin_work_f(struct work_struct *work)
{
	int rc = 0;
	int device_id = 0;
	int retry = PEDESTAL_I2C_RETRAY_MAX;
	unsigned int gpio_state = 0;
	unsigned int icount = 0;

	while(!(gpio_state =gpio_get_value(PEDESTAL_DECT_GPIO)))
	{
		icount++;
		mdelay(10);
		if(icount > 50)
			break;
	}
	if(gpio_state)
		return;
	
	while (retry-- > 0) 
	{			
		device_id = pedestal_i2c_read_byte_data(g_pdriverdata->pedestal_client, PEDESTAL_PRODUCT_ID_REG);		
		if (device_id >= 0)		
		{
		//	break;		
		}
		msleep(10);
	}
	
	
	if(device_id >= 0)//sound box pedestal
	{
		memcpy(&g_pdriverdata->pedestal_driver_func,&pedestal_driver_func[PEDESTAL_TYPE_SOUNDBOX],sizeof(struct pedestal_func));
	}
	else // interface pedestal
	{
		memcpy(&g_pdriverdata->pedestal_driver_func,&pedestal_driver_func[PEDESTAL_TYPE_INTERFACE],sizeof(struct pedestal_func));
	}
	
	rc = g_pdriverdata->pedestal_driver_func.driver_init((void *)g_pdriverdata);
	if(rc)
	{
		PEDESTAL_ERR("pedestal: driver init failed err code = %d.\n",rc);
		g_pdriverdata->pedestal_driver_func.driver_exit((void *)g_pdriverdata);
		return;
	}
}

static void pedestal_plugout_work_f(struct work_struct *work)
{
	g_pdriverdata->pedestal_driver_func.driver_exit((void *)g_pdriverdata);
//	enable_irq(g_pdriverdata->pedestal_dect_irq);
}

irqreturn_t pedestal_dect_interrupt(int irq, void *dev_id)
{
	irqreturn_t rc = 0;
	struct pedestal_driver_data *dd = dev_id;
	u32 gpio_state = 0;

	disable_irq(dd->pedestal_dect_irq);
	gpio_state = gpio_get_value(PEDESTAL_DECT_GPIO);

	if(gpio_state == 0) //plug in
	{
		schedule_work(&dd->pedestal_plugin_work);
	}
	else //plug out
	{
		//remove pedestal
		schedule_work(&dd->pedestal_plugout_work);
	}
	enable_irq(dd->pedestal_dect_irq);
	return rc;
}

static int32_t __devinit pedestal_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int32_t rc = -ENODEV;
	struct pedestal_platform_data  *pd;
	struct pedestal_driver_data *dd;
	//register dect and common part


	memset(&g_gpio_irq_state,0,sizeof(struct pedestal_gpio_state));
	dd = kzalloc(sizeof(struct pedestal_driver_data), GFP_KERNEL);
	if (!dd) {
		rc = -ENOMEM;
		PEDESTAL_ERR("pedestal: can't malloc pedestal_driver_data mem.\n");
		goto probe_exit;
	}

	if (!i2c_check_functionality(client->adapter,
				(I2C_FUNC_SMBUS_BYTE_DATA|I2C_FUNC_SMBUS_I2C_BLOCK))) {
		PEDESTAL_ERR("pedestal: does not support SMBUS_BYTE_DATA.\n");
		rc = -EINVAL;
		goto probe_i2c_driver_err;
	}

	dd->pedestal_client = client;
	pd = client->dev.platform_data;	
	if (!pd) {
		PEDESTAL_ERR("pedestal: platform data not set\n");
		rc = -EFAULT;
		goto probe_i2c_driver_err;
	}

	if (pd->gpio_setup == NULL)
	{
		PEDESTAL_ERR("pedestal: platform data gpio_setup is null.\n");
		rc = -EINVAL;
		goto probe_i2c_driver_err;
	}
	rc = pd->gpio_setup();
	if (rc)
	{
		PEDESTAL_ERR("pedestal: gpio_setup require dect gpio failed.\n");
		goto probe_i2c_driver_err;
	}
	
	dd->pedestal_dev = input_allocate_device();
	if (!dd->pedestal_dev) {
		rc = -ENOMEM;
		PEDESTAL_ERR("pedestal: input_allocate_device failed.\n");
		goto probe_i2c_driver_err;
	}

	memcpy(&dd->pedestal_driver_func,&pedestal_driver_func[PEDESTAL_TYPE_UNDEF],sizeof(struct pedestal_func));
	g_pdriverdata = dd;
	
	input_set_drvdata(dd->pedestal_dev, dd);
	dd->pedestal_dev->open       	= pedestal_dev_open;
	dd->pedestal_dev->close      	= pedestal_dev_close;
	dd->pedestal_dev->name       	= PEDESTAL_DRIVER_NAME;
	dd->pedestal_dev->phys       	= PEDESTAL_DEVICE;
	dd->pedestal_dev->id.bustype 	= BUS_I2C;
	dd->pedestal_dev->id.product 	= 1;
	dd->pedestal_dev->id.version 	= 1;
	
	__set_bit(EV_KEY, dd->pedestal_dev->evbit);
	__set_bit(BTN_MOUSE, dd->pedestal_dev->evbit);
	
	rc = input_register_device(dd->pedestal_dev);
	if (rc) {
		PEDESTAL_ERR("pedestal: input_register_device failed, err code =%d\n",rc);
		goto probe_inputdev_err;
	}
	
	spin_lock_init(&dd->pedestal_lock);
	

	dd->pedestal_dect_irq = gpio_to_irq(pd->dect_gpio);
	dd->pedestal_gpio_irq = gpio_to_irq(pd->interrupt_gpio);

	INIT_WORK(&dd->pedestal_plugin_work, pedestal_plugin_work_f);
	INIT_WORK(&dd->pedestal_plugout_work, pedestal_plugout_work_f);

	return 0;
	
probe_inputdev_err:
	if (pd->gpio_release != NULL)
		pd->gpio_release();
	input_free_device(dd->pedestal_dev);
probe_i2c_driver_err:
	kfree(dd);
probe_exit:
	PEDESTAL_ERR("pedestal: pedestal_probe failed, err code =%d\n",rc);
	return rc;
}

static int32_t pedestal_remove(struct i2c_client *client)
{
	int32_t rc = -ENODEV;
	struct pedestal_platform_data  *pd = NULL;
	struct pedestal_driver_data *dd = NULL;

	dd = i2c_get_clientdata(client);
	pd = client->dev.platform_data;

	if(!dd || !pd)
		return rc;

	cancel_work_sync(&dd->pedestal_plugin_work);
	cancel_work_sync(&dd->pedestal_plugout_work);
	
	memset(&g_gpio_irq_state,0,sizeof(struct pedestal_gpio_state));
	g_pdriverdata = NULL;

	dd->pedestal_driver_func.driver_exit((void *)dd);

	pedestal_dect_irq_free(dd->pedestal_dect_irq, dd);
	
	if (pd->gpio_release != NULL)
		pd->gpio_release();
	
	input_unregister_device(dd->pedestal_dev);
	
	kfree(dd);
	
	i2c_set_clientdata(client, NULL);
	
	return rc;
}

#if 1
//#ifdef CONFIG_PM
static int32_t pedestal_suspend(struct i2c_client *client,
			    pm_message_t state)
{
#if 1
    int32_t rc = -ENODEV;
	struct pedestal_platform_data  *pd = NULL;
	struct pedestal_driver_data *dd = NULL;


	dd = i2c_get_clientdata(client);
	pd = client->dev.platform_data;

	if(!dd || !pd)
		return rc;

	disable_irq(dd->pedestal_dect_irq);
	cancel_work_sync(&dd->pedestal_plugin_work);
	cancel_work_sync(&dd->pedestal_plugout_work);
	
	rc = dd->pedestal_driver_func.driver_suspend((void *)dd,state);
	return rc;
#else
	return 0;
#endif
}

static int32_t pedestal_resume(struct i2c_client *client)
{
#if 1
	int32_t rc = -ENODEV;
	struct pedestal_platform_data  *pd = NULL;
	struct pedestal_driver_data *dd = NULL;


	enable_irq(dd->pedestal_dect_irq);
	
	dd = i2c_get_clientdata(client);
	pd = client->dev.platform_data;

	if(!dd || !pd)
		return rc;
	
	rc = dd->pedestal_driver_func.driver_resume((void *)dd);
	return rc;
#else
	return 0;
#endif
}
#endif


static const struct i2c_device_id pedestal_id = { PEDESTAL_DEVICE_NAME, 0 };
MODULE_DEVICE_TABLE(i2c, &pedestal_id);

static struct i2c_driver pedestal_i2c_driver = {
	.driver 	= {
		.name   = PEDESTAL_DRIVER_NAME,
		.owner  = THIS_MODULE,
	},
	.probe   	= pedestal_probe,
	.remove  	=  __exit_p(pedestal_remove),
#if 1
//#ifdef CONFIG_PM
	.suspend = pedestal_suspend,
	.resume  	= pedestal_resume,
#endif
	.id_table 	= &pedestal_id,
};

static int __init pedestal_init(void)
{
	int rc;

	rc = i2c_add_driver(&pedestal_i2c_driver);
	if (rc)
	{
		PEDESTAL_ERR("pedestal_init FAILED: i2c_add_driver rc=%d\n",rc);
		return rc;
	}
	
	PEDESTAL_INFO("pedestal_init succeed: i2c_add_driver rc=%d\n",rc);

	return rc;
}

static void __exit pedestal_exit(void)
{
	i2c_del_driver(&pedestal_i2c_driver);
}



int pedestal_irq_request(unsigned int pedestal_type, unsigned int irq, irq_handler_t handler,
		unsigned long irqflags, const char *devname, void *dev_id)
{
	int rc = 1;
//	struct pedestal_driver_data *dd = dev_id;

	if(pedestal_type >= PEDESTAL_TYPE_UNDEF)
		return -EINVAL;
	
//	spin_lock_irq(&dd->pedestal_lock); 
	if(g_gpio_irq_state.pedestal_irq_req_state[pedestal_type] > 0)
	{
		rc = 1;
		PEDESTAL_ERR("pedestal_interface_irq_request: request dect irq again!\n");
		goto dect_irq_request_exit;
	}

	if(can_request_irq(irq,irqflags))
	{
		rc = request_irq(irq,handler,irqflags,devname,dev_id);
		if (rc) {
			PEDESTAL_ERR("pedestal_interface_irq_request: request dect irq failed, err code = %d\n",rc);
			goto dect_irq_request_exit;
		}
		else
		{
			g_gpio_irq_state.pedestal_irq_req_state[pedestal_type]++;
		}
	}
	else
	{
		PEDESTAL_ERR("pedestal_interface_irq_request: can_request_irq failed !\n");
		rc = 2;
	}
	
dect_irq_request_exit:
//	spin_unlock_irq(&dd->pedestal_lock);
	return rc;
}

void pedestal_irq_free(unsigned int pedestal_type, unsigned int irq, void *dev_id)
{
//	struct pedestal_driver_data *dd = dev_id;

	if(pedestal_type >= PEDESTAL_TYPE_UNDEF)
		return;
	
//	spin_lock_irq(&dd->pedestal_lock); 
	PEDESTAL_INFO("pedestal_dect_irq_free: count = %d\n",g_gpio_irq_state.pedestal_irq_req_state[pedestal_type]);
	if(g_gpio_irq_state.pedestal_irq_req_state[pedestal_type] > 0)
	{
		g_gpio_irq_state.pedestal_irq_req_state[pedestal_type]--;
		PEDESTAL_INFO("pedestal_dect_irq_free: before  free_irq!\n");
		free_irq(irq, dev_id);
		PEDESTAL_INFO("pedestal_dect_irq_free: free detect irq succeed !\n");
	}
	else
	{
		PEDESTAL_INFO("pedestal_dect_irq_free: free detect irq again !\n");
	}
//	spin_unlock_irq(&dd->pedestal_lock);
}

module_init(pedestal_init);
module_exit(pedestal_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.1");
MODULE_AUTHOR("HUAWEI Tech. Co., Ltd . wufan");
MODULE_DESCRIPTION("Pedestal driver");
MODULE_ALIAS("platform:pedestal-hid");

