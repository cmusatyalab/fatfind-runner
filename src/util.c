/*
 * FatFind: A Diamond application for adipocyte image exploration
 *
 * Copyright (c) 2006 Carnegie Mellon University. All rights reserved.
 * Additional copyrights may be listed below.
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License v1.0 which accompanies this
 * distribution in the file named LICENSE.
 *
 * Technical and financial contributors are listed in the file named
 * CREDITS.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "util.h"

#include <math.h>


gdouble compute_eccentricity(gdouble a, gdouble b) {
  if (b > a) {
    gdouble c = a;
    a = b;
    b = c;
  }

  return sqrt(1 - ((b*b) / (a*a)));
}


gdouble quadratic_mean_radius(gdouble a, gdouble b) {
  gdouble result;

  if (b > a) {
    gdouble c = a;
    a = b;
    b = c;
  }

  result = sqrt((3.0*a*a + b*b) / 4.0);

  g_debug("qmr: (%g,%g) -> %g", a, b, result);
  return result;
}
