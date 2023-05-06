/* --------------------------------------------------------------------------
 * Copyright (C) 2023 Jorge Giner Cordero
 * This file is part of bas55 (ECMA-55 Minimal BASIC System).
 * License: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>
 * --------------------------------------------------------------------------
 */

/* Array descriptors. Filled by parse.c. Used by vm.c. */

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
