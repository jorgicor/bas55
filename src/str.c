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
 * String constants and garbage collected dynamic strings appearing in the
 * BASIC program.
 * ===========================================================================
 */

#include <config.h>
#include "ecma55.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* Array of the strings that appear in the program or are installed
 * when reading input. */
struct refcnt_str **strings = NULL;

/* Capacity of the strings array. */
static int strings_len = 0;

/* Valid elements in strings. */
int nstrings = 0;

/* Number of constant strings. */
int nconst_strings = 0;

/**
 * Grows the strings array by some factor, at least by 1.
 * strings_len will increment.
 * If it cannot grow, returns E_NO_MEM and strings_len is unchanged.
 */
static int grow_strings(void)
{
	int new_len;
	struct refcnt_str **new_array;

	grow_array((void *) strings, (int) (sizeof *strings), strings_len,
		8, (void **) &new_array, &new_len);

	if (new_len == strings_len)
		return E_NO_MEM;

	strings = new_array;
	strings_len = new_len;
	return 0;
}

/**
 * Puts a string 'start' with len characters in the position i of strings.
 * Returns E_NO_MEM if no memory to allocate the slot.
 */
static int put_string(int i, const char *start, size_t len)
{
	struct refcnt_str *rcstr;
	char *str;

	/* SAFE: we store very short strings. */
	if ((rcstr = malloc(len + 1 + sizeof *rcstr)) == NULL)
		return E_NO_MEM;

	str = ((char *) rcstr) + sizeof *rcstr;
	memcpy(str, start, len);
	str[len] = '\0';
	rcstr->count = 0;
	rcstr->str = str;
	strings[i] = rcstr;
	return 0;
}

/**
 * Frees the strings array if not already freed.
 * nstrings will be 0.
 */
void free_strings(void)
{
	int i;

	if (strings == NULL)
		return;

	for (i = 0; i < nstrings; i++) {
		if (strings[i] != NULL)
			free(strings[i]);
	}
	free(strings);
	strings = NULL;
	nstrings = 0;
	strings_len = 0;
}

/**
 * Init the strings array with a default empty string on position 0.
 * Returns 0 on success and nstrings will be 1. E_NO_MEM if no memory.
 */
int init_strings(void)
{
	if (strings != NULL)
		free_strings();

	if (grow_strings() != 0)
		return E_NO_MEM;

	if (put_string(0, "", 0) != 0)
		return E_NO_MEM;

	nstrings = 1;
	return 0;
}

/**
 * Adds a string to the strings array.
 * Returns 0 on success, pos will be the position where inserted and nstrings
 * will be incremented by 1.
 * Returns E_NO_MEM if no memory.
 */
int add_string(const char *start, size_t len, int *pos)
{
	int i;
	int at_end;
	size_t j;
	const char *q;

	/* search an equal string */
	for (i = 0; i < nstrings; i++) {
		if (strings[i] == NULL)
			continue;
		q = strings[i]->str;
		for (j = 0; j < len; j++, q++) {
			if (*q != start[j])
				break;
		}
		if (j == len && *q == '\0') {
			*pos = i;
			return 0;
		}
	}

	/* look if there is any slot free before nstrings */
	for (i = 0; i < nstrings; i++) {
		if (strings[i] == NULL)
			break;
	}
		
	at_end = i == nstrings;
	if (at_end && nstrings == strings_len) {
		if (grow_strings() != 0)
			return E_NO_MEM;
	}

	if (put_string(i, start, len) != 0)
		return E_NO_MEM;

	*pos = i;
	nstrings += at_end;
	return 0;
}

void inc_string_refcount(int i)
{
	strings[i]->count++;
	//printf("String %d (%s) count %d\n", i, strings[i]->str,
	//	strings[i]->count);
}

void dec_string_refcount(int i)
{
	strings[i]->count--;
	//printf("String %d (%s) count %d\n", i, strings[i]->str,
	//	strings[i]->count);
	if (strings[i]->count == 0) {
		// printf("String deallocated\n");
		free(strings[i]);
		strings[i] = NULL;
	}
}

void set_string_refcount(int i, int n)
{
	strings[i]->count = n;
}

/* Mark the current number of strings added as constant strings.
 * When calling reset_strings, this strings will not be deallocated.
 */
void mark_const_strings(void)
{
	nconst_strings = nstrings;
}

/* Deallocates all strings except constant ones.
 * The const strings refcount is set to 1 except the "" which is set to
 * the total number of allowed string variables + 1, because it is as if
 * all variables start with it assigned.
 * nstrings is set to the number of constant strings.
 */
void reset_strings(void)
{
	int i;
	struct refcnt_str **na;

	set_string_refcount(0, N_VARNAMES + 1);
	for (i = 1; i < nconst_strings; i++)
		set_string_refcount(i, 1);
	for (i = nconst_strings; i < nstrings; i++)
		if (strings[i] != NULL) {
			free(strings[i]);
			strings[i] = NULL;
		}
	nstrings = nconst_strings;

	/* shrink array */
	if (nstrings < strings_len) {
		na = realloc((void *) strings, nstrings * sizeof *strings);
		if (na != NULL) {
			strings = na;
			strings_len = nstrings;
		}
	}
}
