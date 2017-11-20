#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([bas55 (ECMA-55 Minimal BASIC Interpreter)],
	[1.12],
	[jorge.giner@hotmail.com],
	bas55,
	[http://jorgicor.sdfeu.org/bas55])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(-Wall -Werror -Wportability subdir-objects
		 color-tests parallel-tests)
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_YACC
AM_MISSING_PROG(HELP2MAN, help2man, $missing_dir)

# Supported flags.
AX_CHECK_COMPILE_FLAG([-Wall], [WARN_CFLAGS="-Wall"], [], [])
AX_CHECK_COMPILE_FLAG([-Wdeclaration-after-statement],
	      [WARN_CFLAGS="$WARN_CFLAGS -Wdeclaration-after-statement"],
	      [], [])
AC_SUBST(WARN_CFLAGS)

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_SEARCH_LIBS([pow], [m])

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 doc/Makefile
		 tests/Makefile])
AC_OUTPUT
