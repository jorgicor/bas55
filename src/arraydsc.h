/* Array descriptors. Filled by parse.c. Used by vm.c.
 *
 * Copyright (C) 2023 Jorge Giner Cordero
 *
 * This file is part of bas55, an implementation of the Minimal BASIC
 * programming language.
 *
 * bas55 is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * bas55 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * bas55.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ARRAYDSC_H
#define ARRAYDSC_H

struct array_desc {
	int rampos;
	int dim1;
	int dim2;
};

/* Read-only access please. */
extern struct array_desc s_array_descs[];

void reset_array_descriptors(void);
void set_array_descriptor(int vindex, int rampos, int dim1, int dim2);

#endif
