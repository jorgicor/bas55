/*
Copyright (c) 2014 Jorge Giner Cordero

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LIST_H
#define LIST_H

/**
 * Adds 'node' after node 'prev'.
 * 'list' points to the head node of the list or it is NULL.
 * 'prev' must be NULL if 'list' is NULL.
 * 'node' can't be NULL.
 */
#define list_add(list, prev, node)				\
	do {							\
		if ((prev) == NULL) {				\
			(node)->next = list;			\
			list = (node);				\
		} else {					\
			(node)->next = (prev)->next;		\
			(prev)->next = (node);			\
		}						\
	} while (0);


/**
 * Deletes and frees 'node' from 'list.
 * 'prev' is the node before 'node' or NULL if 'node' is the
 * head of the list.
 */
#define list_free(list, prev, node)				\
	do {							\
		if ((prev) != NULL)				\
			(prev)->next = (node)->next;		\
		else						\
			list = (node)->next;			\
		free(node);					\
	} while (0);

/**
 * Deletes all nodes from list 'list'.
 */
#define list_free_all(list)					\
	while (list != NULL) {					\
		void *ptmp = list;				\
		list = list->next;				\
		free(ptmp);					\
	}

#endif
