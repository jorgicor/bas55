/* ===========================================================================
 * bas55, an implementation of the Minimal BASIC programming language.
 *
 * Virtual machine that can execute the BASIC program compiled in code.c, str.c
 * and data.c .
 * ===========================================================================
 */

#include <config.h>
#include "ecma55.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#if defined(_WIN32)
	#include <windows.h>
#else
	#include <sys/time.h>
#endif

enum {
	PRINT_COLUMN_INC = 14, 		/* This is -d.dddddE-123s */
	NPRINT_COLUMNS = 5,
	PRINT_MARGIN = 80,
	NUM_CHARS_UNSCALED = 9,		/* -d.dddddd */
	NUM_CHARS_SCALED = 13		/* -1.23456E+123 */
};

/* Program RAM */
union ram_value {
	double d;
	int i;
};

static union ram_value *s_ram = NULL;

/* Program counter. */
static int s_pc;

/* For debug purposes, we track if a variable has been assigned a value
 * before use.
 * For each variable name (A, B, etc), we have an integer.
 * Bits 0-9 are for variables with a digit, bit 10 for a variable without
 * digit, bit 11 for a string variable.
 * A bit set means the something has been assigned to the variable.
 * If nothing has been assigned to the variable but we use it, we set the
 * bit so we don't issue a warning again.
 */
static int s_inited_vars[N_VARNAMES];

/* Stack. */
static union ram_value *s_stack = NULL;
static int s_stack_capacity = 0;
static int s_sp;

/* Fatal error occurred. */
static int s_fatal;

/* Current running line. */
static int s_cur_line_num;

/* The GOSUB s_stack */
static int *s_gosub_stack = NULL;
static int s_gosub_stack_capacity = 0;
static int s_default_gosub_stack_capacity = 256;

/* GOSUB s_stack pointer */
static int s_gosub_sp;

/* For arrays, this stores 0 or 1, the base index for arrays. */
static int s_base_ix;

/* The current printing column. */
static int s_print_column;

/* User pressed Ctrl+C in interactive mode while running program. */
static volatile sig_atomic_t s_break = 0;

static void sigint_handler(int sig)
{
	s_break = 1;
}

static void free_stack(void)
{
	if (s_stack != NULL) {
		free(s_stack);
		s_stack = NULL;
		s_stack_capacity = 0;
		s_sp = 0;
	}
}

static void free_gosub_stack(void)
{
	if (s_gosub_stack != NULL) {
		free(s_gosub_stack);
		s_gosub_stack = NULL;
		s_gosub_stack_capacity = 0;
	}
}

static enum error_code alloc_gosub_stack(void)
{
	assert(s_gosub_stack == NULL);

	if ((s_gosub_stack = calloc(s_default_gosub_stack_capacity,
		sizeof *s_gosub_stack)) == NULL)
	{
		return E_NO_MEM;
	} else {
		s_gosub_stack_capacity = s_default_gosub_stack_capacity;
		return 0;
	}
}

static enum error_code alloc_stack(int n)
{
	assert(s_stack == NULL);

	if ((s_stack = malloc(n * sizeof *s_stack)) == NULL)
		return E_NO_MEM;

	s_stack_capacity = n;
	return 0;
}

static void reset_inited_vars(void)
{
	memset(s_inited_vars, 0, sizeof(s_inited_vars));
}

static int is_var_initialized(int coded_var)
{
	int index1, index2;
	
	index1 = var_index1(coded_var);
	index2 = var_index2(coded_var);
	return (s_inited_vars[index1] & (1 << index2)) != 0;
}

static void set_var_initialized(int coded_var)
{
	int index1, index2;
	
	index1 = var_index1(coded_var);
	index2 = var_index2(coded_var);
	s_inited_vars[index1] |= (1 << index2);
}

static void remove_leading_zero(int len, char *num)
{
	num += 1;
	memmove(num, num + 1, len - 1);
}

static int sprint_unscaled(char *rnum, double d, int after, int exponent)
{
	int nchars;
	char fmt[] = "% .0f";
	char num[NUM_CHARS_UNSCALED + 8];	/* \0 + safe space */

	after -= exponent;
	if (after < 0)
		after = 0;
	fmt[3] = '0' + after;
	nchars = sprintf(num, fmt, d);
	assert(nchars <= NUM_CHARS_UNSCALED);
	if (num[1] == '0')
		remove_leading_zero(nchars, num);
	return sprintf(rnum, "%s ", num);
}

/**
 * Analyzes a floating point number printed used " %.nE". That is,
 * the number has a leading whitespace or minus sign, one digit, a full stop,
 * n decimal digits, an E, a plus or minus sign and exponent with 2 digits
 * minimum.
 * '*zero' will be 1 if the digit before the decimal point is a 0.
 * '*nafter' will be the number of significant digits after the decimal point.
 * '*exponent' will be the value of the exponent.
 */
static void explore_number(char *num, int *zero, int *nafter, int *exponent)
{
	int esign;

	num++;
	*zero = *num == '0';
	while (*num != 'E')
		num++;
	num--;
	*nafter = 0;
	while (*num == '0')
		num--;
	while (*num != '.') {
		(*nafter)++;
		num--;
	}
	while (*num != 'E')
		num++;
	num++;
	esign = (*num == '+') ? 1 : -1;
	num++;
	*exponent = 0;
	while (*num != '\0') {
		*exponent = (*exponent * 10) + (*num - '0');
		num++;
	}
	if (esign < 0)
		*exponent = -*exponent;
}

/**
 * num is the string representation of a number in scaled notation:
 * sd.ddddddEsddd
 * This removes the trailing ceros of the decimal part and the leading zeros
 * of the exponent.
 */
static void remove_zeros_from_scaled_number(int len, char *num)
{
	int n;

	while (*num != 'E') {
		len--;
		num++;
	}
	num--;
	n = 0;
	while (*num == '0') {
		num--;
		n++;
	}
	num++;
	if (n > 0) {
		memmove(num, num + n, len + 1);
	}
	num += 2;
	len -= 2;
	n = 0;
	while (*num == '0') {
		len--;
		n++;
		num++;
	}
	if (n > 0)
		memmove(num - n, num, len + 1);
}

static int sprint_num(char *rnum, double d)
{
	int nchars, zero, after, exponent, infsign;
	char num[NUM_CHARS_SCALED + 8]; /* \0 + safe space */

	infsign = infinite_sign(d);
	if (infsign == 1)
		return sprintf(rnum, " INF ");
	
	if (infsign == -1)
		return sprintf(rnum, "-INF ");

	/* It is not infinite */
	if (is_nan(d))
		return sprintf(rnum, " NAN ");

	/* It is not INF and not NAN */
	nchars = sprintf(num, "% .5E", d);
	assert(nchars <= NUM_CHARS_SCALED); /* -1.23456E+308 */
	explore_number(num, &zero, &after, &exponent);
	assert(!zero || (zero && after == 0));
	if (zero) {
		return sprintf(rnum, " 0 ");
	} else if (exponent < 0 && after - exponent <= 6) {
		return sprint_unscaled(rnum, d, after, exponent);
	} else if (exponent >= 0 && 1 + exponent <= 6) {
		return sprint_unscaled(rnum, d, after, exponent);
	} else {
		remove_zeros_from_scaled_number(nchars, num);
		return sprintf(rnum, "%s ", num);
	}
}

static int print_num(FILE *f, double d)
{
	char num[NUM_CHARS_SCALED + 8]; /* \0 + safe space */

	sprint_num(num, d);
	return fprintf(f, "%s", num);
}

static void push_num_op(void)
{
	s_stack[s_sp++].d = code[s_pc++].num;
}

static void push_str_op(void)
{
	s_stack[s_sp++].i = code[s_pc++].id;
}

static void print_nl_op(void)
{
	putc('\n', stdout);
	s_print_column = 0;
}

static void print_comma_op(void)
{
	int n;

	n = PRINT_COLUMN_INC - (s_print_column % PRINT_COLUMN_INC);
	s_print_column += n;
	if (s_print_column >= NPRINT_COLUMNS * PRINT_COLUMN_INC) {
		s_print_column = 0;
		putc('\n', stdout);
	} else while (n != 0) {
		putc(' ', stdout);
		n--;
	}
}

static void print_tab_op(void)
{
	double d;
	int n;

	d = s_stack[--s_sp].d;
	n = round_to_int(d);
	if (n <= 0) {
		wprintln(E_INVAL_TAB, s_cur_line_num);
		putc('(', stderr);
		print_num(stderr, n);
		putc(')', stderr);
		enl();
		n = 1;
	}

	/* Consider columns from 0. */
	n--;
	if (n >= PRINT_MARGIN)
		n = n % PRINT_MARGIN;

	if (s_print_column > n) {
		putc('\n', stdout);
		s_print_column = 0;
	}

	while (s_print_column < n) {
		putc(' ', stdout);
		s_print_column++;
	}
}

static void print_num_op(void)
{	
	double d;
	int nchars;
	char num[NUM_CHARS_SCALED + 8]; /* \0 + safe space */

	d = s_stack[--s_sp].d;
	nchars = sprint_num(num, d);
	if (s_print_column + nchars > PRINT_MARGIN) {
		putc('\n', stdout);
		s_print_column = 0;
	}
	s_print_column += fprintf(stdout, "%s", num);
}

static void print_str_op(void)
{
	char fmt1[] = "%0s\n";
	char fmt2[] = "%00s\n";
	int stri;
	const char *str;
	int n, len;

	stri = s_stack[--s_sp].i;
	str = strings[stri]->str;
	len = strlen(str);
	if (s_print_column + len > PRINT_MARGIN) {
		putc('\n', stdout);
		s_print_column = 0;
	}
	while (s_print_column + len > PRINT_MARGIN) {
		n = PRINT_MARGIN - s_print_column;
		if (n > 9) {
			fmt2[1] = '0' + (n / 10);
			fmt2[2] = '0' + (n % 10);
			printf(fmt2, str);
		} else {
			fmt1[1] = '0' + n;
			printf(fmt1, str);
		}
		str += n;
		s_print_column = 0;
		len -= n;
	}
	if (len > 0)
		s_print_column += printf("%s", str);
}

static void let_var_op(void)
{
	int rampos;

	rampos = code[s_pc++].id;
	s_ram[rampos].d = s_stack[--s_sp].d;
}

static void let_strvar_op(void)
{
	int rampos, stri, oldi;

	rampos = code[s_pc++].id;
	stri = s_stack[--s_sp].i;
	oldi = s_ram[rampos].i;
	if (oldi != stri) {
		dec_string_refcount(oldi);
		s_ram[rampos].i = stri;
		inc_string_refcount(stri);
	}
}

/* Returns 0 if the index is ok, else E_INDEX_RANGE. */
static int check_index(double index, int dim)
{
	if (index < 0 || index >= dim) {
		eprintln(E_INDEX_RANGE, s_cur_line_num);
		putc('(', stderr);
		print_num(stderr, index + s_base_ix);
		putc(')', stderr);
		enl();
		s_fatal = 1;
		return E_INDEX_RANGE;
	}

	return 0;
}

static void let_list_op(void)
{
	double value, dindex;
	int rampos, index, dim;

	rampos = code[s_pc++].id;
	dim = code[s_pc++].id;
	value = s_stack[--s_sp].d;
	dindex = round(s_stack[--s_sp].d) - s_base_ix;
	if (check_index(dindex, dim) != 0)
		return;

	index = (int) dindex;
	s_ram[rampos + index].d = value;
}

static void let_table_op(void)
{
	double value;
	int rampos, index1, index2, dim1, dim2;
	double dindex1, dindex2;

	rampos = code[s_pc++].id;
	dim1 = code[s_pc++].id;
	dim2 = code[s_pc++].id;
	value = s_stack[--s_sp].d;
	dindex2 = round(s_stack[--s_sp].d) - s_base_ix;
	dindex1 = round(s_stack[--s_sp].d) - s_base_ix;

	if (check_index(dindex1, dim1) != 0)
		return;

	if (check_index(dindex2, dim2) != 0)
		return;

	index1 = (int) dindex1;
	index2 = (int) dindex2;
	s_ram[rampos + index1 * dim2 + index2].d = value;
}

static void input_list_op(void)
{
	double value, dindex;
	int rampos, index, dim;

	rampos = code[s_pc++].id;
	dim = code[s_pc++].id;
	dindex = round(s_stack[--s_sp].d) - s_base_ix;
	value = s_stack[--s_sp].d;

	if (check_index(dindex, dim) != 0)
		return;

	index = (int) dindex;
	s_ram[rampos + index].d = value;
}

static void input_table_op(void)
{
	double value, dindex1, dindex2;
	int rampos, index1, index2, dim1, dim2;

	rampos = code[s_pc++].id;
	dim1 = code[s_pc++].id;
	dim2 = code[s_pc++].id;
	dindex2 = round(s_stack[--s_sp].d) - s_base_ix;
	dindex1 = round(s_stack[--s_sp].d) - s_base_ix;
	value = s_stack[--s_sp].d;

	if (check_index(dindex1, dim1) != 0)
		return;

	if (check_index(dindex2, dim2) != 0)
		return;

	index1 = (int) dindex1;
	index2 = (int) dindex2;
	s_ram[rampos + index1 * dim2 + index2].d = value;
}

static double read_double(void)
{
	double d;
	int stri, ecode;
	const char *str;
	union data_elem delem;
	size_t len;
	enum data_elem_type t;
	enum data_datum_type datum_type;
	int serrno;

	if ((ecode = read_data_str(&stri, &datum_type)) != 0) {
		eprintln(E_READ_OFLOW, s_cur_line_num);
		enl();
		s_fatal = 1;
		return 0.0;
	}

	if (datum_type == DATA_DATUM_QUOTED_STR) {
		eprintln(E_READ_STR, s_cur_line_num);
		enl();
		s_fatal = 1;
		return 0.0;
	}

	str = strings[stri]->str;
	t = parse_data_elem(&delem, str, &len, DATA_ELEM_AS_IS);
	serrno = errno;
	d = delem.num;
	if (t != DATA_ELEM_NUM) {
		eprintln(E_READ_STR, s_cur_line_num);
		enl();
		s_fatal = 1;
		return 0.0;
	}

	str += len;
	t = parse_data_elem(&delem, str, &len, DATA_ELEM_AS_IS);
	if (t != DATA_ELEM_EOF) {
		eprintln(E_READ_STR, s_cur_line_num);
		enl();
		s_fatal = 1;
		return 0.0;
	}

	if (serrno == ERANGE)
	{
		wprintln(E_CONST_OVERFLOW, s_cur_line_num);
		enl();
	}

	return d;
}

static void read_var_op(void)
{
	int rampos;

	rampos = code[s_pc++].id;
	s_ram[rampos].d = read_double();
}

static void read_list_op(void)
{
	double dindex;
	int rampos, index, dim;

	rampos = code[s_pc++].id;
	dim = code[s_pc++].id;	
	dindex = round(s_stack[--s_sp].d) - s_base_ix;
	if (check_index(dindex, dim) != 0)
		return;

	index = (int) dindex;
	s_ram[rampos + index].d = read_double();
}

static void read_table_op(void)
{
	double dindex1, dindex2;
	int rampos, index1, index2, dim1, dim2;

	rampos = code[s_pc++].id;
	dim1 = code[s_pc++].id;
	dim2 = code[s_pc++].id;
	dindex2 = round(s_stack[--s_sp].d) - s_base_ix;
	dindex1 = round(s_stack[--s_sp].d) - s_base_ix;

	if (check_index(dindex1, dim1) != 0)
		return;

	if (check_index(dindex2, dim2) != 0)
		return;

	index1 = (int) dindex1;
	index2 = (int) dindex2;
	s_ram[rampos + index1 * dim2 + index2].d = read_double();
}

static void read_strvar_op(void)
{
	int dst, stri, oldi;
	enum error_code ecode;

	dst = code[s_pc++].id;
	if ((ecode = read_data_str(&stri, NULL)) != 0) {
		eprintln(ecode, s_cur_line_num);
		enl();
		s_fatal = 1;
		return;
	}

	oldi = s_ram[dst].i;
	if (stri != oldi) {
		dec_string_refcount(oldi);
		s_ram[dst].i = stri;
		inc_string_refcount(stri);
	}
}

static void check_init_var_op(void)
{
	int coded_var;

	coded_var = code[s_pc++].id;
	if (!is_var_initialized(coded_var)) {
		/* Do not issue the warning again. */
		set_var_initialized(coded_var);

		wprintln(E_INIT_VAR, s_cur_line_num);
		putc('(', stderr);
		print_var(stderr, coded_var);
		putc(')', stderr);
		enl();
	}
}

static void set_init_var_op(void)
{
	int coded_var;

	coded_var = code[s_pc++].id;
	set_var_initialized(coded_var);
}

static void get_var_op(void)
{
	int rampos;

	rampos = code[s_pc++].id;
	s_stack[s_sp++].d = s_ram[rampos].d;
}

static void get_strvar_op(void)
{
	int rampos;

	rampos = code[s_pc++].id;
	s_stack[s_sp++].i = s_ram[rampos].i;
}

static void get_list_op(void)
{
	int rampos, index, dim;
	double dindex;

	rampos = code[s_pc++].id;
	dim = code[s_pc++].id;
	dindex = round(s_stack[--s_sp].d) - s_base_ix;

	if (check_index(dindex, dim) != 0)
		return;

	index = (int) dindex;
	s_stack[s_sp++].d = s_ram[rampos + index].d;
}

static void get_table_op(void)
{
	double dindex1, dindex2;
	int rampos, index1, index2, dim1, dim2;

	rampos = code[s_pc++].id;
	dim1 = code[s_pc++].id;
	dim2 = code[s_pc++].id;
	dindex2 = round(s_stack[--s_sp].d) - s_base_ix;
	dindex1 = round(s_stack[--s_sp].d) - s_base_ix;

	if (check_index(dindex1, dim1) != 0)
		return;

	if (check_index(dindex2, dim2) != 0)
		return;

	index1 = (int) dindex1;
	index2 = (int) dindex2;
	s_stack[s_sp++].d = s_ram[rampos + index1 * dim2 + index2].d;
}

static void add_op(void)
{
	double d1, d2;

	d2 = s_stack[--s_sp].d;
	d1 = s_stack[--s_sp].d;
	s_stack[s_sp++].d = d1 + d2;
}

static void sub_op(void)
{
	double d1, d2;

	d2 = s_stack[--s_sp].d;
	d1 = s_stack[--s_sp].d;
	s_stack[s_sp++].d = d1 - d2;
}

static void mul_op(void)
{
	double d1, d2, d;

	d2 = s_stack[--s_sp].d;
	d1 = s_stack[--s_sp].d;
	d = d1 * d2;
	if (infinite_sign(d) != 0 &&
		(infinite_sign(d1) == 0 || infinite_sign(d2) == 0))
	{
		wprintln(E_OP_OVERFLOW, s_cur_line_num);
		enl();
	}
	s_stack[s_sp++].d = d;
}

static void div_op(void)
{
	double d2, d1;

	d2 = s_stack[--s_sp].d;
	d1 = s_stack[--s_sp].d;
	if (d2 == 0.0) {
		wprintln(E_DIV_BY_ZERO, s_cur_line_num);
		enl();
	}
	s_stack[s_sp++].d = d1 / d2;
}

static void pow_op(void)
{
	double d1, d2;
	int err;

	err = 0;
	d2 = s_stack[--s_sp].d;
	d1 = s_stack[--s_sp].d;
	if (d1 == 0.0 && d2 < 0.0) {
		err = 1;
		wprintln(E_ZERO_POW_NEG, s_cur_line_num);
		fprintf(stderr, "(0 ^");
		print_num(stderr, d2);
		putc(')', stderr);
		enl();
	}
	if (d1 < 0 && d2 != floor(d2)) {
		err = 1;
		eprintln(E_NEG_POW_REAL, s_cur_line_num);
		putc('(', stderr);
		print_num(stderr, d1);
		putc('^', stderr);
		print_num(stderr, d2);
		putc(')', stderr);
		enl();
		s_fatal = 1;
	}
	errno = 0;
	s_stack[s_sp++].d = pow(d1, d2);
	if (!err && errno == ERANGE) {
		wprintln(E_OP_OVERFLOW, s_cur_line_num);
		enl();
	}
}

static void neg_op(void)
{
	s_stack[s_sp-1].d = -s_stack[s_sp-1].d;
}

static void gosub_op(void)
{
	int gopc;

	gopc = code[s_pc++].id;
	if (s_gosub_sp >= s_gosub_stack_capacity) {
		eprintln(E_STACK_OFLOW, s_cur_line_num);
		enl();
		s_fatal = 1;
		return;
	}

	s_gosub_stack[s_gosub_sp++] = s_pc;
	s_pc = gopc;
}

static void return_op(void)
{
	if (s_gosub_sp == 0) {
		eprintln(E_STACK_UFLOW, s_cur_line_num);
		enl();
		s_fatal = 1;
		return;
	}

	s_pc = s_gosub_stack[--s_gosub_sp];
}

static void goto_op(void)
{
	s_pc = code[s_pc].id;
}

static void on_goto_op(void)
{
	int nlines;
	int i;
	
	nlines = code[s_pc++].id;
	i = round_to_int(s_stack[--s_sp].d);
	if (i < 1 || i > nlines) {
		eprintln(E_INDEX_RANGE, s_cur_line_num);
		enl();
		s_fatal = 1;
		return;
	}

	i--;
	s_pc = code[s_pc + i].id;
}

static void goto_if_true_op(void)
{
	int i;

	i = s_stack[--s_sp].d == 1.0;
	if (i == 1)
		s_pc = code[s_pc].id;
	else
		s_pc++;
}

static void less_op(void)
{
	double a, b;

	b = s_stack[--s_sp].d;
	a = s_stack[--s_sp].d;
	s_stack[s_sp++].d = a < b;
}

static void greater_op(void)
{
	double a, b;

	b = s_stack[--s_sp].d;
	a = s_stack[--s_sp].d;
	s_stack[s_sp++].d = a > b;
}

static void less_eq_op(void)
{
	double a, b;

	b = s_stack[--s_sp].d;
	a = s_stack[--s_sp].d;
	s_stack[s_sp++].d = a <= b;
}

static void greater_eq_op(void)
{
	double a, b;

	b = s_stack[--s_sp].d;
	a = s_stack[--s_sp].d;
	s_stack[s_sp++].d = a >= b;
}

static void eq_op(void)
{
	double a, b;

	b = s_stack[--s_sp].d;
	a = s_stack[--s_sp].d;
	s_stack[s_sp++].d = a == b;
}

static void not_eq_op(void)
{
	double a, b;

	b = s_stack[--s_sp].d;
	a = s_stack[--s_sp].d;
	s_stack[s_sp++].d = a != b;
}

static void eq_str_op(void)
{
	int a, b;

	b = s_stack[--s_sp].i;
	a = s_stack[--s_sp].i;
	s_stack[s_sp++].d = a == b;
}

static void not_eq_str_op(void)
{
	int a, b;

	b = s_stack[--s_sp].i;
	a = s_stack[--s_sp].i;
	s_stack[s_sp++].d = a != b;
}

/*
 * Thomas Wang's 32 Bit Mix Function:
 * http://www.cris.com/~Ttwang/tech/inthash.htm
 */
static unsigned int mix(unsigned int n)
{
	n = (n ^ 61) ^ (n >> 16);
	n = n + (n << 3);
	n = n ^ (n >> 4);
	n = n * 0x27d4eb2d;
	n = n ^ (n >> 15);
	return n;
}
	
static void randomize_op(void)
{
	unsigned int t;
	#if !defined(_WIN32)
	struct timeval tv;
	#endif

	#if defined(_WIN32)
		t = GetTickCount();
	#else
		gettimeofday(&tv, NULL);
		t = tv.tv_sec*1e6 + tv.tv_usec;
	#endif
	
	/* mix to avoid increasing sequence */
	t = mix(t);

 	srand(t);
}

static void ifun0_op(void)
{
	int ifun;

	ifun = code[s_pc++].id;
	s_stack[s_sp++].d = call_ifun0(ifun);
}

static void ifun1_op(void)
{
	int ifun;
	double d;

	ifun = code[s_pc++].id;
	d = s_stack[s_sp - 1].d;
	s_stack[s_sp - 1].d = call_ifun1(ifun, d);
	if (errno == EDOM) {
		eprintln(E_DOM, s_cur_line_num);
		fprintf(stderr, "( %s(", get_ifun_name(ifun));
		print_num(stderr, d);
		fprintf(stderr, ") )\n");
		s_fatal = 1;
	} else if (errno == ERANGE) {
		wprintln(E_OP_OVERFLOW, s_cur_line_num);
		fprintf(stderr, "(%s)\n", get_ifun_name(ifun));
	}
}

static void for_op(void)
{
	int i, rampos;
	double val;

	/* step, limit, var */
	for (i = 0; i < 3; i++) {
		rampos = code[s_pc++].id;
		val = s_stack[--s_sp].d;
		s_ram[rampos].d = val;
	}
}

static double sign(double d)
{
	if (d < 0.0)
		return -1.0;
	else if (d > 0.0)
		return 1.0;
	else
		return 0.0;
}

static void for_cmp_op(void)
{
	int var_pos, step_pos, limit_pos, endpc;
	double step, limit;

	var_pos = code[s_pc - 2].id;
	limit_pos = code[s_pc - 3].id;
	step_pos = code[s_pc - 4].id;
	endpc = code[s_pc++].id;

	step = s_ram[step_pos].d;
	limit = s_ram[limit_pos].d;
	if ((s_ram[var_pos].d - limit) * sign(step) > 0.0)
		s_pc = endpc;
}

static void next_op(void)
{
	int var_pos, step_pos;
	double step;

	/* go to for_cmp_op */
	s_pc = code[s_pc].id;

	step_pos = code[s_pc - 3].id;
	step = s_ram[step_pos].d;
	var_pos = code[s_pc - 1].id;
	s_ram[var_pos].d += step;
}

static void restore_op(void)
{
	restore_data();
}

static int s_input_pc;
static int s_input_pass;
static int s_input_comma;
static const char *s_input_p;
static char s_input_line[LINE_MAX_CHARS + 3];

static void input_op(void)
{
	int r;

	s_input_pass = 1;
	s_input_pc = s_pc - 1;
	s_input_comma = 0;
	s_print_column = 0;

retry:	r = get_line("? ", s_input_line, sizeof s_input_line, stdin);
	if (r == E_EOF) {
		eprint(E_VOID_INPUT);
		enl();
		s_fatal = 1;
	} else if (r == E_LINE_TOO_LONG) {
		eprint(r);
		enl();
		goto retry;
	}

	s_input_p = s_input_line;
}

static void retry_input(enum error_code ecode)
{
	eprint(ecode);
	enl();
	s_pc = s_input_pc;
}

static void input_num_op_pass1(void)
{
	size_t len;
	union data_elem delem;
	enum data_elem_type t;

	t = parse_data_elem(&delem, s_input_p, &len, DATA_ELEM_AS_IS);
	if (t == DATA_ELEM_NUM && errno == ERANGE) {
		eprint(E_CONST_OVERFLOW);
		putc('(', stderr);
		print_chars(stderr, s_input_p, len);
		putc(')', stderr);
		enl();
		s_pc = s_input_pc;
	} else if (t == DATA_ELEM_NUM) {
		s_input_p += len;
		/* Consume and check next token, that must be a comma
		 * or EOF. */
		t = parse_data_elem(&delem, s_input_p, &len,
		    DATA_ELEM_AS_IS);

		if (t == DATA_ELEM_COMMA || t == DATA_ELEM_EOF) {
			s_input_comma = (t == DATA_ELEM_COMMA);
			s_input_p += len;
			/* jump to the next input op */
			s_pc = code[s_pc].id;				
		} else {
			retry_input(E_SYNTAX);
		}
	} else if (t == DATA_ELEM_EOF) {
		retry_input(E_TOO_FEW_INPUT);
	} else if (t == DATA_ELEM_QUOTED_STR || t == DATA_ELEM_UNQUOTED_STR) {
		retry_input(E_TYPE_MISMATCH);
	} else {
		retry_input(E_SYNTAX);
	}
}

static void input_num_op_pass2(void)
{
	size_t len;
	union data_elem delem;

	/* It is a number, checked in pass 1 */
	parse_data_elem(&delem, s_input_p, &len, DATA_ELEM_AS_IS);
	s_input_p += len;

	/* push */
	s_stack[s_sp++].d = delem.num;
	
	/* Consume next token that we know is correct */
	parse_data_elem((union data_elem *) NULL, s_input_p, &len,
	    DATA_ELEM_AS_IS);
	s_input_p += len;
	
	/* Skip goto s_pc */
	s_pc++;
}

static void input_num_op(void)
{
	if (s_input_pass == 1) {
		/* First pass, only check type. */
		input_num_op_pass1();
	} else {
		/* second pass */
		input_num_op_pass2();
	}
}

static void input_str_op_pass1(void)
{
	size_t len;
	union data_elem delem;
	enum data_elem_type t;

	t = parse_data_elem(&delem, s_input_p, &len,
		DATA_ELEM_AS_UNQUOTED_STR);
	if (t == DATA_ELEM_QUOTED_STR || t == DATA_ELEM_UNQUOTED_STR) {
		s_input_p += len;
		if (t == DATA_ELEM_QUOTED_STR &&
		    delem.str.start[delem.str.len] != '\"') {
			retry_input(E_STR_NOEND);
		} else {
			/* Consume and check next token, that must be a comma
			 * or EOF. */
			t = parse_data_elem(&delem, s_input_p, &len,
			    DATA_ELEM_AS_IS);

			if (t == DATA_ELEM_COMMA || t == DATA_ELEM_EOF) {
				s_input_comma = (t == DATA_ELEM_COMMA);
				s_input_p += len;
				/* jump to the next input op */
				s_pc = code[s_pc].id;				
			} else {
				retry_input(E_SYNTAX);
			}
		}
	} else if (t == DATA_ELEM_EOF) {
		retry_input(E_TOO_FEW_INPUT);
	} else if (t == DATA_ELEM_NUM) {
		retry_input(E_TYPE_MISMATCH);
	} else {
		retry_input(E_SYNTAX);
	}
}

static void input_str_op_pass2(void)
{
	int pos;
	size_t len;
	union data_elem delem;

	/* Taken as a string, checked in pass 1 */
	parse_data_elem(&delem, s_input_p, &len, DATA_ELEM_AS_UNQUOTED_STR);
	s_input_p += len;

	/* Add the string */
	if (add_string(delem.str.start, delem.str.len, &pos) != 0) {
		eprintln(E_NO_MEM, s_cur_line_num);
		enl();
		s_fatal = 1;
		return;
	}

	/* push */
	s_stack[s_sp++].i = pos;
	
	/* Consume next token that we know is correct */
	parse_data_elem((union data_elem *) NULL, s_input_p, &len,
	    DATA_ELEM_AS_IS);
	s_input_p += len;
	
	/* Skip goto s_pc */
	s_pc++;
}

static void input_str_op(void)
{
	if (s_input_pass == 1)
		input_str_op_pass1();
	else
		input_str_op_pass2();
}

static void input_end_op(void)
{
	enum data_elem_type t;

	if (s_input_pass == 1) {
		t = parse_data_elem((union data_elem *) NULL, s_input_p,
		    (size_t *) NULL, DATA_ELEM_AS_IS);
		if (t == DATA_ELEM_EOF && s_input_comma == 0) {
			s_pc = s_input_pc + 1;
			s_input_pass = 2;
			s_input_p = s_input_line;
		} else {
			retry_input(E_TOO_MUCH_INPUT);
		}
	}
}

static void end_op(void)
{
}

static void line_op(void)
{
	s_cur_line_num = code[s_pc++].id;
}

struct vm_op {
	void (*func)(void);
	signed char stack_inc;
	signed char stack_dec;
};

static struct vm_op vm_ops[] = {
	{ push_num_op, 1, 0 },
	{ push_str_op, 1, 0 },
	{ print_nl_op, 0, 0 },
	{ print_comma_op, 0, 0 },
	{ print_tab_op, 0, -1 },
	{ print_num_op, 0, -1 },
	{ print_str_op, 0, -1 },
	{ let_var_op, 0, -1 },
	{ let_list_op, 0, -2 },
	{ let_table_op, 0, -3 },
	{ let_strvar_op, 0, -1 },
	{ get_var_op, 1, 0 },
	{ get_strvar_op, 1, 0 },
	{ get_list_op, 0, 0 },
	{ get_table_op, 0, -1 },
	{ add_op, 0, -1 },
	{ sub_op, 0, -1 },
	{ mul_op, 0, -1 },
	{ div_op, 0, -1 },
	{ pow_op, 0, -1 },
	{ neg_op, 0, 0 },
	{ line_op, 0, 0 },
	{ gosub_op, 0, 0 },
	{ return_op, 0, 0 },
	{ goto_op, 0, 0 },
	{ on_goto_op, 0, -1 },
	{ goto_if_true_op, 0, -1 },
	{ less_op, 0, -1 },
	{ greater_op, 0, -1 },
	{ less_eq_op, 0, -1 },
	{ greater_eq_op, 0, -1 },
	{ eq_op, 0, -1 },
	{ not_eq_op, 0, -1 },
	{ eq_str_op, 0, -1 },
	{ not_eq_str_op, 0, -1 },
	{ for_op, 0, -3 },
	{ for_cmp_op, 0, 0 },
	{ next_op, 0, 0 },
	{ restore_op, 0, 0 },
	{ read_var_op, 0, 0 },
	{ read_list_op, 0, -1 },
	{ read_table_op, 0, -2 },
	{ read_strvar_op, 0, 0 },
	{ ifun0_op, 1, 0 },
	{ ifun1_op, 0, 0 },
	{ randomize_op, 0, 0 },
	{ input_op, 0, 0 },
	{ input_num_op, 1, 0 },
	{ input_str_op, 1, 0 },
	{ input_end_op, 0, 0 },
	{ input_list_op, 0, -2 },
	{ input_table_op, 0, -3 },
	{ check_init_var_op, 0, 0 },
	{ set_init_var_op, 0, 0 },
	{ end_op, 0, 0 },
};

int get_opcode_stack_inc(int opcode)
{
	return vm_ops[opcode].stack_inc;
}

int get_opcode_stack_dec(int opcode)
{
	return vm_ops[opcode].stack_dec;
}

static void free_ram(void)
{
	if (s_ram != NULL) {
		free(s_ram);
		s_ram = NULL;
	}
}

/**
 * Runs the current program stored in 'code' which needs an s_ram of size
 * 'ramsize' and the string constants stored in 'strings'.
 *
 * 'array_base_index' is the base index for arrays declared in the program
 * through OPTION BASE 0 or 1. Must be 0 or 1.
 * 
 * Caller must guarantee that 'ramsize * sizeof *s_ram' does not overflow a
 * signed int (use is_ram_too_big()).
 */
void run(int ramsize, int array_base_index, int stack_size)
{
	int ir;
	struct vm_op *vmop;

	assert(ramsize >= 0);
	assert(array_base_index == 0 || array_base_index == 1);
	assert(stack_size >= 0);

	reset_strings();
	reset_inited_vars();
	restore_data();

#if 0
	fprintf(stderr, "Allocating GOSUB stack (%d).\n",
		s_default_gosub_stack_capacity);
#endif
	if (alloc_gosub_stack() != 0) {
		eprint(E_NO_MEM);
		enl();
		return;
	}

#if 0
	fprintf(stderr, "Allocating stack (%d).\n", stack_size);
#endif
	if (alloc_stack(stack_size) != 0) {
		eprint(E_NO_MEM);
		enl();
		goto stack_fail;
	}

#if 0
	fprintf(stderr, "Allocating program ram (%d).\n", ramsize);
#endif
	if (ramsize > 0) {
		if ((s_ram = calloc(ramsize, sizeof *s_ram)) == NULL) {
			eprint(E_NO_MEM);
			enl();
			goto ram_fail;
		}
	}

#if 0
	fprintf(stderr, "Running program.\n");
#endif

	s_base_ix = array_base_index;
	s_fatal = 0;
	s_pc = 0;
	s_gosub_sp = 0;
	s_sp = 0;
	s_print_column = 0;
	srand(0);
	s_break = 0;
	signal(SIGINT, sigint_handler);
	while (!s_break && !s_fatal && code[s_pc].opcode != END_OP) {
		ir = s_pc++;
		vmop = &vm_ops[code[ir].opcode];
		assert(vmop->stack_inc + s_sp <= s_stack_capacity);
		vmop->func();
	}
	signal(SIGINT, SIG_DFL);
	if (s_print_column != 0) {
		s_print_column = 0;
		putc('\n', stdout);
	}
	if (s_break)
		printf("* break at %d *\n", s_cur_line_num);

	free_ram();
ram_fail:
	free_stack();
stack_fail:
	free_gosub_stack();
}

/* Returns 1 if a s_ram with ramsize (0 or positive) will be too big. */
int is_ram_too_big(int ramsize)
{
	assert(ramsize >= 0);
	return imul_overflows_int(ramsize, (int) sizeof *s_ram);
}

void set_gosub_stack_capacity(int capacity)
{
	s_default_gosub_stack_capacity = capacity;
}
