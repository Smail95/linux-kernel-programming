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
	//Allouer un nouveau commit
	struct commit *com = NULL;
	com = (struct commit*)malloc(sizeof(struct commit));
	if(com == NULL){
		perror("malloc"); exit(-1);	
	}
	com->comment = NULL;	
	com->major_parent = com;
	com->operations.display = display_major_commit;
	com->operations.extract = extract_major;
	
	//init Id
	com->id = nextId++;
	//init Comment
	com->comment = (char*)malloc(sizeof(char)*strlen(comment)+1);
	if(com->comment == NULL){
		perror("malloc"); exit(-1);	
	}
	strcpy(com->comment, comment);
	//Init Version
	com->version.major = major;
	com->version.minor = minor;
	com->version.flags = 0;	
	
	//Init list
	INIT_LIST_HEAD(&com->list);
	//Init major_list
	INIT_LIST_HEAD(&com->major_list);
	
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
static struct commit *insert_minor_commit(struct commit *from, struct commit *new)
{
	//Chainage normal - add to commit.list
	list_add(&(new->list), &(from->list));
	new->major_parent = from->major_parent;	
	return new;
}	

static struct commit *insert_major_commit(struct commit *from, struct commit *new)
{
	//Chainage normal - add to commit.list
	list_add(&(new->list), &(from->list));
	
	//chainage major - add to commit.major_list
	if(from == from->major_parent){	//'from' is a major version
		list_add(&(new->major_list), &(from->major_list));
	}else{	//'from' is a minor version
		list_add(&(new->major_list),&(from->major_parent->major_list));	
	}	
	
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
	new = insert_minor_commit(from, new);
	
	//Init fonctions correspondante aux commits minor
	new->operations.display = display_commit;
	new->operations.extract = extract_minor;
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
	new = insert_major_commit(from, new);
	
	//Init fonctions correspondante aux commits major
	new->operations.display = display_major_commit;
	new->operations.extract = extract_major;
	
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
	return victim->operations.extract(victim);
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
  * display_commit - affiche un commit major : "### version 0 :  'First !' ####"
  *
  * @c: commit qui sera affiche
  */
void display_major_commit(struct commit *c)
{
	printf("  %lu:   ### ",c->id);
	printf("version %2u : ",c->version.major);
	printf("'%s' ####\n",c->comment);
	
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

/**
  * freeCommitList -libere les ressources occupées par la liste de commit
  *
  * @list: pointer vers la tete de liste
  *
  * @return: void
  */
void freeCommitList(struct commit *list){
	int offset_l = (void*)(&((struct commit*)0)->list) - (void*)((struct commit*)0);
	struct commit *tmp;
	
	struct list_head *head = &list->list;
	struct list_head *pos = head->next;
	while(pos != head){
		tmp = (struct commit*)((void*)pos - offset_l);
		pos = pos->next;
		freeCommit(tmp);
	}	
	freeCommit(list);
	return;
}

void freeCommit(struct commit *commit){
	free(commit->comment);
	free(commit);
}

/**
  * extract_major_commit -suppression du commit majeur et de l'ensemble des commits mineurs associés
  *
  *@victim: pointer vers le commit majeur
  *
  *@return: void
*/
struct commit* extract_major(struct commit *victim){
	struct commit tmp;
	int offset = (void*)&tmp.list - (void*)&tmp;
	struct commit *todel = NULL;
	struct list_head *head = &victim->list;
	struct list_head *pos = head;
	
	//Liberer la sous-liste 'victim->list'
	while(pos != pos->next){		
		todel = (struct commit*)((void*)pos - offset);	
		if(!same_major(&todel->version, victim->version.major ))
			break;
		pos = pos->next;		
		extract_minor(todel);		
		freeCommit(todel);
	}
	//se détacher de la liste 'victim->major_list'
	list_del(&victim->major_list);	
	return victim;
}

struct commit* extract_minor(struct commit *victim){
	list_del(&victim->list);
	return victim;
}

