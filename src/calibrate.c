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

#include <gtk/gtk.h>
#include <cairo.h>
#include <glade/glade.h>
#include <stdlib.h>

#include <math.h>

#include "define.h"
#include "calibrate.h"
#include "util.h"
#include "drawutil.h"
#include "circles4.h"


GdkPixbuf *c_pix;
static GdkPixbuf *c_pix_scaled;

static GdkPixmap *hitmap;
static gboolean show_circles;
static gfloat scale;

guint32 reference_circle = -1;
circle_type *reference_circle_object;

static void list_deleter(gpointer data, gpointer user_data) {
  g_free(data);
}

static void draw_calibrate_offscreen_items(gint allocation_width,
					   gint allocation_height) {
  // clear old scaled pix
  if (c_pix_scaled != NULL) {
    g_object_unref(c_pix_scaled);
    c_pix_scaled = NULL;
  }
  if (hitmap != NULL) {
    g_object_unref(hitmap);
    hitmap = NULL;
  }

  // if something selected?
  if (c_pix) {
    float p_aspect =
      (float) gdk_pixbuf_get_width(c_pix) /
      (float) gdk_pixbuf_get_height(c_pix);
    int w = allocation_width;
    int h = allocation_height;
    float w_aspect = (float) w / (float) h;

    //g_debug("w: %d, h: %d, p_aspect: %g, w_aspect: %g",
    //	    w, h, p_aspect, w_aspect);

    /* is window wider than pixbuf? */
    if (p_aspect < w_aspect) {
      /* then calculate width from height */
      w = h * p_aspect;
      scale = (float) allocation_height
	/ (float) gdk_pixbuf_get_height(c_pix);
    } else {
      /* else calculate height from width */
      h = w / p_aspect;
      scale = (float) allocation_width
	/ (float) gdk_pixbuf_get_width(c_pix);
    }

    c_pix_scaled = gdk_pixbuf_scale_simple(c_pix,
					   w, h,
					   GDK_INTERP_BILINEAR);


    hitmap = gdk_pixmap_new(NULL, w, h, 32);
    draw_hitmap(circles, hitmap, scale);
  }
}

static void set_show_circles(gboolean state) {
  if (show_circles != state) {
    show_circles = state;
    gtk_widget_queue_draw(glade_xml_get_widget(g_xml, "selectedImage"));
  }
}

static void set_reference_circle(guint32 c) {
  if (c == reference_circle) {
    return;
  }

  GtkImage *im = GTK_IMAGE(glade_xml_get_widget(g_xml, "calibrateRefImage"));
  GtkLabel *text = GTK_LABEL(glade_xml_get_widget(g_xml, "calibrateRefInfo"));

  GtkImage *im2 = GTK_IMAGE(glade_xml_get_widget(g_xml, "defineRefImage"));
  GtkLabel *text2 = GTK_LABEL(glade_xml_get_widget(g_xml, "defineRefInfo"));

  reference_circle = c;

  if (c == -1) {
    // clear the image and the text
    gtk_image_clear(im);
    gtk_label_set_label(text, "");

    gtk_image_clear(im2);
    gtk_label_set_label(text2, "");

    // set object
    reference_circle_object = NULL;
  } else {
    // set image
    circle_type *circ = (circle_type *)
      (g_list_nth(circles, reference_circle))->data;

    float extra = 1.2;
    float r = MAX(circ->a, circ->b);
    float x = circ->x - extra * r;
    float y = circ->y - extra * r;
    int width = gdk_pixbuf_get_width(c_pix);
    int height = gdk_pixbuf_get_height(c_pix);

    GdkPixbuf *sub;
    GdkPixbuf *sub_scaled;

    gchar *new_text;

    int xsize = 2 * extra * r;
    int ysize = 2 * extra * r;

    int scaledX = 150;
    int scaledY = 150;

    if (x < 0) {
      x = 0;
    }
    if (y < 0) {
      y = 0;
    }
    if (x + xsize > width) {
      xsize = width - x;
    }
    if (y + ysize > height) {
      ysize = height - y;
    }

    if (xsize > ysize) {
      scaledY *= ((double) ysize / (double) xsize);
    } else if (ysize > xsize) {
      scaledX *= ((double) xsize / (double) ysize);
    }

    sub = gdk_pixbuf_new_subpixbuf(c_pix,
				   x,
				   y,
				   xsize,
				   ysize);
    sub_scaled = gdk_pixbuf_scale_simple(sub,
					 scaledX,
					 scaledY,
					 GDK_INTERP_BILINEAR);
    gtk_image_set_from_pixbuf(im, sub_scaled);
    gtk_image_set_from_pixbuf(im2, sub_scaled);
    g_object_unref(sub);
    g_object_unref(sub_scaled);


    // set text
    new_text = g_strdup_printf("Radius: %g\n", r);
    gtk_label_set_text(text, new_text);
    gtk_label_set_text(text2, new_text);
    g_free(new_text);

    // set object
    reference_circle_object = (g_list_nth(circles, reference_circle))->data;
  }
}

static void foreach_select_calibration(GtkIconView *icon_view,
				       GtkTreePath *path,
				       gpointer data) {
  gchar *pix;

  GError *err = NULL;

  GtkTreeIter iter;
  GtkTreeModel *m = gtk_icon_view_get_model(icon_view);
  GtkWidget *w;

  FILE *f;

  gtk_tree_model_get_iter(m, &iter, path);
  gtk_tree_model_get(m, &iter,
		     1, &pix,
		     -1);
  if (c_pix != NULL) {
    g_object_unref(c_pix);
  }

  if (circles != NULL) {
    // clear
    g_list_foreach(circles, list_deleter, NULL);
    g_list_free(circles);
    circles = NULL;
  }


  c_pix = gdk_pixbuf_new_from_file(pix, &err);
  if (err != NULL) {
    g_critical("error: %s", err->message);
    g_error_free(err);
  }

  // compute the circles
  circlesFromImage(gdk_pixbuf_get_width(c_pix),
		   gdk_pixbuf_get_height(c_pix),
		   gdk_pixbuf_get_rowstride(c_pix),
		   gdk_pixbuf_get_n_channels(c_pix),
		   gdk_pixbuf_get_pixels(c_pix));
  //    circles = g_list_prepend(circles, c);

  g_debug("c_pix: %p", c_pix);
  //g_debug("c_txt: %s", c_txt);

  w = glade_xml_get_widget(g_xml, "selectedImage");
  gtk_widget_queue_draw(w);
}

gboolean on_selectedImage_expose_event (GtkWidget *d,
					GdkEventExpose *event,
					gpointer user_data) {
  //  g_debug("selected image expose event");
  if (c_pix) {
    gdk_draw_pixbuf(d->window,
		    d->style->fg_gc[GTK_WIDGET_STATE(d)],
		    c_pix_scaled,
		    0, 0, 0, 0,
		    -1, -1,
		    GDK_RGB_DITHER_NORMAL,
		    0, 0);
    /*
    gdk_draw_drawable(d->window,
		      d->style->fg_gc[GTK_WIDGET_STATE(d)],
		      hitmap,
		      0, 0, 0, 0,
		      -1, -1);
    */

    // draw circles
    if (show_circles) {
      draw_circles_into_widget(d, circles, scale, filter_by_in_result);
    }
  }
  return TRUE;
}

void on_calibrationImages_selection_changed (GtkIconView *view,
					     gpointer user_data) {
  GtkWidget *w;
  GtkWidget *w2;

  // reset reference image
  set_reference_circle(-1);

  // load the image
  gtk_icon_view_selected_foreach(view, foreach_select_calibration, NULL);

  // draw the offscreen items
  w = glade_xml_get_widget(g_xml, "selectedImage");
  draw_calibrate_offscreen_items(w->allocation.width,
				 w->allocation.height);
  w2 = glade_xml_get_widget(g_xml, "simulatedSearch");
  draw_define_offscreen_items(w2->allocation.width,
			      w2->allocation.height);
}


gboolean on_selectedImage_button_press_event (GtkWidget      *widget,
					      GdkEventButton *event,
					      gpointer        user_data) {
  gint w, h;
  GdkImage *hit_data;
  guint32 hit = -1;

  // if not selected, do nothing
  if (c_pix == NULL) {
    return FALSE;
  }

  if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
    g_debug("x: %g, y: %g", event->x, event->y);
    set_show_circles(TRUE);

    // hit?
    hit = get_circle_at_point(hitmap, event->x, event->y);

    // if so, then illuminate selected item and update reference
    if (hit != -1) {
      set_reference_circle(hit);
      gtk_widget_queue_draw(widget);
    }
    return TRUE;
  }
  return FALSE;
}


gboolean on_selectedImage_motion_notify_event (GtkWidget      *widget,
					       GdkEventMotion *event,
					       gpointer        user_data) {
  //g_message("x: %g, y: %g", event->x, event->y);
  set_show_circles(TRUE);
  return TRUE;
}

gboolean on_selectedImage_configure_event (GtkWidget         *widget,
					   GdkEventConfigure *event,
					   gpointer          user_data) {
  draw_calibrate_offscreen_items(event->width, event->height);
  return TRUE;
}


gboolean on_selectedImage_enter_notify_event (GtkWidget        *widget,
					      GdkEventCrossing *event,
					      gpointer          user_data) {
  g_debug("enter selectedImage");
  set_show_circles(TRUE);
  return TRUE;
}

gboolean on_selectedImage_leave_notify_event (GtkWidget        *widget,
					      GdkEventCrossing *event,
					      gpointer          user_data) {
  g_debug("leave selectedImage");
  set_show_circles(FALSE);
  return TRUE;
}
