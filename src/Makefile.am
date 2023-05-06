# ---------------------------------------------------------------------------
# Copyright (C) 2023 Jorge Giner Cordero
# This file is part of bas55 (ECMA-55 Minimal BASIC System).
# bas55 license: GNU GPL v3 or later.
# ---------------------------------------------------------------------------

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
