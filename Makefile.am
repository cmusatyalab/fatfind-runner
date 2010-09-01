## Process this file with automake to produce Makefile.in

SUBDIRS = src

EXTRA_DIST = LICENSE CREDITS

# diamond-filter-kit is only built for 32-bit
DISTCHECK_CONFIGURE_FLAGS = CPPFLAGS=-m32 LDFLAGS=-m32
