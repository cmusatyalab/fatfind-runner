#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "fatfind.h"
#include "util.h"

#include <cairo.h>
#include <math.h>


void draw_circles_into_widget (GtkWidget *d, GList *l, double scale) {
  cairo_t *cr = gdk_cairo_create(d->window);
  draw_circles(cr, l, scale);
  cairo_destroy(cr);
}


void draw_circles(cairo_t *cr, GList *l, double scale) {
  cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);

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

    l = l->next;
  }

  cairo_set_line_width(cr, 1.0);
  cairo_stroke(cr);
}


void draw_into_thumbnail(GdkPixbuf *pix2, GdkPixbuf *pix,
			 GList *clist, double image_scale,
			 double circle_scale,
			 gint w, gint h) {
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

  draw_circles(cr, clist, circle_scale);
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

gchar *make_thumbnail_title(int len) {
  gchar *title;
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
