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

BUILT_SOURCES = grammar.h
AM_YFLAGS = -d

# When using per object flags (bas55_CFLAGS, bas55_CPPFLAGS, etc), automake
# generates the object files with bas55_ prefixed, just in case we build the
# same program with different flags. But we don't need it for this project;
# AM_CFLAGS is enough.
AM_CPPFLAGS = $(LIBEDIT_CFLAGS)
AM_CFLAGS = $(WARN_CFLAGS)

bin_PROGRAMS = bas55
bas55_LDADD = $(LIBEDIT_LIBS)
bas55_SOURCES =	ecma55.c ecma55.h bmath.c \
		cmd.c code.c \
		codedvar.c data.c \
		datalex.c edit.c err.c \
		grammar.y ifun.c lex.c line.c list.h \
		ngetopt.c ngetopt.h \
		arraydsc.c arraydsc.h \
		dbg.c dbg.h \
		getlin.c parse.c str.c util.c vm.c 
