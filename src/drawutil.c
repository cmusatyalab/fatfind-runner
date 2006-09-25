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


void draw_circle(cairo_t *cr, circle_type *circle, double scale,
		 circle_fill_t fill) {
    float x = circle->x;
    float y = circle->y;
    float a = circle->a;
    float b = circle->b;
    float t = circle->t;

    double dash;

    double device_x = 0;
    double device_y = 0;
    double device_a = 1.0;
    double device_b = 1.0;

    //    g_debug("drawing %g %g %g %g %g", x, y, a, b, t);

    // draw
    cairo_save(cr);
    cairo_scale(cr, scale, scale);

    cairo_translate(cr, x, y);
    cairo_rotate(cr, t);
    cairo_scale(cr, a, b);

    cairo_move_to(cr, 1.0, 0.0);
    cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, 2 * M_PI);

    /*
    cairo_user_to_device(cr, &device_x, &device_y);
    cairo_user_to_device(cr, &device_a, &device_b);
    g_debug("drawing at device coords (%g,%g) (%g,%g)",
	    device_x, device_y, device_a, device_b);
    */

    cairo_restore(cr);

    switch (fill) {
    case CIRCLE_FILL_DASHED:
      dash = 5.0;
      cairo_set_line_width(cr, 1.0);
      cairo_set_dash(cr, &dash, 1, 0.0);
      cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
      cairo_stroke(cr);
      break;

    case CIRCLE_FILL_SOLID:
      // show fill, no dash
      cairo_set_line_width(cr, 2.0);
      cairo_set_dash(cr, NULL, 0, 0.0);
      cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
      cairo_stroke_preserve(cr);
      cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.2);
      cairo_fill(cr);
      break;

    case CIRCLE_FILL_HAIRLINE:
      cairo_set_line_width(cr, 1.0);
      cairo_set_dash(cr, NULL, 0, 0.0);
      cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
      cairo_stroke(cr);
      break;
    }
}

void draw_circles(cairo_t *cr, GList *l, double scale, circle_filter f) {
  while (l != NULL) {
    circle_type *c = (circle_type *) l->data;
    draw_circle(cr, c, scale, f(c) ? CIRCLE_FILL_SOLID : CIRCLE_FILL_DASHED);
    l = l->next;
  }
}

void convert_cairo_argb32_to_pixbuf(guchar *pixels,
				    gint w, gint h, gint stride) {
  gint x, y;

  // swap around the data
  // XXX also handle pre-multiplying?
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      // XXX check endian
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
      guchar *p = pixels + (stride * y) + (4 * x);

      guchar r = p[2];
      guchar b = p[0];

      p[0] = r;
      p[2] = b;
#else
      guint *p = (guint *) (pixels + (stride * y) + (4 * x));
      *p = GUINT32_SWAP_LE_BE(*p);
#endif
    }
  }
}

void draw_into_thumbnail(GdkPixbuf *pix2, GdkPixbuf *pix,
			 GList *clist, double image_scale,
			 double circle_scale,
			 gint w, gint h,
			 circle_filter f) {
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

  convert_cairo_argb32_to_pixbuf(pixels, w, h, stride);
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

int get_circle_at_point(guchar *hitmap,
			gint x, gint y,
			gint w, gint h) {
  int hit = 0;

  //g_debug("get_circle_at_point: x=%d, w=%d, y=%d, h=%d", x, w, y, h);

  // check to see if hits hitmap
  if ((x < w) && (y < h)) {
    hit = (((int *) hitmap)[y * w + x]) & 0xFFFFFF;
    g_debug("hit: %d", hit);
  }

  return hit - 1;
}

void draw_hitmap(GList *circles, guchar *hitmap,
		 gint w, gint h,
		 gdouble scale) {
  GList *l = circles;

  cairo_surface_t *surface =
    cairo_image_surface_create_for_data(hitmap,
					CAIRO_FORMAT_ARGB32,
					w, h, w * 4);
  cairo_t *cr = cairo_create(surface);
  cairo_surface_destroy(surface);

  // disable antialiasing since we are abusing the image backend
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

  int color = 1;

  // clear hitmap
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  cairo_paint(cr);

  while (l != NULL) {
    double red = ((color & 0xFF0000) >> 16) / 255.0;
    double green = ((color & 0xFF00) >> 8) / 255.0;
    double blue = (color & 0xFF) / 255.0;
    circle_type *c = (circle_type *) l->data;

    float x = c->x;
    float y = c->y;
    float t = c->t;
    float a = c->a;
    float b = c->b;

    // draw
    cairo_save(cr);
    cairo_scale(cr, scale, scale);

    cairo_translate(cr, x, y);
    cairo_rotate(cr, t);
    cairo_scale(cr, a, b);

    cairo_move_to(cr, 1.0, 1.0);
    cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, 2 * M_PI);
    cairo_restore(cr);

    cairo_set_source_rgb(cr, red, green, blue);
    cairo_stroke_preserve(cr);
    cairo_fill(cr);

    l = l->next;
    color++;
  }

  cairo_destroy(cr);
}
