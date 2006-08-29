#ifndef FATFIND_H
#define FATFIND_H

#include <glade/glade.h>
#include <gtk/gtk.h>

#include "circles4.h"

extern GladeXML *g_xml;
extern GdkPixbuf *c_pix;


extern GtkListStore *saved_search_store;
extern guint32 reference_circle;
extern circle_type *reference_circle_object;

extern GtkListStore *found_items;


#endif
