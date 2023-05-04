/* --------------------------------------------------------------------------
 * Copyright (C) 2023 Jorge Giner Cordero
 * License: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>
 * --------------------------------------------------------------------------
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
