/*
linphone, gtk-glade interface.
Copyright (C) 2009  Simon MORLAT (simon.morlat@linphone.org)

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
/*
*  C Implementation: incall_frame
*
* Description: 
*
*
* Author: Simon Morlat <simon.morlat@linphone.org>, (C) 2009
*
*
*/

#include "linphone.h"


gboolean linphone_gtk_use_in_call_view(){
	static int val=-1;
	if (val==-1) val=linphone_gtk_get_ui_config_int("use_incall_view",1);
	return val;
}

LinphoneCall *linphone_gtk_get_currently_displayed_call(){
	LinphoneCore *lc=linphone_gtk_get_core();
	GtkWidget *main_window=linphone_gtk_get_main_window ();
	GtkNotebook *notebook=(GtkNotebook *)linphone_gtk_get_widget(main_window,"viewswitch");
	const MSList *calls=linphone_core_get_calls(lc);
	if (!linphone_gtk_use_in_call_view() || ms_list_size(calls)==1){
		if (calls) return (LinphoneCall*)calls->data;
	}else{
		int idx=gtk_notebook_get_current_page (notebook);
		GtkWidget *page=gtk_notebook_get_nth_page(notebook,idx);
		if (page!=NULL){
			LinphoneCall *call=(LinphoneCall*)g_object_get_data(G_OBJECT(page),"call");
			return call;
		}
	}
	return NULL;
}

static GtkWidget *make_tab_header(int number){
	GtkWidget *w=gtk_hbox_new (FALSE,0);
	GtkWidget *i=create_pixmap ("status-green.png");
	GtkWidget *l;
	gchar *text=g_strdup_printf("Call #%i",number);
	l=gtk_label_new (text);
	gtk_box_pack_start (GTK_BOX(w),i,FALSE,FALSE,0);
	gtk_box_pack_end(GTK_BOX(w),l,TRUE,TRUE,0);
	gtk_widget_show_all(w);
	return w;
}

static void linphone_gtk_transfer_call(LinphoneCall *dest_call){
	LinphoneCall *call=linphone_gtk_get_currently_displayed_call();
	linphone_core_transfer_call_to_another (linphone_gtk_get_core(),call,dest_call);
}

static void transfer_button_clicked(GtkWidget *button, gpointer call_ref){
	GtkWidget *menu_item;
	GtkWidget *menu=gtk_menu_new();
	LinphoneCall *call=(LinphoneCall*)call_ref;
	LinphoneCore *lc=linphone_gtk_get_core();
	const MSList *elem=linphone_core_get_calls(lc);
	
	for(;elem!=NULL;elem=elem->next){
		LinphoneCall *other_call=(LinphoneCall*)elem->data;
		GtkWidget *call_view=(GtkWidget*)linphone_call_get_user_pointer(other_call);
		if (other_call!=call){
			int call_index=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(call_view),"call_index"));
			char *remote_uri=linphone_call_get_remote_address_as_string (other_call);
			char *text=g_strdup_printf("Transfer to call #%i with %s",call_index,remote_uri);
			menu_item=gtk_image_menu_item_new_with_label(text);
			ms_free(remote_uri);
			g_free(text);
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),create_pixmap("status-green.png"));
			gtk_widget_show(menu_item);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
			g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_transfer_call,other_call);
		}
	}
	gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,0,
		gtk_get_current_event_time());
	gtk_widget_show(menu);
}

void linphone_gtk_enable_transfer_button(LinphoneCore *lc, gboolean value){
	const MSList *elem=linphone_core_get_calls(lc);
	for(;elem!=NULL;elem=elem->next){
		LinphoneCall *call=(LinphoneCall*)elem->data;
		GtkWidget *call_view=(GtkWidget*)linphone_call_get_user_pointer(call);
		GtkWidget *box=linphone_gtk_get_widget (call_view,"mute_pause_buttons");
		GtkWidget *button=(GtkWidget*)g_object_get_data(G_OBJECT(box),"transfer");
		if (button && value==FALSE){
			gtk_widget_destroy(button);
			button=NULL;
		}else if (!button && value==TRUE){
			button=gtk_button_new_with_label (_("Transfer"));
			gtk_button_set_image(GTK_BUTTON(button),gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD,GTK_ICON_SIZE_BUTTON));
			g_signal_connect(G_OBJECT(button),"clicked",(GCallback)transfer_button_clicked,call);
			gtk_widget_show_all(button);
			gtk_container_add(GTK_CONTAINER(box),button);
		}
		g_object_set_data(G_OBJECT(box),"transfer",button);
	}
}

void linphone_gtk_create_in_call_view(LinphoneCall *call){
	GtkWidget *call_view=linphone_gtk_create_widget("main","in_call_frame");
	GtkWidget *main_window=linphone_gtk_get_main_window ();
	GtkNotebook *notebook=(GtkNotebook *)linphone_gtk_get_widget(main_window,"viewswitch");
	static int call_index=1;
	int idx;

	if (ms_list_size(linphone_core_get_calls(linphone_gtk_get_core()))==1){
		/*this is the only call at this time */
		call_index=1;
	}
	g_object_set_data(G_OBJECT(call_view),"call",call);
	g_object_set_data(G_OBJECT(call_view),"call_index",GINT_TO_POINTER(call_index));

	linphone_call_set_user_pointer (call,call_view);
	linphone_call_ref(call);
	gtk_notebook_append_page (notebook,call_view,make_tab_header(call_index));
	gtk_widget_show(call_view);
	idx = gtk_notebook_page_num(notebook, call_view);
	gtk_notebook_set_current_page(notebook, idx);
	call_index++;
	linphone_gtk_enable_hold_button (call,FALSE,TRUE);
	linphone_gtk_enable_mute_button(
					GTK_BUTTON(linphone_gtk_get_widget(call_view,"incall_mute")),FALSE);
}

void linphone_gtk_remove_in_call_view(LinphoneCall *call){
	GtkWidget *w=(GtkWidget*)linphone_call_get_user_pointer (call);
	GtkWidget *main_window=linphone_gtk_get_main_window ();
	GtkWidget *nb=linphone_gtk_get_widget(main_window,"viewswitch");
	int idx;
	g_return_if_fail(w!=NULL);
	idx=gtk_notebook_page_num(GTK_NOTEBOOK(nb),w);
	gtk_notebook_remove_page (GTK_NOTEBOOK(nb),idx);
	gtk_widget_destroy(w);
	linphone_call_set_user_pointer (call,NULL);
	linphone_call_unref(call);
	gtk_notebook_set_current_page(GTK_NOTEBOOK(nb), 0);
}

static void display_peer_name_in_label(GtkWidget *label, const LinphoneAddress *from){
	const char *displayname=NULL;
	const char *id;
	char *uri_label;
	displayname=linphone_address_get_display_name(from);
	id=linphone_address_as_string_uri_only(from);
	
	if (displayname!=NULL){
		uri_label=g_markup_printf_escaped("<span size=\"large\">%s</span>\n<i>%s</i>", 
			displayname,id);
	}else
		uri_label=g_markup_printf_escaped("<span size=\"large\"><i>%s</i></span>\n",id);
	gtk_label_set_markup(GTK_LABEL(label),uri_label);
	g_free(uri_label);
}

void linphone_gtk_in_call_view_set_calling(LinphoneCall *call){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *status=linphone_gtk_get_widget(callview,"in_call_status");
	GtkWidget *callee=linphone_gtk_get_widget(callview,"in_call_uri");
	GtkWidget *duration=linphone_gtk_get_widget(callview,"in_call_duration");
	GtkWidget *animation=linphone_gtk_get_widget(callview,"in_call_animation");
	GdkPixbufAnimation *pbuf=create_pixbuf_animation("calling_anim.gif");
	
	gtk_label_set_markup(GTK_LABEL(status),_("<b>Calling...</b>"));
	display_peer_name_in_label(callee,linphone_call_get_remote_address (call));
	
	gtk_label_set_text(GTK_LABEL(duration),_("00::00::00"));
	if (pbuf!=NULL){
		gtk_image_set_from_animation(GTK_IMAGE(animation),pbuf);
		g_object_unref(G_OBJECT(pbuf));
	}else gtk_image_set_from_stock(GTK_IMAGE(animation),GTK_STOCK_FIND,GTK_ICON_SIZE_DIALOG);
}

void linphone_gtk_in_call_view_set_incoming(LinphoneCall *call, bool_t with_pause){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *status=linphone_gtk_get_widget(callview,"in_call_status");
	GtkWidget *callee=linphone_gtk_get_widget(callview,"in_call_uri");
	GtkWidget *animation=linphone_gtk_get_widget(callview,"in_call_animation");
	GdkPixbufAnimation *pbuf=create_pixbuf_animation("calling_anim.gif");
	GtkWidget *answer_button;
	GtkWidget *image;

	gtk_label_set_markup(GTK_LABEL(status),_("<b>Incoming call</b>"));
	gtk_widget_show_all(linphone_gtk_get_widget(callview,"answer_decline_panel"));
	gtk_widget_hide(linphone_gtk_get_widget(callview,"duration_frame"));
	gtk_widget_hide(linphone_gtk_get_widget(callview,"mute_pause_buttons"));
	display_peer_name_in_label(callee,linphone_call_get_remote_address (call));

	answer_button=linphone_gtk_get_widget(callview,"accept_call");
	image=create_pixmap (linphone_gtk_get_ui_config("start_call_icon","startcall-green.png"));
	if (with_pause){
		gtk_button_set_label(GTK_BUTTON(answer_button),
		                     _("Pause all calls\nand answer"));
	}else gtk_button_set_label(GTK_BUTTON(answer_button),_("Answer"));
	gtk_button_set_image(GTK_BUTTON(answer_button),image);
	gtk_widget_show(image);
	
	image=create_pixmap (linphone_gtk_get_ui_config("stop_call_icon","stopcall-red.png"));
	gtk_button_set_image(GTK_BUTTON(linphone_gtk_get_widget(callview,"decline_call")),image);
	gtk_widget_show(image);
	
	if (pbuf!=NULL){
		gtk_image_set_from_animation(GTK_IMAGE(animation),pbuf);
		g_object_unref(G_OBJECT(pbuf));
	}else gtk_image_set_from_stock(GTK_IMAGE(animation),GTK_STOCK_EXECUTE,GTK_ICON_SIZE_DIALOG);
}

void linphone_gtk_in_call_view_set_in_call(LinphoneCall *call){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *status=linphone_gtk_get_widget(callview,"in_call_status");
	GtkWidget *callee=linphone_gtk_get_widget(callview,"in_call_uri");
	GtkWidget *duration=linphone_gtk_get_widget(callview,"in_call_duration");
	GtkWidget *animation=linphone_gtk_get_widget(callview,"in_call_animation");
	GdkPixbufAnimation *pbuf=create_pixbuf_animation("incall_anim.gif");
	
	display_peer_name_in_label(callee,linphone_call_get_remote_address (call));

	gtk_widget_show(linphone_gtk_get_widget(callview,"duration_frame"));
	gtk_widget_show(linphone_gtk_get_widget(callview,"mute_pause_buttons"));
	gtk_widget_hide(linphone_gtk_get_widget(callview,"answer_decline_panel"));
	gtk_label_set_markup(GTK_LABEL(status),_("<b>In call</b>"));

	gtk_label_set_text(GTK_LABEL(duration),_("00::00::00"));
	if (pbuf!=NULL){
		gtk_image_set_from_animation(GTK_IMAGE(animation),pbuf);
		g_object_unref(G_OBJECT(pbuf));
	}else gtk_image_set_from_stock(GTK_IMAGE(animation),GTK_STOCK_EXECUTE,GTK_ICON_SIZE_DIALOG);
	linphone_gtk_enable_mute_button(
					GTK_BUTTON(linphone_gtk_get_widget(callview,"incall_mute")),TRUE);
}

void linphone_gtk_in_call_view_set_paused(LinphoneCall *call){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *status=linphone_gtk_get_widget(callview,"in_call_status");
	GtkWidget *animation=linphone_gtk_get_widget(callview,"in_call_animation");
	gtk_widget_hide(linphone_gtk_get_widget(callview,"answer_decline_panel"));
	gtk_label_set_markup(GTK_LABEL(status),_("<b>Paused call</b>"));
	gtk_image_set_from_stock(GTK_IMAGE(animation),GTK_STOCK_MEDIA_PAUSE,GTK_ICON_SIZE_DIALOG);
}

void linphone_gtk_in_call_view_update_duration(LinphoneCall *call){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *duration_label=linphone_gtk_get_widget(callview,"in_call_duration");
	int duration=linphone_call_get_duration(call);
	char tmp[256]={0};
	int seconds=duration%60;
	int minutes=(duration/60)%60;
	int hours=duration/3600;
	snprintf(tmp,sizeof(tmp)-1,_("%02i::%02i::%02i"),hours,minutes,seconds);
	gtk_label_set_text(GTK_LABEL(duration_label),tmp);
}

static gboolean in_call_view_terminated(LinphoneCall *call){
	linphone_gtk_remove_in_call_view(call);
	return FALSE;
}

void linphone_gtk_in_call_view_terminate(LinphoneCall *call, const char *error_msg){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *status=linphone_gtk_get_widget(callview,"in_call_status");
	GtkWidget *animation=linphone_gtk_get_widget(callview,"in_call_animation");
	GdkPixbuf *pbuf=create_pixbuf(linphone_gtk_get_ui_config("stop_call_icon","stopcall-red.png"));

	if (error_msg==NULL)
		gtk_label_set_markup(GTK_LABEL(status),_("<b>Call ended.</b>"));
	else{
		char *msg=g_markup_printf_escaped("<span color=\"red\"><b>%s</b></span>",error_msg);
		gtk_label_set_markup(GTK_LABEL(status),msg);
		g_free(msg);
	}
	if (pbuf!=NULL){
		gtk_image_set_from_pixbuf(GTK_IMAGE(animation),pbuf);
		g_object_unref(G_OBJECT(pbuf));
	}
	gtk_widget_hide(linphone_gtk_get_widget(callview,"answer_decline_panel"));
	linphone_gtk_enable_mute_button(
		GTK_BUTTON(linphone_gtk_get_widget(callview,"incall_mute")),FALSE);
	linphone_gtk_enable_hold_button(call,FALSE,TRUE);
	g_timeout_add_seconds(2,(GSourceFunc)in_call_view_terminated,call);
}

void linphone_gtk_draw_mute_button(GtkButton *button, gboolean active){
	g_object_set_data(G_OBJECT(button),"active",GINT_TO_POINTER(active));
	if (active){
		GtkWidget *image=create_pixmap("mic_muted.png");
		gtk_button_set_label(GTK_BUTTON(button),_("Unmute"));
		if (image!=NULL) {
			gtk_button_set_image(GTK_BUTTON(button),image);
			gtk_widget_show(image);
		}
	}else{
		GtkWidget *image=create_pixmap("mic_active.png");
		gtk_button_set_label(GTK_BUTTON(button),_("Mute"));
		if (image!=NULL) {
			gtk_button_set_image(GTK_BUTTON(button),image);
			gtk_widget_show(image);
		}
	}
}

void linphone_gtk_mute_clicked(GtkButton *button){
	int active=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button),"active"));
	linphone_core_mute_mic(linphone_gtk_get_core(),!active);
	linphone_gtk_draw_mute_button(button,!active);
}

void linphone_gtk_enable_mute_button(GtkButton *button, gboolean sensitive)
{
	gtk_widget_set_sensitive(GTK_WIDGET(button),sensitive);
	linphone_gtk_draw_mute_button(button,FALSE);
}

void linphone_gtk_draw_hold_button(GtkButton *button, gboolean active){
	g_object_set_data(G_OBJECT(button),"active",GINT_TO_POINTER(active));
	if (active){
		GtkWidget *image=create_pixmap("hold_off.png");
		gtk_button_set_label(GTK_BUTTON(button),_("Resume"));
		if (image!=NULL) {
			gtk_button_set_image(GTK_BUTTON(button),image);
			gtk_widget_show(image);
		}
	}else{
		GtkWidget *image=create_pixmap("hold_on.png");
		gtk_button_set_label(GTK_BUTTON(button),_("Pause"));
		if (image!=NULL) {
			gtk_button_set_image(GTK_BUTTON(button),image);
			gtk_widget_show(image);
		}
	}
}

void linphone_gtk_hold_clicked(GtkButton *button){
	int active=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button),"active"));
	LinphoneCall *call=linphone_gtk_get_currently_displayed_call ();
	if(!active)
	{
		linphone_core_pause_call(linphone_gtk_get_core(),call);
	}
	else
	{
		linphone_core_resume_call(linphone_gtk_get_core(),call);
	}
}

void linphone_gtk_enable_hold_button(LinphoneCall *call, gboolean sensitive, gboolean holdon){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer (call);
	GtkWidget *button;
	g_return_if_fail(callview!=NULL);
	button=linphone_gtk_get_widget(callview,"hold_call");
	gtk_widget_set_sensitive(GTK_WIDGET(button),sensitive);
	linphone_gtk_draw_hold_button(GTK_BUTTON(button),!holdon);
}
