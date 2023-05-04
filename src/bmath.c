/* --------------------------------------------------------------------------
 * Copyright (C) 2023 Jorge Giner Cordero
 * License: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>
 * --------------------------------------------------------------------------
 */

/* Implementation of mathematical functions sin, cos, etc.
 *
 * The aproximations for some of the mathematical funcionts like sin,
 * cos, exp, etc are calculated using Newton's divided differences
 * interpolation polynomials.
 *
 * The constants P and D are needed for these calculations and are
 * obtained by using the program tools/newton.c included with bas55.
 * The P constants are actually not needed, the can be calculated
 * on the fly, but I keep the algorithm using them just in case
 * in the future we use the Chevishev nodes (P would contain the
 * precalculated Chevished nodes in that case).
 *
 * I don't use the Chevyshev nodes because although in overall they
 * approximate better, for the important points (for example sin(0),
 * sin(pi/2)) they don't give fully exact results, while the simplest
 * Lagrange nodes do.
 */

#include <config.h>
#include "ecma55.h"
#include <math.h>
#include <errno.h>
#include <limits.h>
#include <inttypes.h>
#include <float.h>
#include <assert.h>

# define K_E		2.7182818284590452354	/* e */
# define K_LOG2E	1.4426950408889634074	/* log_2 e */
# define K_LOG10E	0.43429448190325182765	/* log_10 e */
# define K_LN2		0.69314718055994530942	/* log_e 2 */
# define K_LN10		2.30258509299404568402	/* log_e 10 */
# define K_PI		3.14159265358979323846	/* pi */
# define K_PI_2		1.57079632679489661923	/* pi/2 */
# define K_PI_4		0.78539816339744830962	/* pi/4 */
# define K_1_PI		0.31830988618379067154	/* 1/pi */
# define K_2_PI		0.63661977236758134308	/* 2/pi */
# define K_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
# define K_SQRT2	1.41421356237309504880	/* sqrt(2) */
# define K_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */

static const double K_NAN = 0.0/0.0;
static const double K_POS_INF = 1.0/0.0;
static const double K_NEG_INF = -1.0/0.0;

union mix {
	double d;
	uint64_t n;
};

/* Compute approximation by Newton's differences method.
 * n is number of iterations. */
static double newton(double x, const double p[], const double d[], int n)
{
	double s, dif;

	s = 0;
	if (n > 0) {
		n--;
		s = d[n];
		n--;
		while (n >= 0) {
			dif = x - p[n];
			s *= dif;
			s += d[n];
			n--;
		}
	}
	return s;
}

int bm_isinf(double x)
{
	return isinf(x);
}

int bm_isnan(double x)
{
	return isnan(x);
}

/* To support bm_floor.
 * This is the only place where we manipulate the bits of
 * a double float directly... */
static double bm_modf(double x, double *iptr)
{
	union mix mix;
	int e;
	uint64_t mask;

	if (x == 0 || bm_isnan(x) || bm_isinf(x)) {
		*iptr = x;
		return x;
	}

	mask = -1;
	mix.d = x;
	e = (int) ((mix.n >> 52) & 0x7ff) - 1023;
	if (e >= 0) {
		if (e > 52) {
			e = 52;
		}
		mask <<= 52 - e;
		mix.n &= mask;
	} else {
		mix.d = 0;
	}
	*iptr = mix.d;
	return x - mix.d;
}

/* Floor was not giving the same results on Windows, so we had
 * to reimplement it.
 */
double bm_floor(double x)
{
	double ix;

	if (x == 0 || bm_isnan(x) || bm_isinf(x)) {
		return x;
	}

	if (x < 0) {
		x = bm_modf(-x, &ix);
		if (x > 0) {
			ix += 1;
		}
		ix = -ix;
	} else {
		x = bm_modf(x, &ix);
	}

	return ix;
}

double bm_fabs(double x)
{
	if (bm_isnan(x)) {
		return x;
	} else if (bm_isinf(x)) {
		return K_POS_INF;
	} else if (x == 0) {
		return 0;
	} else if (x < 0) {
		return -x;
	} else {
		return x;
	}
}

/* ldexp was not returning ERANGE on Windows, we had wrap it. */
double bm_ldexp(double x, int n)
{
	if (bm_isnan(x) || bm_isinf(x)) {
		return x;
	} else if (x == 0) {
		return 0;
	}

	x = ldexp(x, n);
	if (errno == 0) {
       		if (bm_isinf(x)) {
			// overflow
			errno = ERANGE;
		} else if (n < 0 && x == 0) {
			// underflow
			errno = ERANGE;
		}
	}
	return x;
}

/* Special cases:
 *  nan -> nan
 * -inf -> 0
 *  inf -> inf
 *  0   -> 1
 */
double bm_exp(double x)
{
	// Values for 2^x, x in [-0.5, 0.5]
	// for the Newton polynomial.
	static const double P[] = {
		-5.0000000000000000e-01,
		-4.3333333333333335e-01,
		-3.6666666666666670e-01,
		-2.9999999999999999e-01,
		-2.3333333333333334e-01,
		-1.6666666666666669e-01,
		-9.9999999999999978e-02,
		-3.3333333333333326e-02,
		 3.3333333333333326e-02,
		 9.9999999999999978e-02,
		 1.6666666666666663e-01,
		 2.3333333333333328e-01,
		 3.0000000000000004e-01,
		 3.6666666666666670e-01,
		 5.0000000000000000e-01,
	};

	static const double D[] = {
		 7.0710678118654757e-01,
		 5.0162992435101761e-01,
		 1.7793110439570800e-01,
		 4.2075477524401439e-02,
		 7.4622105068904848e-03,
		 1.0587561011207378e-03,
		 1.2518234718347126e-04,
		 1.2686570379691785e-05,
		 1.1249617811302552e-06,
		 8.8572445966323754e-08,
		 7.4620798617129940e-09,
		-5.4008382381826045e-09,
		 2.0995766261041853e-08,
		-6.1257496733847167e-08,
		 1.2962762270223545e-07,
	};

	double y, z, zi;

	assert(NELEMS(P) == NELEMS(D));
	if (bm_isnan(x)) {
		return x;
	}

	if (bm_isinf(x)) {
		if (x < 0) {
			return 0;
		} else {
			return x;
		}
	} else if (x == 0) {
		return 1.0;
	}

	// exp(x)=e^x=2^y with y=x*log2(e)
	y = x * K_LOG2E;

	// get fractional part in z
	zi = bm_floor(y + 0.5);
	if (zi > INT_MAX) {
		errno = ERANGE;
		return K_POS_INF;
	} else if (zi < INT_MIN) {
		errno = ERANGE;
		return 0;
	}
	z = y - zi;
	// z is now in [-0.5, 0.5]
	
	// approximate function 2^z
	z = newton(z, P, D, NELEMS(P));

	// scale back
	z = bm_ldexp(z, (int) zi);

	// mix.d = z;
	// printf(" %.16lx errno=%d\n", mix.n, errno);

	return z;
}

/* Special cases:
 *  nan -> 0
 *  0   -> 0
 *  inf -> nan
 */
double bm_sin(double x)
{
	// Values for sin(x), x in [0, pi/2]
	// for the Newton polynomial.
	static const double P[] = {
		 0.0000000000000000e+00,
		 1.0471975511965977e-01,
		 2.0943951023931953e-01,
		 3.1415926535897931e-01,
		 4.1887902047863906e-01,
		 5.2359877559829882e-01,
		 6.2831853071795862e-01,
		 7.3303828583761832e-01,
		 8.3775804095727813e-01,
		 9.4247779607693793e-01,
		 1.0471975511965976e+00,
		 1.1519173063162573e+00,
		 1.2566370614359172e+00,
		 1.3613568165555769e+00,
		 1.5707963267948966e+00,
	};

	static const double D[] = {
		 0.0000000000000000e+00,
		 9.9817329737079952e-01,
		-5.2216487303131991e-02,
		-1.6438920711832500e-01,
		 8.6471667527368134e-03,
		 8.0310113351671746e-03,
		-4.2801512464208519e-04,
		-1.8464263573077482e-04,
		 1.0050904212505487e-05,
		 2.4452952585758986e-06,
		-1.3715354434788969e-07,
		-2.0909578220438959e-08,
		 1.2140729467765088e-09,
		 1.6027154930007635e-10,
		-8.0464379302615368e-11,
	};

	double w;
	int s;

	assert(NELEMS(P) == NELEMS(D));
	if (bm_isnan(x) || x == 0) {
		return x;
	} else if (bm_isinf(x)) {
		errno = EDOM;
		return K_NAN;
	} else if (!isnormal(x)) {
		return x;
	}

	// w will be in [0, 1], 0 is 0, 1 is 2*pi.
	w = x / (2 * K_PI);
	w -= bm_floor(w + 0.5);

	// After this, w will be [0-1] or [-1,0] is first quadrant,
	// [1,2] or [-2,-1] second, [2,3] or [-2,-3] third,
	// [3,4] or [-4,-3] fourth.
	w = 4 * w;

	// We translate everything to the first quadrant (0-pi/2),
	// that is, w will be in [0, 1]
	// (probably there is a clever way to do this...).
	s = 1;
	if (w >= 3) {
		w = 4 - w;
		s = -1;
	} else if (w >= 2) {
		w = w - 2;
		s = -1;
	} else if (w >= 1) {
		w = 2 - w;
	} else if (w <= -3) {
		w = 4 + w;
	} else if (w <= -2) {
		w = 2 + w;
	} else if (w <= -1) {
		w = 2 + w;
		s = -1;
	} else if (w < 0) {
		w = -w;
		s = -1;
	}

	return s * newton(w * K_PI_2, P, D, NELEMS(P));
}

/* arctan returns value in [-pi/2, pi/2].
 * Special cases:
 *  nan -> 0 
 *  0   -> 0
 * -inf -> -pi/2
 * +inf ->  pi/2
 */
double bm_atan(double x)
{
	// Values for atan(x), x in [0, 1]
	// for the Newton polynomial.
	static const double P[] = {
		 0.0000000000000000e+00,
		 6.6666666666666666e-02,
		 1.3333333333333333e-01,
		 2.0000000000000001e-01,
		 2.6666666666666666e-01,
		 3.3333333333333331e-01,
		 4.0000000000000002e-01,
		 4.6666666666666667e-01,
		 5.3333333333333333e-01,
		 5.9999999999999998e-01,
		 6.6666666666666663e-01,
		 7.3333333333333328e-01,
		 8.0000000000000004e-01,
		 8.6666666666666670e-01,
		 1.0000000000000000e+00,
	};

	static const double D[] = {
		 0.0000000000000000e+00,
		 9.9852245663735717e-01,
		-6.5789466184529855e-02,
		-3.1193196337680179e-01,
		 1.1958262994556518e-01,
		 1.2460123133093823e-01,
		-1.3080645421218065e-01,
		 4.7955815623348941e-04,
		 7.7897412811846825e-02,
		-5.5900089947184900e-02,
		-1.4579407460506809e-03,
		 3.1772948712582232e-02,
		-2.6004925029627670e-02,
		 6.7355124677991761e-03,
		 5.7422987205191468e-03,
	};

	int s, inv;

	assert(NELEMS(P) == NELEMS(D));
	if (bm_isnan(x) || x == 0) {
		return x;
	}

	if (bm_isinf(x)) {
		if (x > 0) {
			return K_PI_2;
		} else {
			return -K_PI_2;
		}
	}

	// TODO: sub normal number
	
	// atan(-x) = -atan(x)
	if (x < 0) {
		x = -x;
		s = -1;
	} else {
		s = 1;
	}

	// (1) atan(x) = pi/2 - atan(1/x)
	if (x > 1) {
		x = 1 / x;
		inv = 1;
	} else {
		inv = 0;
	}

	// calculate atan in [0, 1]
	x = newton(x, P, D, NELEMS(P));
	if (inv) {
		// because of (1)
		x = K_PI_2 - x;
	}
	return x * s;
}

/* Special cases:
 *  nan -> nan
 *  0   -> 1
 *  inf -> nan
 */
double bm_cos(double x)
{
	if (bm_isnan(x)) {
	       return x;
       	} else if (x == 0) {
		return 1.0;
	} else if (bm_isinf(x) != 0) {
		errno = EDOM;
		return K_NAN;
	} else if (!isnormal(x)) {
		return x;
	}

	/* TODO: sub normal number? */

	return bm_sin(x + K_PI_2);
}

/* Special cases:
 *  nan -> nan
 *  0   -> 0
 *  inf -> nan
 */
double bm_tan(double x)
{
	double c;

	if (bm_isnan(x) || x == 0) {
		return x;
	} else if (bm_isinf(x) != 0) {
		errno = EDOM;
		return K_NAN;
	}

	c = bm_cos(x);
	if (c == 0) {
		errno = ERANGE;
	}
	return bm_sin(x) / c;
}

/* Some identities:
 *  ln(a b) = ln(a) + ln(b)
 *  ln (m 2^e) = log2(m 2^e) ln(2) = e ln(2) + ln(m)
 * 
 * Special cases:
 *  nan -> nan
 *  < 0 -> nan
 * +inf -> +inf
 *  0   -> -inf
 *  1   -> 0
 */ 
double bm_log(double x)
{
	int i, e;
	double b, bb, r, d;

	if (bm_isnan(x)) {
		return x;
	} else if (x < 0) {
		errno = EDOM;
		return K_NAN;
	}

	if (bm_isinf(x)) { // && x >= 0
		return x;
	} else if (x == 0) {
		errno = ERANGE;
		return K_NEG_INF;
	} else if (x == 1) {
		return 0;
	}

	// extract mantissa and exponent
	x = frexp(x, &e);
	// x is in [0.5, 1[

	// we use the series ln(x)=2*sum
	// where sum is, for k=0,1,2... 
	//
	//    1     / z-1 \ 2k+1
	// ------ x |-----|
	//  2k+1    \ z+1 /
	
	b = x - 1.0;
	r = x + 1.0;
	b /= r;
	bb = b * b;
	r = b;
	d = 1.0;
	for (i = 0; i < 15; i++) {
		b *= bb;
		d += 2.0;
		r += b / d;
	}
	r *= 2.0;
	return e * K_LN2 + r;
}

/* Special cases:
 *  nan -> nan
 *  < 0 -> nan
 *  0   -> 0
 *  inf -> inf
 */
double bm_sqrt(double x)
{
	int i, e;
	double r, m, t;

	if (bm_isnan(x)) {
		return x;
	} else if (x < 0) {
		errno = EDOM;
		return K_NAN;
	} else if (x == 0 || bm_isinf(x)) {
		return x;
	}

	// extract mantissa and exponent
	// x = 2^e * m
	m = frexp(x, &e);
	// m is in [0.5, 1[
	
	// approx by Newton
	r = 1.0;
	for (i = 0; i < 5; i++) {
		t = m / r;
		t += r;
		r = 0.5 * t;
	}

	// if e is odd, sqrt(x)=2^(e/2 -1) * sqrt(2) * sqrt(m)
	// else sqrt(x)=2^(e/2) sqrt(m)
	if (e & 1) {
		r *= K_SQRT2;
		e--;
	}
	return bm_ldexp(r, e / 2);
}

/* Some cases:
 *
 * if x any, y=0: 1
 * if x=1, y any: 1
 * if x=0, y>0: 0
 * if x=0, y<0: Inf
 * if x>0, y>0: exp(y*log(x))
 * if x>0, y<0: 1/exp(-y*log(x))
 * if x<0, y not integer: error NaN
 * if x<0, y>0 even: exp(y*log(-x))
 * if x<0, y>0 odd: -exp(y*log(-x))
 * if x<0, y<0 even: 1/exp(log(-x))
 * if x<0, y<0 odd: -1/exp(log(-x))
 */
double bm_pow(double x, double y)
{
	double z;
	int s, inv;

	
	if (y == 0 || x == 1) {
		return 1;
	} else if (isnan(x) || isnan(y)) {
		return K_NAN;
	}
	
	s = 1;
	if (x == 0) {
		if (y > 0) {
			return 0.0;
		} else {
			errno = ERANGE;
			return K_POS_INF;
		}
	} else if (x < 0) {
		x = -x;
		if (x == 1 && isinf(y)) {
			return 1;
		} else if (y != bm_floor(y)) {
			errno = EDOM;
			return K_NAN;
		} else if (!isinf(y) && fmod(y, 2) != 0) {
			s = -1;
		}
	}

	inv = 0;
	if (y < 0) {
		y = -y;
		inv = 1;
	}

	z = bm_exp(y * bm_log(x));
	if (inv) {
		z = 1 / z;
	}
	if (!inv && y == bm_floor(y) && x == bm_floor(x)) {
		z = bm_floor(z + 0.5);
	}
	return s * z;
}
