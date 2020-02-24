#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/pid.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/kthread.h>


MODULE_DESCRIPTION("Module \"Task Monitor\" pour noyau linux");
MODULE_AUTHOR("AIDER Smail, LIP6");
MODULE_LICENSE("GPL");

/* parameters */
static int target = 0;
module_param(target, int, 0000);
MODULE_PARM_DESC(target, "pid process");

/* global variables & structures */
struct task_monitor {
	struct pid *pid;
};

static struct task_monitor *task = NULL;
static struct task_struct *stat_thread;


/* Functions */
/* 
 * Allocate task_monitor structure and return the <struct pid> from <pid_t> 
 */
int monitor_pid(pid_t p)
{	
	task = kzalloc(sizeof(struct task_monitor), GFP_KERNEL);
	if(task == NULL){
		pr_err("kzalloc error");
		return -1;	
	}
	/* get <struct pid> from pid_t */
	task->pid = find_get_pid(p);
	if(!task->pid)
		goto ERROR;	
	
	return 0;
ERROR:
	kfree(task);
	return -1;	
}

/* 
 * Function executed by the thread, find the <struct task_strcut> corresponding to <struct task>
 * and retrieve some statistics of the given process(stime & utime). 
 */
int monitor_fn(void *unused)
{
	struct task_struct *task_pid = NULL;
	
	task_pid = get_pid_task(task->pid, PIDTYPE_PID);
	if(task_pid == NULL){
		pr_err("Error: get_pid_task");
		return -1;	
	}
	while(!kthread_should_stop()){
		if(pid_alive(task_pid) == 0)
			pr_info("pid %d is dead !!\n", target);	
		else
			pr_info("pid %d usr %llu sys %llu (ms)\n", target, task_pid->utime , task_pid->stime);		
		/*sleep until timeout */		
		set_current_state(TASK_UNINTERRUPTIBLE);		
		schedule_timeout(HZ*2);
	}
	
	put_task_struct(task_pid);
	return 0;
}

/* Init & Exit */
static int __init taskmonitor_init(void)
{	
	int ret;
	pr_debug("INIT TASK MONITOR");
	
	/* find <struct pid> */
	ret = monitor_pid(target);
	if(ret < 0){
		pr_err("Error: monitor_pid: pid does not exist !!! \n");
		return ret;
	}
	pr_info("struct pid found for pid_t: %d \n", target);
	
	/* create and run thread */
	stat_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	
	return 0;
}

static void __exit taskmonitor_exit(void)
{
	pr_debug("EXIT TASK MONITOR");
	
	if(stat_thread)
		kthread_stop(stat_thread);
		
	/* Release */
	put_pid(task->pid);
	kfree(task);
	
	return;
} 

module_init(taskmonitor_init);
module_exit(taskmonitor_exit);