#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/pid.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/kthread.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "taskmonitor.h"


MODULE_DESCRIPTION("Module \"Task Monitor\" pour noyau linux");
MODULE_AUTHOR("AIDER Smail, LIP6");
MODULE_LICENSE("GPL");

/* parameters */
static int target = 0;
module_param(target, int, 0000);
MODULE_PARM_DESC(target, "pid process");

/* Prototypes */
static ssize_t taskmonitor_show(struct kobject *kobj, struct kobj_attribute *attr,char *buf);
static ssize_t taskmonitor_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static int monitor_fn(void *unused);

/* global variables & structures */
struct task_monitor {
	struct pid *pid;
};
static struct kobj_attribute taskmonitor = __ATTR_RW(taskmonitor);
static struct task_monitor *task_m = NULL;
static struct task_sample *task_s = NULL;
static struct task_struct *stat_thread = NULL;
static int major;

/* Functions */
static ssize_t taskmonitor_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	if(memcmp(buf, "start", count-1) == 0){
		if(stat_thread)
			pr_info("Info: Thread already running !!\n");
		else
			stat_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	}else if(memcmp(buf,"stop", count-1) == 0){
		if(stat_thread){
			kthread_stop(stat_thread);
			stat_thread = NULL;
		}
		else{ 
			pr_info("Info: No thread running !!\n");
		}
	}else {
		pr_info("Command not found, please write: 'stop' or 'start' !!!\n");	
	}
	return count;
}

static ssize_t taskmonitor_show(struct kobject *kobj, struct kobj_attribute *attr,char *buf)
{	
	return sprintf(buf, "pid %d usr %llu sys %llu \n", target, task_s->utime, task_s->stime);
}

static int get_sample(struct task_monitor *tm, struct task_sample *sample)
{
	int ret = 0;
	
	struct task_struct *task_pid = NULL;
	task_pid = get_pid_task(task_m->pid, PIDTYPE_PID);
	if(task_pid == NULL){
		pr_err("Error: get_pid_task");
		return -1;	
	}
	if(pid_alive(task_pid) == 1){
		sample->pid = target;
		sample->utime = task_pid->utime;
		sample->stime = task_pid->stime;
		ret = 1;
	}
	put_task_struct(task_pid);
	return ret;
}

/* 
 * return the <struct pid> from <pid_t> 
 */
static int monitor_pid(pid_t p)
{	
	/* get <struct pid> from <pid_t> */
	task_m->pid = find_get_pid(p);
	if(!task_m->pid)
		return -1;	
	return 0;
}

/* 
 * Function executed by the thread, find the <struct task_strcut> corresponding to <struct task>
 * and retrieve some statistics of the given process(stime & utime). 
 */
static int monitor_fn(void *unused)
{
	while(!kthread_should_stop()){
		if(get_sample(task_m, task_s) < 0){
			pr_err("Error: get_sample");
			return -1;		
		}
		/*sleep until timeout */		
		set_current_state(TASK_UNINTERRUPTIBLE);		
		schedule_timeout(HZ);
	}	
	return 0;
}

static int get_sample_char(struct file *file, unsigned int cmd, unsigned long args)
{	
	char *buf = NULL;
	buf = kzalloc(MSG_SIZE ,GFP_KERNEL);
	if(buf == NULL){
		pr_err("kzalloc error");
		return -1;	
	}
	sprintf(buf, "pid %d usr %llu sys %llu", target, task_s->utime, task_s->stime);
	if(copy_to_user((void*)args, (void*)buf ,_IOC_SIZE(cmd)) != 0){
		pr_err("copu_to_user");
		return -1;	
	}
	return 0;
}
static int get_sample_struct(struct file *file, unsigned int cmd, unsigned long args)
{
	if(copy_to_user((void*)args,(void*)task_s, _IOC_SIZE(cmd)) != 0){
		pr_err("copu_to_user");
		return -1;	
	}
	return 0;
}
static int start_kthread(struct file *file, unsigned int cmd, unsigned long args)
{	
	if(stat_thread){
		pr_info("Info: Thread already running !!\n");
	}else{
		stat_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
		pr_info("Info: Start new Kthread !!\n");
	}
	return 0;
}
static int stop_kthread(struct file *file, unsigned int cmd, unsigned long args)
{
	if(stat_thread){
		kthread_stop(stat_thread);
		stat_thread = NULL;
		pr_info("Info: Current kthread stopped !!\n");
	}else{ 
		pr_info("Info: No thread running !!\n");
	}
	return 0;
}
static int set_pid(struct file *file, unsigned int cmd, unsigned long args)
{
	int pid;
	if(copy_from_user(&pid, (void*)args, _IOC_SIZE(cmd)) != 0){
		pr_err("copy_from_user");
		return -1;
	}
	if(monitor_pid(pid) < 0){
		pr_err("Error: monitor_pid: pid does not exist !!! \n");
		return -1;
	}
	stop_kthread(NULL, 0, 0);
	target = pid;
	start_kthread(NULL, 0, 0);
	return 0;
}

static long ioctl_monitor(struct file *file, unsigned int cmd, unsigned long args)
{
	int ret;
	/* check the magic number */
	if(_IOC_TYPE(cmd) != IOC_MAGIC)
		return -EINVAL;	
		
	switch(cmd) {
		case GET_SAMPLE_1:
			pr_debug("IOCTL GET_SAMPLE_1 CHAR");
			ret = get_sample_char(file, cmd, args);
		break;
		case GET_SAMPLE_2:
			pr_debug("IOCTL GET_SAMPLE_2 STRUCT");
			ret = get_sample_struct(file, cmd, args);
		break;
		case TASKMON_START:
			pr_debug("IOCTL TASKMON_START");
			ret = start_kthread(file, cmd, args);		
		break;
		case TASKMON_STOP:
			pr_debug("IOCTL TASKMON_STOP");
			ret = stop_kthread(file, cmd, args);		
		break;
		case TASKMON_SET_PID:
			pr_debug("IOCTL TASKMON_SET_PID");
			ret = set_pid(file, cmd, args);	
		break;
		default: 
			return -ENOTTY;
	}//switch
	if(ret == -1)
		return -EINVAL;
	return 0;
}

static struct file_operations fops = {
	.unlocked_ioctl		= ioctl_monitor
};

/* Init & Exit */
static int __init taskmonitor_init(void)
{	
	int ret;
	pr_debug("INIT TASK MONITOR");
	
	/* Allocate memory */	
	task_m = kzalloc(sizeof(struct task_monitor), GFP_KERNEL);
	task_s = kzalloc(sizeof(struct task_sample), GFP_KERNEL);
	if(task_m == NULL || task_s == NULL){
		pr_err("kzalloc error");
		return -1;	
	}
	/* get <struct pid> from <pid_t> */
	ret = monitor_pid(target);
	if(ret < 0){
		pr_err("Error: monitor_pid: pid does not exist !!! \n");
		return ret;
	}
	pr_info("struct pid found for pid_t: %d \n", target);
	/* create attribute */
	ret = sysfs_create_file(kernel_kobj, &(taskmonitor.attr));	
	if(ret)
		return ret;
	/* create and run thread */
	stat_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	
	/* ioctl */	
	major = register_chrdev(0, "taskmonitor", &fops);
	if(major < 0){
		pr_err("register_chrdev");
		return -1;	
	}
	return 0;
}

static void __exit taskmonitor_exit(void)
{
	pr_debug("EXIT TASK MONITOR");
	
	if(stat_thread != NULL)
		kthread_stop(stat_thread);
		
	/* Release */
	sysfs_remove_file(kernel_kobj, &(taskmonitor.attr));
	unregister_chrdev(major, "taskmonitor");
	put_pid(task_m->pid);
	kfree(task_m);
	kfree(task_s);
	
	return;
} 

module_init(taskmonitor_init);
module_exit(taskmonitor_exit);