
#ifndef MT9D113_BACK_H
#define MT9D113_BACK_H

#include <linux/types.h>
#include <mach/camera.h>

#undef CDBG
#define CDBG(fmt, args...) printk(KERN_ERR "mt9d113_back: " fmt, ##args)

extern struct mt9d113_back_reg mt9d113_back_regs;

enum mt9d113_back_width_t {
	WORD_LEN,
	BYTE_LEN
};

struct mt9d113_back_i2c_reg_conf {
	unsigned short waddr;
	unsigned short wdata;
	enum mt9d113_back_width_t width;
	unsigned short mdelay_time;
};


struct mt9d113_back_reg {
	
	struct register_address_value_pair const *prev_snap_reg_init;
	uint16_t prev_snap_reg_init_size;	
	struct register_address_value_pair const *prev_snap_reg_settings;
	uint16_t prev_snap_reg_settings_size;
    	struct register_address_value_pair const *sunny_lens_reg_settings;
	uint16_t sunny_lens_reg_settings_size;
    	struct register_address_value_pair const *byd_lens_reg_settings;
	uint16_t byd_lens_reg_settings_size;
    	struct register_address_value_pair const *liteon_lens_reg_settings;
	uint16_t liteon_lens_reg_settings_size;
    	struct register_address_value_pair const *byd_sunny_awb_reg_settings;
	uint16_t byd_sunny_awb_reg_settings_size;
    	struct register_address_value_pair const *liteon_awb_reg_settings;
	uint16_t liteon_awb_reg_settings_size;
};


#endif /* MT9D113_BACK_H */

