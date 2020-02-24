#include<stdlib.h>
#include<stdio.h>
#include <string.h>

#include"history.h"

/**
  * new_history - alloue, initialise et retourne un historique.
  *
  * @name: nom de l'historique
  */
  
struct history *new_history(char *name)
{
	struct history *his = NULL;
	his = (struct history*)malloc(sizeof(struct history));
	if(his == NULL){
		perror("malloc"); exit(-1);	
	}
	his->name = NULL;
	his->commit_list = NULL;
	
	//Init Commit_count
	his->commit_count = 0;
	//Init Commit_name
	his->name = (char*)malloc(sizeof(name));
	if(his->name == NULL){
		perror("malloc"); exit(-1);	
	}
	strcpy(his->name, name);
	//Init Commit_list
	his->commit_list = new_commit(0,0,"DO NOT PRINT ME !!!");
	
	return his;
}

/**
  * last_commit - retourne l'adresse du dernier commit de l'historique.
  *
  * @h: pointeur vers l'historique
  */
struct commit *last_commit(struct history *h)
{
	return h->commit_list->next;
}

/**
  * display_history - affiche tout l'historique, i.e. l'ensemble des commits de
  *                   la liste
  *
  * @h: pointeur vers l'historique a afficher
  */
void display_history(struct history *h)
{
	printf("Historique de '%s' : \n", h->name);
	struct commit *tmp = h->commit_list->next;
	while(tmp != h->commit_list){
		display_commit(tmp);
		tmp = tmp->next;
	}
}

/**
  * infos - affiche le commit qui a pour numero de version <major>-<minor> ou
  *         'Not here !!!' s'il n'y a pas de commit correspondant.
  *
  * @major: major du commit affiche
  * @minor: minor du commit affiche
  */
void infos(struct history *h, int major, unsigned long minor)
{
	struct commit *tmp = h->commit_list->next;
	while(tmp != h->commit_list){
		if(cmp_version(&tmp->version, major, minor)){
			display_commit(tmp);
			return;
		}			
		tmp = tmp->next;	
	}	
	printf(" Not here !!!\n");
	return;
}
