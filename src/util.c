#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "fatfind.h"

#include <cairo.h>
#include <math.h>



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
