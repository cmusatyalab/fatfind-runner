/*
 *  FatFind
 *  A Diamond application for adipocyte image exploration
 *  Version 1
 *
 *  Copyright (c) 2006 Carnegie Mellon University
 *  All Rights Reserved.
 *
 *  This software is distributed under the terms of the Eclipse Public
 *  License, Version 1.0 which can be found in the file named LICENSE.
 *  ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS SOFTWARE CONSTITUTES
 *  RECIPIENT'S ACCEPTANCE OF THIS AGREEMENT
 */

#ifndef CALIBRATE_H
#define CALIBRATE_H

#include <gtk/gtk.h>

void on_calibrationImages_selection_changed (GtkIconView *view,
					     gpointer user_data);

gboolean on_selectedImage_expose_event (GtkWidget *d,
					GdkEventExpose *event,
					gpointer user_data);

gboolean on_selectedImage_button_press_event (GtkWidget      *widget,
					      GdkEventButton *event,
					      gpointer        user_data);

gboolean on_selectedImage_motion_notify_event (GtkWidget      *widget,
					       GdkEventMotion *event,
					       gpointer        user_data);

gboolean on_selectedImage_configure_event (GtkWidget         *widget,
					   GdkEventConfigure *event,
					   gpointer          user_data);

gboolean on_selectedImage_enter_notify_event (GtkWidget        *widget,
					      GdkEventCrossing *event,
					      gpointer          user_data);

gboolean on_selectedImage_leave_notify_event (GtkWidget        *widget,
					      GdkEventCrossing *event,
					      gpointer          user_data);


#endif
