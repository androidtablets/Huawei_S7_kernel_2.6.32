/* Add app_info proc file, ported from U8220 proc_misc.c. suchangyu. 20100119. Begin */
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>

#include <asm/setup.h>


#define MAX_VERSION_CHAR 40
static char appsboot_version[MAX_VERSION_CHAR + 1];

#define ATAG_BOOT_VERSION 0x4d534D71 /* ATAG BOOT VERSION */
static int __init parse_tag_boot_version(const struct tag *tag)
{
  char *tag_boot_ver=(char*)&tag->u;

  memset(appsboot_version, 0, MAX_VERSION_CHAR + 1);
  memcpy(appsboot_version, tag_boot_ver, MAX_VERSION_CHAR);
     
  printk(KERN_INFO "appsboot_version = %s\n", appsboot_version);

  return 0;
}
__tagtable(ATAG_BOOT_VERSION, parse_tag_boot_version);


/**/
#define PROC_MANUFACTURER_STR_LEN 30
#define ATAG_BOOT_READ_FLASH_ID 0x4d534D72
static char str_flash_nand_id[PROC_MANUFACTURER_STR_LEN] = {0};

static int __init parse_tag_boot_flash_id(const struct tag *tag)
{
  char *tag_flash_id=(char*)&tag->u;
  memset(str_flash_nand_id, 0, PROC_MANUFACTURER_STR_LEN);
  memcpy(str_flash_nand_id, tag_flash_id, PROC_MANUFACTURER_STR_LEN);
    
  printk("tag_boot_flash_id = %s\n", tag_flash_id);

  return 0;
}
__tagtable(ATAG_BOOT_READ_FLASH_ID, parse_tag_boot_flash_id);
/**/

static int app_info_proc_show(struct seq_file *m, void *v)
{
  // TODO: APPSBOOT_VERSION should be set in bootloader, not in KERNEL
  //seq_printf(m, "APPSBOOT:\n%s\nAPP:\n%s\n", APPSBOOT_VERSION, HUAWEI_KERNEL_VERSION);
  seq_printf(m, "APPSBOOT:\n%s\n"
	     "APP:\n%s\n"
	     "FLASH_ID:\n%s\n", appsboot_version, HUAWEI_KERNEL_VERSION, str_flash_nand_id);
  return 0;
}

static int app_info_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, app_info_proc_show, NULL);
}

static const struct file_operations app_info_proc_fops = {
	.open		= app_info_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_app_info_init(void)
{
	proc_create("app_info", 0, NULL, &app_info_proc_fops);
	return 0;
}
module_init(proc_app_info_init);
