#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>

#include "fatfind.h"
#include "investigate.h"
#include "util.h"
#include "diamond_interface.h"

GtkListStore *found_items;

static GdkPixbuf *i_pix;
static GdkPixbuf *i_pix_scaled;

static GdkPixmap *hitmap;
static gdouble prescale;
static gfloat scale;

static GList *circles_list;

static ls_search_handle_t dr;
static guint search_idle_id;

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

  GList *l = circles;

    while (l != NULL) {
    gdk_gc_set_foreground(gc, &col);
    circle_type *c = (circle_type *) l->data;

    float x = c->x;
    float y = c->y;
    float r = MAX(c->a, c->b);

    // draw
    x *= scale;
    y *= scale;
    r *= scale;

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

static void foreach_select_investigation(GtkIconView *icon_view,
					 GtkTreePath *path,
					 gpointer data) {
  GError *err = NULL;

  GtkTreeIter iter;
  GtkTreeModel *m = gtk_icon_view_get_model(icon_view);
  GtkWidget *w;

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
    GdkColor black = {-1, 0, 0, 0};

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
      scale = (float) allocation_height
	/ (float) gdk_pixbuf_get_height(i_pix);
    } else {
      /* else calculate height from width */
      h = w / p_aspect;
      scale = (float) allocation_width
	/ (float) gdk_pixbuf_get_width(i_pix);
    }

    scale *= prescale;

    i_pix_scaled = gdk_pixbuf_scale_simple(i_pix,
					   w, h,
					   GDK_INTERP_BILINEAR);


    hitmap = gdk_pixmap_new(NULL, w, h, 32);
    gc = gdk_gc_new(hitmap);

    // clear hitmap
    gdk_gc_set_foreground(gc, &black);
    gdk_draw_rectangle(hitmap, gc, TRUE, 0, 0, w, h);
    g_object_unref(gc);

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

    draw_circles_into_widget(d, circles_list, scale);
  }

  return TRUE;
}

gboolean on_selectedResult_button_press_event (GtkWidget      *widget,
					       GdkEventButton *event,
					       gpointer        user_data) {
  g_debug("button press");
  return TRUE;
}

gboolean on_selectedResult_button_release_event (GtkWidget      *widget,
						 GdkEventButton *event,
						 gpointer        user_data) {
  g_debug("button release");
  return TRUE;
}

gboolean on_selectedResult_motion_notify_event (GtkWidget      *widget,
						GdkEventMotion *event,
						gpointer        user_data) {
  g_debug("motion");
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
