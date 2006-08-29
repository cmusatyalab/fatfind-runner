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
