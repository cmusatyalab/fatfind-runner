#include <glib.h>

#include "fatfind.h"
#include "diamond_interface.h"

static struct collection_t collections[MAX_ALBUMS+1] = { { NULL } };
static gid_list_t diamond_gid_list;

static ls_search_handle_t init_diamond(void) {
  int i;
  int j;
  int err;
  void *cookie;
  char *name;

  ls_search_handle_t diamond_handle;

  if (diamond_gid_list.num_gids != 0) {
    // already initialized
    return;
  }

  printf("reading collections...\n");
  {
    int pos = 0;
    err = nlkup_first_entry(&name, &cookie);
    while(!err && pos < MAX_ALBUMS)
      {
	collections[pos].name = name;
	collections[pos].active = pos ? 0 : 1; /* first one active */
	pos++;
	err = nlkup_next_entry(&name, &cookie);
      }
    collections[pos].name = NULL;
  }


  for (i=0; i<MAX_ALBUMS && collections[i].name; i++) {
    if (collections[i].active) {
      int err;
      int num_gids = MAX_ALBUMS;
      groupid_t gids[MAX_ALBUMS];
      err = nlkup_lookup_collection(collections[i].name, &num_gids, gids);
      g_assert(!err);
      for (j=0; j < num_gids; j++) {
	printf("adding gid: %lld\n", gids[j]);
	diamond_gid_list.gids[diamond_gid_list.num_gids++] = gids[j];
      }
    }
  }

  diamond_handle = ls_init_search();
  err = ls_set_searchlist(diamond_handle, 1, diamond_gid_list.gids);
  g_assert(!err);

  err = ls_set_searchlet(diamond_handle, DEV_ISA_IA32,
			 FATFIND_FILTERDIR "/fil_rgb.so",
			 FATFIND_FILTERDIR "/rgb-filter.txt");
  g_assert(!err);


  return diamond_handle;

  // XXX
  ls_start_search(diamond_handle);
  while(1) {
    ls_obj_handle_t obj;
    void *data;
    int len;
    err = ls_next_object(diamond_handle, &obj, 0);
    if (err) {
      break;
    }

    printf("got object: %p\n", obj);

    printf("calling lf_first_attr with %p %p %p %p %p\n",
	   obj, &name, &len, &data, &cookie);
    err = lf_first_attr(obj, &name, &len, &data, &cookie);
    while (!err) {
      printf(" attr name: %s, length: %d\n", name, len);
      err = lf_next_attr(obj, &name, &len, &data, &cookie);
    }

    err = ls_release_object(diamond_handle, obj);
    g_assert(!err);
  }
}


