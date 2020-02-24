#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#include"comment.h"

struct comment *new_comment(
	int title_size, char *title,
	int author_size, char *author,
	int text_size, char *text)
{
	struct comment *c = (struct comment *) malloc(sizeof(struct comment));
	struct comment *tmp = c;

	c->title_size = title_size;
	if(! (c->title = malloc(title_size))){
		goto END_NFREE;
	}
	memcpy(c->title, title, title_size);

	c->author_size = author_size;
	if(! (c->author = malloc(author_size))){		
		goto END_FREE_T;
	}
	memcpy(c->author, author, author_size);

	c->text_size = text_size;
	if(! (c->text = malloc(text_size))){
		goto END_FREE_T_A;
	}
	memcpy(c->text, text, text_size);
	
	return c;

END_FREE_T_A:
	free(c->author);	
END_FREE_T:
	free(c->title);
END_NFREE:
	return NULL;
}

void freeComment(struct comment **c){
	free((*c)->title);
	free((*c)->author);
	free((*c)->text);
	free(*c);
	*c = NULL;
}

void display_comment(struct comment *c)
{
	printf("%s from %s \"%s\"\n", c->title, c->author, c->text);
}
