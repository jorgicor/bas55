/* --------------------------------------------------------------------------
 * Copyright (C) 2023 Jorge Giner Cordero
 * License: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>
 * --------------------------------------------------------------------------
 */

/* Compiled DATA statements. */

#include <config.h>
#include "ecma55.h"
#include <stdlib.h>

struct data_datum {
	enum data_datum_type type;
	int i;		/* string index */
};

/* Store for each s_data elem we see in source */
static struct data_datum *s_data = NULL;

/* Number of elements in 's_data'. */
static int s_size = 0;

/* Actual size of 's_data'. */
static int s_capacity = 0;

/* Current index int 's_data'. */
static int s_ptr = 0;

/*
 * Frees 's_data'.
 * The internal index is reset. s_size will be 0.
 * s_capacity will be 0.
 */
void free_data(void)
{
	if (s_data != NULL) {
		free(s_data);
		s_data = NULL;
		s_size = 0;
		s_capacity = 0;
		s_ptr = 0;
	}
}

static int grow(void)
{
	struct data_datum *new_data;
	int new_cap;

	grow_array((void *)s_data, sizeof *s_data, s_capacity, 16,
		(void **) &new_data, &new_cap);

	if (new_cap == s_capacity)
		return E_NO_MEM;

	s_data = new_data;
	s_capacity = new_cap;
	return 0;
}

/* 
 * Adds a string index at the end of the 's_data'.
 * Returns E_NO_MEM if no memory.
 */
enum error_code add_data_str(int i, enum data_datum_type type)
{
	if (s_size == s_capacity && grow() != 0)
		return E_NO_MEM;

	s_data[s_size].type = type;
	s_data[s_size].i = i;
	s_size++;
	return 0;
}

/* Sets the index pointer to the first s_data element. */
void restore_data(void)
{
	s_ptr = 0;
}

/*
 * Reads the current s_data element as a string index and its type.
 * Returns E_INDEX_RANGE if there are no more elements.
 */
enum error_code read_data_str(int *i, enum data_datum_type *type)
{
	if (s_ptr >= s_size)
		return E_INDEX_RANGE;

	if (type != NULL)
		*type = s_data[s_ptr].type;
	if (i != NULL)
		*i = s_data[s_ptr].i;
	s_ptr++;
	return 0;
}
