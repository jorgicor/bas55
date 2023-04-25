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
 * Parsing of quoted ot unquoted strings, numbers or comma separated lists of
 * these elements.
 * ===========================================================================
 */

#include <config.h>
#include "ecma55.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdlib.h>
#include <inttypes.h>

/**
 * Valid character set for an unquoted string.
 */
static int is_unquoted_str_char(int c)
{
	if (c >= 'A' && c <= 'Z')
		return 1;
	else if (c >= '0' && c <= '9')
		return 1;
	else if (c == ' ' || c == '+' || c == '-' || c == '.')
		return 1;
	else
		return 0;
}

/* Parses an unquoted string.
 * From 'start' it collects all characters until a ',' or a '\0' is found.
 * '*parsed_len' will be the length of this sequence from 'start'.
 * '*str_len' will be the length of the parsed string from 'start' to the
 * last character that is not space before the ',' or '\0'.
 */
static void parse_unquoted_str(const char *start, size_t *str_len,
    size_t *parsed_len)
{
	int c;
	const char *p, *q;

	p = start;
	c = *p;
	while (is_unquoted_str_char(c))
		c = *++p;
	q = p;

	/* don't use trailing white space */
	while (q > start && isspace(q[-1]))
		q--;

	if (str_len != NULL)
		*str_len = (size_t) (q - start);
	if (parsed_len != NULL)
		*parsed_len = (size_t) (p - start);
}

/* Parses a quoted string. It collects all characters form 'start' until
 * a '\"' or '\0' is found.
 * 'start' must point to one character past the opening quote.
 * '*len' will be the length of the string.
 * Note that 'start[*len]' will be '\"' or '\0'.
 * If it is '\0', the string was not well terminated.
 */
void parse_quoted_str(const char *start, size_t *len)
{
	int c;
	const char *q;
	
	c = *(q = start);
	while (c != '\0' && c != '\"') {
		c = *++q;
	}
	if (len != NULL) {
		*len = (size_t) (q - start);
	}
}

enum num_type check_if_number_suffix(const char *p, enum num_type t)
{
	int c;

	c = *p;
	while (isdigit(*p))
		c = *++p;
	if (c == 'E') {
		c = *++p;
		if (c == '-' || c == '+')
			c = *++p;
		if (isdigit(c))
			t = NUM_TYPE_FLOAT;
	}

	return t;
}

/* 
 * Tries to parse a number (integer or floating point) following BASIC
 * syntax only (don't allow all strtod parsing options like hexadecimal, etc).
 * A sign can precede the number.
 * Returns the type found.
 */
enum num_type check_if_number(const char *p)
{
	int c;
	enum num_type t;

	t = NUM_TYPE_NONE;
	c = *p;
	if (c == '+' || c == '-')
		c = *++p;
	if (isdigit(c)) {
		t = NUM_TYPE_INT;
		while (isdigit(c))
			c = *++p;
		if (c == '.') {
			t = NUM_TYPE_FLOAT;
			c = *++p;
		}
		t = check_if_number_suffix(p, t);
	} else if (c == '.') {
		c = *++p;
		if (isdigit(c))
			t = check_if_number_suffix(p, NUM_TYPE_FLOAT);
	}

	return t;
}

/* Parses an integer and returns it.
 * 'start[*len]' will point to one character past the last number character.
 * At entry 'start' must point to a digit or a sign symbol (+,-);
 * if not, the results are undefined. Use check_if_number to know for sure.
 * If there is an overflow, returns INT_MAX or INT_MIN, and
 * errno will be ERANGE.
 * Sets errno to 0 if no error.
 */
int parse_int(const char *start, size_t *len)
{
	// long is 32 bits on Windows, 64 on Linux, but in this case
	// it will work ok.
	long longn;
	int i;
	const char *end;

	errno = 0;
	longn = strtol(start, (char **) &end, 10);
	if (errno == ERANGE) {
		if (longn == LONG_MIN) {
			i = INT_MIN;
		} else {
			/* longn == LONG_MAX */
			i = INT_MAX;
		}
	} else if (longn < (long) INT_MIN) {
		i = INT_MIN;
		errno = ERANGE;
	} else if (longn > (long) INT_MAX) {
		i = INT_MAX;
		errno = ERANGE;
	} else {
		i = (int) longn;
	}

	if (len != NULL)
		*len = (size_t) (end - start);
	return i;
}

static double powten[DBL_MAX_10_EXP + 1] = { 0.0 };

static double mulexpo(double num, int e)
{
	while (e > 0) {
		if (e > DBL_MAX_10_EXP) {
			num *= powten[DBL_MAX_10_EXP];
			e -= DBL_MAX_10_EXP;
		} else {
			num *= powten[e];
			e = 0;
		}
	}
	return num;
}

static double divexpo(double num, int e)
{
	while (e > 0) {
		if (e > -DBL_MIN_10_EXP) {
			num /= powten[-DBL_MIN_10_EXP];
			e += DBL_MIN_10_EXP;
		} else {
			num /= powten[e];
			e = 0;
		}
	}
	return num;
}

static double expo(double num, int e)
{
	int i;

	if (powten[0] == 0.0) {
		powten[0] = 1.0;
		for (i = 1; i < NELEMS(powten); i++) {
			powten[i] = powten[i - 1] * 10;
		}
	}

	if (e < 0)
		return divexpo(num, -e);
	else if (e > 0)
		return mulexpo(num, e);
	else
		return num;
}

/**
 * Reads a floating point number from s as defined by the ECMA-55.
 * First whitespace is skipped.
 * Then the number is read if any.
 * If len is 0, no number could be read and returns 0.
 * If overflow, HUGE_VAL or -HUGE_VAL is returned and errno is set to ERANGE;
 * else errno is 0.
 * len can be passed NULL.
 * digits is the number of significant digits we want to read. Must be > 0.
 */
static double strtod55(const char *s, int sdigits, size_t *len)
{
	const char *p, *q;
	int64_t num;
	int sign;	/* number sign */
	int bp, ap;	/* before point, after point */
	int d;		/* significant digits count */
	int ed;		/* exponent for digits before E */
	int e;		/* exponent */
	int esign;	/* exponent sign */
	double dnum;

	assert(sdigits > 0);

	errno = 0;
	if (len != NULL) {
		*len = 0;
	}

	p = s;
	while (isspace(*p)) {
		p++;
	}

	sign = 1;
	if (*p == '+') {
		p++;
	} else if (*p == '-') {
		sign = -1;
		p++;
	}

	if (!isdigit(*p) && *p != '.') {
		return 0.0;
	}

	bp = 0;
	num = 0;
	d = 0;
	ap = 0;
	ed = 0;
	if (isdigit(*p)) {
		bp = 1;
	}
	while (*p == '0') {
		p++;
	}
	while (isdigit(*p)) {
		if (d < sdigits) {
			num = num * 10 + (*p - '0');
			d++;
		} else if (d == sdigits) {
			num += (*p >= '5');
			d++;
			ed++;
		} else {
			ed++;
		}
		p++;
	}
	if (*p == '.') {
		p++;
		if (isdigit(*p)) {
			ap = 1;
		}
		if (num == 0) {
			while (*p == '0') {
				ed--;
				p++;
			}
		}
		while (isdigit(*p)) {
			if (d < sdigits) {
				num = num * 10 + (*p - '0');
				d++;
				ed--;
			} else if (d == sdigits) {
				num += (*p >= '5');
				d++;
			}
			p++;
		}
	}

	if (bp == 0 && ap == 0) {
		return 0.0;
	}

	/* take exponent */
	q = p;
	e = 0;
	esign = 1;
	if (*p == 'e' || *p == 'E') {
		p++;
		if (*p == '+') {
			p++;
		} else if (*p == '-') {
			esign = -1;
			p++;
		}
		if (!isdigit(*p)) {
			p = q;
		} else while (isdigit(*p)) {
			// A double exponent is in [-307,308] max.
			// e is not going to overflow an 'int'
			if (e <= DBL_MAX_10_EXP) {
				e = e * 10 + (*p - '0');
			}
			p++;
		}
	}
		
	if (len != NULL) {
		*len = p - s;
	}

	// since lines are of limited length (~80),
	// ed + e * sign fits in an 'int'
	dnum = sign * expo(num, ed + (e * esign));
	if (m_isinf(dnum)) {
		errno = ERANGE;
	}
	return dnum;
}

/**
 * Sets errno to ERANGE on overflow and +INF or -INF is returned.
 * If no overflow errno is 0.
 */
double parse_double(const char *start, size_t *len)
{
	return strtod55(start, READ_PRECISION_DIGITS, len);
}

/* Parses an element of a basic DATA, that is, an element from a comma
 * separated list of numbers, quoted strings or unquoted strings.
 * 'start' points to the start of the string to parse.
 * '*len' will be the length of the characters considered for this element,
 * so on the next call you can set 'start' to 'start + *len' to get the
 * next element. Note that if DATA_ELEM_EOF is returned, '*len' will be 0.
 * If DATA_ELEM_INVAL_CHAR is returned, 'start[*len - 1]' will contain the
 * character that is invalid for an unquoted string.
 *
 * First any space is skiped.
 * Then, given the parsed element:
 *	1. If a '\0' is found, returns DATA_ELEM_EOF.
 *	2. If a ',' is found, returns DATA_ELEM_COMMA.
 *	3. If a '\"' is found, parses a quoted string and returns
 *	   DATA_ELEM_QUOTED_STR. 'delem.str.start' will point to the character
 *	   past the opening quote. 'delem.str.len' will be the length of the
 *	   string without opening or closing quote. Note that if
 *	   'delem.str.start[delem.str.len]' is '\0' instead of '\"', the
 *	   string was not correctly terminated.
 *	4. If it is a number, DATA_ELEM_NUM is returned. 'delem.num' will
 *	   be the number value. If an overflow occurs, the number will be
 *	   HUGE_VAL or -HUGE_VAL depending on the sign and errno will be
 *	   ERANGE.
 *	5. If the element cannot be parsed as a number, we try to parse an
 *	   unquoted string. All characters allowed for an unquoted string
 *	   are parsed and DATA_ELEM_UNQUOTED_STR is returned.
 *	   'delem.str.start' will point to the first character.
 *	   'delem.str.len' will be the length of the string without trailing
 *	   space.
 *	6. At this point, if the first character is not allowed for an
 *	   unquoted string, DATA_ELEM_INVAL_CHAR is returned.
 *
 * If parse_as is DATA_ELEM_AS_UNQUOTED_STR and a number is found, the number
 * is parsed as an unquoted string and point 5 applies.
 */
enum data_elem_type parse_data_elem(union data_elem *delem,
    const char *start, size_t *len, enum data_elem_as parse_as)
{
	int c;
	double d;
	enum num_type numt;
	const char *p;
	size_t len1, len2;

	errno = 0;
	p = start;
	while (isspace(*p))
		p++;

	c = *p;
	if (c == '\0') {
		if (len != NULL)
			*len = (size_t) (p - start);
		return DATA_ELEM_EOF;
	} else if (c == ',') {
		p++;
		if (len != NULL)
			*len = (size_t) (p - start);
		return DATA_ELEM_COMMA;
	} else if (c == '\"') {
		p++;
		parse_quoted_str(p, &len1);
		if (delem != NULL) {
			delem->str.start = p;
			delem->str.len = len1;
		}
		p += len1 + (p[len1] == '\"');
		if (len != NULL)
			*len = (size_t) (p - start);
		return DATA_ELEM_QUOTED_STR;
	} else if (!is_unquoted_str_char(c)) {
		p++;
		if (len != NULL)
			*len = (size_t) (p - start);
		return DATA_ELEM_INVAL_CHAR;
	} else if (parse_as == DATA_ELEM_AS_UNQUOTED_STR ||
	    (numt = check_if_number(p)) == NUM_TYPE_NONE) {
		parse_unquoted_str(p, &len1, &len2);
		if (delem != NULL) {
			delem->str.start = p;
			delem->str.len = len1;
		}
		p += len2;
		if (len != NULL)
			*len = (size_t) (p - start);
		return DATA_ELEM_UNQUOTED_STR;
	} else {	/* numt == NUM_TYPE_INT || numt == NUM_TYPE_FLOAT */
		d = parse_double(p, &len1);
		p += len1;
		if (delem != NULL)
			delem->num = d;
		if (len != NULL)
			*len = (size_t) (p - start);
		return DATA_ELEM_NUM;
	}
}
