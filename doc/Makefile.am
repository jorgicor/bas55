# ---------------------------------------------------------------------------
# Copyright (C) 2023 Jorge Giner Cordero
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.
# ---------------------------------------------------------------------------

AM_MAKEINFOHTMLFLAGS = --no-headers --no-split
info_TEXINFOS = bas55.texi
bas55_TEXINFOS = fdl.texi
dist_man_MANS = bas55.1

bas55.1: $(top_srcdir)/src/ecma55.c $(top_srcdir)/configure.ac
	$(HELP2MAN) --name="ECMA-55 Minimal BASIC interpreter and editor" --version-string="$(PACKAGE_STRING)" --output=$@ $(top_builddir)/src/bas55$(EXEEXT)
