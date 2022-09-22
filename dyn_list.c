#include "dyn_list.h"
#include <stdlib.h>
#include <stdio.h>

/* 
	Initializes dynamic list with a initial capacity of four.
*/
void init_dyn_list(struct dyn_list *l) {
	l->list = malloc(4 * sizeof(void *));
	l->length = 0;
	l->capacity = 4;
};

/* 
	Adds pointer to dynamic list (resizing if necessary).
*/
void dyn_list_add(struct dyn_list *l, void *data) {
	if (l->length == l->capacity) {
		l->capacity <<= 1;
		l->list = realloc(l->list, l->capacity * sizeof(void *));
	}
	l->list[l->length] = data;
	++l->length;
}