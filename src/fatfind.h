#ifndef FATFIND_H
#define FATFIND_H

#include <glade/glade.h>
#include <gtk/gtk.h>

#include "diamond_consts.h"
#include "diamond_types.h"

extern GladeXML *g_xml;
extern GdkPixbuf *c_pix;
extern GList *circles;

#define MAX_ALBUMS 32

typedef struct {
  float x;
  float y;
  float r;
} circle_type;

typedef struct {
  int num_gids;
  groupid_t gids[MAX_ALBUMS];
} gid_list_t;

struct collection_t
{
  char *name;
  //int id;
  int active;
};


extern GtkListStore *saved_search_store;
extern guint32 reference_circle;
extern circle_type *reference_circle_object;

extern GtkListStore *found_items;


#endif
