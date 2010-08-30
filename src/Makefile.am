## Process this file with automake to produce Makefile.in

fatfind_runner_CPPFLAGS = @GLIB_CFLAGS@ @LTILIB_CFLAGS@

bin_PROGRAMS = fatfind-runner

EXTRA_DIST = filter.c

fatfind_runner_SOURCES = circles4.cpp circles4.h fatfind-runner.cpp util.c util.h

fatfind_runner_LDADD = @GLIB_LIBS@ @LTILIB_LIBS@
