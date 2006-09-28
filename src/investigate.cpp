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
#include <math.h>

#include "fatfind.h"
#include "investigate.h"
#include "util.h"
#include "drawutil.h"
#include "diamond_interface.h"

#include "ltiHistogram.h"


GtkListStore *found_items;

static GdkPixbuf *i_pix;
static GdkPixbuf *i_pix_scaled;

static guchar *hitmap;
static gdouble prescale;
static gdouble display_scale = 1.0;

static gboolean show_circles;

static GList *circles_list;

static ls_search_handle_t dr;
static guint search_idle_id;

static GtkTreePath *current_result_path;

static gboolean is_adding;
static gint x_add_start;
static gint y_add_start;
static gint x_add_current;
static gint y_add_current;


static void stop_search(void) {
  if (dr != NULL) {
    printf("terminating search\n");
    g_source_remove(search_idle_id);
    ls_terminate_search(dr);
    dr = NULL;
  }
}

static void set_show_circles(gboolean state) {
  if (show_circles != state) {
    show_circles = state;
    gtk_widget_queue_draw(glade_xml_get_widget(g_xml, "selectedResult"));
  }
}

static void set_rulers_upper(gdouble width, gdouble height) {
  GtkRuler *v = GTK_RULER(glade_xml_get_widget(g_xml, "resultVRuler"));
  GtkRuler *h = GTK_RULER(glade_xml_get_widget(g_xml, "resultHRuler"));

  GValue hMax = {0,};
  GValue vMax = {0,};

  g_value_init(&hMax, G_TYPE_DOUBLE);
  g_value_set_double(&hMax, width);
  g_object_set_property(G_OBJECT(h), "upper", &hMax);

  g_value_init(&vMax, G_TYPE_DOUBLE);
  g_value_set_double(&vMax, height);
  g_object_set_property(G_OBJECT(v), "upper", &vMax);
}

static void foreach_select_investigation(GtkIconView *icon_view,
					 GtkTreePath *path,
					 gpointer data) {
  GError *err = NULL;

  GtkTreeIter iter;
  GtkTreeModel *m = gtk_icon_view_get_model(icon_view);
  GtkWidget *w;

  // save path
  if (current_result_path != NULL) {
    gtk_tree_path_free(current_result_path);
  }
  current_result_path = gtk_tree_path_copy(path);

  gtk_tree_model_get_iter(m, &iter, path);
  gtk_tree_model_get(m, &iter,
		     2, &circles_list,
		     3, &i_pix,
		     4, &prescale,
		     -1);

  w = glade_xml_get_widget(g_xml, "selectedResult");
  gtk_widget_queue_draw(w);
}


static void draw_investigate_offscreen_items(gint allocation_width,
					     gint allocation_height) {
  // clear old scaled pix
  if (i_pix_scaled != NULL) {
    g_object_unref(i_pix_scaled);
    i_pix_scaled = NULL;
  }
  if (hitmap != NULL) {
    g_free(hitmap);
    hitmap = NULL;
  }

  // if something selected?
  if (i_pix) {
    GdkGC *gc;

    float p_aspect =
      (float) gdk_pixbuf_get_width(i_pix) /
      (float) gdk_pixbuf_get_height(i_pix);
    int w = allocation_width;
    int h = allocation_height;
    float w_aspect = (float) w / (float) h;

    /* is window wider than pixbuf? */
    if (p_aspect < w_aspect) {
      /* then calculate width from height */
      w = (int) (h * p_aspect);
      display_scale = (float) allocation_height
	/ (float) gdk_pixbuf_get_height(i_pix);
    } else {
      /* else calculate height from width */
      h = (int) (w / p_aspect);
      display_scale = (float) allocation_width
	/ (float) gdk_pixbuf_get_width(i_pix);
    }

    display_scale *= prescale;

    i_pix_scaled = gdk_pixbuf_scale_simple(i_pix,
					   w, h,
					   GDK_INTERP_BILINEAR);


    hitmap = (guchar *) g_malloc(w * h * 4);
    draw_hitmap(circles_list, hitmap, w, h, display_scale);
  }
}



void on_clearSearch_clicked (GtkButton *button,
			     gpointer   user_data) {
  // buttons
  GtkWidget *stopSearch = glade_xml_get_widget(g_xml, "stopSearch");
  GtkWidget *startSearch = glade_xml_get_widget(g_xml, "startSearch");

  gtk_widget_set_sensitive(stopSearch, FALSE);
  gtk_widget_set_sensitive(startSearch, TRUE);

  // stop
  stop_search();

  // clear search thumbnails
  gtk_list_store_clear(found_items);

  // clear result
  if (i_pix != NULL) {
    g_object_unref(i_pix);
    i_pix = NULL;
  }
  gtk_widget_queue_draw(glade_xml_get_widget(g_xml, "selectedResult"));
}

void on_stopSearch_clicked (GtkButton *button,
			    gpointer user_data) {
  GtkWidget *stopSearch = glade_xml_get_widget(g_xml, "stopSearch");
  GtkWidget *startSearch = glade_xml_get_widget(g_xml, "startSearch");
  gtk_widget_set_sensitive(stopSearch, FALSE);
  gtk_widget_set_sensitive(startSearch, TRUE);

  stop_search();
}

void on_startSearch_clicked (GtkButton *button,
			     gpointer   user_data) {
  // get the selected search
  GtkTreeIter s_iter;
  GtkTreeSelection *selection =
    gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(g_xml,
								   "definedSearches")));
  GtkTreeModel *model = GTK_TREE_MODEL(saved_search_store);

  g_debug("selection: %p", selection);

  if (gtk_tree_selection_get_selected(selection,
				      &model,
				      &s_iter)) {
    gdouble r_min;
    gdouble r_max;
    gdouble max_eccentricity;
    GtkWidget *stopSearch = glade_xml_get_widget(g_xml, "stopSearch");


    g_debug("saved_search_store: %p", model);
    gtk_tree_model_get(model,
		       &s_iter,
		       1, &r_min,
		       2, &r_max,
		       3, &max_eccentricity,
		       -1);

    g_debug("searching from %g to %g", r_min, r_max);

    // reset stats
    total_objects = 0;
    processed_objects = 0;
    dropped_objects = 0;

    // diamond
    dr = diamond_circle_search(r_min, r_max, max_eccentricity);

    // take the handle, put it into the idle callback to get
    // the results?
    search_idle_id = g_idle_add(diamond_result_callback, dr);

    // activate the stop search button
    gtk_widget_set_sensitive(stopSearch, TRUE);

    // deactivate our button
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE);
  }
}

gboolean on_selectedResult_expose_event (GtkWidget *d,
					 GdkEventExpose *event,
					 gpointer user_data) {
  if (i_pix) {
    gdk_draw_pixbuf(d->window,
		    d->style->fg_gc[GTK_WIDGET_STATE(d)],
		    i_pix_scaled,
		    0, 0, 0, 0,
		    -1, -1,
		    GDK_RGB_DITHER_NORMAL,
		    0, 0);
    if (show_circles) {
      draw_circles_into_widget(d, circles_list, display_scale,
			       filter_by_in_result);
    }

    if (is_adding) {
      cairo_t *cr = gdk_cairo_create(d->window);

      double xd = x_add_current - x_add_start;
      double yd = y_add_current - y_add_start;
      double r = sqrt((xd * xd) + (yd * yd));

      g_debug("dynamic circle (%d,%d,%g)\n",
	      x_add_start, y_add_start, r);

      cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
      cairo_arc(cr, x_add_start, y_add_start,
		r, 0.0, 2 * M_PI);

      cairo_set_line_width(cr, 1.0);
      cairo_stroke(cr);
      cairo_destroy(cr);
    }
  }

  return TRUE;
}


gboolean on_selectedResult_button_press_event (GtkWidget      *widget,
					       GdkEventButton *event,
					       gpointer        user_data) {
  gint x, y;
  gint w, h;
  gint32 hit = -1;

  // if not selected, do nothing
  if (i_pix == NULL) {
    return FALSE;
  }

  if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
    g_debug("x: %g, y: %g", event->x, event->y);
    set_show_circles(TRUE);

    if (event->state & GDK_SHIFT_MASK) {
      // delete
      hit = get_circle_at_point(hitmap,
				(int) event->x, (int) event->y,
				gdk_pixbuf_get_width(i_pix_scaled),
				gdk_pixbuf_get_height(i_pix_scaled));

      // if so, then delete selected item and update reference
      if (hit != -1) {
	GdkPixbuf *pix2;
	gchar *title;

	double scale;

	// get the model
	GError *err = NULL;
	GtkTreeIter iter;
	GtkListStore *store = GTK_LIST_STORE(gtk_icon_view_get_model(GTK_ICON_VIEW(glade_xml_get_widget(g_xml,
												    "searchResults"))));

	// get the circle and free it
	GList *item = g_list_nth(circles_list, hit);
	g_free(item->data);
	circles_list = g_list_delete_link(circles_list, item);

	// get things
	gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, current_result_path);
	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
			   0, &pix2,
			   1, &title,
			   -1);
	g_free(title);
	title = make_thumbnail_title(circles_list);

	w = gdk_pixbuf_get_width(pix2);
	h = gdk_pixbuf_get_height(pix2);
	scale = (double) w / (double) gdk_pixbuf_get_width(i_pix);
	g_object_unref(pix2);

	// make new thumbnail
	pix2 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
	draw_into_thumbnail(pix2, i_pix, circles_list, scale,
			    scale * prescale, w, h, filter_by_in_result);

	// add it back
	gtk_list_store_set(store, &iter,
			   0, pix2,
			   1, title,
			   2, circles_list,
			   -1);

	g_object_unref(pix2);
	draw_hitmap(circles_list, hitmap,
		    gdk_pixbuf_get_width(i_pix_scaled),
		    gdk_pixbuf_get_height(i_pix_scaled),
		    display_scale);
	gtk_widget_queue_draw(widget);
      }
      return TRUE;
    } else {
      // start add
      is_adding = TRUE;
      x_add_current = x_add_start = (int) event->x;
      y_add_current = y_add_start = (int) event->y;

      return TRUE;
    }
  }
  return FALSE;
}

gboolean on_selectedResult_button_release_event (GtkWidget      *widget,
						 GdkEventButton *event,
						 gpointer        user_data) {
  if (is_adding) {
    // get the model
    GError *err = NULL;
    GtkTreeIter iter;
    GtkListStore *store = GTK_LIST_STORE(gtk_icon_view_get_model(GTK_ICON_VIEW(glade_xml_get_widget(g_xml,
												    "searchResults"))));

    GdkPixbuf *pix2;

    circle_type *c;
    double xd = event->x - x_add_start;
    double yd = event->y - y_add_start;
    double r = sqrt((xd * xd) + (yd * yd));

    gint w = gdk_pixbuf_get_width(i_pix);
    gint h = gdk_pixbuf_get_height(i_pix);

    double scale;

    gchar *title;

    g_debug("end adding");
    is_adding = FALSE;

    if (r < 1) {
      // too small, this toggles the exclusion filter
      int hit = get_circle_at_point(hitmap,
				    (int) event->x, (int) event->y,
				    gdk_pixbuf_get_width(i_pix_scaled),
				    gdk_pixbuf_get_height(i_pix_scaled));
      if (hit != -1) {
	c = (circle_type *) (g_list_nth(circles_list, hit)->data);
	c->in_result = !c->in_result;
      }
    } else {
      // make a new circle
      c = (circle_type *) g_malloc(sizeof(circle_type));
      c->x = (double) x_add_start / display_scale;
      c->y = (double) y_add_start / display_scale;
      c->t = 0;
      c->a = (double) r / display_scale;
      c->b = (double) r / display_scale;

      circles_list = g_list_prepend(circles_list, c);
    }

    // get
    gtk_tree_model_get_iter(GTK_TREE_MODEL(store), &iter, current_result_path);
    gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
		       0, &pix2,
		       1, &title,
		       -1);
    g_free(title);
    title = make_thumbnail_title(circles_list);

    // draw
    w = gdk_pixbuf_get_width(pix2);
    h = gdk_pixbuf_get_height(pix2);
    scale = (double) w / (double) gdk_pixbuf_get_width(i_pix);
    g_object_unref(pix2);

    pix2 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
    draw_into_thumbnail(pix2, i_pix, circles_list, scale,
			scale * prescale, w, h, filter_by_in_result);

    // update
    gtk_list_store_set(store, &iter,
		       0, pix2,
		       1, title,
		       2, circles_list,
		       -1);

    g_object_unref(pix2);

    draw_hitmap(circles_list, hitmap,
		gdk_pixbuf_get_width(i_pix_scaled),
		gdk_pixbuf_get_height(i_pix_scaled),
		display_scale);
    gtk_widget_queue_draw(widget);
    return TRUE;
  }

  return FALSE;
}

gboolean on_selectedResult_motion_notify_event (GtkWidget      *widget,
						GdkEventMotion *event,
						gpointer        user_data) {
  // update rulers
  GtkRuler *v = GTK_RULER(glade_xml_get_widget(g_xml, "resultVRuler"));
  GtkRuler *h = GTK_RULER(glade_xml_get_widget(g_xml, "resultHRuler"));

  GValue hPos = {0,};
  GValue vPos = {0,};

  g_value_init(&hPos, G_TYPE_DOUBLE);
  g_value_set_double(&hPos, event->x / display_scale);
  g_object_set_property(G_OBJECT(h), "position", &hPos);

  g_value_init(&vPos, G_TYPE_DOUBLE);
  g_value_set_double(&vPos, event->y / display_scale);
  g_object_set_property(G_OBJECT(v), "position", &vPos);


  if (is_adding) {
    // draw dynamic circle
    x_add_current = (int) event->x;
    y_add_current = (int) event->y;

    gtk_widget_queue_draw(glade_xml_get_widget(g_xml, "selectedResult"));
  }

  return TRUE;
}

gboolean on_selectedResult_configure_event (GtkWidget         *widget,
					    GdkEventConfigure *event,
					    gpointer          user_data) {
  draw_investigate_offscreen_items(event->width, event->height);
  set_rulers_upper(event->width / display_scale, event->height / display_scale);
  return TRUE;
}

void on_searchResults_selection_changed (GtkIconView *view,
					 gpointer user_data) {
  GtkWidget *w;

  // load the image
  gtk_icon_view_selected_foreach(view, foreach_select_investigation, NULL);

  // draw the offscreen items
  w = glade_xml_get_widget(g_xml, "selectedResult");
  draw_investigate_offscreen_items(w->allocation.width,
				   w->allocation.height);

  // update rulers
  set_rulers_upper(w->allocation.width / display_scale,
		   w->allocation.height / display_scale);
}

gboolean on_selectedResult_enter_notify_event (GtkWidget        *widget,
					       GdkEventCrossing *event,
					       gpointer          user_data) {
  set_show_circles(TRUE);
  return TRUE;
}

gboolean on_selectedResult_leave_notify_event (GtkWidget        *widget,
					       GdkEventCrossing *event,
					       gpointer          user_data) {
  set_show_circles(FALSE);
  return TRUE;
}


// XXX this is a crappy histogram
static GdkPixbuf *draw_histogram(lti::histogram1D &hist,
				 double minR, double maxR) {
  int w = 320;
  int h = 240;

  int margin = 5;

  // initialize
  GdkPixbuf *result = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
  guchar *pixels = gdk_pixbuf_get_pixels(result);
  int stride = gdk_pixbuf_get_rowstride(result);
  cairo_surface_t *surface =
    cairo_image_surface_create_for_data(pixels, CAIRO_FORMAT_ARGB32,
					w, h, stride);
  cairo_t *cr = cairo_create(surface);
  cairo_surface_destroy(surface);

  // clear
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_paint(cr);

  // draw frame
  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  cairo_move_to(cr, margin, margin);
  cairo_rel_line_to(cr, 0.0, h - (2 * margin));
  cairo_rel_line_to(cr, w - (2 * margin), 0.0);
  cairo_stroke(cr);

  double maxCount = hist.at(hist.getIndexOfMaximum());

  double numBins = hist.getLastCell() + 1;
  double binWidth = (maxR - minR) / numBins;

  double lineWidth = (w - ((4 + numBins) * margin)) / numBins;
  double xAdvance = lineWidth + margin;
  double x = xAdvance / 2.0;

  cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
  cairo_set_line_width(cr, lineWidth);
  for (int i = hist.getFirstCell(); i <= hist.getLastCell(); i++) {
    double barHeight = ((double) hist.at(i) / (double) maxCount) *
      (h - (4 * margin));

    x += xAdvance;
    cairo_move_to(cr, x, h - margin);
    cairo_rel_line_to(cr, 0.0, -barHeight);

    //    double low = minR + (width * i);
    //    double high = minR + (width * (i + 1));

    //    tmp_str = g_strdup_printf(" (%#g -- %#g): %5g\n", low, high, hist.at(i));
    //    gtk_text_buffer_insert(text, &text_iter, tmp_str, -1);
    //    g_free(tmp_str);
  }
  cairo_stroke(cr);

  // done
  cairo_destroy(cr);
  convert_cairo_argb32_to_pixbuf(pixels, w, h, stride);

  return result;
}

void on_generateHistogram_clicked (GtkButton *button,
				   gpointer user_data) {
  GtkWidget *w = glade_xml_get_widget(g_xml, "histogramWindow");
  GtkTextView *text_view = GTK_TEXT_VIEW(glade_xml_get_widget(g_xml, "histogram"));
  GtkTextBuffer *text = gtk_text_view_get_buffer(text_view);
  GtkIconView *view = GTK_ICON_VIEW(glade_xml_get_widget(g_xml, "searchResults"));
  GtkTreeModel *m = gtk_icon_view_get_model(view);
  GtkTextIter text_iter;

  GdkPixbuf *histogram_pix;

  gchar *tmp_str;

  GtkTreeIter iter;
  gboolean valid;

  PangoFontDescription *font_desc;

  gint row_count = 0;

  gtk_widget_show_all(w);

  double minR = HUGE_VAL;
  double maxR = 0.0;

  // clear text
  gtk_text_buffer_set_text(text, "", -1);

  // set font
  font_desc = pango_font_description_from_string ("Monospace");
  gtk_widget_modify_font (GTK_WIDGET(text_view), font_desc);
  pango_font_description_free(font_desc);

  // populate
  valid = gtk_tree_model_get_iter_first (m, &iter);

  // for selected?
  if (i_pix) {
    GList *c_list = circles_list;
    std::vector<double> radii;

    // do something
    while(c_list != NULL) {
      circle_type *c = (circle_type *) c_list->data;
      if (c->in_result) {
	double r = quadratic_mean_radius(c->a, c->b);
	radii.push_back(r);
	minR = MIN(r, minR);
	maxR = MAX(r, maxR);
      }

      c_list = g_list_next(c_list);
    }

    if (maxR == minR) {
      maxR += 10.0;
    }

    // print histogram info
    lti::histogram1D hist(30);
    int total = 0;
    for (int i = 0; i < radii.size(); i++) {
      int cell = (int) ((radii[i] - minR) / (maxR - minR) * hist.getLastCell());
      total++;
      hist.put(cell);
    }

    gtk_text_buffer_get_iter_at_offset(text, &text_iter, -1);

    tmp_str = g_strdup_printf("Histogram for selected image\n\n");
    gtk_text_buffer_insert(text, &text_iter, tmp_str, -1);
    g_free(tmp_str);


    double width = (maxR - minR) / (hist.getLastCell() + 1);
    for (int i = hist.getFirstCell(); i <= hist.getLastCell(); i++) {
      double low = minR + (width * i);
      double high = minR + (width * (i + 1));

      tmp_str = g_strdup_printf(" (%#g -- %#g): %5g\n", low, high, hist.at(i));
      gtk_text_buffer_insert(text, &text_iter, tmp_str, -1);
      g_free(tmp_str);
    }

    tmp_str = g_strdup_printf("\n Total circles: %d\n", total);
    gtk_text_buffer_insert(text, &text_iter, tmp_str, -1);
    g_free(tmp_str);

    // insert graphic
    gtk_text_buffer_insert(text, &text_iter, "\n", -1);
    histogram_pix = draw_histogram(hist, minR, maxR);
    gtk_text_buffer_insert_pixbuf(text, &text_iter, histogram_pix);
    g_object_unref(histogram_pix);

    gtk_text_buffer_insert(text, &text_iter, "\n\n\n", -1);
  }

  // for rest
  minR = HUGE_VAL;
  maxR = 0;

  std::vector<double> radii;

  while (valid) {
    // Walk through the list, reading each row
    GList *c_list;
    gtk_tree_model_get (m, &iter,
			2, &c_list,
			-1);

    // do something
    while(c_list != NULL) {
      circle_type *c = (circle_type *) c_list->data;
      if (c->in_result) {
	double r = quadratic_mean_radius(c->a, c->b);
	radii.push_back(r);
	minR = MIN(r, minR);
	maxR = MAX(r, maxR);
      }

      c_list = g_list_next(c_list);
    }

    row_count++;
    valid = gtk_tree_model_iter_next (m, &iter);
  }

  if (maxR == minR) {
    maxR += 10.0;
  }

  // print histogram info
  lti::histogram1D hist(30);
  int total = 0;
  for (int i = 0; i < radii.size(); i++) {
    int cell = (int) ((radii[i] - minR) / (maxR - minR) * hist.getLastCell());
    total++;
    hist.put(cell);
  }

  gtk_text_buffer_get_iter_at_offset(text, &text_iter, -1);

  tmp_str = g_strdup_printf("Histogram for all results\n\n");
  gtk_text_buffer_insert(text, &text_iter, tmp_str, -1);
  g_free(tmp_str);


  double width = (maxR - minR) / (hist.getLastCell() + 1);
  //  printf("minR: %g, maxR: %g, width: %g\n", minR, maxR, width);
  for (int i = hist.getFirstCell(); i <= hist.getLastCell(); i++) {
    double low = minR + (width * i);
    double high = minR + (width * (i + 1));

    tmp_str = g_strdup_printf(" (%#g -- %#g): %5g\n", low, high, hist.at(i));
    gtk_text_buffer_insert(text, &text_iter, tmp_str, -1);
    g_free(tmp_str);
  }
  tmp_str = g_strdup_printf("\n Total circles: %d\n", total);
  gtk_text_buffer_insert(text, &text_iter, tmp_str, -1);
  g_free(tmp_str);

  // insert graphic
  gtk_text_buffer_insert(text, &text_iter, "\n", -1);
  histogram_pix = draw_histogram(hist, minR, maxR);
  gtk_text_buffer_insert_pixbuf(text, &text_iter, histogram_pix);
  g_object_unref(histogram_pix);

  gtk_text_buffer_insert(text, &text_iter, "\n", -1);
}
