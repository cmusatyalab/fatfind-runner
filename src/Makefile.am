## Process this file with automake to produce Makefile.in

gladedir = $(datadir)/fatfind/glade
filterdir = $(datadir)/fatfind/filter

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DFATFIND_GLADEDIR=\""$(gladedir)"\" \
	-DFATFIND_FILTERDIR=\""$(filterdir)"\" \
	@OPENDIAMOND_CFLAGS@ @GLADE_CFLAGS@

bin_PROGRAMS = fatfind

fatfind_SOURCES = \
	main.c fatfind.h \
	calibrate.c calibrate.h \
	define.c define.h \
	investigate.cpp investigate.h \
	drawutil.c drawutil.h \
	util.c util.h \
	circles4.cpp circles4.h \
	diamond_interface.c diamond_interface.h

#fatfind_runner_SOURCES = circles4.cpp circles4.h fatfind-runner.cpp util.c

fatfind_LDADD = @OPENDIAMOND_LIBS@ @GLADE_LIBS@

#fatfind_runner_LDADD = @OPENDIAMOND_LIBS@ @GLIB_LIBS@

fatfind_LDFLAGS = -export-dynamic
