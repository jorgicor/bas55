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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "ecma55.h"

/* Code segment. */
union instruction *code = NULL;

/* Capacity of code. */
static int s_capacity  = 0;

/* Current code size (number of instructions filled). */
static int s_size = 0;

/*
 * Adds an instruction to the end of 'code'.
 * Returns E_NO_MEM if not enough memory.
 */
enum error_code add_code_instr(union instruction instr)
{
	union instruction *new_code;
	int new_len;

	if (s_size == s_capacity) {
		grow_array((void *) code, (int) sizeof *code, s_capacity, 256,
			(void **) &new_code, &new_len);

		if (s_capacity == new_len)
			return E_NO_MEM;

		code = new_code;
		s_capacity = new_len;
	}

	code[s_size++] = instr;
	return 0;
}

/*
 * Frees the code segment memory.
 * The size of the code segment will be 0.
 */
void free_code(void)
{
	if (code == NULL)
		return;

	free(code);
	code = NULL;
	s_capacity = 0;
	s_size = 0;
}

/* Returns the current size of the code segment. */
int get_code_size(void)
{
	return s_size;
}

/* Changes the 'id' of the instruction at index i. */
void set_id_instr(int i, int id)
{
	assert(i < s_size);
	code[i].id = id;
}

