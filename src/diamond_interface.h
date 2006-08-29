#ifndef DIAMOND_INTERFACE_H
#define DIAMOND_INTERFACE_H

#include "diamond_consts.h"
#include "diamond_types.h"
#include "lib_searchlet.h"

#include <stdio.h>

#define MAX_ALBUMS 32

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


ls_search_handle_t diamond_circle_search (int minRadius, int maxRadius);
gboolean diamond_result_callback(gpointer data);


#endif
