#ifndef MT9V114_H
#define  MT9V114_H
#include <linux/types.h> 
#include <mach/camera.h> 


#undef CDBG 
#define CDBG(fmt, args...) printk(KERN_ERR "mt9v114: " fmt, ##args)


struct reg_struct {
	uint16_t vt_pix_clk_div;        /*  0x0300 */ 
	uint16_t vt_sys_clk_div;        /*  0x0302 */
	uint16_t pre_pll_clk_div;       /*  0x0304 */
	uint16_t pll_multiplier;        /*  0x0306 */
	uint16_t op_pix_clk_div;        /*  0x0308 */
	uint16_t op_sys_clk_div;        /*  0x030A */
	uint16_t scale_m;               /*  0x0404 */
	uint16_t row_speed;             /*  0x3016 */
	uint16_t x_addr_start;          /*  0x3004 */
	uint16_t x_addr_end;            /*  0x3008 */
	uint16_t y_addr_start;        	/*  0x3002 */
	uint16_t y_addr_end;            /*  0x3006 */
	uint16_t read_mode;             /*  0x3040 */
	uint16_t x_output_size;         /*  0x034C */
	uint16_t y_output_size;         /*  0x034E */
	uint16_t line_length_pck;       /*  0x300C */
	uint16_t frame_length_lines;	/*  0x300A */
	uint16_t coarse_int_time; 		/*  0x3012 */
	uint16_t fine_int_time;   		/*  0x3014 */
};

 struct camsensor_mt9v114_reg_type
{
  unsigned short addr;
  unsigned short  val;
} ;

typedef enum
{
    GPIO_LOW_VALUE  = 0,
    GPIO_HIGH_VALUE = 1
} gpio_value_type;

#endif


