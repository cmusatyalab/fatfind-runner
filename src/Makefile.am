## Process this file with automake to produce Makefile.in

gladedir = $(datadir)/fatfind/glade

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DFATFIND_GLADEDIR=\""$(gladedir)"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = fatfind

fatfind_SOURCES = \
	main.c fatfind.h \
	callbacks.c callbacks.h \
	image.c

fatfind_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)

