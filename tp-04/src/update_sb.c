#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/uuid.h>
#include <linux/timekeeping.h>
#include <linux/mm.h>
#include <linux/slab.h>

MODULE_DESCRIPTION("Module \"UPDATE_SUPER_BLOCK\" pour noyau linux");
MODULE_AUTHOR("AIDER Smail, UPMC");
MODULE_LICENSE("GPL");

static char *type;
module_param(type, charp, 0660);
MODULE_PARM_DESC(type, "Systeme de fichiers, ex. ext4");


static void update(struct super_block *sb)
{
	if(!sb->alreay_init){
		sb->last_access  = 0;	
		sb->alreay_init = 1;
	}
	printk("uuid=%pUb type=%s time=%llx\n", &sb->s_uuid, sb->s_id, (long long)sb->last_access);
	sb->last_access = (long long) ktime_get();
}	

static int __init update_sb_init(void)
{	
	pr_info("update_sn_init\n");
	
	/* Trouver le systeme de fichiers "type" */
	struct file_system_type *fs = get_fs_type(type);
	
	/* iterer à la recherche du systeme de fichiers */
	iterate_supers_type(fs, (void*)update, NULL);
	
	/* rendre la reference */
	put_filesystem(fs);
	return 0;
}
module_init(update_sb_init);

static void __exit update_sb_exit(void)
{
	pr_info("update_sn_exit\n");
	
}
module_exit(update_sb_exit);

