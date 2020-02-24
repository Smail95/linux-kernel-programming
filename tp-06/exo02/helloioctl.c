#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include "helloioctl.h"


MODULE_DESCRIPTION("Module \"hello ioctl\" pour noyau linux");
MODULE_AUTHOR("AIDER Smail, LIP6");
MODULE_LICENSE("GPL");

static int major;


static long ioctl_hello(struct file *file, unsigned int cmd, unsigned long args)
{
	static struct msg m;
	struct msg tmp;
	
	/* check the magic number */
	if(_IOC_TYPE(cmd) != IOC_MAGIC)
		return -EINVAL;
	
	switch(cmd) {
		case HELLO:
			pr_debug("IOCTL HELLO");
			if(copy_to_user((void*)args, (void*)&m, _IOC_SIZE(cmd)) != 0){
				pr_err("copy_to_user");			
				return -EINVAL;
			}	
		break;
		case WHO:
			pr_debug("IOCTL WHO");
			if(copy_from_user(&tmp, (void*)args, _IOC_SIZE(cmd)) != 0){
				pr_err("copy_from_user");			
				return -EINVAL;
			}
			sprintf(m.message, "HELLO %s", tmp.message);
		break;
		default:
			return -ENOTTY;
		
	}//switch	
	
	return 0;
}


static struct file_operations fops = 
{
	.unlocked_ioctl		= ioctl_hello
};

static int __init helloioctl_init(void)
{
	pr_debug("INIT HELLO_IOCTL \n");
	major = register_chrdev(0, "helloioctl", &fops);
	if(major < 0){
		pr_err("register_chrdev");
		return -1;	
	}
	return 0;
}

static void __exit helloioctl_exit(void)
{
	pr_debug("EXIT HELLO_IOCTL \n");
	unregister_chrdev(major, "helloioctl");
	return;
}

module_init(helloioctl_init);
module_exit(helloioctl_exit);