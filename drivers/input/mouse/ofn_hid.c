/* Driver for OFN device 

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
 *  FO1W-I2C Optical finger navigation driver.
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
#include <mach/ofn_hid.h>

#define DRIVER_NAME                    "ofn_hid"
#define DEVICE_NAME                    "fo1w"
#define OPTNAV_NAME                    "HID OFN"
#define OPTNAV_NAME_BUTTON_CENTER           "OFN button center"
#define OPTNAV_DEVICE                  "/dev/ofn"
#define OPTNAV_STEP				  0x02
#define OPTNAV_FILTER			  0x02
#define OPTNAV_REPORT			  0x0

struct optnav_data {
	struct input_dev  *optnav_dev;
	struct i2c_client *clientp;
	u32                irq_button_center;
	u32                gpio_button_center;
	bool               rotate_xy;
	struct work_struct optnav_work;
};

static void optnav_work_f(struct work_struct *work);

static int optnav_i2c_write(struct i2c_client *client, u8 reg, u8 data)
{
	int rc;
    int ret;
	
	ret = i2c_smbus_write_byte_data(client, reg, data);
  

	rc = i2c_smbus_read_byte_data(client, reg);

	if (rc < 0) {
		dev_err(&client->dev,
			"optnav_i2c_write FAILED reading reg 0x%x\n",
			reg);
		return rc;
	}

	if ((u8)rc != data) {
		dev_err(&client->dev,
			"optnav_i2c_write FAILED writing 0x%x to reg 0x%x\n",
			rc,reg);
		rc = -EIO;
		return rc;
	} else {

    }
	return 0;
}

static int optnav_i2c_read(struct i2c_client *client, u8 reg)
{
	int rc;	

	rc = i2c_smbus_read_byte_data(client, reg);

	return rc;
}

static void optnav_work_f(struct work_struct *work)
{

	struct optnav_data *dd = container_of(work, struct optnav_data, optnav_work);
	int m,x_val, y_val;
	int rel_x, rel_y;	
	int optnav_filter = 0 ;
	
	m = optnav_i2c_read(dd->clientp, 0x02); 
	
	if( m & 0x80 ) {
		x_val = optnav_i2c_read(dd->clientp, 0x03); 
		y_val = optnav_i2c_read(dd->clientp, 0x04);
		if( (( x_val & 0xEF ) > ( y_val & 0xEF )) && (x_val & 0x80) ) 
		{
			optnav_filter=0x0a;
		}
		else if( (( x_val & 0xEF ) < ( y_val & 0xEF )) &&  !(y_val &0x80 )) 
		{
		
			optnav_filter=0x03;
		}
		else if( (( x_val & 0xEF ) < ( y_val & 0xEF )) &&  (y_val &0x80 )) 
		{
			
            optnav_filter=0x01;
		}
		else   
		{
			
			optnav_filter=0x02;
		}		
		if((( x_val & 0xEF ) + ( y_val & 0xEF )) < optnav_filter ){
            
			goto report_end;
		}

		if (( x_val & 0xEF ) > ( y_val & 0xEF ) ){
				
			if ( x_val & 0x80 )
				{
					rel_x = OPTNAV_STEP;
				}
			else
				{
			        	rel_x = - OPTNAV_STEP;
				}

			rel_y = OPTNAV_REPORT;

			}//END if (x_val&0xEF) > (y_val&0xEF)
		else{
			
			if ( y_val & 0x80 )
				{
					rel_y = - OPTNAV_STEP;	
				}
			else
				{
					rel_y =  OPTNAV_STEP;
				}

			rel_x = OPTNAV_REPORT;
			
			}
            
			input_report_rel(dd->optnav_dev, REL_X, rel_x);
			input_report_rel(dd->optnav_dev, REL_Y, rel_y);
			i2c_smbus_write_byte_data(dd->clientp, 0x02, 0xff);
			input_sync(dd->optnav_dev); 
		}
	
	report_end:
    enable_irq(dd->clientp->irq);
    
}

static irqreturn_t optnav_but_interrupt(int irq, void *dev_id)
{

 
	
	struct optnav_data *dd = dev_id;


	u32 state;

	state = gpio_get_value(dd->gpio_button_center );
    
	input_report_key(dd->optnav_dev, BTN_MOUSE, !state);
	input_sync(dd->optnav_dev);
    
	return IRQ_HANDLED;
}

static irqreturn_t optnav_interrupt(int irq, void *dev_id)
{

	struct optnav_data *dd = dev_id;
	
	//modified by z00168965
	//disable_irq(dd->clientp->irq);   
	disable_irq_nosync(dd->clientp->irq);
  	
    //end by z00168965
	schedule_work(&dd->optnav_work);
	return IRQ_HANDLED;
}

static int optnav_dev_open(struct input_dev *dev)
{

	struct optnav_data *dd = input_get_drvdata(dev);
	int rc;
	
	if (!dd->clientp) {
		printk(KERN_ERR "optnav_dev_open: no i2c adapter present\n");
		rc = -ENODEV;
		goto fail_on_open;
	}

   
	rc = request_irq(dd->irq_button_center,
		       &optnav_but_interrupt,
		       IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
		       OPTNAV_NAME_BUTTON_CENTER,
		       dd);  //irq = 98

	if (rc) {
		dev_err(&dd->clientp->dev,
		       "optnav_dev_open FAILED: request_irq rc=%d\n", rc);
		goto fail_on_open;
	} 

	rc = request_irq(dd->clientp->irq,
		       &optnav_interrupt,
		       IRQF_TRIGGER_FALLING,
		       OPTNAV_NAME,
		       dd);
	if (rc) {
		dev_err(&dd->clientp->dev,
		       "optnav_dev_open FAILED: request_irq rc=%d\n", rc);
		goto fail_on_open;
	}
    
	
	return 0;

fail_on_open:
	return rc;
}

static void optnav_dev_close(struct input_dev *dev)
{
	struct optnav_data *dd = input_get_drvdata(dev);

    free_irq(dd->irq_button_center, dd);
	free_irq(dd->clientp->irq, dd);
	cancel_work_sync(&dd->optnav_work);
}

static int __exit optnav_remove(struct i2c_client *client)
{
	struct optnav_data *dd;
	struct ofn_hid_platform_data *pd;

	dd = i2c_get_clientdata(client);
	pd = client->dev.platform_data;
	input_unregister_device(dd->optnav_dev);
	if (pd->gpio_release != NULL)
		pd->gpio_release();
	kfree(dd);
	i2c_set_clientdata(client, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int optnav_suspend(struct i2c_client *client,
			    pm_message_t state)
{
	struct ofn_hid_platform_data *pd;
	pd = client->dev.platform_data;

	gpio_direction_output(pd->shutdown, 1);
	return 0;
}

static int optnav_resume(struct i2c_client *client)
{
	struct ofn_hid_platform_data *pd;
	pd = client->dev.platform_data;

	gpio_direction_output(pd->shutdown, 0);
	return 0;
}
#endif

static const struct i2c_device_id optnav_id[] = {
	{ DEVICE_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, optnav_id);

static void optnav_gpio_reset(struct i2c_client *client)
{
	struct ofn_hid_platform_data  *pd;
	pd = client->dev.platform_data;  

	if (gpio_request(pd->reset, " OFN RESET Enable")) 
   		printk("OFN RESET Enable \n");

	gpio_tlmm_config(GPIO_CFG(pd->reset, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA), GPIO_ENABLE);

    gpio_direction_output(pd->reset, 1);
    mdelay(300);
	gpio_direction_output(pd->reset, 0);
	mdelay(5);
	gpio_direction_output(pd->reset, 1);
	mdelay(10);

	gpio_free(pd->reset);
}

static int optnav_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{

	struct optnav_data *dd;
	int rc = 0;
	struct ofn_hid_platform_data  *pd;

	pd = client->dev.platform_data;  
	dd = kzalloc(sizeof *dd, GFP_KERNEL);
	if (!dd) {
        rc = -ENOMEM;
		goto probe_exit;
	}
	
	if (!i2c_check_functionality(client->adapter,
				(I2C_FUNC_SMBUS_BYTE_DATA))) {
		printk(KERN_ERR "OFN does not support SMBUS_BYTE_DATA.\n");

		return -EINVAL;
	}

	optnav_gpio_reset(client);
	
	optnav_i2c_write(client, 0x3a, 0x5A);
	optnav_i2c_write(client, 0x60, 0xf4);
	optnav_i2c_write(client, 0x62, 0x12);
	optnav_i2c_write(client, 0x63, 0x0e);
	optnav_i2c_write(client, 0x64, 0x08);
	optnav_i2c_write(client, 0x65, 0x06);
	optnav_i2c_write(client, 0x66, 0x40);
	optnav_i2c_write(client, 0x67, 0x08);
	optnav_i2c_write(client, 0x68, 0x48);
	optnav_i2c_write(client, 0x69, 0x0A);
	optnav_i2c_write(client, 0x6A, 0x50);
	optnav_i2c_write(client, 0x6B, 0x48);
	optnav_i2c_write(client, 0x6D, 0xC4);
	optnav_i2c_write(client, 0x6E, 0x34);
	optnav_i2c_write(client, 0x6F, 0x3C);
	optnav_i2c_write(client, 0x70, 0x18);
	optnav_i2c_write(client, 0x71, 0x20);   
    optnav_i2c_write(client, 0x73, 0x99);
    optnav_i2c_write(client, 0x74, 0x02);    
	optnav_i2c_write(client, 0x75, 0x50);
    		
	dd->clientp = client;
	if (!pd) {
		dev_err(&client->dev,
			"optnav_probe: platform data not set\n");
		rc = -EFAULT;
		goto probe_free_exit;
	}

	dd->irq_button_center 		= pd->irq_button_center;
	dd->gpio_button_center 	= pd->gpio_button_center;
	dd->rotate_xy 			= pd->rotate_xy;

	if (pd->gpio_setup == NULL)
		goto probe_free_exit;
	rc = pd->gpio_setup();
	if (rc)
		goto probe_free_exit;

	dd->optnav_dev = input_allocate_device();
	if (!dd->optnav_dev) {
		rc = -ENOMEM;
		goto probe_fail_in_alloc;
	}

	input_set_drvdata(dd->optnav_dev, dd);
	dd->optnav_dev->open       	= optnav_dev_open;
	dd->optnav_dev->close      	= optnav_dev_close;
	dd->optnav_dev->name       	= OPTNAV_NAME;
	dd->optnav_dev->phys       	= OPTNAV_DEVICE;
	dd->optnav_dev->id.bustype 	= BUS_I2C;
	dd->optnav_dev->id.product 	= 1;
	dd->optnav_dev->id.version 	= 1;
	__set_bit(EV_REL, dd->optnav_dev->evbit);
	__set_bit(REL_X, dd->optnav_dev->relbit);
	__set_bit(REL_Y, dd->optnav_dev->relbit);
	__set_bit(EV_KEY, dd->optnav_dev->evbit);
	__set_bit(BTN_MOUSE, dd->optnav_dev->keybit);

	rc = input_register_device(dd->optnav_dev);
	if (rc) {
		dev_err(&client->dev,
			"optnav_probe: input_register_device rc=%d\n",
		       rc);
		goto probe_fail_reg_dev;
	}
	
	INIT_WORK(&dd->optnav_work, optnav_work_f);
	
	return 0;

probe_fail_reg_dev:
	input_free_device(dd->optnav_dev);
probe_fail_in_alloc:
	if (pd->gpio_release != NULL)
		pd->gpio_release();
probe_free_exit:
	kfree(dd);
	i2c_set_clientdata(client, NULL);
probe_exit:
	return rc;
}

static struct i2c_driver optnav_driver = {
	.driver 	= {
		.name   = DRIVER_NAME,
		.owner  = THIS_MODULE,
	},
	.probe   	= optnav_probe,
	.remove  	=  __exit_p(optnav_remove),
#ifdef CONFIG_PM
	.suspend = optnav_suspend,
	.resume  	= optnav_resume,
#endif
	.id_table 	= optnav_id,
};

static int __init optnav_init(void)
{
	int rc;

	rc = i2c_add_driver(&optnav_driver);
	if (rc)
		printk(KERN_ERR "optnav_init FAILED: i2c_add_driver rc=%d\n",
		       rc);
	return rc;
}

static void __exit optnav_exit(void)
{
	i2c_del_driver(&optnav_driver);
}

module_init(optnav_init);
module_exit(optnav_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.1");
MODULE_AUTHOR("HUAWEI Tech. Co., Ltd . LiuLihua");
MODULE_DESCRIPTION(" Optical finger navigation driver");
MODULE_ALIAS("platform:ofn-hid");
