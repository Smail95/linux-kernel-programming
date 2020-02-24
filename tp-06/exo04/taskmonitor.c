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

MODULE_DESCRIPTION("Module \"Task Monitor\" pour noyau linux");
MODULE_AUTHOR("AIDER Smail, LIP6");
MODULE_LICENSE("GPL");

/* parameters */
static int target = 0;
module_param(target, int, 0000);
MODULE_PARM_DESC(target, "pid process");

/* Prototypes */
ssize_t taskmonitor_show(struct kobject *kobj, struct kobj_attribute *attr,char *buf);
ssize_t taskmonitor_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
int monitor_fn(void *unused);

/* global variables & structures */
struct task_monitor {
	struct pid *pid;
};
struct task_sample {
	u64 utime;
	u64 stime;
};
static struct kobj_attribute taskmonitor = __ATTR_RW(taskmonitor);
static struct task_monitor *task_m = NULL;
static struct task_sample *task_s = NULL;
static struct task_struct *stat_thread = NULL;


/* Functions */
ssize_t taskmonitor_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
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

ssize_t taskmonitor_show(struct kobject *kobj, struct kobj_attribute *attr,char *buf)
{	
	return sprintf(buf, "pid %d usr %llu sys %llu \n", target, task_s->utime, task_s->stime);
}

int get_sample(struct task_monitor *tm, struct task_sample *sample)
{
	int ret = 0;
	
	struct task_struct *task_pid = NULL;
	task_pid = get_pid_task(task_m->pid, PIDTYPE_PID);
	if(task_pid == NULL){
		pr_err("Error: get_pid_task");
		return -1;	
	}
	if(pid_alive(task_pid) == 1){
		sample->utime = task_pid->utime;
		sample->stime = task_pid->stime;
		ret = 1;
	}
	put_task_struct(task_pid);
	return ret;
}

/* 
 * Allocate task_monitor structure and return the <struct pid> from <pid_t> 
 */
int monitor_pid(pid_t p)
{	
	task_m = kzalloc(sizeof(struct task_monitor), GFP_KERNEL);
	task_s = kzalloc(sizeof(struct task_sample), GFP_KERNEL);
	
	if(task_m == NULL || task_s == NULL){
		pr_err("kzalloc error");
		return -1;	
	}
	/* get <struct pid> from <pid_t> */
	task_m->pid = find_get_pid(p);
	if(!task_m->pid)
		goto ERROR;	
	
	return 0;
ERROR:
	kfree(task_m);
	kfree(task_s);
	return -1;	
}

/* 
 * Function executed by the thread, find the <struct task_strcut> corresponding to <struct task>
 * and retrieve some statistics of the given process(stime & utime). 
 */
int monitor_fn(void *unused)
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

/* Init & Exit */
static int __init taskmonitor_init(void)
{	
	int ret;
	pr_debug("INIT TASK MONITOR");
	
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
	
	return 0;
}

static void __exit taskmonitor_exit(void)
{
	pr_debug("EXIT TASK MONITOR");
	
	if(stat_thread != NULL)
		kthread_stop(stat_thread);
		
	/* Release */
	sysfs_remove_file(kernel_kobj, &(taskmonitor.attr));
	put_pid(task_m->pid);
	kfree(task_m);
	kfree(task_s);
	
	return;
} 

module_init(taskmonitor_init);
module_exit(taskmonitor_exit);