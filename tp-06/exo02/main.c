#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include "helloioctl.h"


int main(int argc, char **argv)
{
	int fd;
	struct msg m;
	
	if(argc < 2){
		fprintf(stderr, "Args required !\n");
		exit(-1);
	}	
	
	if((fd = open("/dev/helloioctl", O_RDWR)) == -1){
		perror("open");
		exit(-1);	
	}		
	
	if(ioctl(fd, HELLO, (void*)&m) == -1){
		perror("ioctl");
		goto ERROR;
	}				
	printf("Before Message: %s \n", m.message);

	strcpy(m.message, argv[1]);
	if(ioctl(fd, WHO, (void*)&m) == -1){
		perror("ioctl");
		goto ERROR;
	}
	if(ioctl(fd, HELLO, (void*)&m) == -1){
		perror("ioctl");
		goto ERROR;
	}
	printf("After Message: %s \n", m.message);

	close(fd);
	return 0;
ERROR:
	close(fd);
	exit(-1);
}