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

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ecma55.h"
#include "grammar.h"

enum {
	/* RANDOMIZE is the largest */
	MAX_NAME_LEN = 9
};

struct keyword {
	const char *name;
	int value;
};

/* 
 * TAB is not a keyword in the standard, but we keep it here.
 * Carefult with that.
 */
static struct keyword s_keywords[] =
{
	{ "BASE", BASE },
	{ "DATA", DATA },
	{ "DEF", DEF },
	{ "DIM", DIM },
	{ "END", END },
	{ "FOR", FOR },
	{ "GO", GO },
	{ "GOSUB", GOSUB },
	{ "GOTO", GOTO },
	{ "IF", IF },
	{ "INPUT", INPUT },
	{ "LET", LET },
	{ "NEXT", NEXT },
	{ "ON", ON },
	{ "OPTION", OPTION },
	{ "PRINT", PRINT },
	{ "RANDOMIZE", RANDOMIZE },
	{ "READ", READ },
	{ "REM", REM },
	{ "RESTORE", RESTORE },
	{ "RETURN", RETURN },
	{ "STEP", STEP },
	{ "STOP", STOP },
	{ "SUB", SUB },
	{ "TAB", TAB },
	{ "THEN", THEN },
	{ "TO", TO }
};

/**
 * Working scanning pointer.
 */
static const char *s_input_p;

/* If we are in a DATA statement. */
static int s_in_data;

static int get_keyword(const char *name)
{
	int i;

	for (i = 0; i < NELEMS(s_keywords); i++)
		if (strcmp(name, s_keywords[i].name) == 0)
			return s_keywords[i].value;

	return -1;
}

static int spc_must_follow_keyw(int keyw)
{
	if (keyw == TAB)
		return 0;
	else
		return 1;
}

void set_lex_input(const char *str)
{
	s_input_p = str;
	s_in_data = 0;
}

static void skip_rest(void)
{
	while (*s_input_p != '\0')
		s_input_p++;
}

/* Parses a element in a DATA statement: number or unquoted string.
 * Returns the token, sets s_input_p to the character after the element,
 * and yylval.
 */
static int lex_parse_data_elem(void)
{
	enum data_elem_type t;
	union data_elem delem;
	size_t len;

	t = parse_data_elem(&delem, s_input_p, &len,
		DATA_ELEM_AS_UNQUOTED_STR);
	s_input_p += len;
	if (t == DATA_ELEM_EOF) {
		return 0;
	} else if (t == DATA_ELEM_COMMA) {
		return ',';
	} else if (t == DATA_ELEM_QUOTED_STR) {
		yylval.str.start = delem.str.start;
		yylval.str.len = delem.str.len;
		if (delem.str.start[delem.str.len] != '\"')
			cerror(E_STR_NOEND, 1);
		return QUOTED_STR;
	} else if (t == DATA_ELEM_INVAL_CHAR) {
		cerror(E_INVAL_CHARS, 0);
		fprintf(stderr, "(%c)\n", s_input_p[-1]);
		return INVAL_CHAR;
	} else {	/* (t == DATA_ELEM_UNQUOTED_STR) */
		yylval.str.start = delem.str.start;
		yylval.str.len = delem.str.len;
		return STR;
	}
}

/* Parses a number and sets s_input_p after the number and returns the token
 * for the number.
 * If it was not a number, returns the first character as token and s_input_p
 * is set after that character.
 */
static int lex_parse_num(void)
{
	enum num_type t;
	size_t len;

	t = check_if_number(s_input_p);
	if (t == NUM_TYPE_NONE) {
		return *s_input_p++; 
	} else {
		if (t == NUM_TYPE_INT) {
			yylval.num.i = parse_int(s_input_p, &len);
			if (errno == ERANGE) {
				yylval.num.d = parse_double(s_input_p, NULL);
			} else {
				yylval.num.d = (double) yylval.num.i;
			}
		} else {
			yylval.num.d = parse_double(s_input_p, &len);
		}
		s_input_p += len;
		if (errno == ERANGE) {
			cwarn(E_CONST_OVERFLOW);
			enl();
		}
		return (t == NUM_TYPE_INT) ? INT : NUM;
	}
}

/* Parses an identifier. *s_input_p must point to a letter. */
static int lex_parse_id(void)
{
	int i;
	char name[MAX_NAME_LEN + 1];

	i = 0;
	while (isalnum(*s_input_p) || *s_input_p == '$') {
		if (i < MAX_NAME_LEN)
			name[i++] = *s_input_p;
		s_input_p++;
	}
	name[i] = '\0';

	if (name[1] == '\0' || (isdigit(name[1]) && name[2] == '\0')) {
		yylval.i = encode_var(name);
		return NUMVAR;
	} else if (name[1] == '$' && name[2] == '\0') {
		yylval.i = encode_var(name);
		return STRVAR;
	} else if (name[0] == 'F' && name[1] == 'N' && isalpha(name[2]) &&
	    name[3] == '\0') {
		yylval.i = name[2];
		return USRFN;
	}
	
	if ((i = get_keyword(name)) != -1) {
		if (spc_must_follow_keyw(i) && *s_input_p != '\0' &&
			!isspace(*s_input_p))
	       	{
			cerror(E_KEYW_SPC, 0);
			fprintf(stderr, "(%s)\n", name);
		}
		if (i == REM)
			skip_rest();
		else if (i == DATA)
			s_in_data = 1;
		return i;
	}

	if ((yylval.i = get_internal_fun(name)) != -1)
		return IFUN;

	return BAD_ID;
}

static int lex_parse_quoted_str(void)
{
	size_t len;

	s_input_p++;
	parse_quoted_str(s_input_p, &len);

	yylval.str.start = s_input_p;
	yylval.str.len = len;

	s_input_p += len;
	if (*s_input_p == '\0') {
		/* TODO: cur line num */
		cerror(E_STR_NOEND, 1);
	} else {
		/* skip ending quote */
		s_input_p++;
	}
	return QUOTED_STR;
}

static int is_basic_char(int c, int ignore_case)
{
	/* Assumes ASCII */
	if (c >= ' ' && c <= '?')
		return 1;
	else if ((c >= 'A' && c <= 'Z')
		|| (ignore_case && c >= 'a' && c <= 'z'))
	{
		return 1;
	}
	else if (c == '^' || c == '_')
		return 1;
	else
		return 0;
}

/**
 * Returns 0 if the string contains no bad characters.
 * Returns E_INVAL_CHARS if any bad character found; *index will be
 * the index in 's' of that character.
 */
int chk_basic_chars(const char *s, size_t len, int ignore_case, size_t *index)
{
	size_t i;

	for (i = 0; i < len; i++) {
		if (!is_basic_char(s[i], ignore_case)) {
			if (index != NULL)
				*index = i;
			return E_INVAL_CHARS;
		}
	}

	return 0;
}

int yylex(void)
{
	int c;
	int t;

again:
	c = *s_input_p;
	while (isspace(c))
		c = *++s_input_p;

	if (c == '\0') {
		return 0;
	} else if (c == '\"') {
		return lex_parse_quoted_str();
	} else if (s_in_data) {
		if ((t = lex_parse_data_elem()) == INVAL_CHAR)
			goto again;
		return t;
	} else if (c == '.' || isdigit(c)) {
		return lex_parse_num();
	} else if (isalpha(c)) {
		return lex_parse_id();
	} else if (c == '<' && s_input_p[1] == '=') {
		s_input_p += 2;
		return LESS_EQ;
	} else if (c == '<' && s_input_p[1] == '>') {
		s_input_p += 2;
		return NOT_EQ;
	} else if (c == '>' && s_input_p[1] == '=') {
		s_input_p += 2;
		return GREATER_EQ;
	} else {
		s_input_p++;
		return c;
	}
}
