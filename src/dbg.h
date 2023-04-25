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
 * Debug support for vm.c. Filled by parse.c.
 * ===========================================================================
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
