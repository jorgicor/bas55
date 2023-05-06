/* --------------------------------------------------------------------------
 * Copyright (C) 2023 Jorge Giner Cordero
 * This file is part of bas55 (ECMA-55 Minimal BASIC System).
 * License: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>
 * --------------------------------------------------------------------------
 */

/* Editor mode. */

#include <config.h>
#include "ecma55.h"
#include <assert.h>
#include <ctype.h>
#include <limits.h>

static void pready (void)
{
	fputs("Ready.\n", stderr);
}

static void print_prologue(FILE *f)
{
	print_version(f);
	print_copyright(f);
	print_short_license(f);
	fputs("\nType HELP for a list of allowed commands.\n", stderr);
}


void edit(void)
{
	char line[LINE_MAX_CHARS + 1];
	char *start, *end;
	int lineno;
	size_t len;
	enum error_code ecode;

	print_prologue(stderr);
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
