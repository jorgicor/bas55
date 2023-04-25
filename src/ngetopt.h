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
