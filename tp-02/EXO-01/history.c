#include<stdlib.h>
#include<stdio.h>
#include <string.h>

#include"history.h"
#include <string.h>

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
	his->name = (char*)malloc(strlen(name)+1);
	if(his->name == NULL){
		perror("malloc"); exit(-1);	
	}
	strcpy(his->name, name);
	//Init Commit_list
	his->commit_list = new_commit(0,0,"First !");
	
	return his;
}

/**
  * last_commit - retourne l'adresse du dernier commit de l'historique.
  *
  * @h: pointeur vers l'historique
  */
struct commit *last_commit(struct history *h)
{
	struct commit tmp;
	int offset = (void*)&tmp.list - (void*)&tmp;
	return (struct commit*) ((void*)h->commit_list->list.prev - offset);  
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
	struct commit tmp;
	int offset = (void*)&tmp.list - (void*)&tmp;  													//Methode 1
	//int offset =  (void*)(&((struct commit*)0)->list) - (void*)((struct commit*)0);	//Methode 2
	
	struct list_head *head = &h->commit_list->list;
	struct list_head *pos = NULL;
	struct commit *com  = (struct commit*)((void*)head - offset);
	
	com->operations.display(com);	
	list_for_each(pos, head){
		com  = (struct commit*)((void*)pos - offset);
		com->operations.display(com);
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
	int offset = (void*)(&((struct commit*)0)->list) - (void*)((struct commit*)0);
	struct list_head *head = &h->commit_list->list;
	struct list_head *pos = NULL;
	struct commit *com = NULL;

	list_for_each(pos, head){
		com = (struct commit*) ((void*)pos - offset);
		if(cmp_version(&com->version, major, minor)){
			com->operations.display(com);
			return;	
		}
	}
	printf(" Not here !!!\n");
	return;
}

/**
  * infos2 - affiche le commit qui a pour numero de version <major>-<minor> ou
  *          'Not here !!!' s'il n'y a pas de commit correspondant.
  *		  - utilise la liste 'list_major' pour trouver la bonne sous-liste.
  *
  * @major: major du commit affiche
  * @minor: minor du commit affiche
  */
void infos2(struct history *h, int major, unsigned long minor){
	int offset_ml = (void*)(&((struct commit*)0)->major_list) - (void*)((struct commit*)0);
	int offset_l = (void*)(&((struct commit*)0)->list) - (void*)((struct commit*)0);
	struct list_head *head = &h->commit_list->major_list;
	struct list_head *pos = NULL;
	struct commit *com;
	
	//parcourir la liste 'major_list'pour trouver la version 'major'
	list_for_each(pos, head){
		com = (struct commit*)((void*)pos - offset_ml);
		if(same_major(&com->version, major)){
			if(cmp_version(&com->version, major, minor)){
				display_commit(com);
				return;				
			}
			//parcourir la sous-liste 'list' pour trouver la version 'minor'	
			head = &((struct commit*)((void*)pos - offset_ml))->list;
			list_for_each(pos, head){
				com = (struct commit*)((void*)pos - offset_l);
				if(cmp_version(&com->version, major, minor)){
					display_commit(com);
					return;				
				}
				//Arreter la recherche de qu'on atteind la prochaine version 'major'(via 'list')
				if(!same_major(&com->version, major)){
					head = pos->next; break;
				}
			}/*list_for_each*/
		}
	}/*list_for_each*/
	printf(" Not here !!!\n ");
	return;
}

void freeHistory(struct history *h){
	free(h->name);
	freeCommitList(h->commit_list);
	return;
}
