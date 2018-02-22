#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# ===========================================================================
# bas55, an implementation of the Minimal BASIC programming language.
# ===========================================================================

AC_INIT([bas55],
	[1.14],
	[jorge.giner@hotmail.com],
	bas55,
	[http://jorgicor.sdfeu.org/bas55])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(-Wall -Werror -Wportability subdir-objects
		 color-tests parallel-tests)
AC_CONFIG_HEADERS([config.h])

AC_DEFINE([COPYRIGHT_YEARS], ["2018"], [Years of copyright])
AC_SUBST(COPYRIGHT_YEARS, [2018])

AC_ARG_WITH([termcap],
  [AS_HELP_STRING([--with-termcap],
		 [use termcap with readline or libedit])],
		 [],
		 [with_termcap=no])

AC_ARG_WITH([curses],
  [AS_HELP_STRING([--with-curses],
		 [use curses with readline or libedit])],
		 [],
		 [with_curses=no])

AC_ARG_WITH([ncurses],
  [AS_HELP_STRING([--with-ncurses],
		 [use ncurses with readline or libedit])],
		 [],
		 [with_ncurses=no])

# AC_ARG_WITH([readline],
#   [AS_HELP_STRING([--with-readline],
# 		 [support fancy command line editing])],
# 		 [],
# 		 [with_readline=no])

AC_ARG_WITH([libedit],
  [AS_HELP_STRING([--with-libedit],
 		 [support fancy BSD command line editing])],
 		 [],
 		 [with_libedit=no])

# Checks for programs.
# PKG_PROG_PKG_CONFIG
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
AC_SEARCH_LIBS([pow], [m])

# AS_IF([test "x$with_readline" != xno], [check_termlib=yes])
AS_IF([test "x$with_libedit" != xno], [check_termlib=yes])

# Check that only one of --with-termcap, --with-curses, etc is used.
n_termlibs=0
AS_IF([test "x$with_termcap" != xno], [n_termlibs=$(expr "$n_termlibs" + 1)])
AS_IF([test "x$with_curses" != xno], [n_termlibs=$(expr "$n_termlibs" + 1)])
AS_IF([test "x$with_ncurses" != xno], [n_termlibs=$(expr "$n_termlibs" + 1)])
AS_IF([test "$n_termlibs" -gt 1],
      [AC_MSG_ERROR([only one of --with-termcap, --with-curses, --with-ncurses can be used])])

# Check that only one of --with-readline or --with-libedit is used.

# n_editlibs=0
# AS_IF([test "x$with_readline" != xno], [n_editlibs=$(expr "$n_editlibs" + 1)])
# AS_IF([test "x$with_libedit" != xno], [n_editlibs=$(expr "$n_editlibs" + 1)])
# AS_IF([test "$n_editlibs" -gt 1],
#       [AC_MSG_ERROR([only one of --with-readline or --with-libedit can be used])])

# Check that --with-termcap, --with-curses, etc are used only when
# --with-libedit is used.

AS_IF([test "x$with_termcap" != xno],
  AS_IF([test "x$check_termlib" != xyes],
    [AC_MSG_WARN([--with-termcap has no effect; use with --with-libedit])]))

AS_IF([test "x$with_curses" != xno],
  AS_IF([test "x$check_termlib" != xyes],
    [AC_MSG_WARN([--with-curses has no effect; use with --with-libedit])]))

AS_IF([test "x$with_ncurses" != xno],
  AS_IF([test "x$check_termlib" != xyes],
    [AC_MSG_WARN([--with-ncurses has no effect; use with --with-libedit])]))

# Check for the terminal library.

AS_IF([test "x$check_termlib" = xyes],
  [AS_IF([test "x$with_termcap" != xno],
    [AC_CHECK_LIB(termcap, tgetent,,
      [AC_MSG_ERROR([termcap library not found])])])])

AS_IF([test "x$check_termlib" = xyes],
  [AS_IF([test "x$with_curses" != xno],
    [AC_CHECK_LIB(curses, tgetent,,
      [AC_MSG_ERROR([curses library not found])])])])

AS_IF([test "x$check_termlib" = xyes],
  [AS_IF([test "x$with_ncurses" != xno],
    [AC_CHECK_LIB(ncurses, tgetent,,
      [AC_MSG_ERROR([ncurses library not found])])])])

# Check for the libedit library.

# AS_IF([test "x$with_readline" != xno],
#   [AC_CHECK_LIB(readline, readline,,
#       [AC_MSG_ERROR([readline not found or not usable.])])])

AS_IF([test "x$with_libedit" != xno],
  [AC_CHECK_LIB(edit, readline,,
      [AC_MSG_ERROR([libedit not found or not usable.])])])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

# PKG_CHECK_MODULES([LIBEDIT], [libedit >= 3.1],
# 	[ AC_DEFINE([HAVE_LIBEDIT], [1], [Use libedit]) ],
# 	[ ])

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 doc/Makefile
		 doc/copyright.texi
		 tests/Makefile])
AC_OUTPUT
