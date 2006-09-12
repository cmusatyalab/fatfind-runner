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

#ifndef UTIL_H
#define UTIL_H

#include <cairo.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif
  void draw_circles_into_widget (GtkWidget *d, GList *circles, double scale);
  void draw_circles(cairo_t *cr, GList *circles, double scale);
  void draw_into_thumbnail(GdkPixbuf *pix2, GdkPixbuf *pix,
			   GList *clist, double image_scale,
			   double circle_scale,
			   gint w, gint h);
  gchar *make_thumbnail_title(GList *c);
  void compute_thumbnail_scale(double *scale, gint *w, gint *h);
  int get_circle_at_point(GdkPixmap *hitmap, gint x, gint y);
#ifdef __cplusplus
}
#endif

#endif
