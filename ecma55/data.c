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

#include <stdio.h>
#include <stdlib.h>
#include "ecma55.h"

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
