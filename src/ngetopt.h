/* ===========================================================================
 * bas55, an implementation of the Minimal BASIC programming language.
 *
 * Handling of command line options, similar to getopt.
 * ===========================================================================
 */

#ifndef NGETOPT_H
#define NGETOPT_H

struct ngetopt_opt {
	const char *name;
	int has_arg;
	int val;
};

struct ngetopt {
	int optind;
	char *optarg;
	int argc;
	char *const *argv;
	struct ngetopt_opt *ops;
	int subind;
	char str[3];
};

void ngetopt_init(struct ngetopt *p, int argc, char *const *argv,
	struct ngetopt_opt *ops);
int ngetopt_next(struct ngetopt *p);

#endif
