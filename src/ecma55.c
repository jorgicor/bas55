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
 * main(), handling of command line options.
 * ===========================================================================
 */

#include <config.h>
#include "ecma55.h"
#include "ngetopt.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

void print_copyright(FILE *f)
{
	static const char *copyright =
"Copyright (C) " COPYRIGHT_YEARS " Jorge Giner Cordero.\n";

	fputs(copyright, f);
}

static void print_license(FILE *f)
{
	static const char *license[] = {
"bas55, an implementation of the Minimal BASIC programming language.",
"",
"bas55 is free software: you can redistribute it and/or modify it under the",
"terms of the GNU General Public License as published by the Free Software",
"Foundation, either version 3 of the License, or (at your option) any later",
"version.",
"",
"bas55 is distributed in the hope that it will be useful, but WITHOUT ANY",
"WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS",
"FOR A PARTICULAR PURPOSE. See the GNU General Public License for more",
"details.",
"",
"You should have received a copy of the GNU General Public License along with",
"bas55. If not, see <https://www.gnu.org/licenses/>."
       	};

	int i;

	for (i = 0; i < NELEMS(license); i++) {
		fprintf(f, "%s\n", license[i]);
	}
}

void print_version(FILE *f)
{
	fputs(PACKAGE_STRING, f);
	#if defined(HAVE_LIBEDIT)
		fputs(" (with libedit support)", f); 
	#endif
	fputs("\n", f);
}

static void print_help(const char *argv0)
{
	static const char *help =
"Usage: %s [OPTION]... [FILE.BAS]\n"
"\n"
"Run FILE.BAS conforming to the Minimal BASIC programming language as\n"
"defined by the ECMA-55 standard.\n"
"\n"
"If FILE.BAS is not specified, start in editor mode.\n"
"\n"
"Options:\n"
"  -h, --help         Display this help and exit.\n"
"  -v, --version      Output version information and exit.\n"
"  -l, --license      Display the license text and exit.\n"
"  -g n, --gosub n    Allocate n bytes for the GOSUB stack.\n"
"  -d, --debug        Enable debug mode.\n"
"\n"
"Examples:\n"
"  " PACKAGE "              Start in editor mode.\n"
"  " PACKAGE " prog.bas     Run prog.bas .\n"
"\n"
"Report bugs to: <" PACKAGE_BUGREPORT ">.\n"
"Home page: <" PACKAGE_URL ">.\n";

	printf(help, argv0);
}

static void read_gosub_stack_capacity(const char *optarg)
{
	int n;
	size_t len;

	if (!isdigit(optarg[0])) {
		goto error;
	}

	n = parse_int(optarg, &len);
	if (optarg[len] != '\0') {
		goto error;
	}

	if (n <= 0) {
		goto error;
	}

	if (errno == ERANGE) {
		eprogname();
		fprintf(stderr, "GOSUB stack size is too big: %s\n", optarg);
		exit(EXIT_FAILURE);
	}

	set_gosub_stack_capacity(n);
	return;
	
error:	eprogname();
	fprintf(stderr, "bad GOSUB stack size: %s\n", optarg);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	int c;
	struct ngetopt ngo;

	static struct ngetopt_opt ops[] = {
		{ "version", 0, 'v' },
		{ "help", 0, 'h' },
		{ "license", 0, 'l' },
		{ "gosub", 1, 'g' },
		{ "debug", 0, 'd' },
		{ NULL, 0, 0 },
	};

	/* This is required for all our assumptions about overflow to work. */
	assert(((size_t) (-1)) >= INT_MAX);
	
	ngetopt_init(&ngo, argc, argv, ops);
	do {
		c = ngetopt_next(&ngo);
		switch (c) {
		case 'v':
			print_version(stdout);
			exit(EXIT_SUCCESS);
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'l':
			print_copyright(stdout);
			fputs("\n", stdout);
			print_license(stdout);
			exit(EXIT_SUCCESS);
		case 'g':
			read_gosub_stack_capacity(ngo.optarg);
			break;
		case 'd':
			s_debug_mode = 1;
			break;
		case '?':
			eprogname();
			fprintf(stderr, "unrecognized option %s\n",
				ngo.optarg);
			exit(EXIT_FAILURE);
		case ':':
			eprogname();
			fprintf(stderr, "%s needs an argument\n",
				ngo.optarg);
			exit(EXIT_FAILURE);
		case ';':
			eprogname();
			fprintf(stderr, "%s does not allow for arguments\n",
				ngo.optarg);
			exit(EXIT_FAILURE);
		}
	} while (c != -1);

	if (argc - ngo.optind > 1) {
		eprogname();
		fprintf(stderr, "wrong number of arguments\n");
		exit(EXIT_FAILURE);
	}

	get_line_init();

	if (argc == ngo.optind) {
		s_debug_mode = 1;
		edit();
		return 0;
	}

	if (load(argv[ngo.optind], MAX_ERRORS, 1) == 0) {
		run_cmd(NULL, 0);
	} else {
		exit(EXIT_FAILURE);
	}

	return 0;
}
