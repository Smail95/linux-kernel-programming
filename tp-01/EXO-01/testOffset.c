#include <stdio.h>
#include <stdlib.h>

#include "version.h"

int main(int argc, char *agrv[]){

	//Q1
	struct commit c = {
		.id = 1,
		.comment = NULL,
		.next = NULL,
		.prev = NULL,
		.version.major = 5,
		.version.minor = 3,
		.version.flags = 0	
	};
	
	c.comment = (char*)malloc(5*sizeof(char));
	if(c.comment == NULL){
		perror("malloc"); exit(-1);
	}
	c.next = (struct commit*)malloc(sizeof(struct commit));
	c.prev = (struct commit*)malloc(sizeof(struct commit));
	if(c.next == NULL || c.prev == NULL){
		perror("malloc"); exit(-2);	
	}
	
	printf("id(commit): %p\ncomment: %p\nversion: %p\nnext: %p\nprev: %p\n",
			&c.id, &c.comment, &c.version, &c.next, &c.prev);
	
	//Q2
	int dec = (int)&c.version  - (int)&c;
	printf("decalage : %d\n", dec );
	
	printf("@commit: %p\n", commit_of(&c.version));
	
	return 0;
}