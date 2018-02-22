/* ===========================================================================
 * bas55, an implementation of the Minimal BASIC programming language.
 *
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
