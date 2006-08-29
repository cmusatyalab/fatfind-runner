#include <glib.h>

#include "diamond_interface.h"
#include "fatfind.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>

static struct collection_t collections[MAX_ALBUMS+1] = { { NULL } };
static gid_list_t diamond_gid_list;

static void diamond_init(void) {
  int i;
  int j;
  int err;
  void *cookie;
  char *name;

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
}

static ls_search_handle_t generic_search (char *filter_spec_name) {
  ls_search_handle_t diamond_handle;
  int f1, f2;
  int err;

  char buf[1];

  diamond_init();

  diamond_handle = ls_init_search();

  err = ls_set_searchlist(diamond_handle, 1, diamond_gid_list.gids);
  g_assert(!err);

  // append our stuff
  f1 = g_open(FATFIND_FILTERDIR "/rgb-filter.txt", O_RDONLY);
  if (f1 == -1) {
    perror("can't open " FATFIND_FILTERDIR "/rgb-filter.txt");
    return NULL;
  }

  f2 = g_open(filter_spec_name, O_WRONLY | O_APPEND);
  if (f2 == -1) {
    printf("can't open %s", filter_spec_name);
    perror("");
    return NULL;
  }

  while (read(f1, buf, 1) > 0) {
    write(f2, buf, 1);   // PERF
  }

  close(f1);
  close(f2);

  err = ls_set_searchlet(diamond_handle, DEV_ISA_IA32,
			 FATFIND_FILTERDIR "/fil_rgb.so",
			 filter_spec_name);
  g_assert(!err);


  return diamond_handle;
}


gboolean diamond_result_callback(gpointer g_data) {
  // this gets 0 or 1 result from diamond and puts it into the
  // icon view

  ls_obj_handle_t obj;
  void *data;
  char *name;
  void *cookie;
  int len;
  int err;
  int w;
  int h;
  GdkPixbuf *pix, *pix2;
  GdkPixmap *pixmap;

  float p_aspect;

  int i;

  GList *clist = NULL;
  gchar *title;

  GtkTreeIter iter;


  // get handle
  ls_search_handle_t dr = (ls_search_handle_t) g_data;

  err = ls_next_object(dr, &obj, LSEARCH_NO_BLOCK);

  // XXX should be able to use select()
  if (err == EWOULDBLOCK) {
    // no results right now
    return TRUE;
  } else if (err) {
    // no more results
    ls_abort_search(dr);
    return FALSE;
  }

  printf("got object: %p\n", obj);

  err = lf_ref_attr(obj, "circle-data", &len, &data);
  g_assert(!err);

  // XXX
  for (i = 0; i < len / sizeof(float); i += 5) {
    float *p = ((float *) data) + i;
    circle_type *c = g_malloc(sizeof(circle_type));

    c->x = p[0];
    c->y = p[1];
    c->a = p[2];
    c->b = p[3];
    c->t = p[4];

    clist = g_list_prepend(clist, c);
  }

  // text
  title = g_strdup_printf("%d items", g_list_length(clist));

  // thumbnail
  err = lf_ref_attr(obj, "_cols.int", &len, &data);
  g_assert(!err);
  w = *((int *) data);

  err = lf_ref_attr(obj, "_rows.int", &len, &data);
  g_assert(!err);
  h = *((int *) data);

  err = lf_ref_attr(obj, "_rgb_image.rgbimage", &len, &data);
  g_assert(!err);

  printf(" img %dx%d (%d bytes)\n", w, h, len);

  pix = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB,
				 TRUE, 8, w, h, w*4, NULL, NULL);
  p_aspect = (float) w / (float) h;
  if (p_aspect < 1) {
    /* then calculate width from height */
    h = 150;
    w = h * p_aspect;
  } else {
    /* else calculate height from width */
    w = 150;
    h = w / p_aspect;
  }
  pix2 = gdk_pixbuf_scale_simple(pix, w, h, GDK_INTERP_BILINEAR);

  // draw into thumbnail?
  /*
  {
    GtkWidget *w = glade_xml_get_widget(g_xml, "fatfind");
    GdkGC *gc;
    pixmap = gdk_pixmap_new(w->window, w, h, 8);
    gc = gdk_gc_new(pixmap);
    gdk_draw_pixbuf(pixmap, gc, pix2,
		    0, 0, 0, 0, -1, -1, GDK_RGB_DITHER_NORMAL,
		    0, 0);
    g_object_unref(gc);
  }
  */

  gtk_list_store_append(found_items, &iter);
  gtk_list_store_set(found_items, &iter,
		     0, pix2,
		     1, title,
		     2, clist,
		     -1);

  g_object_unref(pix);
  g_object_unref(pix2);
  g_object_unref(pixmap);

  //  err = lf_first_attr(obj, &name, &len, &data, &cookie);
  //  while (!err) {
  //    printf(" attr name: %s, length: %d\n", name, len);
  //    err = lf_next_attr(obj, &name, &len, &data, &cookie);
  //  }

  err = ls_release_object(dr, obj);
  g_assert(!err);

  return TRUE;
}


ls_search_handle_t diamond_circle_search(int minRadius, int maxRadius) {
  ls_search_handle_t dr;
  int fd;
  FILE *f;
  gchar *name_used;
  int err;

  // temporary file
  fd = g_file_open_tmp(NULL, &name_used, NULL);
  g_assert(fd >= 0);

  // write out file for circle search
  f = fdopen(fd, "a");
  g_return_val_if_fail(f, NULL);
  fprintf(f, "\n\n"
	  "FILTER circles\n"
	  "THRESHOLD 1\n"
	  "EVAL_FUNCTION  f_eval_circles\n"
	  "INIT_FUNCTION  f_init_circles\n"
	  "FINI_FUNCTION  f_fini_circles\n"
	  "ARG  %d\n"
	  "ARG  %d\n"
	  "REQUIRES  RGB # dependencies\n"
	  "MERIT  50 # some relative cost\n",
	  minRadius, maxRadius);
  fflush(f);

  // initialize with generic search
  dr = generic_search(name_used);

  // add filter
  err = ls_add_filter_file(dr, DEV_ISA_IA32,
			   FATFIND_FILTERDIR "/fil_circle.so");
  g_assert(!err);

  // now close
  fclose(f);
  g_free(name_used);

  // start search
  ls_start_search(dr);

  // return
  return dr;
}

