#include<stdlib.h>
#include<stdio.h>

#include"history.h"
#include "list.h"

int main(int argc, char const *argv[])
{
	struct history *history = new_history("Tout une histoire");
	struct commit *tmp, *victim, *last;

	display_commit(last_commit(history));
	printf("\n");
	
	tmp = add_minor_commit(last_commit(history), "Work 1");
	tmp = add_minor_commit(tmp, "Work 2");
	victim = add_minor_commit(tmp, "Work 3");
	last = add_minor_commit(victim, "Work 4");
	display_history(history);

	del_commit(victim);
	freeCommit(victim);
	display_history(history);

	tmp = add_major_commit(last, "Realse 1");
	tmp = add_minor_commit(tmp, "Work 1");
	tmp = add_minor_commit(tmp, "Work 2");
	tmp = add_major_commit(tmp, "Realse 2");
	tmp = add_minor_commit(tmp, "Work 1");
	display_history(history);

	add_minor_commit(last, "Oversight !!!");
	//display_history(history);
	
	victim = add_major_commit(tmp,"Realse 3");	
	tmp = add_minor_commit(victim,"work 1");
	tmp = add_minor_commit(tmp,"work 2");	
	display_history(history);
	
	del_commit(victim);
	freeCommit(victim);
	display_history(history);
	
	printf("Recherche du commit 1.2 :   ");
	infos(history, 1, 2);
	printf("\n");

	printf("Recherche du commit 1.7 :   ");
	infos(history, 1, 7);
	printf("\n");

	printf("Recherche du commit 4.2 :   ");
	infos(history, 4, 2);
	printf("\n");
	
	//free
	freeHistory(history);
	free(history);
		
	return 0;
}
