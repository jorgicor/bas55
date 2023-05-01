/* Program to generate the coeficiens for the polynomials used to approximate
 * sin, atan, exp, etc. functions in bas55, an implementation of the Minimal
 * BASIC
 *
 * Copyright (C) 2023 Jorge Giner Cordero
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * This program calculates two interpolation polynomials by Newton's
 * differences.
 * The polynomial called 'simple', computes the desired
 * function (sin, cos), etc at the points given by the desired interval
 * divided by the number of nodes.
 * The polynomial called 'chevi' computes the function at the Chevishev
 * nodes in the interval.
 * The program prints the nodes (P) and the coefficients (D) needed
 * to approximate the function at a point using Newton's method.
 */

#include <assert.h>
#include <stdio.h>
#include <math.h>

#define K_PI 3.14159265358979323846

enum {
	MAX_NODES = 32,
	DECIMALS = 16,
	REAL_WIDTH = 16 + 7,
};

typedef double (*fun_t)(double);

struct polynom {
	double p[MAX_NODES];
	double d[MAX_NODES];
	int nnodes;
	double lower, upper;
	fun_t proc;
};

static struct polynom simple_poly;
static struct polynom chevi_poly;

static void skip_bad_input(void)
{
	int c;

	while ((c = getchar()) != '\n') {
		;
	}
}

static void print_poly(struct polynom *poly)
{
	int i;

	printf("nnodes = %d\n", poly->nnodes);
	printf("range  = [%f, %f]\n", poly->lower, poly->upper);
	printf("P[] = {\n");
	for (i = 0; i < poly->nnodes; i++) {
		printf("\t% *.*e,\n", REAL_WIDTH, DECIMALS, poly->p[i]);
	}
	printf("}\n");
	printf("D[] = {\n");
	for (i = 0; i < poly->nnodes; i++) {
		printf("\t% *.*e,\n", REAL_WIDTH, DECIMALS, poly->d[i]);
	}
	printf("}\n");
}

static void calc_newton_diffs(struct polynom *poly)
{
	int i, j;

	for (j = 1; j < poly->nnodes; j++) {
		for (i = poly->nnodes - 1; i >= j; i--) {
			poly->d[i] = (poly->d[i] - poly->d[i-1]) /
				     (poly->p[i] - poly->p[i-j]);
		}
	}
}

static void make_poly_simple(struct polynom *poly, int nnodes,
	double lower, double upper, fun_t proc)
{
	int i;
	double delta;

	assert(nnodes <= MAX_NODES);

	poly->nnodes = nnodes;
	poly->lower = lower;
	poly->upper = upper;
	poly->proc = proc;

	delta = (upper - lower) / nnodes;
	poly->p[0] = lower;
	for (i = 1; i < nnodes - 1; i++) {
		poly->p[i] = lower + i*delta;
	}
	poly->p[i] = upper;

	for (i = 0; i < nnodes; i++) {
		poly->d[i] = proc(poly->p[i]);
	}

	calc_newton_diffs(poly);
}

static double calc_tk(int k, int nnodes)
{
	int nn;

	nn = 2 * nnodes;
	return cos((nn - 1 - 2*k) * K_PI / nn);
}

static double expand(double tk, double lower, double upper)
{
	return 0.5 * ((upper - lower)*tk + lower + upper);
}

static void make_poly_chevishev(struct polynom *poly, int nnodes,
	double lower, double upper, fun_t proc)
{
	int i;

	assert(nnodes <= MAX_NODES);

	poly->nnodes = nnodes;
	poly->lower = lower;
	poly->upper = upper;
	poly->proc = proc;

	for (i = 0; i < nnodes; i++) {
		poly->p[i] = expand(calc_tk(i, nnodes), lower, upper);
		poly->d[i] = proc(poly->p[i]);
	}

	calc_newton_diffs(poly);
}

static double interpolate(struct polynom *poly, double x)
{
	int i;
	double s;

	s = poly->d[poly->nnodes - 1];
	for (i = poly->nnodes-2; i >= 0; i--) {
		s = s * (x - poly->p[i]) + poly->d[i];
	}
	return s;
}

static int do_loop(void)
{
	double x, a, b, actual;
	int n, stop;

	printf("x? ");
	n = scanf("%lf", &x);
	stop = 0;
	if (n != 1) {
		stop = 1;
	} else {
		a = interpolate(&simple_poly, x);
		b = interpolate(&chevi_poly, x);
		actual = simple_poly.proc(x);
		printf("f(x)  = % *.*e", REAL_WIDTH, DECIMALS,
				actual);
		printf(" (actual value)\n");
		printf("f(x) ~= %*.*e", REAL_WIDTH, DECIMALS, a);
		printf(" (approx. with simple nodes)\n");
		printf("f(x) ~= % *.*e", REAL_WIDTH,
			DECIMALS, b);
		printf(" (approx. with chevishev nodes)\n");
		printf("error = % *e\n", 13, fabs(a - actual)); 
		printf("error = % *e (chevishev)\n", 13, fabs(b - actual)); 
	}
	return stop;
}

static double m_atan(double x){
	return atan(x);
}

static double m_sin(double x){
	return sin(x);
}

static double two_to_y(double x) {
	return pow(2, x);
}

int main(void)
{
	int fnum, n, nnodes, stop;
	double lower, upper;
	fun_t proc;

	do {
		printf("Select a function:\n");
		printf("1 - 2^x\n");
		printf("2 - sin(x)\n");
		printf("3 - atan(x)\n");
		printf("4 - quit\n");
		n = scanf("%d", &fnum);
		if (n == 0) {
			skip_bad_input();
		}
	} while (!(n == 1 && fnum >= 1 && fnum <= 4));

	switch (fnum) {
	case 1: proc = two_to_y; break;
	case 2: proc = m_sin; break;
	case 3: proc = m_atan; break;
	default: return 0;
	}

	printf("Interval lower bound: ");
	do {
		n = scanf("%lf", &lower);
		if (n == 0) {
			skip_bad_input();
		}
	} while (n != 1);
	printf("Lower bound = % f\n", lower);

	printf("Interval upper bound: ");
	do {
		n = scanf("%lf", &upper);
		if (n == 0) {
			skip_bad_input();
		}
	} while (n != 1);
	printf("Upper bound = % f\n", upper);

	printf("Number of nodes of the polynomial (n <= 32): ");
	do {
		n = scanf("%d", &nnodes);
		if (n == 0) {
			skip_bad_input();
		}
	} while (n != 1);
	if (nnodes > 0 && nnodes <= MAX_NODES) {
		printf("Nodes = %d\n", nnodes);
		make_poly_simple(&simple_poly, nnodes, lower, upper, proc); 
		make_poly_chevishev(&chevi_poly, nnodes, lower, upper, proc); 
		printf("----------------------------\n");
		printf("Polynomial with simple nodes\n");
		printf("----------------------------\n");
		print_poly(&simple_poly);
		printf("-------------------------------\n");
		printf("Polynomial with Chevishev nodes\n");
		printf("-------------------------------\n");
		print_poly(&chevi_poly);
		do {
			stop = do_loop();
		} while (!stop);
	}

	return 0;
}
