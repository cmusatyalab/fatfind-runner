/*
 * This file is part of FatFind,
 * a Diamond-based system for adipocyte exploration.
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
