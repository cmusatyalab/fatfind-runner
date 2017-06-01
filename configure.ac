dnl Process this file with autoconf to produce a configure script.

AC_INIT(fatfind-runner, [1.1.3], [diamond@cs.cmu.edu])
AC_PREREQ([2.59])
AC_CONFIG_SRCDIR([src])
AM_INIT_AUTOMAKE([foreign 1.9])
AC_CONFIG_HEADERS([config.h])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_CXX
AC_HEADER_STDC

PKG_CHECK_MODULES(GLIB, [glib-2.0])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
PKG_CHECK_MODULES(OPENDIAMOND, [opendiamond])
AC_SUBST(OPENDIAMOND_CFLAGS)
AC_SUBST(OPENDIAMOND_LIBS)

AC_SEARCH_LIBS([sqrt], [m],, AC_MSG_FAILURE([cannot find math library]))

AC_SUBST(LTILIB_CFLAGS, "$(/usr/local/bin/lti-config nogtk --cxxflags)")
AC_SUBST(LTILIB_LIBS, "$(/usr/local/bin/lti-config nogtk --libs)")

AC_OUTPUT([
Makefile
src/Makefile
])
