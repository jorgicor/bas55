# Copyright (C) 2023 Jorge Giner Cordero
#
# This file is part of bas55, an implementation of the Minimal BASIC
# programming language.
#
# bas55 is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# bas55 is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# bas55. If not, see <https://www.gnu.org/licenses/>.

# bas55.1 man page depends on bas55 executable being built.
# This order of SUBDIRS ensures it.
SUBDIRS = src doc . tests

dist_doc_DATA = AUTHORS README NEWS COPYING
dist_noinst_DATA = README.md TODO
dist_noinst_SCRIPTS = mkwin bootstrap
dist_pkgdata_DATA = data/sieve.bas
EXTRA_DIST = tools/Makefile.newton tools/newton.c

