/* --------------------------------------------------------------------------
 * Copyright (C) 2023 Jorge Giner Cordero
 * This file is part of bas55 (ECMA-55 Minimal BASIC System).
 * License: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>
 * --------------------------------------------------------------------------
 */

#ifndef DBG_H
#define DBG_H

void reset_ram_var_map(void);
void set_ram_var_pos(int rampos, int coded_var);
int get_var_from_rampos(int rampos);

int alloc_inited_ram(int ramsize);
void free_inited_ram(void);
void set_rampos_inited(int rampos);
int is_rampos_inited(int rampos);

#endif
