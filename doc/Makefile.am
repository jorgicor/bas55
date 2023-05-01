# Copyright (C) 2023 Jorge Giner Cordero
#
# This file is part of bas55, an implementation of the Minimal BASIC
# programming language.
#
# bas55 is free software: you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# bas55 is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# bas55.  If not, see <https://www.gnu.org/licenses/>.

AM_MAKEINFOHTMLFLAGS = --no-headers --no-split
info_TEXINFOS = bas55.texi
bas55_TEXINFOS = fdl.texi
dist_man_MANS = bas55.1

bas55.1: $(top_srcdir)/src/ecma55.c $(top_srcdir)/configure.ac
	$(HELP2MAN) --name="ECMA-55 Minimal BASIC interpreter and editor" --version-string="$(PACKAGE_STRING)" --output=$@ $(top_builddir)/src/bas55$(EXEEXT)
