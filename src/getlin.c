/* --------------------------------------------------------------------------
 * Copyright (C) 2023 Jorge Giner Cordero
 * License: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>
 * --------------------------------------------------------------------------
 */

/* Get a line from a file or console. Line editing support. */

#include <config.h>
#include "ecma55.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_LIBREADLINE) || defined(HAVE_LIBEDIT)
	#define LINE_EDITING 1
#else
	#define LINE_EDITING 0
#endif

static int s_question_mode;

#if LINE_EDITING
extern char *readline(const char *);
extern char *rl_line_buffer;
extern int rl_point;
extern int rl_end;
extern char *(*rl_completion_entry_function)(const char *, int);
extern char *rl_filename_completion_function(const char *, int);
extern void add_history(const char *);
#else
static char rl_line_buffer[] = "";
static int rl_point, rl_end;
static char *(*rl_completion_entry_function)(const char *, int) = NULL;

static char *readline(const char *prompt)
{
	return NULL;
}

static void add_history(const char *line)
{
}

static char *rl_filename_completion_function(const char *text, int state)
{
	return NULL;
}
#endif

/*
 * Gets a line from the file and stores it in buf.
 * Up to maxlen - 1 characters are read and buf is always null terminated.
 * Returns E_OK if a complete line has been read.
 * E_LINE_TOO_LONG if the line was longer than maxlen - 1.
 * E_EOF if the end of file is reached; in this case no string is read.
 * maxlen must be greater than 0.
 * The newline character is NOT stored and not counted.
 */
static enum error_code std_get_line(char *buf, int maxlen, FILE *fp)
{
	int c, nread;
	enum error_code ecode;

	/* The buffer at least must have space for the '\0' */
	assert(maxlen > 0);
	assert(buf != NULL);
	assert(fp != NULL);

	nread = 0;
	for (;;) {
		if (nread >= maxlen - 1) {
			if ((c = getc(fp)) == '\n') {
				ecode = E_OK;
				break;
			}
			ecode = E_LINE_TOO_LONG;
			/* skip until eof or past newline */
			do {
				c = getc(fp);
			} while (c != '\n' && c != EOF);
			break;
		}
		c = getc(fp);
		if (c == EOF) {
			if (nread == 0)
				ecode = E_EOF;
			else
				ecode = E_OK;
			break;
		} else if (c == '\n') {
			ecode = E_OK;
			break;
		}
		buf[nread++] = (char) c;
	}

	buf[nread] = '\0';

	assert(nread <= maxlen - 1);

	return ecode;
}

static const char *s_basic_words[] = {
	"ABS", "ATN",
	"BASE",
	"COS",
       	"DATA", "DEF", "DIM",
       	"END", "EXP",
       	"FOR",
	"GO", "GOSUB", "GOTO",
	"IF", "INPUT", "INT",
       	"LET", "LOG",
	"NEXT",
       	"ON", "OPTION",
       	"PRINT",
       	"RANDOMIZE", "READ", "REM", "RESTORE", "RETURN", "RND",
	"SGN", "SIN", "SQR", "STEP", "STOP", "SUB",
      	"TAB", "TAN", "THEN", "TO",
	NULL
};

static const char *s_edit_words[] = {
	"COMPILE",
       	"DEBUG",
	"HELP",
       	"LICENSE", "LIST", "LOAD",
	"NEW",
       	"OFF", "ON",
	"QUIT",
       	"RENUM", "RUN",
	"SAVE", "SETGOSUB",
	NULL
};

static int str_starts_nocase(const char *str, const char *sub)
{
	const char *q;

	q = sub;
	while (*str && *q && toupper(*str) == toupper(*q)) {
		str++;
		q++;
	}

	return *q == '\0' && q != sub;
}

static const char *find_word(const char *text, const char **words, int state)
{
	static int index = 0;
	const char *cstr;

	if (state == 0) {
		index = 0;
	}

	cstr = NULL;
	while (!cstr && words[index] != NULL) {
		if (str_starts_nocase(words[index], text)) {
			cstr = words[index];
		}
		index++;
	}

	return cstr;
}

static int inside_quotes(void)
{
	int flip, i;

	flip = 0;
	for (i = 0; i < rl_point; i++) {
		if (rl_line_buffer[i] == '"') {
			flip = !flip;
		}
	}
	return flip;
}

static const char *find_basic_line(int n)
{
	struct basic_line *bline;

	bline = s_line_list;
	while (bline && bline->number != n) {
		bline = bline->next;
	}

	if (bline && bline->number == n) {
		return bline->str;
	}

	return NULL;
}

static int start_of_basic_line(void)
{
	int i;

	i = 0;
	while (i < rl_point && isspace(rl_line_buffer[i])) {
		i++;
	}

	if (i == rl_point || !isdigit(rl_line_buffer[i])) {
		return 0;
	}

	while (i < rl_point && isdigit(rl_line_buffer[i])) {
		i++;
	}

	return i == rl_end;
}

static int inside_basic_line(void)
{
	int i;

	i = 0;
	while (i < rl_point && isspace(rl_line_buffer[i])) {
		i++;
	}

	if (i == rl_point || !isdigit(rl_line_buffer[i])) {
		return 0;
	}

	while (i < rl_point && isdigit(rl_line_buffer[i])) {
		i++;
	}

	return i < rl_point && isspace(rl_line_buffer[i]);
}

static char *complete_basic_line(const char *text)
{
	int n;
	const char *cstr;
	char *line;

	cstr = text;
	n = 0;
	while (isdigit(*cstr) && n <= LINE_NUM_MAX) {
		n = n * 10 + (*cstr - '0');
		cstr++;
	}

	if (*cstr == '\0' && n <= LINE_NUM_MAX) {
		cstr = find_basic_line(n);
		if (cstr) {
			line = malloc(strlen(cstr) + 1 +
				      strlen(text) + 1);
			if (line != NULL) {
				strcpy(line, text);
				strcat(line, " "); 
				strcat(line, cstr);
				return line;
			}
		}
	}

	return NULL;
}

static char *complete_basic_cmd(const char *text, int state)
{
	const char *word;

	word = find_word(text, s_basic_words, state);
	if (word) {
		return strdup(word);
	}

	return NULL;
}

static char *complete_edit_cmd(const char *text, int state)
{
	const char *word;

	word = find_word(text, s_edit_words, state);
	if (word) {
		return strdup(word);
	}

	return NULL;
}

static char *tab_completion(const char *text, int state)
{
	static int option;

	if (state == 0) {
		option = 0;
		if (start_of_basic_line()) {
			option = 1;
		} else if (inside_basic_line()) {
		       if (!inside_quotes()) {
			       option = 2;
		       }
		} else if (inside_quotes()) {
			option = 3;
		} else {
			option = 4;
		}
	}

	switch (option) {
	case 1:
		option = 0;
		return complete_basic_line(text);
	case 2:
		return complete_basic_cmd(text, state);
	case 3:
		return rl_filename_completion_function(text, state);
	case 4:
		return complete_edit_cmd(text, state);
	default:
		return NULL;
	}
}

void get_line_set_question_mode(int set)
{
	if (set) {
		rl_completion_entry_function = NULL;
	} else {
		rl_completion_entry_function = tab_completion;
	}
	s_question_mode = set;
}

void get_line_init(void)
{
	// using_history();
	get_line_set_question_mode(0);
}

static enum error_code linedit_get_line(const char *prompt, char *buf,
	int maxlen)
{
	int i;
	char *line;
	enum error_code rcode;

	line = readline(prompt);
	if (line == NULL) {
		rcode = E_EOF;
	} else {
		if (*line && !s_question_mode) {
			add_history(line);
		}

		for (i = 0; line[i] != '\0' && i < maxlen - 1; i++) {
			buf[i] = line[i];
		}
		buf[i] = '\0';

		if (line[i] != '\0') {
			rcode = E_LINE_TOO_LONG;
		} else {
			rcode = E_OK;
		}

		free(line);
	}

	return rcode;
}

enum error_code get_line(const char *prompt, char *buf, int maxlen, FILE *fp)
{
	if (LINE_EDITING && fp == stdin) {
		return linedit_get_line(prompt, buf, maxlen);
	} else {
		if (fp == stdin && prompt != NULL) {
			fputs(prompt, stdout);
		}
		return std_get_line(buf, maxlen, fp);
	}
}
