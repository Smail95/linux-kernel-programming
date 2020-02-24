#ifndef _TASKMONITOR_H_
#define _TASKMONITOR_H_

#define FREQ		HZ/1000000		/* Sampling frequence */
#define MEMPOOL_SIZE	10		/* The minimum number of elements guaranteed to be allocated for task_sample_mempool*/
	
/* Functions prototypes */
ssize_t taskmonitor_show(struct kobject *kobj, struct kobj_attribute *attr,char *buf);
ssize_t taskmonitor_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static int monitor_fn(void *unused);
static unsigned long tm_shrink_count(struct shrinker *, struct shrink_control *sc);
static unsigned long tm_shrink_scan(struct shrinker *, struct shrink_control *sc);
static int taskmonitor_open(struct inode *inode, struct file *file);
static void * taskmonitor_seq_start(struct seq_file *seq, loff_t *pos);
static void  taskmonitor_seq_stop(struct seq_file *m, void *v);	
static void * taskmonitor_seq_next(struct seq_file *m, void *v, loff_t *pos);
static int taskmonitor_seq_show (struct seq_file *m, void *v);

/* Global variables & structures */
struct task_monitor {
	struct list_head head;
	struct pid *pid;
	u64 samples;
	struct mutex mtx;
};
struct task_sample {
	struct list_head list;
	struct kref kref;
	u64 utime;
	u64 stime;
};
static struct shrinker tm_shrinker = {
	.count_objects		= tm_shrink_count,
	.scan_objects 		= tm_shrink_scan,
	.seeks			= DEFAULT_SEEKS 
};

static const struct seq_operations taskmonitor_seq_ops = {
	.start		= taskmonitor_seq_start,
	.next		= taskmonitor_seq_next,
	.stop		= taskmonitor_seq_stop,
	.show		= taskmonitor_seq_show
};

static const struct file_operations taskmonitor_fops = {
	.owner		= THIS_MODULE,
	.open 		= taskmonitor_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release 	= seq_release
};

#endif /*_TASKMONITOR_H_*/
