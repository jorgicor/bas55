# ---------------------------------------------------------------------------
# Copyright (C) 2023 Jorge Giner Cordero
# This file is part of bas55 (ECMA-55 Minimal BASIC System).
# bas55 license: GNU GPL v3 or later.
# ---------------------------------------------------------------------------

AM_MAKEINFOHTMLFLAGS = --no-headers --no-split
info_TEXINFOS = bas55.texi
bas55_TEXINFOS = fdl.texi
dist_man_MANS = bas55.1

bas55.1: $(top_srcdir)/src/ecma55.c $(top_srcdir)/configure.ac
	$(HELP2MAN) --name="ECMA-55 Minimal BASIC interpreter and editor" --version-string="$(PACKAGE_STRING)" --output=$@ $(top_builddir)/src/bas55$(EXEEXT)
