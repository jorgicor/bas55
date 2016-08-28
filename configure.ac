#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([bas55 - Minimal BASIC System],
	[1.10],
	[jorge.giner@hotmail.com],
	bas55,
	[http://jorgicor.sdfeu.org/bas55])

AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(-Wall -Werror gnu check-news color-tests parallel-tests)
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_YACC
AM_MISSING_PROG(HELP2MAN, help2man, $missing_dir)

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_SEARCH_LIBS([pow], [m])

AC_CONFIG_FILES([Makefile
		 ecma55/Makefile
                 doc/Makefile
		 data/Makefile
		 tests/Makefile])
AC_OUTPUT
