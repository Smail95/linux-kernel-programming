#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/kobject.h>
#include <linux/slab.h>


MODULE_DESCRIPTION("Module \"hello sysfs\" pour noyau linux");
MODULE_AUTHOR("AIDER Smail, LIP6");
MODULE_LICENSE("GPL");

static char *message = "Hello sysfs!\n";

ssize_t hellosysfs_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%s", message);
}

ssize_t hellosysfs_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	u8 size = 20;
	message = kzalloc(size, GFP_KERNEL);
	sprintf(message, "Hello %s",buf);
	
	return strlen(message);
}

static struct kobj_attribute hellosysfs = __ATTR_RW(hellosysfs);


static int __init hellosysfs_init(void)
{
	int ret;
	pr_debug("HELLO_SYSFS INIT\n");
	ret = sysfs_create_file(kernel_kobj, &(hellosysfs.attr));	
	if(ret)
		return ret;
		
	return 0;
}

static void __exit hellosysfs_exit(void)
{
	pr_debug("HELLO_SYSFS EXIT\n");
	
	sysfs_remove_file(kernel_kobj, &(hellosysfs.attr));
	kfree(message);
	return;
}

module_init(hellosysfs_init);
module_exit(hellosysfs_exit);
