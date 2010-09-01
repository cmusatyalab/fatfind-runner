## Process this file with automake to produce Makefile.in

bin_PROGRAMS = fatfind-runner
fatfind_runner_CPPFLAGS = @GLIB_CFLAGS@ @LTILIB_CFLAGS@
fatfind_runner_SOURCES = circles4.cpp circles4.h fatfind-runner.cpp util.c util.h
fatfind_runner_LDADD = @GLIB_LIBS@ @LTILIB_LIBS@

filterdir = $(datadir)/fatfind/filter
nodist_filter_DATA = fil_circle.so
CLEANFILES = fil_circle.so
FILTER_SOURCE = circles4.cpp util.c filter.c
EXTRA_DIST = filter.c

fil_circle.so: $(FILTER_SOURCE) circles4.h util.h
	g++ -fPIC ${LDFLAGS} ${CPPFLAGS} -fvisibility=hidden -L/opt/diamond-filter-kit/lib @OPENDIAMOND_CFLAGS@ @GLIB_CFLAGS@ @LTILIB_CFLAGS@ -O2 -g -m32 -Wall -shared  -o $@ -Wl,-Bstatic $(addprefix $(srcdir)/,$(FILTER_SOURCE)) $(filter-out -lpthread,@LTILIB_LIBS@) -lltinvr @GLIB_LIBS@ -Wl,-Bdynamic
