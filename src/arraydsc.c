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
 * Array descriptors. Filled by parse.c. Used by vm.c.
 * ===========================================================================
 */

#include <config.h>
#include "arraydsc.h"
#include "ecma55.h"
#include <assert.h>
#include <string.h>

/* Here we store the array descriptors. For arrays we don't generate
 * instructions pointing to their ram positions, but rather to the array
 * descriptors. From here we know their dimensions and the base ram positions.
 */
struct array_desc s_array_descs[N_VARNAMES];

void reset_array_descriptors(void)
{
	memset(s_array_descs, 0, sizeof(s_array_descs));
}

void set_array_descriptor(int vindex, int rampos, int dim1, int dim2)
{
	assert(vindex >= 0 && vindex < N_VARNAMES);
	s_array_descs[vindex].rampos = rampos;
	s_array_descs[vindex].dim1 = dim1;
	s_array_descs[vindex].dim2 = dim2;
}
