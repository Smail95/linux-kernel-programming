#include<stdlib.h>
#include<stdio.h>
#include <string.h>

#include"commit.h"

static int nextId;

/**
  * new_commit - alloue et initialise une structure commit correspondant aux
  *              parametres
  *
  * @major: numero de version majeure
  * @minor: numero de version mineure
  * @comment: pointeur vers une chaine de caracteres contenant un commentaire
  *
  * @return: retourne un pointeur vers la structure allouee et initialisee
  */
struct commit *new_commit(unsigned short major, unsigned long minor, char *comment)
{
	struct commit *com = NULL;
	com = (struct commit*)malloc(sizeof(struct commit));
	if(com == NULL){
		perror("malloc"); exit(-1);	
	}
	com->comment = NULL;
	com->next = NULL;
	com->prev  =NULL;
	
	//init Id
	com->id = nextId++;
	//init Comment
	com->comment = (char*)malloc(sizeof(char)*strlen(comment));
	if(com->comment == NULL){
		perror("malloc"); exit(-1);	
	}
	strcpy(com->comment, comment);
	//Init Version
	com->version.major = major;
	com->version.minor = minor;
	com->version.flags = 0;	
	
	//Init chainage 
	com->prev = com;
	com->next = com;
	
	return com;
}

/**
  * insert_commit - insere sans le modifier un commit dans la liste doublement
  *                 chainee
  *
  * @from: commit qui deviendra le predecesseur du commit insere
  * @new: commit a inserer - seuls ses champs next et prev seront modifies
  *
  * @return: retourne un pointeur vers la structure inseree
  */
static struct commit *insert_commit(struct commit *from, struct commit *new)
{
	new->next = from->next;
	new->prev = from;
	if(from->next != NULL)
		from->next->prev = new;
	from->next = new;
	return new;
}	

/**
  * add_minor_commit - genere et insere un commit correspondant a une version
  *                    mineure
  *
  * @from: commit qui deviendra le predecesseur du commit insere
  * @comment: commentaire du commit
  *
  * @return: retourne un pointeur vers la structure inseree
  */
struct commit *add_minor_commit(struct commit *from, char *comment)
{
	unsigned short major = from->version.major;
	unsigned long minor = from->version.minor + 1;
	
	struct commit *new = new_commit(major, minor,comment);
	new = insert_commit(from, new);
	return new;
}

/**
	* add_major_commit - genere et insere un commit correspondant a une version
  *                    majeure
  *
  * @from: commit qui deviendra le predecesseur du commit insere
  * @comment: commentaire du commit
  *
  * @return: retourne un pointeur vers la structure inseree
  */
struct commit *add_major_commit(struct commit *from, char *comment)
{
	unsigned short major = from->version.major + 1;
	unsigned long minor = 0;
	
	struct commit *new = new_commit(major, minor, comment);
	new = insert_commit(from, new);
	return new;
}

/**
  * del_commit - extrait le commit de l'historique
  *
  * @victim: commit qui sera sorti de la liste doublement chainee
  *
  * @return: retourne un pointeur vers la structure extraite
  */
struct commit *del_commit(struct commit *victim)
{
	victim->prev->next = victim->next;
	if(victim->next != NULL)
		victim->next->prev = victim->prev;
	return victim;
}

/**
  * display_commit - affiche un commit : "2:  0-2 (stable) 'Work 2'"
  *
  * @c: commit qui sera affiche
  */
void display_commit(struct commit *c)
{
		printf("  %lu:  ",c->id);
		display_version(&(c->version));
		printf("	 %s\n",c->comment);
}

/**
  * commitOf - retourne le commit qui contient la version passee en parametre
  *
  * @version: pointeur vers la structure version dont on recherche le commit
  *
  * @return: un pointeur vers la structure commit qui contient 'version'
  *
  * Note:      cette fonction continue de fonctionner meme si l'on modifie
  *            l'ordre et le nombre des champs de la structure commit.
  */
struct commit *commitOf(struct version *version)
{
	/* TODO : Exercice 2 - Question 2 */
	return NULL;
}
