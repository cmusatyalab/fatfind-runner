/*
 *  FatFind
 *  A Diamond application for adipocyte image exploration
 *  Version 1
 *
 *  Copyright (c) 2006, 2009-2010 Carnegie Mellon University
 *  All Rights Reserved.
 *
 *  This software is distributed under the terms of the Eclipse Public
 *  License, Version 1.0 which can be found in the file named LICENSE.
 *  ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS SOFTWARE CONSTITUTES
 *  RECIPIENT'S ACCEPTANCE OF THIS AGREEMENT
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <glib.h>
#include <inttypes.h>
#include "circles4.h"

static int
get_pos_int_or_die(void) {
  char *line = NULL;
  size_t len = 0;
  ssize_t read = getline(&line, &len, stdin);
  if (read == -1) {
    exit(EXIT_FAILURE);
  }

  int result = atoi(line);
  free(line);

  if (result <= 0) {
    exit(EXIT_FAILURE);
  }

  return result;
}

int
main (int argc, char *argv[])
{
  if (argc != 2) {
    printf("Usage: %s min_sharpness\n\nThen send width, newline, height, newline, and RGB data on stdin.\n",
	   argv[0]);
    return 0;
  }

  double minSharpness = atof(argv[1]);

  // read width
  int w = get_pos_int_or_die();

  // read height
  int h = get_pos_int_or_die();

  // read data
  int len = w * h * 3;
  uint8_t *data = (uint8_t *) malloc(len);
  for (int i = 0; i < len; i++) {
    data[i] = getchar();
  }

  // compute
  GList *circles = circlesFromImage(w, h, w * 3, 3, data, minSharpness);

  // print
  for (GList *l = circles; l != NULL; l = l->next) {
    circle_type *c = (circle_type *) l->data;
    printf("%g %g %g %g %g %s\n", c->x, c->y, c->a, c->b, c->t,
	   c->in_result ? "true" : "false");
    g_slice_free(circle_type, c);
  }
  g_list_free(circles);

  return 0;
}
