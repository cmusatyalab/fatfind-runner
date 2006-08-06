#ifndef CIRCLES2_H
#define CIRCLES2_H

#include <glib.h>
#include <cxcore.h>

extern GList *circles;

typedef struct {
  float x;
  float y;
  float r;
} circle_type;


void circlesFromImage(IplImage *initialImage);

#endif
