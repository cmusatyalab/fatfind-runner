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

#include "fatfind.h"
#include "drawutil.h"
#include "util.h"

#include <cairo.h>
#include <math.h>


gboolean filter_by_in_result (circle_type *c) {
  return c->in_result;
}

void draw_circles_into_widget (GtkWidget *d, GList *l, double scale,
			       circle_filter f) {
  cairo_t *cr = gdk_cairo_create(d->window);
  draw_circles(cr, l, scale, f);
  cairo_destroy(cr);
}


void draw_circles(cairo_t *cr, GList *l, double scale, circle_filter f) {
  while (l != NULL) {
    circle_type *c = (circle_type *) l->data;

    float x = c->x;
    float y = c->y;
    float a = c->a;
    float b = c->b;
    float t = c->t;

    //    g_debug("drawing %g %g %g %g %g", x, y, a, b, t);

    // draw
    cairo_save(cr);
    cairo_scale(cr, scale, scale);

    cairo_translate(cr, x, y);
    cairo_rotate(cr, t);
    cairo_scale(cr, a, b);

    cairo_move_to(cr, 1.0, 0.0);
    cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, 2 * M_PI);

    cairo_restore(cr);

    if (!f(c)) {
      // dash
      double dash = 5.0;
      cairo_set_line_width(cr, 1.0);
      cairo_set_dash(cr, &dash, 1, 0.0);
      cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
      cairo_stroke(cr);
    } else {
      // show fill, no dash
      cairo_set_line_width(cr, 2.0);
      cairo_set_dash(cr, NULL, 0, 0.0);
      cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
      cairo_stroke_preserve(cr);
      cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.2);
      cairo_fill(cr);
    }

    l = l->next;
  }
}


void draw_into_thumbnail(GdkPixbuf *pix2, GdkPixbuf *pix,
			 GList *clist, double image_scale,
			 double circle_scale,
			 gint w, gint h,
			 circle_filter f) {
  int x, y;

  guchar *pixels = gdk_pixbuf_get_pixels(pix2);
  int stride = gdk_pixbuf_get_rowstride(pix2);
  cairo_surface_t *surface = cairo_image_surface_create_for_data(pixels,
								 CAIRO_FORMAT_ARGB32,
								 w, h, stride);
  cairo_t *cr = cairo_create(surface);
  cairo_save(cr);
  cairo_scale(cr, image_scale, image_scale);
  gdk_cairo_set_source_pixbuf(cr, pix, 0, 0);
  cairo_paint(cr);
  cairo_restore(cr);

  draw_circles(cr, clist, circle_scale, f);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);

  // swap around the data
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      if (G_BYTE_ORDER == G_LITTLE_ENDIAN) {
	guchar *p = pixels + (stride * y) + (4 * x);

	guchar r = p[2];
	guchar b = p[0];

	p[0] = r;
	p[2] = b;
      } else if (G_BYTE_ORDER == G_BIG_ENDIAN) {
	guint *p = (guint *) (pixels + (stride * y) + (4 * x));
	*p = GUINT32_SWAP_LE_BE(*p);
      }
    }
  }
}

gchar *make_thumbnail_title(GList *c) {
  gchar *title;

  int len = 0;
  while (c != NULL) {
    if (((circle_type *)(c->data))->in_result) {
      len++;
    }
    c = g_list_next(c);
  }

  if (len == 1) {
    title = g_strdup_printf("%d circle", len);
  } else {
    title = g_strdup_printf("%d circles", len);
  }
  return title;
}

void compute_thumbnail_scale(double *scale, gint *w, gint *h) {
  float p_aspect = (float) *w / (float) *h;

  if (p_aspect < 1) {
    /* then calculate width from height */
    *scale = 150.0 / (double) *h;
    *h = 150;
    *w = *h * p_aspect;
  } else {
    /* else calculate height from width */
    *scale = 150.0 / (double) *w;
    *w = 150;
    *h = *w / p_aspect;
  }
}

int get_circle_at_point(GdkPixmap *hitmap, gint x, gint y) {
  GdkImage *hit_data;
  int hit = 0;
  int w, h;

  gdk_drawable_get_size(hitmap, &w, &h);
  hit_data = gdk_drawable_get_image(hitmap, 0, 0, w, h);
  g_debug("get_circle_at_point: x=%d, w=%d, y=%d, h=%d", x, w, y, h);

  // check to see if hits hitmap
  if ((x < w) && (y < h)) {
    hit = gdk_image_get_pixel(hit_data, x, y);
    g_debug("hit: %d", hit);
  }
  g_object_unref(hit_data);

  return hit - 1;
}

void draw_hitmap(GList *circles, GdkDrawable *hitmap,
		 gdouble scale) {
  GdkGC *gc = gdk_gc_new(hitmap);

  GdkColor black = {0, 0, 0, 0};

  GdkColor col = {1, 0, 0, 0};

  GList *l = circles;

  int w, h;

  gdk_drawable_get_size(hitmap, &w, &h);

  // clear hitmap
  gdk_gc_set_foreground(gc, &black);
  gdk_draw_rectangle(hitmap, gc, TRUE, 0, 0, w, h);

  while (l != NULL) {
    gdk_gc_set_foreground(gc, &col);
    circle_type *c = (circle_type *) l->data;

    float x = c->x;
    float y = c->y;
    float r = quadratic_mean_radius(c->a, c->b);

    // draw
    x *= scale;
    y *= scale;
    r *= scale;

    gdk_draw_arc(hitmap, gc, TRUE,
		 x - r, y - r, 2 * r, 2 * r,
		 0, 360*64);
    gdk_draw_arc(hitmap, gc, FALSE,
		 x - r, y - r, 2 * r, 2 * r,
		 0, 360*64);

    l = l->next;
    col.pixel++;
  }

  g_object_unref(gc);
}
