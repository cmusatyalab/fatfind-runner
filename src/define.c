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

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>
#include <string.h>

#include "fatfind.h"
#include "define.h"
#include "util.h"
#include "drawutil.h"

static GList *simulated_circles;

GtkListStore *saved_search_store;
static GdkPixbuf *c_pix_scaled;
static float scale;

static gdouble current_sharpness;

static void list_deleter(gpointer data, gpointer user_data) {
  g_free(data);
}

static gboolean circle_match(circle_type *c) {
  gdouble r_min;
  gdouble r_max;
  gdouble max_eccentricity;

  gdouble a, b, r, ref_r;

  if (reference_circle_object == NULL) {
    return FALSE;
  }

  r_min =
    gtk_range_get_value(GTK_RANGE(glade_xml_get_widget(g_xml, "radiusLower")));
  r_max =
    gtk_range_get_value(GTK_RANGE(glade_xml_get_widget(g_xml, "radiusUpper")));
  max_eccentricity =
    gtk_range_get_value(GTK_RANGE(glade_xml_get_widget(g_xml, "maxEccentricity")));

  a = c->a;
  b = c->b;
  r = quadratic_mean_radius(a, b);
  ref_r = quadratic_mean_radius(reference_circle_object->a,
				reference_circle_object->b);

  // scale by reference
  r_min *= ref_r;
  r_max *= ref_r;

  // compute eccentricity
  gdouble e = compute_eccentricity(a, b);

  return (r >= r_min) && (r <= r_max) && (e <= max_eccentricity);
}


void reset_sharpness(void) {
  // set to 1
  current_sharpness = 1;
  gtk_range_set_value(GTK_RANGE(glade_xml_get_widget(g_xml,
						     "minSharpness")),
		      current_sharpness);
  gtk_widget_set_sensitive(glade_xml_get_widget(g_xml, "recomputePreview"),
			   FALSE);
}


void copy_to_simulated_circles(GList *circles) {
  GList *src;
  GList *dst = NULL;

  if (simulated_circles != NULL) {
    // clear
    g_list_foreach(simulated_circles, list_deleter, NULL);
    g_list_free(simulated_circles);
    simulated_circles = NULL;
  }

  // copy
  src = circles;
  while (src != NULL) {
    circle_type *c = g_malloc(sizeof(circle_type));
    *c = *((circle_type *) src->data);

    dst = g_list_prepend(dst, c);

    src = g_list_next(src);
  }
  simulated_circles = g_list_reverse(dst);
}


void draw_define_offscreen_items(gint a_w, gint a_h) {
    // clear old scaled pix
  if (c_pix_scaled != NULL) {
    g_object_unref(c_pix_scaled);
    c_pix_scaled = NULL;
  }

  // reset sharpness
  reset_sharpness();

  // if something selected?
  if (c_pix) {
    float p_aspect =
      (float) gdk_pixbuf_get_width(c_pix) /
      (float) gdk_pixbuf_get_height(c_pix);

    float w = a_w;
    float h = a_h;
    float w_aspect = (float) w / (float) h;

    /* is window wider than pixbuf? */
    if (p_aspect < w_aspect) {
      /* then calculate width from height */
      w = h * p_aspect;
      scale = (float) a_h
	/ (float) gdk_pixbuf_get_height(c_pix);
    } else {
      /* else calculate height from width */
      h = w / p_aspect;
      scale = (float) a_w
	/ (float) gdk_pixbuf_get_width(c_pix);
    }

    c_pix_scaled = gdk_pixbuf_scale_simple(c_pix,
					   w, h,
					   GDK_INTERP_BILINEAR);
  }

}

gboolean on_simulatedSearch_expose_event (GtkWidget *d,
					  GdkEventExpose *event,
					  gpointer user_data) {
  if (c_pix_scaled) {
    cairo_t *cr = gdk_cairo_create(d->window);

    gint w = gdk_pixbuf_get_width(c_pix_scaled);
    gint h = gdk_pixbuf_get_height(c_pix_scaled);

    GList *l = simulated_circles;

    // draw the background
    gdk_cairo_set_source_pixbuf(cr, c_pix_scaled, 0.0, 0.0);
    cairo_paint(cr);

    // draw the circles
    draw_circles(cr, l, scale, circle_match);

    cairo_destroy(cr);
  }
}

gboolean on_simulatedSearch_configure_event (GtkWidget         *widget,
					     GdkEventConfigure *event,
					     gpointer          user_data) {
  draw_define_offscreen_items(event->width, event->height);
  return TRUE;
}


void on_define_search_value_changed (GtkRange *range,
				     gpointer  user_data) {
  // draw
  gtk_widget_queue_draw(glade_xml_get_widget(g_xml, "simulatedSearch"));
}

void on_saveSearchButton_clicked (GtkButton *button,
				  gpointer   user_data) {
  GtkTreeIter iter;

  const gchar *save_name =
    gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(g_xml, "searchName")));

  gdouble r_min = gtk_range_get_value(GTK_RANGE(glade_xml_get_widget(g_xml, "radiusLower")));
  gdouble r_max = gtk_range_get_value(GTK_RANGE(glade_xml_get_widget(g_xml, "radiusUpper")));
  gdouble max_eccentricity = gtk_range_get_value(GTK_RANGE(glade_xml_get_widget(g_xml,
										"maxEccentricity")));
  gdouble min_sharpness = gtk_range_get_value(GTK_RANGE(glade_xml_get_widget(g_xml,
									     "minSharpness")));

  if (reference_circle_object == NULL) {
    return;
  }

  // scale by reference
  r_min *= MIN(reference_circle_object->a,
	       reference_circle_object->b);
  r_max *= MAX(reference_circle_object->a,
	       reference_circle_object->b);

  g_debug("making new search: %s", save_name);
  gtk_list_store_append(saved_search_store, &iter);
  gtk_list_store_set(saved_search_store, &iter,
		     0, save_name,
		     1, r_min,
		     2, r_max,
		     3, max_eccentricity,
		     4, min_sharpness,
		     -1);

  // don't allow double click
  gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
}

void on_minSharpness_value_changed (GtkRange *range,
				   gpointer  user_data) {
  gdouble val = gtk_range_get_value(range);
  gtk_widget_set_sensitive(glade_xml_get_widget(g_xml, "recomputePreview"),
			   current_sharpness != val);
}

void on_recomputePreview_clicked (GtkButton *button,
				  gpointer   user_data) {
  gdouble min_sharpness = gtk_range_get_value(GTK_RANGE(glade_xml_get_widget(g_xml,
									     "minSharpness")));
  // clear
  g_list_foreach(simulated_circles, list_deleter, NULL);
  g_list_free(simulated_circles);
  simulated_circles = NULL;

  // find
  simulated_circles = circlesFromImage(gdk_pixbuf_get_width(c_pix),
				       gdk_pixbuf_get_height(c_pix),
				       gdk_pixbuf_get_rowstride(c_pix),
				       gdk_pixbuf_get_n_channels(c_pix),
				       gdk_pixbuf_get_pixels(c_pix),
				       min_sharpness);
  current_sharpness = min_sharpness;
  gtk_widget_set_sensitive(glade_xml_get_widget(g_xml, "recomputePreview"),
			   FALSE);
  gtk_widget_queue_draw(glade_xml_get_widget(g_xml, "simulatedSearch"));
}

void on_searchName_changed (GtkEditable *editable,
			    gpointer     user_data) {
  // change the sensitivity of the save as button, to allow clicking
  // only when the user has changed to non-empty string
  gchar *text = gtk_editable_get_chars(editable, 0, -1);
  gboolean empty = (strlen(text) == 0);
  g_free(text);

  gtk_widget_set_sensitive(GTK_WIDGET(glade_xml_get_widget(g_xml, "saveSearchButton")),
			   !empty);
}
