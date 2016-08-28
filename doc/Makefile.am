AM_MAKEINFOHTMLFLAGS = --no-headers --no-split
info_TEXINFOS = bas55.texi
dist_man_MANS = bas55.1

bas55.1:	$(top_srcdir)/ecma55/ecma55.c $(top_srcdir)/configure.ac
		$(HELP2MAN) --output=$@ $(top_srcdir)/ecma55/bas55$(EXEEXT)
