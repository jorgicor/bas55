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
 * Compiled bytecode representing the BASIC program.
 * ===========================================================================
 */

#include <config.h>
#include "ecma55.h"
#include <assert.h>
#include <stdlib.h>

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

