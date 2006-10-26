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

#ifndef CIRCLES4_H
#define CIRCLES4_H

#include <glib.h>

extern GList *circles;

typedef struct {
  float x;
  float y;
  float a;
  float b;
  float t;
  gboolean in_result;
} circle_type;

#ifdef __cplusplus
extern "C" {
#endif
  void circlesFromImage(const int width, const int height, const int stride,
			const int bytesPerPixel,
			void *data, double minSharpness);
#ifdef __cplusplus
}
#endif

#endif
