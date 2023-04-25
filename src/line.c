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
 * BASIC source stored as lines.
 * ===========================================================================
 */

#include <config.h>
#include "ecma55.h"
#include "list.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* The list of all lines in the program, sorted by line number. */
struct basic_line *s_line_list = NULL;

/* Number of lines in the program. */
int s_line_list_size = 0;

/* If the program needs recompiling. Other modules can change. */
int s_program_ok = 0;

/* If lines have changed, have been added or deleted since the last save. */
int s_source_changed = 0;

/* Holds old line num and new line num for renumbering lines. */
struct line_renum {
	int old_num;
	int new_num;
};

/* Deletes a line with number 'line_num' number from 's_line_list', if exists. */
void del_line(int line_num)
{
	struct basic_line *p, *prev;

	prev = NULL;
	for (p = s_line_list; p != NULL; prev = p, p = p->next) {
		if (p->number == line_num) {
			s_program_ok = 0;
			s_source_changed = 1;
			s_line_list_size--;
			list_free(s_line_list, prev, p);
			break;
		}
	}
}

/**
 * Inserts a line in 's_line_list' if no line with line_num exists in the list.
 * If a line with that line number exists, it is replaced.
 * The line is inserted sorted by line number.
 *
 * 'start' is the start of the string. 'end' points to the last character of
 * the string plus 1.
 *
 * Returns 0 on success. E_NO_MEM if no memory.
 * Caller must guarantee that end - start is representable signed int range.
 */
enum error_code add_line(int line_num, const char *start, const char *end)
{
	struct basic_line *p, *prev, *nline;
	int len;

	/* TODO: check */
	len = (int) (end - start);
	if (iadd_overflows_int((int) (sizeof *nline + 1), len))
		return E_NO_MEM;

	if ((nline = malloc(sizeof *nline + len + 1)) == NULL)
		return E_NO_MEM;

	nline->number = line_num;
	nline->str = (char *) nline + sizeof *nline;
	memcpy(nline->str, start, len);
	nline->str[len] = '\0';

	prev = NULL;
	for (p = s_line_list; p != NULL; prev = p, p = p->next) {
		if (p->number > line_num) {
			goto add;
		} else if (p->number == line_num) {
			s_program_ok = 0;
			s_source_changed = 1;
			list_add(s_line_list, prev, nline);
			list_free(s_line_list, nline, p);
			return 0;
		}
	}

	/* Add at the end */
add:	if (s_line_list_size == INT_MAX) {
		goto nomem;
	}

	s_program_ok = 0;
	s_source_changed = 1;
	list_add(s_line_list, prev, nline);
	s_line_list_size++;
	return 0;

nomem:	free(nline);
	return E_NO_MEM;
}

/* Deletes all lines. */
void del_lines(void)
{
	if (s_line_list_size == 0)
	{
		return;
	}

	list_free_all(s_line_list);
	s_program_ok = 0;
	s_source_changed = 0;
	s_line_list_size = 0;
}

/* 1 if line number lineno is in the list. */
int line_exists(int lineno)
{
	struct basic_line *p;

	for (p = s_line_list; p != NULL; p = p->next) {
		if (p->number == lineno)
			return 1;
	}

	return 0;
}

/* 1 if line number lineno is greater than any in the list. */
int is_greatest_line(int lineno)
{
	struct basic_line *p;

	for (p = s_line_list; p != NULL; p = p->next) {
		if (p->number >= lineno)
			return 0;
	}

	return 1;
}

static int find_pattern(const char *s, const char *pat, int *pos)
{
	int i, j, sj;
	int found, match;

	for (sj = found = 0; s[sj] != '\0' && !found; sj++) {
		if ((sj > 0 && pat[0] == s[sj] && s[sj - 1] == ' ')
			|| (sj == 0 && pat[0] == s[sj]))
		{
			i = 0;
			j = sj;
			match = 1;
			while (s[j] != '\0' && pat[i] != '\0' && match) {
				if (pat[i] == ' ') {
					while (s[j] == ' ')
						j++;
					i++;
				} else if (pat[i] == s[j]) {
					i++;
					j++;
				} else {
					match = 0;
				}
			}
			if (match && pat[i] == '\0') {
				if (s[j] == ' ')
					found = 1;
				while (s[j] == ' ')
					j++;
			}
		}
	}

	if (found)
		*pos = j;
	return found;
}

static int find_jmp_list(const char *s, int *pos)
{
	int i;

	static const char *pattern[] = {
		"GO TO",
		"GO SUB",
		"THEN"
	};

	for (i = 0; i < NELEMS(pattern); i++) {
		if (find_pattern(s, pattern[i], pos))
			return 1;
	}

	return 0;
}

static void renum_line_list(char *d, struct line_renum *table, int nlines,
			    const char *s, int pos)
{
	int i, n, found;
	size_t len;

	memcpy(d, s, pos);
	d += pos;
	s += pos;
	while (isdigit(*s)) {
		n = parse_int(s, &len);
		if (errno != ERANGE && n > 0 && n <= LINE_NUM_MAX) {
			found = 0;
			i = 0;
			while (i < nlines && !found) {
				if (table[i].old_num == n)
					found = 1;
				else
					i++;
			}
			if (found) {
				sprintf(d, "%dL", table[i].new_num);
				while (*d != 'L')
					d++;
				s += len;
			} else while (isdigit(*s)) {
				*d++ = *s++;
			}
		} else while (isdigit(*s)) {
			*d++ = *s++;
		}		
		if (*s != ' ' && *s != ',')
			break;
		while (isspace(*s))
			*d++ = *s++;
		if (*s != ',')
			break;
		*d++ = *s++;
		while (isspace(*s))
			*d++ = *s++;
	}

	while (*s != '\0')
		*d++ = *s++;
	*d = '\0';
}

static char *renum_line(struct line_renum *table,
			int nlines,
			const char *str)
{
	char *nstr;
	int pos;

	if (!find_jmp_list(str, &pos)) {
		if ((nstr = malloc(strlen(str) + 1)) == NULL)
			return NULL;
		else
			return strcpy(nstr, str);
	} else {
		if ((nstr = malloc(LINE_NUM_MAX * 4)) == NULL)
			return NULL;
		renum_line_list(nstr, table, nlines, str, pos);
		return nstr;
	}
}

static enum error_code renum(struct line_renum *table,
			     int nlines,
			     struct basic_line *bline)
{
	int i;
	char *nstr;

	i = 0;
	while (bline != NULL) {
		if ((nstr = renum_line(table, nlines, bline->str)) == NULL)
		{
			del_lines();
			return E_NO_MEM;
		}
		if (add_line(table[i].new_num, nstr, nstr + strlen(nstr)) != 0) {
			del_lines();
			free(nstr);
			return E_NO_MEM;
		}
		free(nstr);
		i++;
		bline = bline->next;
	}

	return 0;
}

static void init_renum_table(struct line_renum *table,
			     int nlines,
			     struct basic_line *bline)
{
	int i, inc, n;

	if (nlines <= LINE_NUM_MAX / 10)
		inc = 10;
	else if (nlines <= LINE_NUM_MAX / 5)
		inc = 5;
	else if (nlines <= LINE_NUM_MAX / 2)
		inc = 2;
	else
		inc = 1;

	i = 0;
	n = inc;
	while (bline != NULL) {
		table[i].old_num = bline->number;
		table[i].new_num = n;
		i++;
		n += inc;
		bline = bline->next;
	}
}

enum error_code renum_lines(void)
{
	enum error_code ecode;
	struct line_renum *table;
	struct basic_line *backup_list;
	int nlines = s_line_list_size;

	nlines = s_line_list_size;
	table = (struct line_renum *) malloc(nlines * sizeof *table);
	if (table == NULL)
		return E_NO_MEM;

	init_renum_table(table, nlines, s_line_list);

	backup_list = s_line_list;
	s_line_list = NULL;
	s_line_list_size = 0;

	ecode = renum(table, nlines, backup_list);
	free(table);

	if (ecode == 0) {
		s_program_ok = 0;
		s_source_changed = 1;
		list_free_all(backup_list);
	} else {
		s_line_list = backup_list;
		s_line_list_size = nlines;
	}

	return ecode;
}
