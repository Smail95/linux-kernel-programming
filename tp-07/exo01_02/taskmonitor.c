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
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/shrinker.h>

#define FREQ	HZ

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
static int monitor_fn(void *unused);
static unsigned long tm_shrink_count(struct shrinker *, struct shrink_control *sc);
static unsigned long tm_shrink_scan(struct shrinker *, struct shrink_control *sc);
				       
/* global variables & structures */
struct task_monitor {
	struct list_head head;
	struct pid *pid;
	u64 samples;
	struct mutex mtx;
};
struct task_sample {
	struct list_head list;
	u64 utime;
	u64 stime;
};
static struct shrinker tm_shrinker = {
	.count_objects		= tm_shrink_count,
	.scan_objects 		= tm_shrink_scan,
	.seeks			= DEFAULT_SEEKS 
};
static struct kobj_attribute taskmonitor = __ATTR_RW(taskmonitor);
static struct task_monitor *task_m = NULL;
static struct task_struct *stat_thread = NULL;


/* Functions */

static unsigned long free_list(unsigned long tofree)
{
	u8 offset = (void*)(&((struct task_sample*)0)->list) - (void*)((struct task_sample*)0);
	struct task_sample *sample;
	struct list_head *entry, *todel;
	u64 nr_to_scan = tofree;
	u64 freed = 0;

	mutex_lock(&task_m->mtx);
	
	todel = NULL;
	list_for_each(entry, &task_m->head){
//		if(!--nr_to_scan) /* !!!!!!!!???? */
//			break;
		if(todel != NULL){
			sample = (struct task_sample*)((void*)todel - offset);
			list_del(todel);
			kfree(sample);
			freed++;
		}
		todel = entry;
	}
	if(todel != NULL){
		sample = (struct task_sample*)((void*)todel - offset);
		list_del(todel);
		kfree(sample);
		freed++;
	}
	/* Init simples */
	task_m->samples -= freed;
	
	mutex_unlock(&task_m->mtx);
	pr_info("Total freed %llu \n", freed);
	
	return freed;	
}

static unsigned long tm_shrink_scan(struct shrinker * shrink, struct shrink_control *sc)
{
	u64 freed;
	pr_info("SHRINKER START (nr_to_scan %lu) \n", sc->nr_to_scan);
	
	freed = free_list(sc->nr_to_scan);
	return freed;
}

static unsigned long tm_shrink_count(struct shrinker *shrink, struct shrink_control *sc)
{
	u64 count;
	
	mutex_lock(&task_m->mtx);
	count = task_m->samples;
	mutex_unlock(&task_m->mtx);
	
	if(!count)
		return SHRINK_EMPTY;
	return count;
}

ssize_t taskmonitor_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	if(memcmp(buf, "start", 5) == 0){
		if(stat_thread)
			pr_info("Info: Thread already running !!\n");
		else
			stat_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	}else if(memcmp(buf,"stop", 4) == 0){
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
	u8 offset = (void*)(&((struct task_sample*)0)->list) - (void*)((struct task_sample*)0);
	s16 page_size = PAGE_SIZE;
	struct list_head *pos;
	struct task_sample *sample;
	char tmp[100];
	
	mutex_lock(&task_m->mtx);
	list_for_each(pos, &task_m->head){
		sample = (struct task_sample*)((void*)pos - offset);
		sprintf(tmp, "pid %d usr %llu sys %llu\n", target, sample->utime, sample->stime);
		
		page_size -= strlen(tmp);
		if(page_size < 0)
			break;
		strcat(buf, tmp);
	}
	mutex_unlock(&task_m->mtx);
	pr_info("Total stats %llu \n", task_m->samples);
	return strlen(buf);
}
/*
 * Return CPU statistics in struct task_sample 
 * Return:
 * 0: pid died/Error
 * 1: pid alive  */
int get_sample(struct task_monitor *tm, struct task_sample *sample)
{
	int ret = 0;
	struct task_struct *task_pid = NULL;
	
	task_pid = get_pid_task(task_m->pid, PIDTYPE_PID);
	if(task_pid == NULL){
		pr_err("Error: get_pid_task");
		return 0;	
	}
	if(pid_alive(task_pid) == 1){
		sample->utime = task_pid->utime / 1000000;
		sample->stime = task_pid->stime / 1000000;
		ret = 1;
	}
	put_task_struct(task_pid);
	return ret;
}

/* 
 * Return the <struct pid> from <pid_t> 
 */
int monitor_pid(pid_t p)
{	
	INIT_LIST_HEAD(&task_m->head);
	task_m->samples = 0;
	mutex_init(&task_m->mtx);	
	task_m->pid = find_get_pid(p);
	
	if(!task_m->pid)
		return -1;		
	return 0;	
}
/*
 * Save CPU statistics 
 */
static void save_sample(void)
{
	struct task_sample *sample = NULL;
	sample = kmalloc(sizeof(struct task_sample), GFP_KERNEL);
	
	pr_info("ksize %lu sizeof %lu ", ksize(sample), sizeof(sample));
	
	if(sample == NULL){
		pr_err("Error: save_sample::kmalloc");
		return;
	}
	if(get_sample(task_m, sample)){
		mutex_lock(&task_m->mtx);
		list_add(&(sample->list), &(task_m->head));
		task_m->samples++;
		mutex_unlock(&task_m->mtx);
	}
}
/* 
 * Function executed by the thread, find the <struct task_strcut> corresponding to <struct task>
 * and retrieve some statistics of the given process(stime & utime). 
 */
static int monitor_fn(void *unused)
{
	while(!kthread_should_stop()){
		save_sample();
		/*sleep until timeout */		
		set_current_state(TASK_UNINTERRUPTIBLE);		
		schedule_timeout(FREQ);
	}	
	return 0;
}

/* Init & Exit */
static int __init taskmonitor_init(void)
{	
	int ret;
	pr_debug("INIT TASK MONITOR");

	/* Allocate memory */
	task_m = kzalloc(sizeof(struct task_monitor), GFP_KERNEL);	
	if(task_m == NULL){
		pr_err("kzalloc error");
		return -1;	
	}
	/* create attribute */
	ret = sysfs_create_file(kernel_kobj, &(taskmonitor.attr));
	if(ret)
		goto ERROR;
	
	/* Get <struct pid> from <pid_t> */
	ret = monitor_pid(target);
	if(ret){
		pr_err("Error: monitor_pid: pid does not exist !!! \n");
		goto ERROR;
	}
	pr_info("struct pid found for pid_t: %d \n", target);
	/* create and run thread */
	stat_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	/* Register shrinker */
	ret = register_shrinker(&tm_shrinker);
	if(ret)
		goto ERROR;
		
	
	return 0;
ERROR:	
	kfree(task_m);
	return -1;
}

static void __exit taskmonitor_exit(void)
{	
	pr_debug("EXIT TASK MONITOR");
	
	/* Stop kthread */
	if(stat_thread != NULL)
		kthread_stop(stat_thread);
		
	/* Release */
	sysfs_remove_file(kernel_kobj, &(taskmonitor.attr));
	put_pid(task_m->pid);
	/* Free list */
	free_list(0);
	/* Shrinker */
	unregister_shrinker(&tm_shrinker);
	
	return;
} 

module_init(taskmonitor_init);
module_exit(taskmonitor_exit);
