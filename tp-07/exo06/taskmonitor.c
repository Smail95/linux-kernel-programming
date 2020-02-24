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
#include <linux/mempool.h>
#include <linux/kref.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include "taskmonitor.h"

	
MODULE_DESCRIPTION("Module \"Task Monitor\" pour noyau linux");
MODULE_AUTHOR("AIDER Smail, LIP6");
MODULE_LICENSE("GPL");


/* Module parameters */
static int target = 0;
static int target2 = 0;
module_param(target, int, 0000);
module_param(target2, int, 0000);
MODULE_PARM_DESC(target, "pid process");

static struct dentry *taskmonitor_debugfs;
static struct kobj_attribute taskmonitor = __ATTR_RW(taskmonitor);	/* task monitor attribute in sysfs */
static struct task_monitor *task_m = NULL;				/* task monitor structure */
static struct task_struct *stat_thread = NULL;				/* kthread who execute monitor_fn function */
static struct kmem_cache *task_sample_cache = NULL;			/* cache for task_sample structure */
static mempool_t *task_sample_mempool = NULL;				/* memory pool for task_sample strucutre */

/* --- SEQ_FILE OPERATIONS --- */

/*
 * Print the information pointed by the iterator
 */
static int taskmonitor_seq_show (struct seq_file *m, void *v)
{
	struct list_head *entry = v;
	struct task_sample *sample;
	
	sample = container_of(entry, struct task_sample, list);
	seq_printf(m, "pid %d sys %llu usr %llu\n", target, sample->utime, sample->stime);
	
	return 0;
}

/*
 * Return the next object in the linked list, NULL if end
 */
static void * taskmonitor_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	//struct task_sample *sample;
	struct list_head *prev_entry = v;
	struct list_head *next_entry = NULL;
	
	if(prev_entry->next != &(task_m->head))
		next_entry = prev_entry->next;
	else
		*pos = -1; //End of List => STOP
/*	
	sample = container_of(prev_entry, struct task_sample, kref);
	kref_put(&sample->kref, release_ts_kref);
	
	if(next_entry != NULL){
		sample = container_of(next_entry, struct task_sample, kref);
		kref_get(&sample->kref);
	}
*/	return next_entry;
}

/*
 * Release the mutex taken by start()
 */
static void  taskmonitor_seq_stop(struct seq_file *m, void *v)
{
	//struct task_sample *sample;
	//struct list_head *prev_entry = v;
	//sample = container_of(prev_entry, struct task_sample, kref);
	//kref_put(&sample->kref, release_ts_kref);
	mutex_unlock(&task_m->mtx);
}	

/* Return the first element of the list, NULL if empty or aleardy iterated 
 * Iterate over the task_monitor list and return the first object
 * Lock the task_monitor mutex
 */
 static void * taskmonitor_seq_start(struct seq_file *seq, loff_t *pos)
{
	//struct task_sample *sample;
	struct list_head *entry;
	
	mutex_lock(&task_m->mtx);
	/* Stop if the list is empty  or end of list */
	if(task_m->samples == 0 || *pos == -1)
		return NULL;
	entry = task_m->head.next;
	//sample = container_of(entry, struct task_sample, kref);
	//kref_get(&sample->kref);
	
	return entry; 	
}

/* --- FILE_OPERATIONS --- */

static int taskmonitor_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &taskmonitor_seq_ops);
}

/*
 * 
 * */
static ssize_t taskmonitor_write(struct file *file, const char __user *user_buf, size_t size, loff_t *ppos)
{
	
	return 0;
}

/* --- OTHERS --- */

/* release_ts_kref - clean up the object when the last refrence 
 * reference to the object is released.
 * @kref: pointer to kref member  
 */
void release_ts_kref(struct kref *kref)
{
	struct task_sample *sample;
	
	sample = container_of(kref, struct task_sample, kref);
	list_del(&sample->list);
	mempool_free(sample, task_sample_mempool);
}

/*
 * free_tm_list - free list of strcut task_monitor 
 * 
 * @tofree: number of object to be freed from the list
 * @return: the number of items being freed  
 */
static unsigned long free_tm_list(unsigned long tofree)
{
	u8 offset = (void*)(&((struct task_sample*)0)->list) - (void*)((struct task_sample*)0);
	struct task_sample *sample;
	struct list_head *entry, *todel;
	u64 nr_to_scan = tofree;
	u64 freed = 0;

	mutex_lock(&task_m->mtx);
	
	todel = NULL;
	list_for_each(entry, &task_m->head){
		if(!--nr_to_scan) /* !!!!!!!!???? */
			break;
		if(todel != NULL){
			sample = (struct task_sample*)((void*)todel - offset);
			//kref_put(&sample->kref, release_ts_kref); 
			list_del(&sample->list);
			mempool_free(sample, task_sample_mempool);
			freed++;
		}
		todel = entry;
	}
	if(todel != NULL){
		sample = (struct task_sample*)((void*)todel - offset);
		//kref_put(&sample->kref, release_ts_kref);
		list_del(&sample->list);
		mempool_free(sample, task_sample_mempool);
		freed++;
	}
	/* update the number of remainig samples */
	task_m->samples -= freed;
	
	mutex_unlock(&task_m->mtx);
	pr_info("Total freed: %llu \n", freed);
	
	return freed;	
}

static unsigned long tm_shrink_scan(struct shrinker * shrink, struct shrink_control *sc)
{
	u64 freed;
	pr_info("SHRINKER START (nr_to_scan %lu) \n", sc->nr_to_scan);
	
	freed = free_tm_list(sc->nr_to_scan);
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
/*
 * taskmonitr_store - sysfs strore function for kobj_attribute taskmonitor
 * Start/Stop kthread (stat_thread) gathering CPU statistics
 */
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
/*
 * taskmonitor_show - sysfs show function for kobj_attribute taskmonitor
 * Return only PAGE_SIZE bytes
 */
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
		//kref_get(&sample->kref);
		sprintf(tmp, "pid %d usr %llu sys %llu\n", target, sample->utime, sample->stime);
		//kref_put(&sample->kref, release_ts_kref);

		page_size -= strlen(tmp);
		if(page_size < 0)
			break;
		strcat(buf, tmp);
	}
	mutex_unlock(&task_m->mtx);
	
	pr_info("Total stats: %llu \n", task_m->samples);
	return strlen(buf);
}
/*
 * Return CPU statistics in sample argument 
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
	INIT_LIST_HEAD(&task_m->tasks);
	task_m->pid = find_get_pid(p);
	mutex_init(&task_m->mtx);
	task_m->samples = 0;
	
	if(!task_m->pid)
		return -1;		
	return 0;	
}
/*
 * Save CPU statistics in a linked list 
 */
static struct task_sample *save_sample(void)
{
	struct task_sample *sample;
	sample = mempool_alloc(task_sample_mempool, GFP_KERNEL);
	if(sample == NULL){
		pr_err("Error: save_sample::kmem_cache_alloc");
		return NULL;
	}
	//kref_init(&sample->kref);
	
	if(get_sample(task_m, sample)){
		mutex_lock(&task_m->mtx);
		list_add(&(sample->list), &(task_m->head));
		task_m->samples++;
		mutex_unlock(&task_m->mtx);
	}
	return sample;
}
/* 
 * Function executed by the thread, find the <struct task_strcut> corresponding to <struct task>
 * and retrieve some statistics of the given process(stime & utime). 
 */
static int monitor_fn(void *unused)
{
	struct task_sample *sample;
	
	while(!kthread_should_stop()){
		sample  = save_sample();
		
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

	/* Allocate memory for task_monitor structure */
	task_m = kmalloc(sizeof(struct task_monitor), GFP_KERNEL);	
	if(task_m == NULL){
		pr_err("kzalloc error");
		return -1;	
	}
	/* Create new cache of <task_sample> */
	task_sample_cache = KMEM_CACHE(task_sample, 0);
	if(task_sample_cache == NULL){
		pr_err("Error: kmem_cache_create");
		goto ERROR;
	}
	/* Create a memory pool for <task_sample> Cache  */
	task_sample_mempool = mempool_create(MEMPOOL_SIZE, mempool_alloc_slab, mempool_free_slab, task_sample_cache);
	if(task_sample_mempool == NULL){
		pr_err("Error: mempool_create");
		goto ERROR;
	}
	/* create sysfs attribute - /sys/kernel/taskmonitor */
	ret = sysfs_create_file(kernel_kobj, &(taskmonitor.attr));
	if(ret){
		pr_err("Error: sysfs_create_file");
		goto ERROR;
	}
	/* create debugfs attribue - /sys/kernel/debug/taskmonitor */
	taskmonitor_debugfs = debugfs_create_file("taskmonitor", 0666, NULL, NULL, &taskmonitor_fops);
	if(!taskmonitor_debugfs){
		pr_err("Error: debugfs_create_file\n");
		goto ERROR;
	}
	/* Get <struct pid> from <pid_t> */
	ret = monitor_pid(target);
	if(ret){
		pr_err("Error: monitor_pid: pid does not exist !!! \n");
		goto ERROR;
	}
	/* create and run thread */
	stat_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	/* Register shrinker */
	ret = register_shrinker(&tm_shrinker);
	if(ret){
		pr_err("Error: register_shrinker");
		goto ERROR;
	}
		
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
	/* Remove sysfs & debugfs */
	sysfs_remove_file(kernel_kobj, &(taskmonitor.attr));
	debugfs_remove(taskmonitor_debugfs);
	/* Destroy task_sample Cache */
	kmem_cache_destroy(task_sample_cache);
	/* Destroy task_sample Mempool */
	mempool_destroy(task_sample_mempool);
	/* Release task_monitor structure */
	free_tm_list(task_m->samples);
	put_pid(task_m->pid);
	kfree(task_m);
	/* Unregister shrinker */
	unregister_shrinker(&tm_shrinker);
	
	return;
} 

module_init(taskmonitor_init);
module_exit(taskmonitor_exit);
