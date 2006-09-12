/******************************************************************************
 * Copyright (c) 2006 Carnegie Mellon University.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *    Adam Goode - initial implementation
 *****************************************************************************/

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
			void *data);
#ifdef __cplusplus
}
#endif

#endif
