#ifndef _TASKMONITOR_H_
#define _TASKMONITOR_H_

#include <linux/types.h>

#define MSG_SIZE	50
struct task_sample_char {
	char message[MSG_SIZE];
};
struct task_sample {
	pid_t pid;
	__u64 utime;
	__u64 stime;
};
struct command{
	char cmd[10];
};

#define	IOC_MAGIC	'N'
#define GET_SAMPLE_1	_IOR(IOC_MAGIC, 0, struct task_sample_char)
#define GET_SAMPLE_2	_IOR(IOC_MAGIC, 1, struct task_sample)
#define TASKMON_STOP	_IO(IOC_MAGIC, 2)
#define TASKMON_START	_IO(IOC_MAGIC, 3)
#define TASKMON_SET_PID _IOW(IOC_MAGIC, 4, int)


#endif /*_TASKMONITOR_H_*/