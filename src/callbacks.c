#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade.h>

#include "fatfind.h"
#include "callbacks.h"


void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_startButton_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{

  GtkWidget * progress = glade_xml_get_widget(g_xml, "progressBar");
  GtkWidget * thumb = glade_xml_get_widget(g_xml, "thumb1");

  gdouble fraction = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(progress));
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), fraction+0.1);

  gtk_image_set_from_file(GTK_IMAGE(thumb), "/diamonddata/thumb.bmp");

}


void
on_sortButton_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_radiotoolbutton1_toggled            (GtkToggleToolButton *toggletoolbutton,
                                        gpointer         user_data)
{
  // Display large version of image
  GtkWidget * image = glade_xml_get_widget(g_xml, "largeImage");
  gtk_image_set_from_file(GTK_IMAGE(image), "/diamonddata/original.bmp");

}


void
on_radiotoolbutton2_toggled            (GtkToggleToolButton *toggletoolbutton,
                                        gpointer         user_data)
{
  // Display large version of image
  GtkWidget * image = glade_xml_get_widget(g_xml, "largeImage");
  gtk_image_set_from_file(GTK_IMAGE(image), "/diamonddata/edges.bmp");

}


void
on_radiotoolbutton3_toggled            (GtkToggleToolButton *toggletoolbutton,
                                        gpointer         user_data)
{
  // Display large version of image
  GtkWidget * image = glade_xml_get_widget(g_xml, "largeImage");
  gtk_image_set_from_file(GTK_IMAGE(image), "/diamonddata/hough.bmp");

}


void
on_radiotoolbutton4_toggled            (GtkToggleToolButton *toggletoolbutton,
                                        gpointer         user_data)
{
  // Display large version of image
  GtkWidget * image = glade_xml_get_widget(g_xml, "largeImage");
  gtk_image_set_from_file(GTK_IMAGE(image), "/diamonddata/gaussian.bmp");

}


void
on_radiotoolbutton5_toggled            (GtkToggleToolButton *toggletoolbutton,
                                        gpointer         user_data)
{
  // Display large version of image
  GtkWidget * image = glade_xml_get_widget(g_xml, "largeImage");
  gtk_image_set_from_file(GTK_IMAGE(image), "/diamonddata/circles.bmp");

}


void
on_thumbButton1_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{

  // Display large version of image
  GtkWidget * image = glade_xml_get_widget(g_xml, "largeImage");
  gtk_image_set_from_file(GTK_IMAGE(image), "/diamonddata/original.bmp");

  // Update the Image Info fields
  GtkWidget * info = glade_xml_get_widget(g_xml, "imageInfo");
  gtk_label_set_text(GTK_LABEL(info), "/diamonddata/frame0001.png");

}


void
on_thumbButton2_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_thumbButton3_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{

}

