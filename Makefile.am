# ---------------------------------------------------------------------------
# Copyright (C) 2023 Jorge Giner Cordero
# This file is part of bas55 (ECMA-55 Minimal BASIC System).
# bas55 license: GNU GPL v3 or later. <https://gnu.org/licenses/gpl.html>
# ---------------------------------------------------------------------------

# bas55.1 man page depends on bas55 executable being built.
# This order of SUBDIRS ensures it.
SUBDIRS = src doc . tests

dist_doc_DATA = AUTHORS README NEWS COPYING
dist_noinst_DATA = README.md TODO THANKS
dist_noinst_SCRIPTS = mkwin bootstrap
dist_pkgdata_DATA = data/SIEVE.BAS data/HAMURABI.BAS data/BAGELS.BAS \
		    data/README
EXTRA_DIST = tools/Makefile.newton tools/newton.c
