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




void on_clearSearch_clicked (GtkButton *button,
			     gpointer   user_data) {
  // clear search thumbnails
  gtk_list_store_clear(found_items);
}

void on_startSearch_clicked (GtkButton *button,
			     gpointer   user_data) {
  ls_search_handle_t dr;

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

    g_debug("saved_search_store: %p", model);
    gtk_tree_model_get(model,
		       &s_iter,
		       1, &r_min, 2, &r_max, -1);

    g_debug("searching from %g to %g", r_min, r_max);

    // diamond
    dr = diamond_circle_search(1, 0, 2, 400, r_min, r_max, 2, 100);

    // take the handle, put it into the idle callback to get
    // the results?
    g_idle_add(diamond_result_callback, dr);
  }
}

