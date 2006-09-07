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
#include "diamond_interface.h"

GtkListStore *found_items;

static GdkPixbuf *i_pix;
static GdkPixbuf *i_pix_scaled;

static GdkPixmap *hitmap;
static gdouble prescale;
static gfloat display_scale;

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
    g_assert(g_source_remove(search_idle_id));
    ls_terminate_search(dr);
    dr = NULL;
  }
}

static void draw_hitmap(void) {
  GdkGC *gc = gdk_gc_new(hitmap);

  GdkColor col = {0, 0, 0, 0};

  GList *l = circles_list;

  GdkColor black = {-1, 0, 0, 0};

  int w, h;

  gdk_drawable_get_size(hitmap, &w, &h);

  // clear hitmap
  gdk_gc_set_foreground(gc, &black);
  gdk_draw_rectangle(hitmap, gc, TRUE, 0, 0, w, h);

  while (l != NULL) {
    gdk_gc_set_foreground(gc, &col);
    circle_type *c = (circle_type *) l->data;

    float x = c->x;
    float y = c->y;
    float r = MAX(c->a, c->b);

    // draw
    x *= display_scale;
    y *= display_scale;
    r *= display_scale;

    gdk_draw_arc(hitmap, gc, TRUE,
		 x - r, y - r, 2 * r, 2 * r,
		 0, 360*64);
    gdk_draw_arc(hitmap, gc, FALSE,
		 x - r, y - r, 2 * r, 2 * r,
		 0, 360*64);

    l = l->next;
    col.pixel++;
  }

  g_object_unref(gc);
}

static void set_show_circles(gboolean state) {
  if (show_circles != state) {
    show_circles = state;
    gtk_widget_queue_draw(glade_xml_get_widget(g_xml, "selectedResult"));
  }
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


static draw_investigate_offscreen_items(gint allocation_width,
					gint allocation_height) {
  // clear old scaled pix
  if (i_pix_scaled != NULL) {
    g_object_unref(i_pix_scaled);
    i_pix_scaled = NULL;
  }
  if (hitmap != NULL) {
    g_object_unref(hitmap);
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
      w = h * p_aspect;
      display_scale = (float) allocation_height
	/ (float) gdk_pixbuf_get_height(i_pix);
    } else {
      /* else calculate height from width */
      h = w / p_aspect;
      display_scale = (float) allocation_width
	/ (float) gdk_pixbuf_get_width(i_pix);
    }

    display_scale *= prescale;

    i_pix_scaled = gdk_pixbuf_scale_simple(i_pix,
					   w, h,
					   GDK_INTERP_BILINEAR);


    hitmap = gdk_pixmap_new(NULL, w, h, 32);
    draw_hitmap();
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
    GtkWidget *stopSearch = glade_xml_get_widget(g_xml, "stopSearch");


    g_debug("saved_search_store: %p", model);
    gtk_tree_model_get(model,
		       &s_iter,
		       1, &r_min, 2, &r_max, -1);

    g_debug("searching from %g to %g", r_min, r_max);

    // diamond
    dr = diamond_circle_search(r_min, r_max);

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
      draw_circles_into_widget(d, circles_list, display_scale);
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
  guint32 hit = -1;

  // if not selected, do nothing
  if (i_pix == NULL) {
    return FALSE;
  }

  if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
    g_debug("x: %g, y: %g", event->x, event->y);
    set_show_circles(TRUE);

    if (event->state & GDK_SHIFT_MASK) {
      // delete
      hit = get_circle_at_point(hitmap, event->x, event->y);

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
			    scale * prescale, w, h);

	// add it back
	gtk_list_store_set(store, &iter,
			   0, pix2,
			   1, title,
			   2, circles_list,
			   -1);

	g_object_unref(pix2);
	draw_hitmap();
	gtk_widget_queue_draw(widget);
      }
      return TRUE;
    } else {
      // start add
      is_adding = TRUE;
      x_add_current = x_add_start = event->x;
      y_add_current = y_add_start = event->y;

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
      int hit = get_circle_at_point(hitmap, event->x, event->y);
      if (hit != -1) {
	c = (circle_type *) (g_list_nth(circles_list, hit)->data);
	c->in_result = !c->in_result;
      }
    } else {
      // make a new circle
      c = g_malloc(sizeof(circle_type));
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
			scale * prescale, w, h);

    // update
    gtk_list_store_set(store, &iter,
		       0, pix2,
		       1, title,
		       2, circles_list,
		       -1);

    g_object_unref(pix2);

    draw_hitmap();
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
  g_value_set_double(&hPos, event->x);
  g_object_set_property(G_OBJECT(h), "position", &hPos);

  g_value_init(&vPos, G_TYPE_DOUBLE);
  g_value_set_double(&vPos, event->y);
  g_object_set_property(G_OBJECT(v), "position", &vPos);


  if (is_adding) {
    // draw dynamic circle
    x_add_current = event->x;
    y_add_current = event->y;

    gtk_widget_queue_draw(glade_xml_get_widget(g_xml, "selectedResult"));
  }

  return TRUE;
}

gboolean on_selectedResult_configure_event (GtkWidget         *widget,
					    GdkEventConfigure *event,
					    gpointer          user_data) {
  draw_investigate_offscreen_items(event->width, event->height);
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

void on_generateHistogram_clicked (GtkButton *button,
				   gpointer user_data) {
  GtkWidget *w = glade_xml_get_widget(g_xml, "histogramWindow");
  GtkIconView *view = GTK_ICON_VIEW(glade_xml_get_widget(g_xml, "searchResults"));
  GtkTreeModel *m = gtk_icon_view_get_model(view);

  GtkTreeIter iter;
  gboolean valid;

  gint row_count = 0;

  gtk_widget_show_all(w);

  // populate
  /* Get the first iter in the list */
  valid = gtk_tree_model_get_iter_first (m, &iter);

  while (valid) {
    /* Walk through the list, reading each row */
    GList *c_list;

    /* Make sure you terminate calls to gtk_tree_model_get()
     * with a '-1' value
     */
    gtk_tree_model_get (m, &iter,
			2, &c_list,
			-1);

    /* Do something with the data */
    g_print ("Item %d has %d circles\n", row_count, g_list_length(c_list));

    row_count++;
    valid = gtk_tree_model_iter_next (m, &iter);
  }


  // print histogram info
  printf("Histogram\n\n");
  printf("Items found: %d\n", row_count);

  printf("\n\n");
}
