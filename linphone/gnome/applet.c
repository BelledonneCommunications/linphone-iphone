/***************************************************************************
                          applet.c  -  Applet code for linphone's gnome 
										  interface
                             -------------------
    begin                : Sat Dec 14 2002
    copyright            : (C) 2001 by Simon Morlat
    email                : simon.morlat@linphone.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "linphone.h"
#include <panel-applet.h>

#define get_uiobj()	(uiobj)

LinphoneCore core;
LinphoneGnomeUI ui;
static int show=0;
static gulong signal_ref;
static GtkWidget *applet_button=NULL;
static GdkPixbuf *original_icon=NULL;
static GtkWidget *icon=NULL;

void draw_icon(GtkWidget *button, int size)
{
	GdkPixbuf *resized;
	if (original_icon==NULL){
		original_icon=gdk_pixbuf_new_from_file(PACKAGE_DATA_DIR "/pixmaps/linphone/linphone2.xpm",
												NULL);
		g_return_if_fail( original_icon!=NULL);
	}
	if (icon!=NULL){
		gtk_container_remove(GTK_CONTAINER(button),icon);
		gtk_widget_destroy(icon);
	}
	resized=gdk_pixbuf_scale_simple(original_icon,size,size,GDK_INTERP_BILINEAR);
	g_return_if_fail(resized!=NULL);
	icon=gtk_image_new_from_pixbuf(resized);
	g_return_if_fail(icon!=NULL);
	gdk_pixbuf_unref(resized);
	gtk_container_add(GTK_CONTAINER(button),icon);
	gtk_widget_show(icon);
}

void linphone_applet_about_cb(gpointer p)
{
	GtkWidget *about2;
	about2 = create_about2 ();
	gtk_widget_show (about2);
}



static void applet_change_pixel_size(GtkWidget *applet, int size)
{
	g_return_if_fail(applet_button!=NULL);
	draw_icon(applet_button,size);
}

static void applet_destroy_cb(GtkWidget *widget, gpointer data)
{
	if (get_uiobj()->main_window.window!=NULL){
		gtk_widget_destroy(get_uiobj()->main_window.window);
	}
	linphone_gnome_uninit(get_uiobj());
}

static gboolean
gui_destroy_cb (GtkWidget *widget, gpointer data)
{
	linphone_gnome_ui_uninit(get_uiobj());
	show=0;
	return FALSE;
}

static gboolean button_press_cb(GtkWidget *applet, GdkEventButton* event, gpointer data)
{
	if (event->button!=1) return FALSE;
	if (show){
		g_signal_handlers_disconnect_by_func(G_OBJECT(get_uiobj()->main_window.window),
						G_CALLBACK(gui_destroy_cb),NULL);
		linphone_gnome_ui_hide(get_uiobj());
		
		show=0;
	}else {
		linphone_gnome_ui_show(get_uiobj());
		signal_ref=g_signal_connect(G_OBJECT(get_uiobj()->main_window.window),
						"destroy",
						G_CALLBACK(gui_destroy_cb),NULL);
		show=1;
	}
	return FALSE;
}

const BonoboUIVerb linphone_applet_menu_verbs [] = {
	BONOBO_UI_UNSAFE_VERB ("About",    linphone_applet_about_cb),
	BONOBO_UI_VERB_END
};

static gboolean
linphone_applet_fill (PanelApplet *applet)
{
	gint size=panel_applet_get_size(applet);
	
	applet_button=gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(applet),applet_button);
	gtk_widget_show(applet_button);

	draw_icon(applet_button,size);
	
	g_signal_connect(G_OBJECT(applet),"button-press-event",G_CALLBACK(button_press_cb),NULL);
	

	g_signal_connect(G_OBJECT(applet),"change_size",
				G_CALLBACK(applet_change_pixel_size),
				NULL);

		
	g_signal_connect (G_OBJECT (applet), "destroy",
			  G_CALLBACK (applet_destroy_cb), NULL);
				
	//sizehint = panel_applet_get_size (PANEL_APPLET (applet));
	panel_applet_setup_menu_from_file (applet,
					   NULL,
					   "GNOME_LinphoneApplet.xml",
					   NULL,
					   linphone_applet_menu_verbs,
					   NULL);
	
	/* tracing for osip */
	TRACE_INITIALIZE(5,stdout);
	
	linphone_gnome_init(&ui,&core);
	gtk_widget_show_all (GTK_WIDGET (applet));
	  
	return TRUE;
}


static gboolean
linphone_applet_factory (PanelApplet *applet,
			    const gchar *iid,
			    gpointer     data)
{
	static int instances=0;
	GtkWidget *dialog;
	if (!strcmp (iid, "OAFIID:GNOME_LinphoneApplet")){
		if (instances>0){
			dialog = gtk_message_dialog_new (GTK_WINDOW(applet),
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  GTK_MESSAGE_WARNING,
                                  GTK_BUTTONS_CLOSE,
                                  (const gchar*) _("Cannot run multiples instances of the linphone applet."));
			/* Destroy the dialog when the user responds to it (e.g. clicks a button) */
			g_signal_connect_swapped (G_OBJECT (dialog), "response",
                           G_CALLBACK (gtk_widget_destroy),
                           G_OBJECT (dialog));
			gtk_widget_show(GTK_WIDGET(dialog));
			return FALSE;
		}
		return linphone_applet_fill (applet); 
    }
	return FALSE;
}

#define GNOMELOCALEDIR PACKAGE_LOCALE_DIR

PANEL_APPLET_BONOBO_FACTORY ("OAFIID:GNOME_LinphoneApplet_Factory",
			     PANEL_TYPE_APPLET,
			     "linphone_applet",
			     "0",
			     linphone_applet_factory,
			     NULL)
