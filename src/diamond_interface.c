/*
 * FatFind: A Diamond application for adipocyte image exploration
 *
 * Copyright (c) 2006 Carnegie Mellon University. All rights reserved.
 * Additional copyrights may be listed below.
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License v1.0 which accompanies this
 * distribution in the file named LICENSE.
 *
 * Technical and financial contributors are listed in the file named
 * CREDITS.
 */

#include <glib.h>

#include "diamond_interface.h"
#include "fatfind.h"
#include "util.h"
#include "drawutil.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include "lib_filter.h"
#include "lib_dconfig.h"

static struct collection_t collections[MAX_ALBUMS+1] = { { NULL } };
static gid_list_t diamond_gid_list;

int total_objects;
int processed_objects;
int dropped_objects;

int displayed_objects;

static GSList *collection_menu_group;

static void update_gids(void) {
  int i, j;

  // clear
  diamond_gid_list.num_gids = 0;
  for (i=0; i<MAX_ALBUMS && collections[i].name; i++) {
    if (collections[i].active) {
      int err;
      int num_gids = MAX_ALBUMS;
      groupid_t gids[MAX_ALBUMS];
      err = nlkup_lookup_collection(collections[i].name, &num_gids, gids);
      g_assert(!err);
      for (j=0; j < num_gids; j++) {
	printf("adding gid: %lld for collection %s\n", gids[j], collections[i].name);
	diamond_gid_list.gids[diamond_gid_list.num_gids++] = gids[j];
      }
    }
  }
}

static void scope_menu_item_activated(GtkMenuItem *menuitem, gpointer user_data) {
  GtkCheckMenuItem *m = GTK_CHECK_MENU_ITEM(menuitem);
  int length = g_slist_length(collection_menu_group);
  int position = (length - 1) - g_slist_index(collection_menu_group, m);

  // update active
  collections[position].active = m->active;
  if (m->active) {
    g_debug("collection %d (%s) added", position, collections[position].name);
  } else {
    g_debug("collection %d (%s) removed", position, collections[position].name);
  }
  update_gids();
}


void diamond_init(void) {
  int i;
  int j;
  int err;
  void *cookie;
  char *name;

  GtkMenuShell *menu;
  GtkMenuItem *scopeMenu;

  static gboolean initialized;

  if (initialized) {
    // already initialized
    return;
  }
  initialized = TRUE;

  scopeMenu = GTK_MENU_ITEM(glade_xml_get_widget(g_xml, "scopeMenu"));
  menu = GTK_MENU_SHELL(gtk_menu_new());
  gtk_menu_item_set_submenu(scopeMenu, GTK_WIDGET(menu));

  printf("reading collections...\n");
  {
    int pos = 0;
    GtkWidget *mi = NULL;

    err = nlkup_first_entry(&name, &cookie);
    while(!err && pos < MAX_ALBUMS) {

      collections[pos].name = name;
      mi = gtk_radio_menu_item_new_with_label(collection_menu_group,
					      name);
      collection_menu_group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(mi));
      gtk_menu_shell_prepend(menu, mi);
      collections[pos].active = 0;

      g_signal_connect(mi, "activate", G_CALLBACK(scope_menu_item_activated),
		       NULL);
      pos++;
      err = nlkup_next_entry(&name, &cookie);
    }

    if (mi != NULL) {
      // set the last (first) item active
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi),
				     TRUE);
    }
    collections[pos].name = NULL;
  }
}

static ls_search_handle_t generic_search (char *filter_spec_name) {
  ls_search_handle_t diamond_handle;
  int f1, f2;
  int err;

  char buf[1];

  diamond_handle = ls_init_search();

  err = ls_set_searchlist(diamond_handle, diamond_gid_list.num_gids,
			  diamond_gid_list.gids);
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


static void update_stats(ls_search_handle_t dr) {
  int num_dev;
  ls_dev_handle_t dev_list[16];
  int i, err, len;
  dev_stats_t *dstats;
  int tobj = 0, sobj = 0, dobj = 0;
  GtkLabel *stats_label = GTK_LABEL(glade_xml_get_widget(g_xml, "statsLabel"));

  guchar *tmp;

  dstats = (dev_stats_t *) g_malloc(DEV_STATS_SIZE(32));

  num_dev = 16;

  err = ls_get_dev_list(dr, dev_list, &num_dev);
  if (err != 0) {
    g_error("update stats: %d", err);
  }

  for (i = 0; i < num_dev; i++) {
    len = DEV_STATS_SIZE(32);

    err = ls_get_dev_stats(dr, dev_list[i], dstats, &len);
    if (err) {
      g_error("Failed to get dev stats");
    }
    tobj += dstats->ds_objs_total;
    sobj += dstats->ds_objs_processed;
    dobj += dstats->ds_objs_dropped;
  }
  g_free(dstats);

  total_objects = tobj;
  processed_objects = sobj;
  dropped_objects = dobj;
  tmp =
    g_strdup_printf("Total objects: %d, Processed objects: %d, Dropped objects: %d, Displayed objects: %d",
		    total_objects, processed_objects, dropped_objects, displayed_objects);
  gtk_label_set_text(stats_label, tmp);
  g_free(tmp);
}


gboolean diamond_result_callback(gpointer g_data) {
  // this gets 0 or 1 result from diamond and puts it into the
  // icon view

  ls_obj_handle_t obj;
  void *data;
  char *name;
  void *cookie;
  unsigned int len;
  int err;
  int w, origW;
  int h, origH;
  GdkPixbuf *pix, *pix2, *pix3;

  static time_t last_time;

  int i;

  GList *clist = NULL;
  gchar *title;

  GtkTreeIter iter;

  double scale, prescale;

  ls_search_handle_t dr;


  // get handle
  dr = (ls_search_handle_t) g_data;

  // get handle
  err = ls_next_object(dr, &obj, LSEARCH_NO_BLOCK);

  // XXX should be able to use select()
  if (err == EWOULDBLOCK) {
    // no results right now
    time_t now = time(NULL);
    if (now > last_time) {
      update_stats(dr);
    }
    last_time = now;

    return TRUE;
  } else if (err || (displayed_objects > 100)) {
    // no more results
    GtkWidget *stopSearch = glade_xml_get_widget(g_xml, "stopSearch");
    GtkWidget *startSearch = glade_xml_get_widget(g_xml, "startSearch");
    gtk_widget_set_sensitive(stopSearch, FALSE);
    gtk_widget_set_sensitive(startSearch, TRUE);

    update_stats(dr);

    ls_abort_search(dr);
    return FALSE;
  }

  // more results
  printf("got object: %p\n", obj);

  err = lf_ref_attr(obj, "circle-data", &len, (unsigned char **) &data);
  g_assert(!err);

  displayed_objects++;

  // XXX
  for (i = 0; i < len / sizeof(circle_type); i++) {
    circle_type *p = ((circle_type *) data) + i;
    circle_type *c = g_malloc(sizeof(circle_type));

    *c = *p;

    clist = g_list_prepend(clist, c);
  }

  // text
  title = make_thumbnail_title(clist);

  // thumbnail
  err = lf_ref_attr(obj, "_cols.int", &len, (unsigned char **) &data);
  g_assert(!err);
  origW = w = *((int *) data);

  err = lf_ref_attr(obj, "_rows.int", &len, (unsigned char **) &data);
  g_assert(!err);
  origH = h = *((int *) data);

  err = lf_ref_attr(obj, "_rgb_image.rgbimage", &len, (unsigned char **) &data);
  g_assert(!err);

  printf(" img %dx%d (%d bytes)\n", w, h, len);

  pix = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB,
				 TRUE, 8, w, h, w*4, NULL, NULL);
  compute_thumbnail_scale(&scale, &w, &h);

  // draw into thumbnail
  pix2 = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
  draw_into_thumbnail(pix2, pix, clist, scale, scale, w, h,
		      filter_by_in_result);

  // draw into scaled-down image
  w *= 4;
  h *= 4;
  pix3 = gdk_pixbuf_scale_simple(pix,
				 w, h,
				 GDK_INTERP_BILINEAR);
  prescale = (double) w / (double) origW;

  // store
  gtk_list_store_append(found_items, &iter);
  gtk_list_store_set(found_items, &iter,
		     0, pix2,
		     1, title,
		     2, clist,
		     3, pix3,
		     4, prescale,
		     -1);

  g_object_unref(pix);
  g_object_unref(pix2);
  g_object_unref(pix3);


  //  err = lf_first_attr(obj, &name, &len, &data, &cookie);
  //  while (!err) {
  //    printf(" attr name: %s, length: %d\n", name, len);
  //    err = lf_next_attr(obj, &name, &len, &data, &cookie);
  //  }

  err = ls_release_object(dr, obj);
  g_assert(!err);

  return TRUE;
}


ls_search_handle_t diamond_circle_search(double minRadius, double maxRadius,
					 double maxEccentricity) {
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
	  "ARG  %1.20e\n"
	  "ARG  %1.20e\n"
	  "ARG  %1.20e\n"
	  "REQUIRES  RGB # dependencies\n"
	  "MERIT  50 # some relative cost\n",
	  minRadius, maxRadius, maxEccentricity);
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

