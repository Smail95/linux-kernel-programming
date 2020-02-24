#ifndef VERSION_H
#define VERSION_H

struct version {
	unsigned short major;
	unsigned long minor;
	char flags;
};

struct commit{
	unsigned long id;
	char *comment;
	struct version version;	
	struct commit *next;
	struct commit *prev;
};


int is_unstable_bis(struct version *v);

int is_unstable(struct version *v);

void display_version(struct version *v);

int cmp_version(struct version *v, unsigned short major, unsigned long minor);

struct commit *commit_of(struct version *version);

#endif
