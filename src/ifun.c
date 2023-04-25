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
 * Internal BASIC functions (SIN, COS, etc.).
 * ===========================================================================
 */

#include <config.h>
#include "ecma55.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
	//to fix error c2099 in visual c++
	#pragma function (floor)
#endif

#define USE_CUSTOM_RAND

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

static double ifun_sgn(double d)
{
	if (d == 0.0)
		return 0.0;
	else if (d > 0.0)
		return 1.0;
	else return -1.0;
}

static double ifun_log(double d)
{
	double r;

	errno = 0;
	r = m_log(d);
	if (d <= 0 && errno != EDOM) {
		errno = EDOM;
	}
	return r;
}

/* D. H. Lehmer random number generator.
   Steve Park and Keith Miller minimal standad using Schrage's method.
   Generates all 2147483646 numbers in the range [1, 2147483646],
   randomly, and then cycles again.
   Seed must be initialized with a value in that range.
*/
#ifdef USE_CUSTOM_RAND
static int s_rand_seed = 1;
enum { RAND_M = 2147483647 };
#endif

static int bas55_rand(void)
{
#ifdef USE_CUSTOM_RAND
	enum {
		a = 16807, /* or 48271 or 69621 */
		m = RAND_M,
		q = m / a,
		r = m % a
	};

  	s_rand_seed = (s_rand_seed % q) * a - (s_rand_seed / q) * r;
	if (s_rand_seed < 0) {
		s_rand_seed += m;
	}
	return s_rand_seed; 
#else
	return rand();
#endif
}

void bas55_srand(unsigned int seed)
{
#ifdef USE_CUSTOM_RAND
	seed %= RAND_M;
	if (seed < 1) {
		seed = 1;
	} else if (seed >= RAND_M) {
		seed = RAND_M - 1;
	}
	s_rand_seed = seed;
#else
	srand(seed);
#endif
}

static double ifun_rnd(void)
{
#ifdef USE_CUSTOM_RAND
	return (bas55_rand() - 1) / (double) (RAND_M - 1); 
#else
	return rand() / (RAND_MAX + 1.0);
#endif
}

static double (*s_funs_0[])(void) = {
	ifun_rnd
};

static double (*s_funs_1[])(double) = {
	m_fabs,
	m_atan,
	m_cos,
	m_exp,
	m_floor,
	ifun_log,
	ifun_sgn,
	m_sin,
	m_sqrt,
	m_tan
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
