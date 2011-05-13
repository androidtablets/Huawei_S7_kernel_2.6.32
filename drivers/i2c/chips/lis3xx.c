/* Copyright (c) 2009, Huawei Technologies Ltd., Corp. All rights reserved.
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

#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/moduleparam.h>

#include <mach/msm_gsensor.h>

#define lis_dbg(format, arg...)		\
	printk(KERN_INFO "[%s,%d] "format"\n" , __FUNCTION__, __LINE__, ##arg)

/* LIS3XX registers */
#ifdef CONFIG_LIS302
#define LIS3XX_WHOAMI			0x0F
#endif

#define LIS3XX_CTRL_REG1		0x20
#define LIS3XX_CTRL_REG2		0x21
#define LIS3XX_CTRL_REG3		0x22
#define LIS3XX_HP_FILTER_RESET	0x23
#define LIS3XX_STATUS_REG		0x27
#define LIS3XX_OUTX				0x29
#define LIS3XX_OUTY				0x2B
#define LIS3XX_OUTZ				0x2D
#define LIS3XX_FF_WU_CFG_1		0x30
#define LIS3XX_FF_WU_SRC_1		0x31
#define LIS3XX_FF_WU_THS_1		0x32
#define LIS3XX_FF_WU_DURATION_1	0x33

#if defined(CONFIG_LIS302) || defined(CONFIG_LIS35)
#define LIS3XX_FF_WU_CFG_2		0x34
#define LIS3XX_FF_WU_SRC_2		0x35
#define LIS3XX_FF_WU_THS_2		0x36
#define LIS3XX_FF_WU_DURATION_2	0x37
#define LIS3XX_CLICK_CFG		0x38
#define LIS3XX_CLICK_SRC		0x39
#define LIS3XX_CLICK_THSY_X		0x3B
#define LIS3XX_CLICK_THSZ		0x3C
#define LIS3XX_TIMELIMIT		0x3D
#define LIS3XX_LATENCY			0x3E
#define LIS3XX_WINDOW			0x3F
#endif

struct register_attr {
	unsigned char regaddr;
	unsigned char regtype;
};

/* register bit */
#define LCR1_DR     	0x80
#define LCR1_PD     	0x40
#define LCR1_FS     	0x20
#define LCR1_ZEN    	0x04
#define LCR1_YEN    	0x02
#define LCR1_XEN    	0x01

#define LCR2_SIM        0x80
#define LCR2_BOOT       0x40
#define LCR2_FDS        0x01
#define LCR2_HP_FF_WU2  0x08
#define LCR2_HP_FF_WU1  0x04
#define LCR2_HP_COEFF2  0x02
#define LCR2_HP_COEFF1  0x01

#define LCR3_IHL        0x80
#define LCR3_PP_OD      0x40
#define LCR3_I2CFG2     0x20
#define LCR3_I2CFG1     0x10
#define LCR3_I2CFG0     0x08
#define LCR3_I1CFG2     0x04
#define LCR3_I1CFG1     0x02
#define LCR3_I1CFG0     0x01

#define LSR_ZXYOR       0x80
#define LSR_ZOR         0x40
#define LSR_YOR         0x20
#define LSR_XOR         0x10
#define LSR_ZXYDA       0x08
#define LSR_ZDA         0x04
#define LSR_YDA         0x02
#define LSR_XDA         0x01

#define LFFWUCFG_AOI   0x80
#define LFFWUCFG_LIR   0x40
#define LFFWUCFG_ZHIE  0x20
#define LFFWUCFG_ZLIE  0x10
#define LFFWUCFG_YHIE  0x08
#define LFFWUCFG_YLIE  0x04
#define LFFWUCFG_XHIE  0x02
#define LFFWUCFG_XLIE  0x01

#define LFFWUSRC_IA    0x40
#define LFFWUSRC_ZH    0x20
#define LFFWUSRC_ZL    0x10
#define LFFWUSRC_YH    0x08
#define LFFWUSRC_YL    0x04
#define LFFWUSRC_XH    0x02
#define LFFWUSRC_XL    0x01

#define LFFWUTHS_DCRM	0x80

#define LCLCKS_IA 		0x40
#define LCLCKS_DOUBLE_Z 0x20
#define LCLCKS_SINGLE_Z 0x10
#define LCLCKS_DOUBLE_Y 0x08
#define LCLCKS_SINGLE_Y 0x04
#define LCLCKS_DOUBLE_X 0x02
#define LCLCKS_SINGLE_X 0x01

#define REGTYPE_RD		0x01
#define REGTYPE_WR		0x02
#define REGTYPE_RW		(REGTYPE_RD | REGTYPE_WR)

static int lis_ints = 1;
module_param(lis_ints, int, 0);
static int lis_dr = 1;
module_param(lis_dr, int, 0);
static int lis_fs = 0;
module_param(lis_fs, int, 0);

static int lis_duration = 500;		// ms
static int lis_xyz_ths = 350;		// mg
static struct semaphore work_sem1, work_sem2;

#define LIS_IRQ_TRIGGER IRQF_TRIGGER_FALLING // (IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING)

static struct register_attr register_list[] = {
#ifdef CONFIG_LIS302
	{LIS3XX_WHOAMI, REGTYPE_RD},
#endif

	{LIS3XX_CTRL_REG1, REGTYPE_RW},
	{LIS3XX_CTRL_REG2, REGTYPE_RW},
	{LIS3XX_CTRL_REG3, REGTYPE_RW},
	{LIS3XX_HP_FILTER_RESET, REGTYPE_RD},
	{LIS3XX_STATUS_REG, REGTYPE_RD},
	{LIS3XX_OUTX, REGTYPE_RD},
	{LIS3XX_OUTY, REGTYPE_RD},
	{LIS3XX_OUTZ, REGTYPE_RD},
	{LIS3XX_FF_WU_CFG_1, REGTYPE_RW},
	{LIS3XX_FF_WU_SRC_1, REGTYPE_RD},
	{LIS3XX_FF_WU_THS_1, REGTYPE_RW},
	{LIS3XX_FF_WU_DURATION_1, REGTYPE_RW},

#if defined(CONFIG_LIS302) || defined(CONFIG_LIS35)
	{LIS3XX_FF_WU_CFG_2, REGTYPE_RW},	
	{LIS3XX_FF_WU_SRC_2, REGTYPE_RD},	
	{LIS3XX_FF_WU_THS_2, REGTYPE_RW},	
	{LIS3XX_FF_WU_DURATION_2, REGTYPE_RW},	
	{LIS3XX_CLICK_CFG, REGTYPE_RW},	
	{LIS3XX_CLICK_SRC, REGTYPE_RD},		
	{LIS3XX_CLICK_THSY_X, REGTYPE_RW},	
	{LIS3XX_CLICK_THSZ, REGTYPE_RW},	
	{LIS3XX_TIMELIMIT, REGTYPE_RW},	
	{LIS3XX_LATENCY, REGTYPE_RW}, 
	{LIS3XX_WINDOW, REGTYPE_RW},	
#endif
};

static struct i2c_client *lis3xx_client;
static struct msm_gsensor_platform_data *lis3xx_client_pdata;
static struct work_struct lis3xx_work1;
static struct work_struct lis3xx_work2;

static void latch_interrupt(struct i2c_client *client, int chn, int flags)
{
	i2c_smbus_write_byte_data(client, chn ? LIS3XX_FF_WU_CFG_2 : LIS3XX_FF_WU_CFG_1, 
	                          LFFWUCFG_LIR | flags);
}

static void lis3xx_get_xyz(struct work_struct *work, int chn)
{
	int x, y, z, ths, ie;
    signed char sx, sy, sz;
    unsigned char event;

    if (lis3xx_client == NULL)
        return;

	x = i2c_smbus_read_byte_data(lis3xx_client, LIS3XX_OUTX);
	y = i2c_smbus_read_byte_data(lis3xx_client, LIS3XX_OUTY);
	z = i2c_smbus_read_byte_data(lis3xx_client, LIS3XX_OUTZ);
    ths = i2c_smbus_read_byte_data(lis3xx_client, 
        							chn ? LIS3XX_FF_WU_THS_2 : LIS3XX_FF_WU_THS_1);
    event = i2c_smbus_read_byte_data(lis3xx_client, 
        							chn ? LIS3XX_FF_WU_SRC_2 : LIS3XX_FF_WU_SRC_1);

	if (x < 0 || y < 0 || z < 0 || ths < 0) {
		lis_dbg("get X.Y.Z failed");
        return;
    }

    ths = (unsigned char)ths & (~LFFWUTHS_DCRM);
    sx = (unsigned char)x;
    sy = (unsigned char)y;
    sz = (unsigned char)z;

	ie = 0;
    ie |= (event & LFFWUSRC_XH) ? LFFWUSRC_XL : LFFWUSRC_XH;
    ie |= (event & LFFWUSRC_YH) ? LFFWUSRC_YL : LFFWUSRC_YH;
    ie |= (event & LFFWUSRC_ZH) ? LFFWUSRC_ZL : LFFWUSRC_ZH;
    latch_interrupt(lis3xx_client, chn, ie);

	lis_dbg("gsensor point: X.Y.Z = (%d).(%d).(%d)", sx, sy, sz);
	lis_dbg("gsensor int%d threshold: threshold = %d", chn ? 2 : 1, ths);
    lis_dbg("gsensor int%d event: IA XH XL YH YL ZH ZL", chn ? 2 : 1);
    lis_dbg("gsensor int%d event: %2d %2d %2d %2d %2d %2d %2d", chn ? 2 : 1, 
        	(event & LFFWUSRC_IA) ? 1 : 0,
	        (event & LFFWUSRC_XH) ? 1 : 0,
	        (event & LFFWUSRC_XL) ? 1 : 0,
        	(event & LFFWUSRC_YH) ? 1 : 0,
        	(event & LFFWUSRC_YL) ? 1 : 0,
            (event & LFFWUSRC_ZH) ? 1 : 0,
            (event & LFFWUSRC_ZL) ? 1 : 0);
}

static void lis3xx_get_xyz1(struct work_struct *work)
{
	if (down_interruptible(&work_sem1) == EINTR) 
		return;
    lis3xx_get_xyz(work, 0);
    up(&work_sem1);
}

static void lis3xx_get_xyz2(struct work_struct *work)
{
	if (down_interruptible(&work_sem2) == EINTR) 
		return;
    lis3xx_get_xyz(work, 1);
    up(&work_sem2);
}

static ssize_t lis3xx_attr_show(struct device *dev, 
								struct device_attribute *attr, 
								char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	int i, j, rc;

	buf[0] = '\0';

	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE_DATA)) {
		lis_dbg("LIS3XX does not support SMBUS_BYTE_DATA");
		return -EINVAL;
	}

	if (strcmp(attr->attr.name, "reglist") == 0) {
		for (i = 0, j = 0; i < ARRAY_SIZE(register_list); i++) {
			if (register_list[i].regtype & REGTYPE_RD) {
				if ((rc = i2c_smbus_read_byte_data(client, register_list[i].regaddr)) < 0)
					return -EIO;
				else {
					sprintf(buf + strlen(buf), "R%2X=%02X ", 
							register_list[i].regaddr, 
							(unsigned char)rc);
					j++;
				}
			}
		}

		sprintf(buf + strlen(buf), "\n");

		return 7 * j + 1;
	}

	return 0;
}

static ssize_t lis3xx_attr_store(struct device *dev, 
							     struct device_attribute *attr, 
							     const char *buf, 
							     size_t len)
{
	char *str;

	struct i2c_client *client = to_i2c_client(dev);

	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE_DATA)) {
		lis_dbg("LIS3XX does not support SMBUS_BYTE_DATA");
		return -EINVAL;
	}

	/* use strict syntax to write register */
	if (strcmp(attr->attr.name, "reglist") == 0) {
		str = (char *)buf;
		if (*str == 'R') {
			while ((str = strchr(str, 'R')) != NULL) {
				if (*str == 'R'
					&& ((*(str + 1) >= '0' && *(str + 1) <= '9') 
						|| (*(str + 1) >= 'A' && *(str + 1) <= 'F')
						|| (*(str + 1) >= 'a' && *(str + 1) <= 'f'))
					&& ((*(str + 2) >= '0' && *(str + 2) <= '9') 
					    || (*(str + 2) >= 'A' && *(str + 2) <= 'F')
					    || (*(str + 2) >= 'a' && *(str + 2) <= 'f'))
					&& *(str + 3) == '='
					&& ((*(str + 4) >= '0' && *(str + 4) <= '9') 
						 || (*(str + 4) >= 'A' && *(str + 4) <= 'F')
						 || (*(str + 4) >= 'a' && *(str + 4) <= 'f'))
					&& ((*(str + 5) >= '0' && *(str + 5) <= '9') 
						 || (*(str + 5) >= 'A' && *(str + 5) <= 'F')
						 || (*(str + 5) >= 'a' && *(str + 5) <= 'f'))
					&& (*(str + 6) == ' ' 
						 || *(str + 6) == '\0' 
						 || *(str + 6) == '\n')) {
					int i;
					unsigned char regaddr;
					unsigned char regval;
					
					regaddr = (unsigned char)simple_strtoul(str + 1, NULL, 16);
					regval = (unsigned char)simple_strtoul(str + 4, NULL, 16);
					for (i = 0; i < ARRAY_SIZE(register_list); i++) {
						if (register_list[i].regaddr == regaddr) {
							if (register_list[i].regtype & REGTYPE_WR)
								break;
							else {
								lis_dbg("Read-only for register 0x%02X", regaddr);
								return -EACCES;
							}
						} else {
							if (i == ARRAY_SIZE(register_list) - 1) {
								lis_dbg("No register 0x%02X", regaddr);
								return -EINVAL;
							}
						}
					}
					
					if (i2c_smbus_write_byte_data(client, regaddr, regval) < 0) {
						lis_dbg("Writing 0x%02X to register 0x%02X failed", regval, regaddr);
						return -EIO;
					} else {
						lis_dbg("Writing 0x%02X to register 0x%02X succeeded", regval, regaddr);
						str += 6;
					}
				} else {
					lis_dbg("Wrong format, should be: RXX=XX %c%c%c%c%c%c0x%02X", *str, *(str + 1), *(str + 2),*(str + 3),*(str + 4),*(str + 5), *(str + 6));
					return -EINVAL;
				}
			}
		} else {
			lis_dbg("Wrong format, should be: RXX=XX");
			return -EINVAL;
		}
	}

	return len;
}

static struct device_attribute lis3xx_reglist_attr = {
	.attr = {
        .name = "reglist",
        .owner = THIS_MODULE,
        .mode = S_IRUGO | S_IWUSR,
    }, 
	.show = lis3xx_attr_show,
	.store = lis3xx_attr_store,
};

static irqreturn_t lis3xx_irq_handler1(int irq, void *dev_id)
{
	lis_dbg("g-sensor interrupt 1");

	disable_irq(lis3xx_client_pdata->gpioirq1);
	schedule_work(&lis3xx_work1);
	enable_irq(lis3xx_client_pdata->gpioirq1);
	
	return IRQ_HANDLED;
}

static irqreturn_t lis3xx_irq_handler2(int irq, void *dev_id)
{
	lis_dbg("g-sensor interrupt 2");

	disable_irq(lis3xx_client_pdata->gpioirq2);
	schedule_work(&lis3xx_work2);
	enable_irq(lis3xx_client_pdata->gpioirq2);
	
	return IRQ_HANDLED;
}

static int lis3xx_probe(struct i2c_client *client,
		const struct i2c_device_id *dev_id)
{
	int rc;
    int duration_value;
	
	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE_DATA)) {
		lis_dbg("LIS3XX does not support SMBUS_BYTE_DATA");
		return -EINVAL;
	}

	lis3xx_client = client;
	if ((rc = device_create_file(&client->dev, &lis3xx_reglist_attr)))
		return -1;

	lis3xx_client_pdata = lis3xx_client->dev.platform_data;
	if (lis3xx_client_pdata->gpio_setup())
		goto err_in_probe;
	
	i2c_smbus_write_byte_data(client, LIS3XX_CTRL_REG1, 
        					  (lis_dr ? LCR1_DR : 0) 
        					  | (lis_fs ? LCR1_FS : 0)
        					  | LCR1_PD
        					  | LCR1_XEN | LCR1_YEN | LCR1_ZEN);

	/* interface and source */
#if 0
    i2c_smbus_write_byte_data(client, LIS3XX_CTRL_REG2, 
	   						  LCR2_FDS
                              | LCR2_HP_FF_WU1 | LCR2_HP_FF_WU2
                              | ((LCR2_HP_COEFF2 & 0) | LCR2_HP_COEFF1));
#else
	i2c_smbus_write_byte_data(client, LIS3XX_CTRL_REG2, 0);
#endif
	/* select internal interrupt source config */
	i2c_smbus_write_byte_data(client, LIS3XX_CTRL_REG3, 
						      LCR3_IHL 
						      | LCR3_I2CFG1 
						      | LCR3_I1CFG0);

    i2c_smbus_write_byte_data(client, LIS3XX_FF_WU_CFG_1, LFFWUCFG_LIR);
    i2c_smbus_write_byte_data(client, LIS3XX_FF_WU_CFG_2, LFFWUCFG_LIR); 

    i2c_smbus_write_byte_data(client, LIS3XX_FF_WU_THS_1, 
                            LFFWUTHS_DCRM | (lis_xyz_ths / (lis_fs ? 72 : 18)));
    i2c_smbus_write_byte_data(client, LIS3XX_FF_WU_THS_2, 
                            LFFWUTHS_DCRM | (lis_xyz_ths / (lis_fs ? 72 : 18)));

	if (lis_dr) {
		lis_duration = lis_duration > 673 ? 673 : lis_duration;
    } else {
    	lis_duration = lis_duration > 2500 ? 2500 : lis_duration;
    }

    duration_value = (lis_duration * 10) / (lis_dr ? 25 : 100);

    /* avoid zero */
    duration_value += 1;
    duration_value = (duration_value > 255) ? 255 : duration_value;

    i2c_smbus_write_byte_data(client, LIS3XX_FF_WU_DURATION_1, 
                              duration_value);
    i2c_smbus_write_byte_data(client, LIS3XX_FF_WU_DURATION_2, 
                              duration_value);

	/* request IRQ */
	lis3xx_client_pdata->gpioirq1 = gpio_to_irq(lis3xx_client_pdata->gpioint1);
	lis3xx_client_pdata->gpioirq2 = gpio_to_irq(lis3xx_client_pdata->gpioint2);
	if (lis_ints & 1) {
		lis_dbg("Using LIS3XX int 1(irq=%d, GPIO%d)", 
				lis3xx_client_pdata->gpioirq1,
				lis3xx_client_pdata->gpioint1);
		if (request_irq(lis3xx_client_pdata->gpioirq1, 
						lis3xx_irq_handler1, 
						LIS_IRQ_TRIGGER, 
						"lis3xx_int1", 
						NULL)) {
			lis_dbg("requesting irq 1 for lis3xx failed");
			goto err_in_probe;
		}
        latch_interrupt(client, 
            			0, 
            			LFFWUCFG_XHIE | LFFWUCFG_YHIE | LFFWUCFG_ZHIE
            			| LFFWUCFG_XLIE | LFFWUCFG_YLIE | LFFWUCFG_ZLIE);
	}

	if (lis_ints & 2) {
	    lis_dbg("Using LIS3XX int 2(irq=%d, GPIO%d)", 
	            lis3xx_client_pdata->gpioirq2,
	            lis3xx_client_pdata->gpioint2);
	    if (request_irq(lis3xx_client_pdata->gpioirq2, 
	                    lis3xx_irq_handler2, 
	                    LIS_IRQ_TRIGGER, 
	                    "lis3xx_int2", 
	                    NULL)) {
	        lis_dbg("requesting irq 2 for lis3xx failed");
			if (lis_ints & 1)
				free_irq(lis3xx_client_pdata->gpioirq1, NULL);
	        goto err_in_probe;
	    }
        latch_interrupt(client, 
            			1, 
            			LFFWUCFG_XHIE | LFFWUCFG_YHIE | LFFWUCFG_ZHIE
            			| LFFWUCFG_XLIE | LFFWUCFG_YLIE | LFFWUCFG_ZLIE);
	}
    
	lis_dbg("motion sensor probed");
	lis_dbg("device name = %s", lis3xx_client->name);
	lis_dbg("device address = 0x%02X", lis3xx_client->addr);
	lis_dbg("device flags = %d", lis3xx_client->flags);

	return 0;

err_in_probe:
	device_remove_file(&client->dev, &lis3xx_reglist_attr);
	lis3xx_client = NULL;

	lis_dbg("probe failed");
	
	return -1;
}

static int __devexit lis3xx_remove(struct i2c_client *client)
{
    if (lis_ints & 1) {
        disable_irq(lis3xx_client_pdata->gpioirq1);
		free_irq(lis3xx_client_pdata->gpioirq1, NULL);
    }
    
	if (lis_ints & 2) {
        disable_irq(lis3xx_client_pdata->gpioirq2);
	    free_irq(lis3xx_client_pdata->gpioirq2, NULL);
    }

	lis3xx_client_pdata->gpio_shutdown();
	device_remove_file(&client->dev, &lis3xx_reglist_attr);
	lis3xx_client = NULL;
	
	return 0;
}

static const struct i2c_device_id lis3xx_id[] = {
	{ "lis3xx", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lis3xx_id);

static struct i2c_driver lis3xx_driver = {
	.driver = {
		.name   = "lis3xx",
		.owner  = THIS_MODULE,
	},
	.probe  = lis3xx_probe,
	.remove = __devexit_p(lis3xx_remove),
	.id_table = lis3xx_id,
};

static int __init lis3xx_init(void)
{
	lis_dbg("Compiled at %s %s", __TIME__, __DATE__);

    lis3xx_client = NULL;
    init_MUTEX(&work_sem1);
    init_MUTEX(&work_sem2);

	INIT_WORK(&lis3xx_work1, lis3xx_get_xyz1);
	INIT_WORK(&lis3xx_work2, lis3xx_get_xyz2);

	return i2c_add_driver(&lis3xx_driver);
}


static void __exit lis3xx_exit(void)
{
	if (down_interruptible(&work_sem1) == EINTR) 
		return;
	if (down_interruptible(&work_sem2) == EINTR) {
        up(&work_sem1);
		return;
    }

	i2c_del_driver(&lis3xx_driver);
    
    up(&work_sem2);
    up(&work_sem1);
}

module_init(lis3xx_init);
module_exit(lis3xx_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Lis3xx motion sensor");
MODULE_AUTHOR("Huang Zhikui");


