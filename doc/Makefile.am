# ===========================================================================
# bas55, an implementation of the Minimal BASIC programming language.
# ===========================================================================

AM_MAKEINFOHTMLFLAGS = --no-headers --no-split
info_TEXINFOS = bas55.texi
dist_man_MANS = bas55.1

bas55.1: $(top_srcdir)/src/ecma55.c $(top_srcdir)/configure.ac
	$(HELP2MAN) --name="ECMA-55 Minimal BASIC interpreter and editor" --version-string="$(PACKAGE_STRING)" --output=$@ $(top_builddir)/src/bas55$(EXEEXT)
