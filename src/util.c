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

#include <config.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_LIBEDIT
#include <editline/readline.h>
#endif

#include "ecma55.h"

/**
 * Grows the array p with elements of size elem_size by a grow
 * factor grow_k. The current length of the array is cur_len.
 * If it cannot grow by that factor, retries with grow_k = 1.
 *
 * Returns 0 on success, new_array will point to the new array and new_len
 * will be the new length of the array.
 *
 * Returns E_NO_MEM if no memory; new_array == p and new_len == cur_len.
 *
 * grow_array will only grow the array up to INT_MAX. If it cannot grow
 * anymore, it will return E_NO_MEM; new_array == p and new_len == cur_len.
 *
 * new_len accepts NULL.
 */
enum error_code grow_array(void *p, int elem_size, int cur_len, int grow_k,
    void **new_array, int *new_len)
{
	int n;
	void *new_p;

	assert(elem_size > 0);
	assert(cur_len >= 0);
	assert(grow_k >= 1);
	assert(new_array != NULL);

	n = INT_MAX - cur_len;
	if (n == 0)
		goto nomem;
	else if (n > grow_k)
		n = grow_k;
	else
		n = 1;

	/* Try with n, if not try with 1, if not no mem */
	while (imul_overflows_int(cur_len + n, elem_size)) {
		if (n > 1)
			n = 1;
		else
			goto nomem;
	}

	/* Try with n, if not try with 1, if not no mem */
	while ((new_p = realloc(p, (size_t) (cur_len + n) * elem_size)) ==
		NULL)
	{
		if (n > 1)
			n = 1;
		else
			goto nomem;
	}

	*new_array = new_p;
	if (new_len != NULL)
		*new_len = cur_len + n;
	return 0;

nomem:	*new_array = p;
	if (new_len != NULL)
		*new_len = cur_len;
	return E_NO_MEM;
}

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

#if !HAVE_LIBEDIT
static char *readline(const char *prompt)
{
	return NULL;
}

static void add_history(const char *line)
{
}
#endif

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
       	"LIST", "LOAD",
	"NEW",
       	"OFF", "ON",
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

void init_readline(void)
{
	rl_completion_entry_function = tab_completion;
	using_history();
}

static enum error_code libedit_get_line(const char *prompt, char *buf,
	int maxlen)
{
	int i;
	char *line;
	enum error_code rcode;

	line = readline(prompt);
	if (line == NULL) {
		rcode = E_EOF;
	} else {
		if (*line) {
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
	if (HAVE_LIBEDIT && fp == stdin) {
		return libedit_get_line(prompt, buf, maxlen);
	} else {
		if (fp == stdin && prompt != NULL) {
			puts(prompt);
		}
		return std_get_line(buf, maxlen, fp);
	}
}

size_t min_size(size_t a, size_t b)
{
	if (a <= b)
		return a;
	else
		return b;
}

void copy_to_str(char *dst, const char *src, size_t len)
{
	memcpy(dst, src, len);
	dst[len] = '\0';
}

/* Set all characters of 'str' to upper case. */
void toupper_str(char *str)
{
	while (*str != '\0') {
		*str = (char) toupper(*str);
		str++;
	}
}


/* Returns 1 if this machine is little endian. */
int is_little_endian(void)
{
	union {
		float f;
		unsigned char bytes[4];
	} mix;

	/* 1.0 is represented as 3f80 0000 */
	mix.f = 1.0f;
	return mix.bytes[0] != 0x3f;
}

/**
 * Returns:
 * 1 if +inf
 * -1 if -inf
 * 0 if not infinite
 */
int infinite_sign(double d)
{
	int i;
	union {
		unsigned char bytes[8];
		double d;
	} mix;

	mix.d = d;
	if (is_little_endian()) {
		if (mix.bytes[6] != 0xf0)
			return 0;
		for (i = 0; i < 6; i++)
			if (mix.bytes[i] != 0)
				return 0;
		if (mix.bytes[7] == 0x7f)
			return 1;
		else if (mix.bytes[7] == 0xff)
			return -1;
		else
			return 0;
	} else if (mix.bytes[1] == 0xf0) {
		for (i = 2; i < 8; i++)
			if (mix.bytes[i] != 0)
				return 0;
		if (mix.bytes[0] == 0x7f)
			return 1;
		else if (mix.bytes[1] == 0xff)
			return -1;
		else return 0;
	} else {
		return 0;
	}
}

int is_nan(double d)
{
	int i;
	union {
		unsigned char bytes[8];
		double d;
	} mix;

	mix.d = d;
	if (is_little_endian()) {
		if (mix.bytes[7] != 0x7f && mix.bytes[7] != 0xff)
			return 0;
		if ((mix.bytes[6] & 0xf0) != 0xf0)
			return 0;
		for (i = 0; i < 6; i++) {
			if (mix.bytes[i] != 0)
				return 1;
		}
		if ((mix.bytes[6] & 0x0f) != 0)
			return 1;
		return 0;
	} else {
		if (mix.bytes[0] != 0x7f && mix.bytes[0] != 0xff)
			return 0;
		if ((mix.bytes[1] & 0xf0) != 0xf0)
			return 0;
		for (i = 2; i < 8; i++) {
			if (mix.bytes[i] != 0)
				return 1;
		}
		if ((mix.bytes[1] & 0x0f) != 0)
			return 1;
		return 0;
	}
}

double round(double d)
{
	return floor(d + 0.5);
}

int round_to_int(double d)
{
	return (int) round(d);
}

void print_chars(FILE *f, const char *s, size_t len)
{
	while (len > 0) {
		putc(*s, f);
		s++;
		len--;
	}
}
