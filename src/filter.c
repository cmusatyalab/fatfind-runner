/*
 *  FatFind
 *  A Diamond application for adipocyte image exploration
 *  Version 1
 *
 *  Copyright (c) 2006-2008 Carnegie Mellon University
 *  All Rights Reserved.
 *
 *  This software is distributed under the terms of the Eclipse Public
 *  License, Version 1.0 which can be found in the file named LICENSE.
 *  ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS SOFTWARE CONSTITUTES
 *  RECIPIENT'S ACCEPTANCE OF THIS AGREEMENT
 */

#include "circles4.h"
#include "util.h"

#include <math.h>
#include <string.h>

#include "lib_filter.h"


static void free_1_circle(gpointer data, gpointer user_data) {
  g_slice_free(circle_type, data);
}

// inspired by
// http://www.csee.usf.edu/~christen/tools/moments.c
static void compute_moments(double *data_array, int len,
			    double *mean,
			    double *variance,
			    double *skewness,
			    double *kurtosis) {
  *mean = 0.0;
  for (int i = 0; i < len; i++) {
    *mean += data_array[i] / (double) len;
  }

  double raw_moments[4] = {*mean, 0, 0, 0};
  double central_moments[4] = {0, 0, 0, 0};

  for (int i = 0; i < len; i++) {
    for (int j = 1; j < 4; j++) {
      raw_moments[j] += (pow(data_array[i], j + 1.0) / len);
      central_moments[j] += (pow((data_array[i] - *mean), j + 1.0) / len);
    }
  }

  *variance = central_moments[1];
  *skewness = central_moments[2];
  *kurtosis = central_moments[3];
}

// 2 functions for diamond filter interface
static
int f_init_circles (int num_arg, const char * const *args,
		    int bloblen, const void *blob_data,
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


static
int f_eval_circles (lf_obj_handle_t ohandle, void *filter_args) {
  // circles
  GList *clist;
  circles_state_t *cr = (circles_state_t *) filter_args;
  int num_circles;
  int num_circles_in_result = 0;

  char buf[G_ASCII_DTOSTR_BUF_SIZE];

  // for attributes from diamond
  size_t len;
  const void *data;



  // get the data from the ohandle, convert to IplImage
  int w;
  int h;

  // width
  lf_ref_attr(ohandle, "_rows.int", &len, &data);
  h = *((const int *) data);

  // height
  lf_ref_attr(ohandle, "_cols.int", &len, &data);
  w = *((const int *) data);

  // image data
  lf_ref_attr(ohandle, "_rgb_image.rgbimage", &len, &data);
  lf_omit_attr(ohandle, "_rgb_image.rgbimage");

  // feed it to our processor
  clist = circlesFromImage2(cr, w, h, w * 4, 4, data);

  // add the list of circles to the cache and the object
  // XXX !
  num_circles = g_list_length(clist);
  double *areas = (double *) g_malloc(sizeof(double) * num_circles);
  double *eccentricities = (double *) g_malloc(sizeof(double) * num_circles);
  double total_area = 0.0;
  if (num_circles > 0) {
    GList *l = clist;
    int i = 0;
    circle_type *circle_data =
      (circle_type *) g_malloc(sizeof(circle_type) * num_circles);

    // pack in and count
    while (l != NULL) {
      circle_type *p = circle_data + i;
      circle_type *c = (circle_type *) l->data;
      *p = *c;

      if (c->in_result) {
	total_area += areas[num_circles_in_result] = M_PI * c->a * c->b;
	eccentricities[num_circles_in_result] = compute_eccentricity(c->a, c->b);
	num_circles_in_result++;
      }

      i++;
      l = g_list_next(l);
    }
    lf_write_attr(ohandle, "circle-data", sizeof(circle_type) * num_circles, circle_data);
    g_free(circle_data);
  }

  // compute aggregate stats
  double n = num_circles_in_result;
  g_ascii_dtostr (buf, sizeof (buf), n);
  lf_write_attr(ohandle, "circle-count", strlen(buf) + 1, (unsigned char *) buf);


  double area_fraction = total_area / (w * h);
  g_ascii_dtostr (buf, sizeof (buf), area_fraction);
  lf_write_attr(ohandle, "circle-area-fraction", strlen(buf) + 1, (unsigned char *) buf);

  double area_m1, area_cm2, area_cm3, area_cm4;
  compute_moments(areas, num_circles_in_result, &area_m1, &area_cm2, &area_cm3, &area_cm4);
  g_ascii_dtostr (buf, sizeof (buf), area_m1);
  lf_write_attr(ohandle, "circle-area-m1", strlen(buf) + 1, (unsigned char *) buf);
  g_ascii_dtostr (buf, sizeof (buf), area_cm2);
  lf_write_attr(ohandle, "circle-area-cm2", strlen(buf) + 1, (unsigned char *) buf);
  g_ascii_dtostr (buf, sizeof (buf), area_cm3);
  lf_write_attr(ohandle, "circle-area-cm3", strlen(buf) + 1, (unsigned char *) buf);
  g_ascii_dtostr (buf, sizeof (buf), area_cm4);
  lf_write_attr(ohandle, "circle-area-cm4", strlen(buf) + 1, (unsigned char *) buf);

  double eccentricity_m1, eccentricity_cm2, eccentricity_cm3, eccentricity_cm4;
  compute_moments(eccentricities, num_circles_in_result,
		  &eccentricity_m1, &eccentricity_cm2,
		  &eccentricity_cm3, &eccentricity_cm4);
  g_ascii_dtostr (buf, sizeof (buf), eccentricity_m1);
  lf_write_attr(ohandle, "circle-eccentricity-m1", strlen(buf) + 1, (unsigned char *) buf);
  g_ascii_dtostr (buf, sizeof (buf), eccentricity_cm2);
  lf_write_attr(ohandle, "circle-eccentricity-cm2", strlen(buf) + 1, (unsigned char *) buf);
  g_ascii_dtostr (buf, sizeof (buf), eccentricity_cm3);
  lf_write_attr(ohandle, "circle-eccentricity-cm3", strlen(buf) + 1, (unsigned char *) buf);
  g_ascii_dtostr (buf, sizeof (buf), eccentricity_cm4);
  lf_write_attr(ohandle, "circle-eccentricity-cm4", strlen(buf) + 1, (unsigned char *) buf);

  // free others
  g_list_foreach(clist, free_1_circle, NULL);
  g_list_free(clist);
  g_free(areas);
  g_free(eccentricities);

  // return number of circles
  return num_circles_in_result;
}

int main(void)
{
  lf_main(f_init_circles, f_eval_circles);
  return 0;
}
