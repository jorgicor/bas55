/* --------------------------------------------------------------------------
 * Copyright (C) 2023 Jorge Giner Cordero
 * License: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>
 * --------------------------------------------------------------------------
 */

/* Utilities for encoding a variable name as an integer. */

#include <config.h>
#include "ecma55.h"
#include <ctype.h>

/* When parsing and when running the program in debug mode, we encode
 * the variables in an integer.
 * The least significant byte holds the suffix character: a number '0'..'9',
 * or a '$' or a '\0' for a variable without suffix.
 * The next byte holds the variable ASCII name: 'A', 'B', etc.
 */

/* letter is 'A' to 'Z', suffix is '\0', '$' or '0'-'9'. */
int encode_var2(char letter, char suffix)
{
	int code;

	code = ((int) letter << 8) | suffix;
	return code;
}

/* var_name must be [A-Z] | [A-Z]$ | [A-Z][0-9] .
 * Returns an integer that represents the var name and type.
 */
int encode_var(const char *var_name)
{
	return encode_var2(var_name[0], var_name[1]);
}

/* If coded_var is of string type. */
int is_strvar(int coded_var)
{
	return (coded_var & 255) == '$';
}

/* If coded_var is of numeric type. */
int is_numvar(int coded_var)
{
	return !is_strvar(coded_var);
}

/* If coded_var is of of numeric type with digit (A0, E5, etc). */
int is_numvar_wdigit(int coded_var)
{
	return isdigit(coded_var & 255);
}

/* Returns the ASCII letter of a coded variable. */
int get_var_letter(int coded_var)
{
	return coded_var >> 8;
}

/* Returns the ASCII suffix a coded variable. It can be '0'..'9', or
 * '$' for a string variable or '\0' for a variable without suffix.
 */
int get_var_suffix(int coded_var)
{
	return coded_var & 255;
}

void print_var(FILE *f, int coded_var)
{
	fprintf(f, "%c", (char) get_var_letter(coded_var));
	if (!is_numvar(coded_var) || is_numvar_wdigit(coded_var))
		fprintf(f, "%c", (char) get_var_suffix(coded_var));
}

/* Given a coded_var, maps the variable name to an index.
 * 0 is A, 1 is B, etc.
 */
int var_index1(int coded_var)
{
	return (coded_var >> 8) - 'A';
}

/* Given a coded_var, returns its variable suffix as an index.
 * 0 - 9 for variable suffixes 0 to 9.
 * 10 if the variable does not have a suffix.
 * 11 for a string variable (suffix is $).
 */
int var_index2(int coded_var)
{
	int i;
	
	i = coded_var & 255;
	if (i == '\0')
		return 10;
	else if (i == '$')
		return 11;
	else
		return i - '0';
}
