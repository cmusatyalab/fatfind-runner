#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"


void
on_fatfind_destroy                     (GtkObject       *object,
                                        gpointer         user_data)
{
  gtk_main_quit();
}


void
on_open1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_save_as1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_quit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  gtk_main_quit();
}

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_startButton_clicked                 (GtkButton       *button,
                                        gpointer         user_data)
{

  GtkWidget * progress = lookup_widget(GTK_WIDGET(button), "progressBar");
  GtkWidget * thumb = lookup_widget(GTK_WIDGET(button), "thumb1");

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
  GtkWidget * image = lookup_widget(GTK_WIDGET(toggletoolbutton), "largeImage");
  gtk_image_set_from_file(GTK_IMAGE(image), "/diamonddata/original.bmp");

}


void
on_radiotoolbutton2_toggled            (GtkToggleToolButton *toggletoolbutton,
                                        gpointer         user_data)
{
  // Display large version of image
  GtkWidget * image = lookup_widget(GTK_WIDGET(toggletoolbutton), "largeImage");
  gtk_image_set_from_file(GTK_IMAGE(image), "/diamonddata/edges.bmp");

}


void
on_radiotoolbutton3_toggled            (GtkToggleToolButton *toggletoolbutton,
                                        gpointer         user_data)
{
  // Display large version of image
  GtkWidget * image = lookup_widget(GTK_WIDGET(toggletoolbutton), "largeImage");
  gtk_image_set_from_file(GTK_IMAGE(image), "/diamonddata/hough.bmp");

}


void
on_radiotoolbutton4_toggled            (GtkToggleToolButton *toggletoolbutton,
                                        gpointer         user_data)
{
  // Display large version of image
  GtkWidget * image = lookup_widget(GTK_WIDGET(toggletoolbutton), "largeImage");
  gtk_image_set_from_file(GTK_IMAGE(image), "/diamonddata/gaussian.bmp");

}


void
on_radiotoolbutton5_toggled            (GtkToggleToolButton *toggletoolbutton,
                                        gpointer         user_data)
{
  // Display large version of image
  GtkWidget * image = lookup_widget(GTK_WIDGET(toggletoolbutton), "largeImage");
  gtk_image_set_from_file(GTK_IMAGE(image), "/diamonddata/circles.bmp");

}


void
on_thumbButton1_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{

  // Display large version of image
  GtkWidget * image = lookup_widget(GTK_WIDGET(button), "largeImage");
  gtk_image_set_from_file(GTK_IMAGE(image), "/diamonddata/original.bmp");

  // Update the Image Info fields
  GtkWidget * info = lookup_widget(GTK_WIDGET(button), "imageInfo");
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

