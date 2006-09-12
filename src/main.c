/******************************************************************************
 * Copyright (c) 2006 Carnegie Mellon University.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    Adam Goode - initial implementation
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "fatfind.h"

static void setup_saved_search_store(void) {
  GtkTreeView *v = GTK_TREE_VIEW(glade_xml_get_widget(g_xml,
						      "definedSearches"));
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  saved_search_store = gtk_list_store_new(3,
					  G_TYPE_STRING,
					  G_TYPE_DOUBLE,
					  G_TYPE_DOUBLE);

  gtk_tree_view_set_model(v, GTK_TREE_MODEL(saved_search_store));


  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes ("Name",
						     renderer,
						     "text", 0,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (v), column);
}

static void setup_thumbnails(GtkIconView *g, gchar *file) {
  GtkListStore *s;
  GtkTreeIter iter;
  gchar buf[1024];
  gchar *dirname = g_path_get_dirname(file);

  // get index
  FILE *f = fopen(file, "r");
  if (f == NULL) {
    perror("Error getting index");
    exit(1);
  }

  // create the model
  s = gtk_list_store_new(3, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);

  // get all the thumbnails
  while (1) {
    GError *err = NULL;
    gchar *tmp, *tmp2;
    gchar *filename, *filename2;
    GdkPixbuf *pix;

    int result;

    result = fscanf(f, "%1024s", buf);
    if (result == EOF) {
      break;
    } else if (result != 1) {
      perror("bad result");
    }

    tmp = g_strdup_printf("%s.JPG", buf);
    tmp2 = g_strdup_printf("%s.txt", buf);

    filename = g_build_filename(dirname, tmp, NULL);
    filename2 = g_build_filename(dirname, tmp2, NULL);

    pix = gdk_pixbuf_new_from_file_at_size(filename,
					   150,
					   -1,
					   &err);
    if (err != NULL) {
      g_critical("error: %s", err->message);
      g_error_free(err);
    }

    g_free(tmp);
    g_free(tmp2);

    gtk_list_store_append(s, &iter);
    gtk_list_store_set(s, &iter,
		       0, pix,
		       1, filename,
		       2, filename2,
		       -1);


    g_object_unref(pix);
    g_free(filename);
    g_free(filename2);
  }

  // set it up
  gtk_icon_view_set_model(g, GTK_TREE_MODEL(s));
  gtk_icon_view_set_pixbuf_column(g, 0);
  //gtk_icon_view_set_text_column(g, 2);

  fclose(f);
  g_free(dirname);
}


static void setup_results_store(GtkIconView *g) {
  GtkTreeIter iter;

  found_items =
    gtk_list_store_new(5,
		       GDK_TYPE_PIXBUF,
		       G_TYPE_STRING,
		       G_TYPE_POINTER,
		       GDK_TYPE_PIXBUF,
		       G_TYPE_DOUBLE);

  gtk_icon_view_set_model(g, GTK_TREE_MODEL(found_items));
  gtk_icon_view_set_pixbuf_column(g, 0);
  gtk_icon_view_set_text_column(g, 1);
}


void on_about1_activate (GtkMenuItem *menuitem, gpointer user_data) {
  gtk_widget_show(glade_xml_get_widget(g_xml, "about1"));
}


GladeXML *g_xml;

int
main (int argc, char *argv[])
{
  GtkWidget *fatfind;

  gtk_init(&argc,&argv);
  g_xml = glade_xml_new(FATFIND_GLADEDIR "/fatfind.glade",NULL,NULL);
  g_assert(g_xml != NULL);

  if (argc != 2) {
    g_critical("No image index given on command line");
    return;
  }

  glade_xml_signal_autoconnect(g_xml);
  fatfind = glade_xml_get_widget(g_xml,"fatfind");


  // setup the thumbnails
  setup_thumbnails(GTK_ICON_VIEW(glade_xml_get_widget(g_xml,
						      "calibrationImages")),
		   argv[1]);

  // init saved searches
  setup_saved_search_store();

  // init results
  setup_results_store(GTK_ICON_VIEW(glade_xml_get_widget(g_xml,
							 "searchResults")));


  gtk_widget_show_all(fatfind);

  gtk_main();
  return 0;
}
