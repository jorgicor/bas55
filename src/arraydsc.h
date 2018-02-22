/* ===========================================================================
 * bas55, an implementation of the Minimal BASIC programming language.
 *
 * Array descriptors. Filled by parse.c. Used by vm.c.
 * ===========================================================================
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
