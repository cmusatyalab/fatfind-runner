#ifndef DEFINE_H
#define DEFINE_H

#include <gtk/gtk.h>

void draw_define_offscreen_items (gint width, gint height);

gboolean on_simulatedSearch_expose_event (GtkWidget *d,
					  GdkEventExpose *event,
					  gpointer user_data);

gboolean on_simulatedSearch_configure_event (GtkWidget         *widget,
					     GdkEventConfigure *event,
					     gpointer          user_data);

void on_define_search_value_changed (GtkRange *range,
				     gpointer  user_data);

void on_saveSearchButton_clicked (GtkButton *button,
				  gpointer   user_data);
#endif
