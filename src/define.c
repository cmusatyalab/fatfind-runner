#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>

#include "fatfind.h"
#include "define.h"

GtkListStore *saved_search_store;
static GdkPixbuf *c_pix_scaled;
static float scale;

static gboolean circle_match(circle_type *c) {
  gdouble r_min =
    gtk_range_get_value(GTK_RANGE(glade_xml_get_widget(g_xml, "radiusLower")));
  gdouble r_max =
    gtk_range_get_value(GTK_RANGE(glade_xml_get_widget(g_xml, "radiusUpper")));

  if (reference_circle_object == NULL) {
    return FALSE;
  }

  // scale by reference
  r_min *= reference_circle_object->r;
  r_max *= reference_circle_object->r;

  return ((c->r >= r_min) && (c->r <= r_max));
	  /*
	  && (c->fuzz >= fuzz - 0.1)
	  && (c->fuzz <= fuzz + 0.1));
	  */

}


void draw_define_offscreen_items(gint a_w, gint a_h) {
    // clear old scaled pix
  if (c_pix_scaled != NULL) {
    g_object_unref(c_pix_scaled);
    c_pix_scaled = NULL;
  }

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
    gint w = gdk_pixbuf_get_width(c_pix_scaled);
    gint h = gdk_pixbuf_get_height(c_pix_scaled);

    GdkBitmap *mask = gdk_pixmap_new(NULL, w, h, 1);
    GdkGC *mask_gc = gdk_gc_new(mask);
    GdkGC *gc = gdk_gc_new(d->window);

    GList *l = circles;

    GdkColor col = {0, 0, 0, 0};

    // draw a neutral background
    GdkPixbuf *bg =
      gdk_pixbuf_composite_color_simple(c_pix_scaled,
					w, h,
					GDK_INTERP_BILINEAR,
					100,
					16,
					d->style->light[GTK_WIDGET_STATE(d)].pixel,
					d->style->light[GTK_WIDGET_STATE(d)].pixel);
    gdk_draw_pixbuf(d->window,
		    d->style->fg_gc[GTK_WIDGET_STATE(d)],
		    bg,
		    0, 0, 0, 0,
		    -1, -1,
		    GDK_RGB_DITHER_NORMAL,
		    0, 0);

    // draw the mask to get the matching subset of circles
    gdk_gc_set_foreground(mask_gc, &col);
    gdk_draw_rectangle(mask, mask_gc, TRUE, 0, 0, w, h);
    col.pixel = 1;
    gdk_gc_set_foreground(mask_gc, &col);
    while (l != NULL) {
      circle_type *c = (circle_type *) l->data;

      if (circle_match(c)) {
	float extra = 1.2;
	float r = c->r;
	float x = c->x - extra * r;
	float y = c->y - extra * r;

	// draw
	x *= scale;
	y *= scale;
	r *= scale;


	gdk_draw_arc(mask, mask_gc, TRUE,
		     x, y, 2 * extra * r, 2 * extra * r,
		     0, 360*64);
      }
      l = l->next;
    }

    // now, draw the whole thing
    gdk_gc_set_clip_mask(gc, mask);

    gdk_draw_pixbuf(d->window,
		    gc,
		    c_pix_scaled,
		    0, 0, 0, 0,
		    -1, -1,
		    GDK_RGB_DITHER_NORMAL,
		    0, 0);

    g_object_unref(gc);
    g_object_unref(bg);
    g_object_unref(mask);
    g_object_unref(mask_gc);
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
  gtk_widget_queue_draw(glade_xml_get_widget(g_xml, "simulatedSearch"));
}

void on_saveSearchButton_clicked (GtkButton *button,
				  gpointer   user_data) {
  GtkTreeIter iter;

  gchar *save_name =
    gtk_entry_get_text(GTK_ENTRY(glade_xml_get_widget(g_xml, "searchName")));

  gdouble r_min = gtk_range_get_value(GTK_RANGE(glade_xml_get_widget(g_xml, "radiusLower")));
  gdouble r_max = gtk_range_get_value(GTK_RANGE(glade_xml_get_widget(g_xml, "radiusUpper")));

  if (reference_circle_object == NULL) {
    return;
  }

  // scale by reference
  r_min *= reference_circle_object->r;
  r_max *= reference_circle_object->r;

  g_debug("making new search: %s", save_name);
  gtk_list_store_append(saved_search_store, &iter);
  gtk_list_store_set(saved_search_store, &iter,
		     0, save_name,
		     1, r_min,
		     2, r_max,
		     -1);
}
