#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include "taskmonitor.h"


int main(int argc, char **argv)
{
	int fd;
	struct task_sample_char sample_char;
	struct task_sample sample_struct;
	struct command cmd;
	
	if(argc < 2){
		fprintf(stderr,"Args required !\n");
		exit(-1);	
	}
	
	if((fd = open("/dev/taskmonitor", O_RDWR)) == -1){
		perror("open");
		exit(-1);	
	}		
	/* Test Command GET_SAMPE with buffer */
	if(ioctl(fd, GET_SAMPLE_1, (void*)&sample_char) == -1){
		perror("ioctl");
		goto ERROR;
	}				
	printf("Simle Task (char):\n  %s \n", sample_char.message);
	
	int pid = atoi(argv[1]);
	/* Test Commande SET_PID */
	if(ioctl(fd, TASKMON_SET_PID, (void*)&pid) == -1){
		perror("ioctl");
		goto ERROR;
	}

	/* Test Command GET_SAMPLE with structure */
	if(ioctl(fd, GET_SAMPLE_2, (void*)&sample_struct) == -1){
		perror("ioctl");
		goto ERROR;
	}
	printf("Simple Taks (struct):\n  pid %d usr %llu sys %llu\n", sample_struct.pid, sample_struct.utime, sample_struct.stime);
	
	/* Test Commands START/STOP */
	if(ioctl(fd, TASKMON_START, NULL) == -1){
		perror("ioctl");
		goto ERROR;
	}
	if(ioctl(fd, TASKMON_STOP, NULL) == -1){
		perror("ioctl");
		goto ERROR;
	}
	if(ioctl(fd, TASKMON_STOP, NULL) == -1){
		perror("ioctl");
		goto ERROR;
	}
	if(ioctl(fd, TASKMON_START, NULL) == -1){
		perror("ioctl");
		goto ERROR;
	}	

	close(fd);
	return 0;
ERROR:
	close(fd);
	exit(-1);
}