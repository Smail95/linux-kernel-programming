#ifndef _TASKMONITOR_H_
#define _TASKMONITOR_H_

#define FREQ		HZ*2		/* Sampling frequence */
#define TS_MEMPOOL_SIZE	10		/* Minimal mempool size of task_sample Cache */
#define TM_MEMPOOL_SIZE 5		/* Minimal mempool size of task_monitor Cache */
	
/* Functions prototypes */
void release_ts_kref(struct kref *kref);
static unsigned long free_tm_list(unsigned long tofree);
struct task_monitor *monitor_pid(pid_t p);
static struct task_sample *save_sample(struct task_monitor *);
static int monitor_fn(void *unused);
ssize_t taskmonitor_show(struct kobject *kobj, struct kobj_attribute *attr,char *buf);
ssize_t taskmonitor_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static int monitor_fn(void *unused);
static unsigned long tm_shrink_count(struct shrinker *, struct shrink_control *sc);
static unsigned long tm_shrink_scan(struct shrinker *, struct shrink_control *sc);
static int taskmonitor_open(struct inode *inode, struct file *file);
static void *taskmonitor_seq_start(struct seq_file *seq, loff_t *pos);
static void  taskmonitor_seq_stop(struct seq_file *m, void *v);	
static void *taskmonitor_seq_next(struct seq_file *m, void *v, loff_t *pos);
static int taskmonitor_seq_show (struct seq_file *m, void *v);
static ssize_t taskmonitor_write(struct file *file, const char __user *user_buf, size_t size, loff_t *ppos);
static int stop_monitor_pid(pid_t pid);

/* Global variables & structures */
struct task_monitor {
	struct list_head head; 		/* linked list head for struct task_sample */
	struct list_head list;		/* linked list node for struct task_monitor */
	struct mutex mtx;
	struct pid *pid;
	u64 samples;
	
};
struct task_sample {
	struct list_head list;	/* linked list node for struct task_sample  */
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
	.write		= taskmonitor_write,
	.llseek		= seq_lseek,	/* not used */
	.release 	= seq_release
};

#endif /*_TASKMONITOR_H_*/
