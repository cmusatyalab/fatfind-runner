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

#ifndef DRAWUTIL_H
#define DRAWUTIL_H

#include <cairo.h>
#include <gtk/gtk.h>

typedef enum {
  CIRCLE_FILL_SOLID,
  CIRCLE_FILL_DASHED,
  CIRCLE_FILL_HAIRLINE
} circle_fill_t;


#ifdef __cplusplus
extern "C" {
#endif
  typedef gboolean (*circle_filter) (circle_type *c);

  gboolean filter_by_in_result (circle_type *c);

  void draw_circles_into_widget (GtkWidget *d, GList *circles, double scale,
				 circle_filter f);
  void draw_circles(cairo_t *cr, GList *circles, double scale,
		    circle_filter f);
  void draw_circle(cairo_t *cr, circle_type *circle, double scale,
		   circle_fill_t fill);
  void convert_cairo_argb32_to_pixbuf(guchar *pixels,
				      gint w, gint h, gint stride);
  void draw_into_thumbnail(GdkPixbuf *pix2, GdkPixbuf *pix,
			   GList *clist, double image_scale,
			   double circle_scale,
			   gint w, gint h, circle_filter f);
  gchar *make_thumbnail_title(GList *c);
  void compute_thumbnail_scale(double *scale, gint *w, gint *h);
  int get_circle_at_point(GdkPixmap *hitmap, gint x, gint y);
  void draw_hitmap(GList *circles_list, GdkDrawable *hitmap,
		   gdouble display_scale);
#ifdef __cplusplus
}
#endif

#endif
