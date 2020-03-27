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

double m_round(double d)
{
	return m_floor(d + 0.5);
}

int round_to_int(double d)
{
	return (int) m_round(d);
}

void print_chars(FILE *f, const char *s, size_t len)
{
	while (len > 0) {
		putc(*s, f);
		s++;
		len--;
	}
}
