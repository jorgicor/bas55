/*
Copyright (c) 2014 Jorge Giner Cordero

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

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>

// #include <stdlib.h>
// #include <string.h>
// #include <editline/readline.h>
// #include <readline/readline.h>
// #include <readline/history.h>

#include "ecma55.h"

static void pready (void)
{
	fputs("Ready.\n", stderr);
}

void edit(void)
{
	char line[LINE_MAX_CHARS + 1];
	char *start, *end;
	int lineno;
	size_t len;
	enum error_code ecode;

	print_version(stderr);
	fputs("\nType HELP for a list of allowed commands.\n\n", stderr);
	pready();
	while ((ecode = get_line("", line, sizeof(line), stdin)) != E_EOF) {
		if (ecode == E_LINE_TOO_LONG) {
			eprint(E_LINE_TOO_LONG);
			enl();
			continue;
		}

		/* skip space at the beginning */
		start = line;
		while (isspace(*start))
			start++;

		/* skip space at end */
		end = start;
		while (*end != '\0')
			end++;
		while (end > start && isspace(end[-1]))
			end--;

		/* empty line */
		if (end == start)
			continue;

		*end = '\0';

		if (isdigit(*start) && 
			chk_basic_chars(start, end - start, 1, &len) != 0)
		{
			eprint(E_INVAL_CHARS);
			fprintf(stderr, "(%c)", start[len]);
			enl();
			continue;
		}

		if (!isdigit(*start)) {
			/* Run command */
			parse_n_run_cmd(start);
			pready();
			continue;
		}

		toupper_str(start);

		switch (check_if_number(start)) {
		case NUM_TYPE_NONE:
		case NUM_TYPE_FLOAT:
			eprint(E_INVAL_LINE_NUM);
			enl();
			continue;
		default:
			;
		}

		lineno = parse_int(start, &len);
		if (lineno <= 0 || lineno > LINE_NUM_MAX) {
			eprint(E_INVAL_LINE_NUM);
			enl();
			continue;
		}

		start += len;
		if (*start != '\0' && !isspace(*start)) {
			eprint(E_SPACE_LINE_NUM);
			enl();
			continue;
		}

		/* skip space after line number */
		start++;

		/* If we only have a line number, delete the line;
		 * if not add or substitute.
		 */
		if (start >= end) {
			del_line(lineno);
		} else if (add_line(lineno, start, end) != 0) {
			eprint(E_NO_MEM);
			enl();
		}
	}
}
