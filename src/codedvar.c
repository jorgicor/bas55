/*
Copyright (c) 2015 Jorge Giner Cordero

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ctype.h>
#include <stdio.h>

/* When parsing and when running the program in debug mode, we encode
 * the variables in an integer.
 * The least significant byte holds the suffix character: a number '0'..'9',
 * or a '$' or a '\0' for a variable without suffix.
 * The next byte holds the variable ASCII name: 'A', 'B', etc.
 */

/* var_name must be [A-Z] | [A-Z]$ | [A-Z][0-9] .
 * Returns an integer that represents the var name and type.
 */
int encode_var(const char *var_name)
{
	int code;

	code = ((int) var_name[0] << 8) | var_name[1];
	return code;
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
