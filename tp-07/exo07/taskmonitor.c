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
#include <linux/uaccess.h>
#include "taskmonitor.h"
	
MODULE_DESCRIPTION("Module \"Task Monitor\" pour noyau linux");
MODULE_AUTHOR("AIDER Smail, LIP6");
MODULE_LICENSE("GPL");


/* Module parameters */
static int target = 0;
module_param(target, int, 0000);
MODULE_PARM_DESC(target, "pid process");

static struct task_struct  *stat_thread;				/* kthread who execute monitor_fn function */
static struct dentry *taskmonitor_debugfs;				/* Task monitor file in debugfs */
static struct kobj_attribute taskmonitor = __ATTR_RW(taskmonitor);	/* Task monitor attribute in sysfs */
static struct list_head tasks;						/* Linked list head for struct task_monitor */
static struct kmem_cache *task_sample_cache;				/* Cache for task_sample structure */
static mempool_t *task_sample_mempool;					/* Memory pool for task_sample strucutre */
static struct kmem_cache *task_monitor_cache;				/* Cache for task_monitor structure */
static mempool_t *task_monitor_mempool;					/* Memory pool for task_monitor structure */


static unsigned long free_ts_list(struct task_monitor *task_m, unsigned long tofree);
/* --- SEQ_FILE OPERATIONS --- */

/*
 * Print the information pointed by the iterator
 */

static int taskmonitor_seq_show (struct seq_file *m, void *v)
{	
	return 0;
}

/*
 * Return the next object in the linked list, NULL if end
 */
static void * taskmonitor_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	return NULL;
}

/*
 * Release the mutex taken by start()
 */
static void  taskmonitor_seq_stop(struct seq_file *m, void *v)
{
	return;
}	

/* Return the first element of the list, NULL if empty or aleardy iterated 
 * Iterate over the task_monitor list and return the first object
 * Lock the task_monitor mutex
 */
 static void * taskmonitor_seq_start(struct seq_file *seq, loff_t *pos)
{
	return NULL;
}

/* --- FILE_OPERATIONS --- */

static int taskmonitor_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &taskmonitor_seq_ops);
}

static int stop_monitor_pid(pid_t pid)
{
	int freed = 0;
	struct list_head *pos;
	struct task_monitor *task_m = NULL;
	
	list_for_each(pos, &tasks){	
		task_m = container_of(pos, struct task_monitor, list);
		if(pid_nr(task_m->pid) == pid){
			freed = free_ts_list(task_m, task_m->samples);
			list_del(&task_m->list);
			pr_info("pid %d freed: %d\n", pid, freed);
			return freed;
		}
	}
	return -1;
}
static ssize_t taskmonitor_write(struct file *file, const char __user *user_buf, size_t size, loff_t *ppos)
{
	struct task_monitor *task_m;
	char buf[20];
	int pid = 1;
	if(copy_from_user(buf, user_buf, size) < 0){
		return -EFAULT;
	}
	buf[size] = '\0';
	//kstrtoint(buf+1, 0, &pid);
	
	/* Stop current kthread */
	if(stat_thread != NULL){
		kthread_stop(stat_thread);
		stat_thread = NULL;
	}
	if(buf[0] == '-'){ //STOP	
		pid = stop_monitor_pid(pid);
		if(pid)
			pr_err("Error: pid does not exist \n");
	}else{	//START
		task_m = monitor_pid(pid);
		if(task_m == NULL){
			pr_err("Error: monitor_pid\n");
			return -1;
		}
		list_add(&task_m->list, &tasks);
	}
	
	/* Run kthread of the first task_monitor struct */
	stat_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	if(IS_ERR(stat_thread)){
		pr_err("Error: kthread_run\n");
		return -1;
	}
	return size;
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
static unsigned long free_ts_list(struct task_monitor *task_m, unsigned long tofree)
{
	u8 offset = (void*)(&((struct task_sample*)0)->list) - (void*)((struct task_sample*)0);
	struct task_sample *sample;
	struct list_head *entry, *todel;
	u64 nr_to_scan = tofree;
	u64 freed = 0;

	mutex_lock(&task_m->mtx);
	
	todel = NULL;
	list_for_each(entry, &task_m->head){
		if(!nr_to_scan--) /* !!!!!!!!???? */
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

	return freed;	
}
static unsigned long free_tm_list(unsigned long tofree)
{
	struct list_head *pos;
	struct task_monitor *task_m;
	u64 tmp, freed  = 0;
	u64 nr_to_scan = tofree;
	
	list_for_each(pos, &tasks){
		task_m = container_of(pos, struct task_monitor, list);
		if(nr_to_scan == 0) //free all 
			nr_to_scan = task_m->samples;
		tmp = free_ts_list(task_m, nr_to_scan); 
		if(!task_m->samples){ //if task_monitor list is empty
			mempool_free(task_m, task_monitor_mempool);
		}
		freed += tmp; 
	}
	pr_info("Total freed: %llu\n", freed);
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
	struct list_head *pos;
	struct task_monitor *task_m;
	u64 count = 0;
	
	list_for_each(pos, &tasks){
		task_m = container_of(pos, struct task_monitor, list);
		mutex_lock(&task_m->mtx);
		count += task_m->samples;
		mutex_unlock(&task_m->mtx);
	}
	
	if(!count)
		return SHRINK_EMPTY;
	return count;
}
/*
 * taskmonitr_store - sysfs store function for kobj_attribute taskmonitor
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
	s16 page_size = PAGE_SIZE;
	struct list_head *pos, *tm_pos;
	struct task_sample *sample;
	struct task_monitor *task_m;
	u64 count = 0;
	char tmp[100];
	
	list_for_each(tm_pos, &tasks){
		task_m = container_of(tm_pos, struct task_monitor, list);
		mutex_lock(&task_m->mtx);
		
		list_for_each(pos, &task_m->head){
			sample = container_of(pos, struct task_sample, list);
			//kref_get(&sample->kref);
			sprintf(tmp, "pid %d usr %llu sys %llu\n", (int)pid_nr(task_m->pid), sample->utime, sample->stime);
			//kref_put(&sample->kref, release_ts_kref);

			page_size -= strlen(tmp);
			if(page_size < 0)
				break;
			strcat(buf, tmp);
		}
		count += task_m->samples;
		mutex_unlock(&task_m->mtx);
		
		if(page_size < 0)
			break;
	}
	pr_info("Total stats: %llu \n", count);
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
	
	task_pid = get_pid_task(tm->pid, PIDTYPE_PID);
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
 * Return initialized task_monitor structure 
 */
struct task_monitor *monitor_pid(pid_t p)
{	
	struct task_monitor *task_m;
	
	task_m = mempool_alloc(task_monitor_mempool, GFP_KERNEL);
	if(task_m == NULL){
		pr_err("Error: mempool_alloc\n");
		return NULL;
	} 
	task_m->pid = find_get_pid(p);
	if(!task_m->pid){
		pr_err("Error: find_get_pid\n");
		return NULL;
	}
	INIT_LIST_HEAD(&task_m->head);
	mutex_init(&task_m->mtx);
	task_m->samples = 0;
			
	return task_m;	
}
/*
 * Save CPU statistics in a linked list 
 */
static struct task_sample *save_sample(struct task_monitor *task_m)
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
	struct task_monitor *task_m;
	struct list_head *pos;
	
	/* iterate over task_monitor linked list and save stats for every struct */
	while(!kthread_should_stop()){
		list_for_each(pos, &tasks){
			task_m = container_of(pos, struct task_monitor, list);
			sample = save_sample(task_m);
		}
		/*sleep until timeout */		
		set_current_state(TASK_UNINTERRUPTIBLE);		
		schedule_timeout(FREQ);
	}	
	return 0;
}

/* Init & Exit */
static int __init taskmonitor_init(void)
{	
	struct task_monitor *task_m;
	int ret;
	pr_debug("INIT TASK MONITOR");

	/* Create new cache of <task_sample> */
	task_sample_cache = KMEM_CACHE(task_sample, 0);
	if(task_sample_cache == NULL){
		pr_err("Error: kmem_cache_create");
		return -1;
	}
	/* Create a memory pool for <task_sample> Cache  */
	task_sample_mempool = mempool_create(TS_MEMPOOL_SIZE, mempool_alloc_slab, mempool_free_slab, task_sample_cache);
	if(task_sample_mempool == NULL){
		pr_err("Error: mempool_create");
		goto ERROR1;
	}
	/* Create new cache of <task_monitor> */
	task_monitor_cache = KMEM_CACHE(task_monitor, 0);
	if(task_monitor_cache == NULL){
		pr_err("Error: kmem_cache_create");
		goto ERROR2;
	}
	/* Create a memory pool for <task_monitor> Cache */
	task_monitor_mempool = mempool_create(TM_MEMPOOL_SIZE, mempool_alloc_slab, mempool_free_slab, task_monitor_cache);
	if(task_monitor_mempool == NULL){
		pr_err("Error: mempool_create");
		goto ERROR3;
	}
	/* create sysfs attribute - /sys/kernel/taskmonitor */
	ret = sysfs_create_file(kernel_kobj, &(taskmonitor.attr));
	if(ret){
		pr_err("Error: sysfs_create_file");
		goto ERROR4;
	}
	/* create debugfs attribue - /sys/kernel/debug/taskmonitor */
	taskmonitor_debugfs = debugfs_create_file("taskmonitor", 0666, NULL, NULL, &taskmonitor_fops);
	if(!taskmonitor_debugfs){
		pr_err("Error: debugfs_create_file\n");
		goto ERROR5;
	}
	/* Get pid struct from pid_t */
	task_m = monitor_pid(target);
	if(task_m == NULL){
		pr_err("Error: monitor_pid\n");
		goto ERROR6;
	}
	/* Initialize linked list head of task_monitor */
	INIT_LIST_HEAD(&tasks);
	list_add(&task_m->list, &tasks);
	/* Run kthread of the first task_monitor struct */
	stat_thread = kthread_run(monitor_fn, NULL, "monitor_fn");
	if(IS_ERR(stat_thread)){
		pr_err("Error: kthread_run\n");
		goto ERROR6;
	}
	/* Register shrinker */
	ret = register_shrinker(&tm_shrinker);
	if(ret){
		pr_err("Error: register_shrinker");
		goto ERROR6;
	}
		
	return 0;
ERROR1:	
	kmem_cache_destroy(task_sample_cache);
	return -1;
ERROR2:
	kmem_cache_destroy(task_sample_cache);
	mempool_destroy(task_sample_mempool);
	return -1;
ERROR3:
	kmem_cache_destroy(task_sample_cache);
	mempool_destroy(task_sample_mempool);
	kmem_cache_destroy(task_monitor_cache);
	return -1;
ERROR4:
	kmem_cache_destroy(task_sample_cache);
	mempool_destroy(task_sample_mempool);
	kmem_cache_destroy(task_monitor_cache);
	mempool_destroy(task_monitor_mempool);
	return -1;
ERROR5:
	sysfs_remove_file(kernel_kobj, &(taskmonitor.attr));
	goto ERROR4;
ERROR6:
	sysfs_remove_file(kernel_kobj, &(taskmonitor.attr));
	debugfs_remove(taskmonitor_debugfs);
	goto ERROR4;
}

static void __exit taskmonitor_exit(void)
{	
	pr_debug("EXIT TASK MONITOR");
	
	/* Stop kthread */
	if(stat_thread != NULL)
		kthread_stop(stat_thread);
	/* free tasks : (0: all) */
	free_tm_list(0);
	/* Remove sysfs & debugfs */
	sysfs_remove_file(kernel_kobj, &(taskmonitor.attr));
	debugfs_remove(taskmonitor_debugfs);
	/* Destroy task_sample Cache & mempool */
	kmem_cache_destroy(task_sample_cache);
	mempool_destroy(task_sample_mempool);
	/* Destroy task_monitor  Cache & Mempool */
	kmem_cache_destroy(task_monitor_cache);
	mempool_destroy(task_monitor_mempool);
	/* Unregister shrinker */
	unregister_shrinker(&tm_shrinker);
	
	return;
} 

module_init(taskmonitor_init);
module_exit(taskmonitor_exit);
