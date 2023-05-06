/* --------------------------------------------------------------------------
 * Copyright (C) 2023 Jorge Giner Cordero
 * This file is part of bas55 (ECMA-55 Minimal BASIC System).
 * License: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>
 * --------------------------------------------------------------------------
 */

/* Debug support for vm.c. Filled by parse.c. */

#include <config.h>
#include "dbg.h"
#include "ecma55.h"
#include <assert.h>
#include <stdlib.h>

/* Holds one bit for each ram position. This bit is set if the ram position has
 * been initialized.
 */
static unsigned char *s_inited_ram = NULL;

/* For each ram position generated for a variable, here we store for which
 * variable it is. This is always sorted by rampos.
 */
struct ram_var_pair {
	int rampos;
	int coded_var;
};

static struct ram_var_pair s_ram_var_map[N_VARNAMES * N_SUBVARS];

/* Valid positions in s_ram_var_map. */
static int s_ram_var_map_len;

void reset_ram_var_map(void)
{
	s_ram_var_map_len = 0;
}

/* Maps a starting ram position to a variable name. */
void set_ram_var_pos(int rampos, int coded_var)
{
	assert(s_ram_var_map_len < NELEMS(s_ram_var_map));
	s_ram_var_map[s_ram_var_map_len].rampos = rampos;
	s_ram_var_map[s_ram_var_map_len].coded_var = coded_var;
	s_ram_var_map_len++;
}

int alloc_inited_ram(int ramsize)
{
	int n;

	assert(s_inited_ram == NULL);
	if (ramsize > 0) {
		n = (ramsize >> 3) + ((ramsize & 7) > 0);
		if ((s_inited_ram = calloc(n, sizeof *s_inited_ram)) == NULL) {
			return -1;
		}
	}

	return 0;
}

void free_inited_ram(void)
{
	if (s_inited_ram != NULL) {
		free(s_inited_ram);
		s_inited_ram = NULL;
	}
}

/* Given a ram position, get the index in s_ram_var_map. */
static int ram_var_index(int rampos)
{
	int le, ri, m, n;

	n = s_ram_var_map_len;
	le = 0;
	ri = n; 
	while (le < ri) {
		m = (le + ri) >> 1;
		if (s_ram_var_map[m].rampos < rampos) {
			le = m + 1;
		} else {
			ri = m;
		}
	}
	if (ri == n) {
		ri--;
	} else if (rampos < s_ram_var_map[ri].rampos) {
		ri--;
	}

	assert(ri >= 0 && ri < n);
	return ri;
}

/* Returns a coded_var knowing its ram position or the ram position of one of
 * its elements if it is an array.
 */
int get_var_from_rampos(int rampos)
{
	int i;

	i = ram_var_index(rampos);
	return s_ram_var_map[i].coded_var;
}

void set_rampos_inited(int rampos)
{
	s_inited_ram[rampos >> 3] |= (1 << (rampos & 7)); 
}

int is_rampos_inited(int rampos)
{
	return s_inited_ram[rampos >> 3] & (1 << (rampos & 7));
}
