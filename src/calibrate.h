#ifndef CALIBRATE_H
#define CALIBRATE_H

#include <gtk/gtk.h>

void on_calibrationImages_selection_changed (GtkIconView *view,
					     gpointer user_data);

gboolean on_selectedImage_expose_event (GtkWidget *d,
					GdkEventExpose *event,
					gpointer user_data);

void on_selectedImage_button_press_event (GtkWidget      *widget,
					  GdkEventButton *event,
					  gpointer        user_data);

void on_selectedImage_motion_notify_event (GtkWidget      *widget,
					   GdkEventMotion *event,
					   gpointer        user_data);

void on_selectedImage_configure_event (GtkWidget         *widget,
				       GdkEventConfigure *event,
				       gpointer          user_data);

gboolean on_selectedImage_enter_notify_event (GtkWidget        *widget,
					      GdkEventCrossing *event,
					      gpointer          user_data);

gboolean on_selectedImage_leave_notify_event (GtkWidget        *widget,
					      GdkEventCrossing *event,
					      gpointer          user_data);


typedef struct {
  float x;
  float y;
  float r;
  float fuzz;
} circle_type;

#endif
