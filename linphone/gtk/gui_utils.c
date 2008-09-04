/*
applet.c - some utils functions that cannot be set in interface.c.

Copyright (C) 2000  Simon MORLAT (simon.morlat@free.fr)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/


#include "linphone.h"


/* creates the applet button*
GtkWidget *create_applet()
{
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *vbox;
  GtkWidget *applet;

  applet = applet_widget_new("linphone_applet");

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  gtk_widget_show(frame);

  vbox = gtk_vbox_new(FALSE, FALSE);
  gtk_container_add(GTK_CONTAINER(frame), vbox);
  gtk_widget_show(vbox);

  button = gtk_button_new();
	gtk_widget_ref(button);
  GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_DEFAULT);
  GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);
	

  	gtk_widget_show(button);
	applet_widget_add (APPLET_WIDGET (applet), frame);
	gtk_object_set_data_full(GTK_OBJECT(applet),"applet_button",button,(GtkDestroyNotify)gtk_widget_unref);
  	gtk_signal_connect(GTK_OBJECT(button), "button_press_event",
                       GTK_SIGNAL_FUNC(on_applet_clicked), NULL);		
  	gtk_signal_connect(GTK_OBJECT(applet), "change_pixel_size",
                       GTK_SIGNAL_FUNC(applet_change_pixel_size), NULL);		 	
	applet_widget_set_tooltip( APPLET_WIDGET (applet),_("linphone"));
	gtk_widget_show(applet);
	return(applet);
};

*/
/* this  just sets level adjustements for startup*/
void set_levels(LinphoneGnomeUI *ui,gint reclev, gint playlev, gint ringlev)
{
	GtkWidget *range;
	GtkWidget *window=ui->main_window.window;
	return;
	if (window)
	{
		range=lookup_widget(window,"rec_vol");
		gtk_adjustment_set_value (gtk_range_get_adjustment(GTK_RANGE(range)),(gfloat)reclev);
		range=lookup_widget(window,"play_vol");
		gtk_adjustment_set_value (gtk_range_get_adjustment(GTK_RANGE(range)),(gfloat)playlev);
		range=lookup_widget(window,"ring_vol");
		gtk_adjustment_set_value (gtk_range_get_adjustment(GTK_RANGE(range)),(gfloat)ringlev);
	}
}


void alt_ressource_display(LinphoneGnomeUI *ui,const gchar *url)
{
	GtkWidget *href;
	GtkWidget *altdisplay;
	GtkLabel *label;
	gchar *pattern;
	
	altdisplay=create_altressource();
	g_object_set_data(G_OBJECT(altdisplay),"ui",(gpointer)ui);
	href=lookup_widget(altdisplay,"alt_href");
	label=GTK_LABEL(GTK_BIN(href)->child);
	gtk_label_set_text(label,url);
	/* pattern used to set underline for string */
	pattern = g_strnfill(strlen(url), '_');
	gtk_label_set_pattern(label,pattern);
	g_free(pattern);
	gtk_widget_show(altdisplay);
}




