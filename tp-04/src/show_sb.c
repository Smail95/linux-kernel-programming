#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/uuid.h>

MODULE_DESCRIPTION("Module \"SHOW_SUPER_BLOCK\" pour noyau linux");
MODULE_AUTHOR("AIDER Smail, UPMC");
MODULE_LICENSE("GPL");

static void get_info(struct super_block *sb)
{		
	printk("uuid=%pUb type=%s\n", &sb->s_uuid, sb->s_id);
}

static int __init show_sb_init(void)
{
	pr_info("show_sn_init\n");
	iterate_supers((void*)get_info, NULL);

	return 0;
}
module_init(show_sb_init);

static void __exit show_sb_exit(void)
{
	pr_info("show_sn_exit\n");
	
}
module_exit(show_sb_exit);


