#include<stdlib.h>
#include<stdio.h>

#include"version.h"

int is_unstable(struct version *v)
{
	return 1 & ((int *)v)[sizeof(unsigned short)];
}

int is_unstable_bis(struct version *v){
	return (v->minor%2 != 0);
}

void display_version(struct version *v)
{
	printf("%2u.%lu %s", v->major, v->minor,
			     is_unstable_bis(v) ? "(unstable)" : "(stable)  ");
}

int cmp_version(struct version *v, unsigned short major, unsigned long minor)
{
	return v->major == major && v->minor == minor;
}

struct commit *commit_of(struct version *version){
	return (struct commit*) (version - 16);
}

int same_major(struct version *v, unsigned short major){
	return v->major == major;
}