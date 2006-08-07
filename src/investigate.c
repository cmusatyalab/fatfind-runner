#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>

#include "fatfind.h"
#include "investigate.h"
#include "diamond_interface.h"

GtkListStore *found_items;


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



void on_clearSearch_clicked (GtkButton *button,
			     gpointer   user_data) {
  // disable stop button
  GtkWidget *stopSearch = glade_xml_get_widget(g_xml, "stopSearch");
  gtk_widget_set_sensitive(stopSearch, FALSE);

  // stop
  stop_search();

  // clear search thumbnails
  gtk_list_store_clear(found_items);
}

void on_stopSearch_clicked (GtkButton *button,
			    gpointer user_data) {
  GtkWidget *stopSearch = glade_xml_get_widget(g_xml, "stopSearch");
  gtk_widget_set_sensitive(stopSearch, FALSE);

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
    dr = diamond_circle_search(1, 0, 2, 400, r_min, r_max, 2, 100);

    // take the handle, put it into the idle callback to get
    // the results?
    search_idle_id = g_idle_add(diamond_result_callback, dr);

    // activate the stop search button
    gtk_widget_set_sensitive(stopSearch, TRUE);
  }
}

