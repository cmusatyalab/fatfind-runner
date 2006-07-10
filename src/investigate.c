#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <stdlib.h>

#include "fatfind.h"
#include "investigate.h"

GtkListStore *found_items;




void on_clearSearch_clicked (GtkButton *button,
			     gpointer   user_data) {
  // clear search thumbnails
  gtk_list_store_clear(found_items);
}

void on_startSearch_clicked (GtkButton *button,
			     gpointer   user_data) {
  // get the selected search
  GtkTreeIter s_iter;
  GtkTreeSelection *selection =
    gtk_tree_view_get_selection(GTK_TREE_VIEW(glade_xml_get_widget(g_xml,
								   "definedSearches")));

  if (gtk_tree_selection_get_selected(selection,
				      saved_search_store,
				      &s_iter)) {
    gdouble r_min;
    gdouble r_max;

    gtk_tree_model_get(saved_search_store, &s_iter, 1, &r_min, 2, &r_max, -1);


    // build model
  }
}

