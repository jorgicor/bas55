/* Error reporting functions and error texts.
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
#include <ctype.h>
#include <string.h>

static const char *s_errors[] = {
	"ok",
	"not enough memory",
	"invalid line number",
	"line too long",
	"invalid command",
	"index out of range",
	"stack overflow",
	"stack underflow",
	"syntax error",
	"non-existing line number",
	"OPTION redeclared",
	"OPTION used after arrays used or DIM",
	"redimensioned variable",
	"type mismatch for variable",
	"invalid DIM subscript",
	"numeric variable used as array",
	"array subscript too high",
	"program ram too big",
	"NEXT without FOR",
	"string not terminated",
	"invalid TAB argument",
	"function redeclared",
	"function argument as array",
	"undefined function",
	"invalid number of arguments for function",
	"insuficient input data",
	"too much input data",
	"error on input stream",
	"numeric constant overflow",
	"jump into FOR block",
	"file name too long",
	"end of file",
	"couldn't open file",
	"no space after line number",
	"line without instructions",
	"wrong number of arguments",
	"bad file name",
	"program must have an END statement",
	"line after an END statement",
	"division by zero",
	"operation overflow",
	"zero raised to negative value",
	"negative value raised to non-integral value",
	"FOR without NEXT",
	"FOR uses the same variable as outer FOR at line",
	"function domain error",
	"invalid characters found",
	"duplicated line number",
	"invalid line order",
	"number too big",
	"variable used before value assigned",
	"array position read before value assigned",
	"insufficient data for READ",
	"reading string into numeric variable",
	"no space after keyword",
	"numeric expression expected",
	"string expression expected",
	"string expressions can only be tested for equality",
	"numeric variable name expected",
	"string datum contains too many characters"
};

/* Prints the error code on stderr. */
void eprint(enum error_code ecode)
{
	fputs("error: ", stderr);
	fputs(s_errors[ecode], stderr);
	fputc(' ', stderr);
}

/*
 * Prints the error code and the line lineno on stderr.
 * If lineno <= 0, doesn't print the line number.
 */
void eprintln(enum error_code ecode, int lineno)
{
	if (lineno > 0)
		fprintf(stderr, "%d: ", lineno);
	eprint(ecode);
}

/* Prints the error code on stderr as a warning. */
void wprint(enum error_code ecode)
{
	fputs("warning: ", stderr);
	fputs(s_errors[ecode], stderr);
	fputc(' ', stderr);
}

/*
 * Prints the error code as a warning and the line lineno on stderr.
 * If lineno <= 0, doesn't print the line number.
 */
void wprintln(enum error_code ecode, int lineno)
{
	if (lineno > 0)
		fprintf(stderr, "%d: ", lineno);
	wprint(ecode);
}

/* Prints \n on stderr. */
void enl(void)
{
	fputc('\n', stderr);
}

/* Prints the program name on stderr. */
void eprogname(void)
{
	fprintf(stderr, PACKAGE": ");
}
