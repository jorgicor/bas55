/* --------------------------------------------------------------------------
 * Copyright (C) 2023 Jorge Giner Cordero
 * License: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>
 * --------------------------------------------------------------------------
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
