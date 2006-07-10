#ifndef FATFIND_H
#define FATFIND_H

#include <glade/glade.h>
#include <gtk/gtk.h>

extern GladeXML *g_xml;
extern GdkPixbuf *c_pix;
extern guint32 reference_circle;
extern GList *circles;

typedef struct {
  float x;
  float y;
  float r;
  float fuzz;
} circle_type;


#endif
