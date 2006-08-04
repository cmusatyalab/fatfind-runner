#ifndef DIAMOND_INTERFACE_H
#define DIAMOND_INTERFACE_H

#include "diamond_consts.h"
#include "diamond_types.h"
#include "lib_searchlet.h"



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


#endif
