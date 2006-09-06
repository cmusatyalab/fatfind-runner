#ifndef UTIL_H
#define UTIL_H

#include <cairo.h>
#include <gtk/gtk.h>

void draw_circles_into_widget (GtkWidget *d, GList *circles, double scale);
void draw_circles(cairo_t *cr, GList *circles, double scale);
void draw_into_thumbnail(GdkPixbuf *pix2, GdkPixbuf *pix,
			 GList *clist, double image_scale,
			 double circle_scale,
			 gint w, gint h);
gchar *make_thumbnail_title(int len);
void compute_thumbnail_scale(double *scale, gint *w, gint *h);

#endif
