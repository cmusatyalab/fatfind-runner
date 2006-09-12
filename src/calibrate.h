/*
 * This file is part of FatFind,
 * a Diamond-based system for adipocyte exploration.
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
