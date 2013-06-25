/***************************************************************************
 *            gtk/conference.c
 *
 *  Mon Sep 12, 2011
 *  Copyright  2011  Belledonne Communications
 *  Author: Simon Morlat
 *  Email simon dot morlat at linphone dot org
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "linphone.h"

#define PADDING_PIXELS 4

/*
 * conferencee_box = a vbox where participants are added or removed
 * conf_frame = the conference tab
 */

static GtkWidget *create_conference_label(void){
	GtkWidget *box=gtk_hbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(box),gtk_image_new_from_stock(GTK_STOCK_ADD,GTK_ICON_SIZE_MENU),FALSE,FALSE,0);
	gtk_box_pack_end(GTK_BOX(box),gtk_label_new(_("Conference")),TRUE,FALSE,0);
	gtk_widget_show_all(box);
	return box;
}

static void init_local_participant(GtkWidget *participant){
	GtkWidget *sound_meter;
	GtkWidget *button=linphone_gtk_get_widget(participant,"conference_control");
	gtk_label_set_markup(GTK_LABEL(linphone_gtk_get_widget(participant,"callee_name_label")),_("Me"));
	sound_meter=linphone_gtk_get_widget(participant,"sound_indicator");
	linphone_gtk_enable_mute_button(GTK_BUTTON(button),TRUE);
	g_signal_connect(G_OBJECT(button),"clicked",(GCallback)linphone_gtk_mute_clicked,NULL);
	gtk_widget_show(button);
	linphone_gtk_init_audio_meter(sound_meter, (get_volume_t) linphone_core_get_conference_local_input_volume, linphone_gtk_get_core());
}

static GtkWidget *get_conferencee_box(GtkWidget *mw){
	GtkWidget *box=(GtkWidget*)g_object_get_data(G_OBJECT(mw),"conferencee_box");
	return box;
}

static GtkWidget *find_conferencee_from_call(LinphoneCall *call){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *conferencee_box=get_conferencee_box(mw);
	GList *elem;
	GtkWidget *ret=NULL;
	
	if (conferencee_box==NULL) return NULL;
	
	if (call!=NULL){
		GList *l=gtk_container_get_children(GTK_CONTAINER(conferencee_box));
		for(elem=l;elem!=NULL;elem=elem->next){
			GtkWidget *frame=(GtkWidget*)elem->data;
			if (call==g_object_get_data(G_OBJECT(frame),"call")){
				ret=frame;
				break;
			}
		}
		g_list_free(l);
	}
	//g_message("find_conferencee_from_call(): found widget %p for call %p",ret,call);
	return ret;
}

static GtkWidget * create_conference_panel(void){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *conf_frame=linphone_gtk_create_widget("main","conf_frame");
	GtkWidget *conf_box=linphone_gtk_get_widget(conf_frame,"conf_box");
	GtkWidget *button_conf=linphone_gtk_get_widget(conf_frame,"terminate_conf");
	GtkWidget *image=create_pixmap("stopcall-small.png");
	GtkWidget *box;
	GtkWidget *viewswitch=linphone_gtk_get_widget(mw,"viewswitch");
	
	gtk_button_set_image(GTK_BUTTON(button_conf),image);
	g_signal_connect_swapped(G_OBJECT(button_conf),"clicked",(GCallback)linphone_gtk_terminate_call,NULL);
	g_object_set_data(G_OBJECT(mw),"conf_frame",(gpointer)conf_frame);
	
	box=gtk_vbox_new(FALSE,0);
	GtkWidget *participant=linphone_gtk_create_widget("main","callee_frame");
	gtk_widget_show(participant);
	gtk_box_set_homogeneous(GTK_BOX(box),TRUE);
	init_local_participant(participant);
	gtk_box_pack_start(GTK_BOX(box),participant,FALSE,FALSE,PADDING_PIXELS);
	gtk_widget_show(box);
	g_object_set_data(G_OBJECT(mw),"conferencee_box",box);
	gtk_box_pack_start(GTK_BOX(conf_box),box,FALSE,FALSE,PADDING_PIXELS);
	
	gtk_notebook_append_page(GTK_NOTEBOOK(viewswitch),conf_frame,
				create_conference_label());
	return conf_frame;
}

void linphone_gtk_set_in_conference(LinphoneCall *call){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *conf_frame=(GtkWidget *)g_object_get_data(G_OBJECT(mw),"conf_frame");
	GtkWidget *viewswitch=linphone_gtk_get_widget(mw,"viewswitch");
	
	if(conf_frame==NULL){
		conf_frame=create_conference_panel();
	}
	GtkWidget *participant=find_conferencee_from_call(call);
	
	if (participant==NULL){
		/*create and add it */
		GtkWidget *conferencee_box=get_conferencee_box(mw);
		GtkWidget *sound_meter;
		const LinphoneAddress *addr=linphone_call_get_remote_address(call);
		gchar *markup;
		
		participant=linphone_gtk_create_widget("main","callee_frame");
		gtk_widget_show(participant);
		if (linphone_address_get_display_name(addr)!=NULL){
			markup=g_strdup_printf("<b>%s</b>",linphone_address_get_display_name(addr));
		}else{
			char *tmp=linphone_address_as_string_uri_only(addr);
			markup=g_strdup_printf("%s",tmp);
			ms_free(tmp);
		}
		gtk_label_set_markup(GTK_LABEL(linphone_gtk_get_widget(participant,"callee_name_label")),markup);
		g_free(markup);
		sound_meter=linphone_gtk_get_widget(participant,"sound_indicator");
		linphone_gtk_init_audio_meter(sound_meter, (get_volume_t) linphone_call_get_play_volume, call);
		gtk_box_pack_start(GTK_BOX(conferencee_box),participant,FALSE,FALSE,PADDING_PIXELS);
		g_object_set_data_full(G_OBJECT(participant),"call",linphone_call_ref(call),(GDestroyNotify)linphone_call_unref);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(viewswitch),
			gtk_notebook_page_num(GTK_NOTEBOOK(viewswitch),conf_frame));
	}
}

void linphone_gtk_terminate_conference_participant(LinphoneCall *call){
	GtkWidget *frame=find_conferencee_from_call(call);
	if (frame){
		gtk_widget_set_sensitive(frame,FALSE);
	}
}

void linphone_gtk_unset_from_conference(LinphoneCall *call){
	GtkWidget *frame=find_conferencee_from_call(call);
	
	if (frame){
		GtkWidget *mw=linphone_gtk_get_main_window();
		GtkWidget *conf_frame=(GtkWidget *)g_object_get_data(G_OBJECT(mw),"conf_frame");
		GtkWidget *conferencee_box=g_object_get_data(G_OBJECT(mw),"conferencee_box");
		GList *children;
		
		g_message("Removing a participant from conference");
		gtk_widget_destroy(frame);
		children=gtk_container_get_children(GTK_CONTAINER(conferencee_box));
		if (g_list_length(children)==1){ /* only local participant */
			/*the conference is terminated */
			g_message("The conference is terminated");
			g_object_set_data(G_OBJECT(mw),"conferencee_box",NULL);
			gtk_widget_destroy(conf_frame);
			g_object_set_data(G_OBJECT(mw),"conf_frame",NULL);
		}
		g_list_free(children);
	}
}


