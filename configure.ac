dnl Process this file with autoconf to produce a configure script.

AC_INIT(fatfind, [1.0.3], [diamond@cs.cmu.edu])
AC_PREREQ([2.59])
AC_CONFIG_SRCDIR([src])
AM_INIT_AUTOMAKE([foreign 1.9])
AC_CONFIG_HEADERS([config.h])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_CXX
AC_HEADER_STDC

pkg_modules="gtk+-2.0 >= 2.8.0 \
	libglade-2.0 >= 2.5.1"

PKG_CHECK_MODULES(GLADE, [$pkg_modules])
AC_SUBST(GLADE_CFLAGS)
AC_SUBST(GLADE_LIBS)

PKG_CHECK_MODULES(OPENDIAMOND, [opendiamond])
AC_SUBST(OPENDIAMOND_CFLAGS)
AC_SUBST(OPENDIAMOND_LIBS)

AC_SEARCH_LIBS([sqrt], [m],, AC_MSG_FAILURE([cannot find math library]))
AC_SEARCH_LIBS([pthread_create],
	[pthread],, AC_MSG_FAILURE([cannot find pthread_create function]))

AC_SUBST(LIBS, "`lti-config nogtk --libs`")


AC_SUBST(AM_CPPFLAGS, "-Werror-implicit-function-declaration `lti-config nogtk --cxxflags`")

# XXX should work on more than 1 arch
AC_SUBST(AM_CFLAGS, "-m32")


AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
data/glade/Makefile
data/filter/Makefile
])