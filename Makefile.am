# ===========================================================================
# bas55, an implementation of the Minimal BASIC programming language.
# ===========================================================================

# bas55.1 man page depends on bas55 executable being built.
# This order of SUBDIRS ensures it.
SUBDIRS = src doc . tests

dist_doc_DATA = AUTHORS README NEWS COPYING
dist_noinst_DATA = README.md TODO
dist_noinst_SCRIPTS = mkwin bootstrap
dist_pkgdata_DATA = data/sieve.bas
EXTRA_DIST = tools/Makefile.newton tools/newton.c

