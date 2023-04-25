/* Copyright (C) 2023 Jorge Giner Cordero
 *
 * This file is part of bas55, an implementation of the Minimal BASIC
 * programming language.
 *
 * bas55 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * bas55 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * bas55. If not, see <https://www.gnu.org/licenses/>.
 */

/* ===========================================================================
 * Single linked list macros.
 * ===========================================================================
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
