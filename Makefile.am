# ---------------------------------------------------------------------------
# Copyright (C) 2023 Jorge Giner Cordero
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.
# ---------------------------------------------------------------------------

# bas55.1 man page depends on bas55 executable being built.
# This order of SUBDIRS ensures it.
SUBDIRS = src doc . tests

dist_doc_DATA = AUTHORS README NEWS COPYING
dist_noinst_DATA = README.md TODO
dist_noinst_SCRIPTS = mkwin bootstrap
dist_pkgdata_DATA = data/SIEVE.BAS data/HAMURABI.BAS data/BAGELS.BAS \
		    data/README
EXTRA_DIST = tools/Makefile.newton tools/newton.c

