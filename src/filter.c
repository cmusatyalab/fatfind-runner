/*
 *  FatFind
 *  A Diamond application for adipocyte image exploration
 *  Version 1
 *
 *  Copyright (c) 2006 Carnegie Mellon University
 *  All Rights Reserved.
 *
 *  This software is distributed under the terms of the Eclipse Public
 *  License, Version 1.0 which can be found in the file named LICENSE.
 *  ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS SOFTWARE CONSTITUTES
 *  RECIPIENT'S ACCEPTANCE OF THIS AGREEMENT
 */

#include "circles4.h"
#include "lib_filter.h"


static void free_1_circle(gpointer data, gpointer user_data) {
  g_free((circle_type *) data);
}


#ifdef __cplusplus
extern "C" {
#endif

// 3 functions for diamond filter interface
diamond_public
int f_init_circles (int num_arg, char **args,
		    int bloblen, void *blob_data,
		    const char *filter_name,
		    void **filter_args) {
  circles_state_t *cr;

  // check parameters
  if (num_arg != 4) {
    return -1;
  }

  // init state
  cr = (circles_state_t *)g_malloc(sizeof(circles_state_t));

  cr->minRadius = g_ascii_strtod(args[0], NULL);
  cr->maxRadius = g_ascii_strtod(args[1], NULL);
  cr->maxEccentricity = g_ascii_strtod(args[2], NULL);
  cr->minSharpness = g_ascii_strtod(args[3], NULL);

  // we're good
  *filter_args = cr;
  return 0;
}


diamond_public
int f_eval_circles (lf_obj_handle_t ohandle, void *filter_args) {
  // circles
  GList *clist;
  circles_state_t *cr = (circles_state_t *) filter_args;
  int num_circles;
  int num_circles_in_result = 0;

  // for attributes from diamond
  size_t len;
  unsigned char *data;



  // get the data from the ohandle, convert to IplImage
  int w;
  int h;

  // width
  lf_ref_attr(ohandle, "_rows.int", &len, &data);
  h = *((int *) data);

  // height
  lf_ref_attr(ohandle, "_cols.int", &len, &data);
  w = *((int *) data);

  // image data
  lf_ref_attr(ohandle, "_rgb_image.rgbimage", &len, &data);
  lf_omit_attr(ohandle, "_rgb_image.rgbimage");

  // feed it to our processor
  clist = circlesFromImage2(cr, w, h, w * 4, 4, data);

  data = NULL;

  // add the list of circles to the cache and the object
  // XXX !
  num_circles = g_list_length(clist);
  if (num_circles > 0) {
    GList *l = clist;
    int i = 0;
    data = (unsigned char *) g_malloc(sizeof(circle_type) * num_circles);

    // pack in and count
    while (l != NULL) {
      circle_type *p = ((circle_type *) data) + i;
      circle_type *c = (circle_type *) l->data;
      *p = *c;

      if (c->in_result) {
	num_circles_in_result++;
      }

      i++;
      l = g_list_next(l);
    }
    lf_write_attr(ohandle, "circle-data", sizeof(circle_type) * num_circles, data);
  }

  // free others
  g_list_foreach(clist, free_1_circle, NULL);
  g_list_free(clist);
  clist = NULL;
  g_free(data);
  data = NULL;

  // return number of circles
  return num_circles_in_result;
}



diamond_public
int f_fini_circles (void *filter_args) {
  g_free((circles_state_t *) filter_args);

  return 0;
}

#ifdef __cplusplus
}
#endif
