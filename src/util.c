/* ===========================================================================
 * bas55, an implementation of the Minimal BASIC programming language.
 *
 * Generic functions.
 * ===========================================================================
 */

#include <config.h>
#include "ecma55.h"
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

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
