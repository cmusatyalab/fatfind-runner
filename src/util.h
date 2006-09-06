#ifndef UTIL_H
#define UTIL_H

#include <cairo.h>
#include <gtk/gtk.h>

void draw_circles_into_widget (GtkWidget *d, GList *circles, double scale);
void draw_circles(cairo_t *cr, GList *circles, double scale);

#endif
