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

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ecma55.h"

#ifdef _MSC_VER
	//to fix error c2099 in visual c++
	#pragma function (floor)
#endif

enum {
	ABS,
	ATN,
	COS,
	EXP,
	INT,
	LOG,
	SGN,
	SIN,
	SQR,
	TAN,
	RND = 256
};

struct internal_fun {
	const char *const name;
	const int code;
};

static const struct internal_fun s_ifuns[] = {
	{ "ABS", ABS },
	{ "ATN", ATN },
	{ "COS", COS },
	{ "EXP", EXP },
	{ "INT", INT },
	{ "LOG", LOG },
	{ "RND", RND },
	{ "SGN", SGN },
	{ "SIN", SIN },
	{ "SQR", SQR },
	{ "TAN", TAN }
};

static double sgn(double d)
{
	if (d == 0.0)
		return 0.0;
	else if (d > 0.0)
		return 1.0;
	else return -1.0;
}

static double log55(double d)
{
	double r;

	errno = 0;
	r = log(d);
	if (d <= 0 && errno != EDOM)
		errno = EDOM;
	return r;
}

static double rnd(void)
{
	return rand() / (RAND_MAX + 1.0);
}

static double (*s_funs_0[])(void) = {
	rnd
};

static double (*s_funs_1[])(double) = {
	fabs,
	atan,
	cos,
	exp,
	floor,
	log55,
	sgn,
	sin,
	sqrt,
	tan
};

int get_internal_fun(const char *name)
{
	int i;

	for (i = 0; i < NELEMS(s_ifuns); i++)
		if (strcmp(name, s_ifuns[i].name) == 0)
			return i;

	return -1;
}

int get_ifun_nparams(int i)
{
	if (s_ifuns[i].code < 256)
		return 1;
	else
		return 0;
}

const char *get_ifun_name(int i)
{
	return s_ifuns[i].name;
}

double call_ifun0(int i)
{
	int ti;

	ti = ((unsigned) s_ifuns[i].code >> 8) - 1;
	return s_funs_0[ti]();
}

double call_ifun1(int i, double d)
{
	errno = 0;
	return s_funs_1[s_ifuns[i].code](d);
}
