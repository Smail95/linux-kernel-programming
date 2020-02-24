#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/uts.h>
#include <linux/utsname.h>

MODULE_DESCRIPTION("Module \"UNAME\" pour noyau linux");
MODULE_AUTHOR("AIDER Smail, UPMC");
MODULE_LICENSE("GPL");

static int __init uname_init(void)
{
	pr_info("UNAME modified\n");
	memcpy(init_uts_ns.name.sysname,"LINUX AIDER\0", 12);
	return 0;	
}
module_init(uname_init);

static void __exit uname_exit(void)
{
	pr_info("UNAME restaured\n");
	strcpy(init_uts_ns.name.sysname, UTS_SYSNAME);
}
module_exit(uname_exit);
