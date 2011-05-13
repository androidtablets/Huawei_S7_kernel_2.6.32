#ifndef __GPIO_I2C_ADPT_H
#define __GPIO_I2C_ADPT_H
#include <linux/i2c.h>
#include <mach/board.h>
#include <linux/mutex.h>
#include <mach/gpio.h>

#define S7_GPIO_I2C_ACK			1
#define S7_GPIO_I2C_NACK			0
#define S7_GPIO_I2C_SUCCESS			1
#define S7_GPIO_I2C_FAIL				0

enum s7_gpio_i2c_xfer_states
{
	STATE_MIC_NOISE_REDUCE=0,
	STATE_HDMI,
	STATE_CAMERA,
};

struct gpio_i2c_s7_dev {
	struct device 			*dev;
	struct mutex 			mlock;
	struct i2c_adapter 	adap_gpio_s7;
	struct gpio_i2c_adpt_platform_data	*pdata;
};

extern void gpio_i2c_adpt_set_state(enum s7_gpio_i2c_xfer_states state);
#endif
