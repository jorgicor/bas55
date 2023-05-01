/* Command handling in editor mode.
 *
 * Copyright (C) 2023 Jorge Giner Cordero
 *
 * This file is part of bas55, an implementation of the Minimal BASIC
 * programming language.
 *
 * bas55 is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * bas55 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * bas55.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>
#include "ecma55.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define CMD_MAX_CHARS		8
#define MAX_PARSE_NERRORS	20

typedef void (*cmd_f)(struct cmd_arg *, int);

struct command {
	const char *name;
	cmd_f fun;
	unsigned char nargs;
	unsigned char nextra_args;
};

/* Debug mode. */
int s_debug_mode = 0;

static int retry_q(const char *str)
{
	static char linebuf[LINE_MAX_CHARS + 1];
	enum error_code ecode;
	const char *p;

	/* 0 invalid, 1 yes, 2 no */
	int op;

	get_line_set_question_mode(1);

	do {
		op = 0;
		ecode = get_line(str, linebuf, sizeof(linebuf), stdin);
		if (ecode == E_OK) {
			p = linebuf;
			while (isspace(*p)) {
				p++;
			}
			if (*p == 'y' || *p == 'Y') {
				op = 1;
			} else if (*p == 'n' || *p == 'N') {
				op = 2;
			}
			if (op != 0) {
				p++;
				while (isspace(*p)) {
					p++;
				}
				if (*p != '\0') {
					op = 0;
				}
			}
		} else if (ecode == E_LINE_TOO_LONG) {
			eprint(E_LINE_TOO_LONG);
			enl();
		} else {
			assert(ecode == E_EOF);
			exit(EXIT_SUCCESS);
		}
	} while (op == 0);

	get_line_set_question_mode(0);

	return (op == 1) ? 'Y' : 'N';
}

static void free_run_data(void)
{
	free_code();
	free_strings();
	free_data();
}

static void compile(void)
{
	int ecode;
	struct basic_line *bline;
	int stopped;
	
	s_program_ok = 0;
	ecode = 0;
	free_run_data();

	if ((ecode = init_strings()) != 0) {
		eprint(ecode);
		enl();
		return;
	}

	if ((ecode = init_parser()) != 0) {
		free_strings();
		eprint(ecode);
		enl();
		return;
	}

	stopped = 0;
	for (bline = s_line_list; bline != NULL; bline = bline->next) {
		if (get_parser_nerrors() >= MAX_PARSE_NERRORS) {
			stopped = 1;
			break;
		}
		compile_line(bline->number, bline->str);
	}

	if (!stopped)
		end_parsing();

	s_program_ok = get_parser_nerrors() == 0;
	free_parser();
	if (s_program_ok)
		mark_const_strings();
	else
		free_run_data();
}

static void compile_cmd(struct cmd_arg *args, int nargs)
{
	compile();
	if (s_program_ok) {
		fprintf(stderr, "Compiled %d instructions.\n",
			get_code_size());
	}
}

void run_cmd(struct cmd_arg *args, int nargs)
{
	if (!s_program_ok)
		compile();
	if (s_program_ok)
		run(get_parsed_ram_size(), get_parsed_base(),
			get_parsed_stack_size());
}

static void quit_cmd(struct cmd_arg *args, int nargs)
{
	if (s_source_changed) {
		if (retry_q("Discard current program? (y/n) ") == 'N')
			return;
	}
	exit(EXIT_SUCCESS);
}

static void new_cmd(struct cmd_arg *args, int nargs)
{
	if (s_source_changed) {
		if (retry_q("Discard current program? (y/n) ") == 'N')
			return;
	}
	del_lines();
}

/**
 * Parses a range. Valid syntax is:
 *   empty
 *   number
 *   number-
 *   -number
 *   number-number
 */
static int get_range(const char *s, int *a, int *b)
{
	size_t len;

	*a = 0;
	if (*s == '\0') {
		*b = INT_MAX;
		return 0;
	} else if (*s != '-' && !isdigit(*s)) {
		return E_SYNTAX;
	}

	len = 0;
	if (isdigit(*s)) {
		*a = parse_int(s, &len);
		if (errno == ERANGE)
			return E_INVAL_LINE_NUM;
	}

	s += len;
	if (*s == '\0') {
		*b = *a;
		return 0;
	}

	if (*s == '-')
		s++;
	else
		return E_SYNTAX;

	if (*s == '\0') {
		*b = INT_MAX;
		return 0;
	}

	if (!isdigit(*s))
		return E_SYNTAX;
	
	*b = parse_int(s, &len);
	if (errno == ERANGE)
		return E_INVAL_LINE_NUM;

	s += len;
	if (*s != '\0')
		return E_SYNTAX;

	return 0;
}

static void list_cmd(struct cmd_arg *args, int nargs)
{
	struct basic_line *p;
	int a, b, ecode;

	if (nargs == 0) {
		a = 0;
		b = INT_MAX;
	} else if ((ecode = get_range(args[0].str, &a, &b)) != 0) {
		eprint(ecode);
		enl();
		return;
	}

	for (p = s_line_list; p != NULL && p->number <= b; p = p->next) {
		if (p->number >= a) {
			printf("%d %s\n", p->number, p->str);
		}
	}
}

static void save(const char *fname)
{
	struct basic_line *p;
	FILE *fp;

	assert(fname != NULL);

	if (strlen(fname) == 0) {
		eprint(E_BAD_FNAME);
		enl();
		return;
	}

	if ((fp = fopen(fname, "r")) != NULL) {
		fclose(fp);
		if (retry_q("File already exists, overwrite? (y/n) ") == 'N')
			return;
	}

	if ((fp = fopen(fname, "w")) == NULL) {
		eprint(E_FOPEN);
		enl();
		return;
	}

	for (p = s_line_list; p != NULL; p = p->next)
		fprintf(fp, "%d %s\n", p->number, p->str);

	fclose(fp);

	s_source_changed = 0;
	fprintf(stderr, "Saved %s.\n", fname);
}

static void save_cmd(struct cmd_arg *args, int nargs)
{
	struct cmd_arg *arg0;
	char fname[LINE_MAX_CHARS + 1];

	arg0 = &args[0];
	if (arg0->len >= sizeof fname) {
		eprint(E_FNAME_TOO_LONG);
		enl();
		return;
	}

	copy_to_str(fname, arg0->str, arg0->len);	
	save(fname);
}

/*
 * Returns the number of errors or 0 if everything is ok.
 * max_errors is the maximum number of errors allowed, must be > 0.
 */
int load(const char *fname, int max_errors, int batch_mode)
{
	FILE *fp;
	size_t len, len2, chari;
	int lineno, linecnt;
	enum error_code ecode;
	int nerrors;
	char line[LINE_MAX_CHARS + 1];

	assert(fname != NULL);
	assert(max_errors > 0);

	del_lines();
	if ((fp = fopen(fname, "r")) == NULL) {
		if (batch_mode)
			eprogname();
		eprint(E_FOPEN);
		enl();
		return 1;
	}

	nerrors = 0;
	linecnt = 0;
	while ((ecode = get_line("", line, sizeof line, fp)) != E_EOF) {
		linecnt++;
		if (ecode == E_LINE_TOO_LONG) {
print_and_continue:	fprintf(stderr, "%s:", fname);
			eprintln(ecode, linecnt);
			enl();
new_error:		nerrors++;
			if (nerrors == max_errors) {
				break;
			} else {
				continue;
			}
		}

		switch (check_if_number(line)) {
		case NUM_TYPE_NONE:
		case NUM_TYPE_FLOAT:
			ecode = E_INVAL_LINE_NUM;
			goto print_and_continue;
		default:
			;
		}

		lineno = parse_int(line, &len);
		if (line[len] != ' ' && line[len] != '\t') {
			ecode = E_SPACE_LINE_NUM;
			goto print_and_continue;
		}

		if (lineno <= 0 || lineno > LINE_NUM_MAX) {
			ecode = E_INVAL_LINE_NUM;
			goto print_and_continue;
		}

		if (line_exists(lineno)) {
			ecode = E_DUP_LINE;
			goto print_and_continue;
		}

		if (!is_greatest_line(lineno)) {
			ecode = E_INVAL_LINE_ORDER;
			goto print_and_continue;
		}

		/* Skip first whitespace */
		len++;

		/* Find the end */
		len2 = len;
		while (line[len2] != '\0') {
			len2++;
		}

		/* Ignore trailing space */
		if (len2 > len && line[len2] == '\0') {
			len2--;
		}
		while (len2 > len && isspace(line[len2])) {
			len2--;
		}
		if (len == len2) {
			ecode = E_EMPTY_LINE;
			goto print_and_continue;
		}
		len2++;
		assert(len2 > len);

		if (chk_basic_chars(&line[len], len2 - len, 0, &chari) != 0) {
			fprintf(stderr, "%s:", fname);
			eprintln(E_INVAL_CHARS, linecnt);
			enl();
			fprintf(stderr, " %s\n", line);
			fprintf(stderr, " %*c\n", (int)(len + chari + 1), '^');
			//fprintf(stderr, "(%c)", line[len + chari]);
			goto new_error;
		}

		add_line(lineno, &line[len], &line[len2]);
	}

	if (nerrors > 0) {
		del_lines();
	}

	fclose(fp);
	return nerrors;
}

void load_cmd(struct cmd_arg *args, int nargs)
{
	struct cmd_arg *arg0;
	char fname[LINE_MAX_CHARS + 1];

	if (s_source_changed) {
		if (retry_q("Discard current program? (y/n) ") == 'N')
			return;
	}

	arg0 = &args[0];
	if (arg0->len >= sizeof fname) {
		eprint(E_FNAME_TOO_LONG);
		enl();
		return;
	}

	copy_to_str(fname, arg0->str, arg0->len);

	printf("%s\n", fname);
	load(fname, MAX_ERRORS, 0);
}

static void renum_cmd(struct cmd_arg *args, int nargs)
{
	if (renum_lines() != 0)
	{
		eprint(E_NO_MEM);
		enl();
	}
}

static const char *s_help[] = {
"RUN            Compile and run the current program.",
"COMPILE or C   Compile the current program.",
"LIST           List the program.",
"LIST N         List line N.",
"LIST A-B       List lines from A to B.",
"LIST -N        List lines from 1 to N.",
"LIST N-        List lines from N to the last.",
"LOAD \"FILE\"    Load a source program from FILE.",
"SAVE \"FILE\"    Save the current program to FILE.",
"NEW            Start a new program discarding the current one.",
"RENUM          Change the line numbers to be evenly spaces.",
"DEBUG ON/OFF   Use DEBUG ON to enable debug mode, DEBUG OFF to disable it.",
"SETGOSUB N     Allow for N GOSUB calls without RETURN.",
"QUIT           Quit the editor."
};

static void help_cmd(struct cmd_arg *args, int nargs)
{
	int i;

	for (i = 0; i < NELEMS(s_help); i++) {
		printf("%s\n", s_help[i]);
	}
}

static void set_gosub_cmd(struct cmd_arg *args, int nargs)
{
	int n;
	size_t len;

	if (!isdigit(args[0].str[0])) {
		eprint(E_SYNTAX);
		enl();
		return;
	}

	n = parse_int(args[0].str, &len);
	if (errno == ERANGE) {
		eprint(E_BIGNUM);
		enl();
		return;
	}

	set_gosub_stack_capacity(n);
}

/* Compares for equality a zero terminated string 'zts' with a non-zero
 * terminated string 'nzts' of length 'nzts_len', in case insensitive mode.
 */
static int strcmp_z_nz_ci(const char *zts, const char *nzts, size_t nzts_len)
{
	while (*zts != '\0' && nzts_len > 0 &&
	    toupper(*zts) == toupper(*nzts)) {
		zts++;
		nzts++;
		nzts_len--;
	}
	return (nzts_len == 0) && (*zts == '\0');
}

static void debug_cmd(struct cmd_arg *args, int nargs)
{
	if (nargs == 0) {
		printf("DEBUG MODE ");
		if (s_debug_mode) {
			printf("ON\n");
		} else {
			printf("OFF\n");
		}
	} else if (strcmp_z_nz_ci("ON", args[0].str, args[0].len)) {
		if (s_debug_mode == 0) {
			s_debug_mode = 1;
		       	s_program_ok = 0;
		}
	} else if (strcmp_z_nz_ci("OFF", args[0].str, args[0].len)) {
		if (s_debug_mode == 1) {
			s_debug_mode = 0;
		       	s_program_ok = 0;
		}
	} else {
		eprint(E_SYNTAX);
		enl();
	}
}

static const struct command s_commands[] = {
	{ "COMPILE", compile_cmd, 0, 0 },
	{ "C", compile_cmd, 0, 0 },
	{ "DEBUG", debug_cmd, 0, 1 },
	{ "HELP", help_cmd, 0, 0 },
	{ "LIST", list_cmd, 0, 1 },
	{ "LOAD", load_cmd, 1, 0 },
	{ "NEW", new_cmd, 0, 0 },
	{ "QUIT", quit_cmd, 0, 0 },
	{ "RENUM", renum_cmd, 0, 0 },
	{ "RUN", run_cmd, 0, 0 },
	{ "SAVE", save_cmd, 1, 0 },
	{ "SETGOSUB", set_gosub_cmd, 1, 0 },
};

/* Finds a command and returns the command index or -1 */
static int find_cmd(const char *str)
{
	int i;

	for (i = 0; i < NELEMS(s_commands); i++)
		if (strcmp(str, s_commands[i].name) == 0)
			return i;

	return -1;
}

static void parse_token(const char *str, size_t *start, size_t *tok_len,
    size_t *parse_len)
{
	size_t i;

	i = 0;
	while (isspace(str[i]))
		i++;

	if (str[i] == '\"') {
		i++;
		*start = i;
		while (str[i] != '\"' && str[i] != '\0')
			i++;
		*tok_len = i - *start;
		if (str[i] == '\"')
			i++;
	} else {
		*start = i;
		while (str[i] != '\0' && str[i] != '\"' && !isspace(str[i]))
				i++;
		*tok_len = i - *start;
	}

	while (isspace(str[i]))
		i++;
	*parse_len = i;
}

static int collect_args(struct cmd_arg *args, size_t nargs, size_t nextra_args,
    const char *from, size_t *ncollected)
{
	size_t start_offs, tok_len, parse_len;
	size_t i, n;

	i = 0;
	n = nargs + nextra_args;
	parse_token(from, &start_offs, &tok_len, &parse_len);
	while (tok_len != 0 && i < n) {
		args[i].str = from + start_offs;
		args[i].len = tok_len;
		from += parse_len;
		i++;
		parse_token(from, &start_offs, &tok_len, &parse_len);
	}

	if (tok_len == 0 && i >= nargs && i <= n) {
		*ncollected = i;
		return 0;
	} else {
		return E_SYNTAX;
	}
}

void parse_n_run_cmd(const char *str)
{
	int i;
	size_t start, tok_len, parse_len, collected;
	char name[CMD_MAX_CHARS + 1];
	struct cmd_arg args[2];

	parse_token(str, &start, &tok_len, &parse_len);
	copy_to_str(name, str + start, 
		min_size(tok_len, (size_t) CMD_MAX_CHARS));
	toupper_str(name);

	i = find_cmd(name);
	if (i < 0) {
		eprint(E_INVAL_CMD);
		enl();
		return;
	}

	if (collect_args(args, s_commands[i].nargs, s_commands[i].nextra_args,
		str + parse_len, &collected) != 0)
	{
		eprint(E_SYNTAX);
		enl();
		return;
	}

	s_commands[i].fun(args, collected);
}
