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

#ifndef FATFIND_H
#define FATFIND_H

#include <glade/glade.h>
#include <gtk/gtk.h>

#include "circles4.h"

extern GladeXML *g_xml;
extern GdkPixbuf *c_pix;


extern GtkListStore *saved_search_store;
extern guint32 reference_circle;
extern circle_type *reference_circle_object;

extern GtkListStore *found_items;


#endif
