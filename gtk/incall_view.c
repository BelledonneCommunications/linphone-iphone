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

LinphoneCall *linphone_gtk_get_currently_displayed_call(gboolean *is_conf){
	LinphoneCore *lc=linphone_gtk_get_core();
	GtkWidget *main_window=linphone_gtk_get_main_window ();
	GtkNotebook *notebook=(GtkNotebook *)linphone_gtk_get_widget(main_window,"viewswitch");
	const MSList *calls=linphone_core_get_calls(lc);
	if (is_conf) *is_conf=FALSE;
	if (!linphone_gtk_use_in_call_view() || ms_list_size(calls)==1){
		if (calls) return (LinphoneCall*)calls->data;
	}else{
		int idx=gtk_notebook_get_current_page (notebook);
		GtkWidget *page=gtk_notebook_get_nth_page(notebook,idx);
		if (page!=NULL){
			LinphoneCall *call=(LinphoneCall*)g_object_get_data(G_OBJECT(page),"call");
			if (call==NULL){
				GtkWidget *conf_frame=(GtkWidget *)g_object_get_data(G_OBJECT(main_window),"conf_frame");
				if (conf_frame==page){
					if (is_conf)
						*is_conf=TRUE;
					return NULL;
				}
			}
			return call;
		}
	}
	return NULL;
}

static GtkWidget *make_tab_header(int number){
	GtkWidget *w=gtk_hbox_new (FALSE,0);
	GtkWidget *i=create_pixmap ("startcall-small.png");
	GtkWidget *l;
	gchar *text=g_strdup_printf(_("Call #%i"),number);
	l=gtk_label_new (text);
	gtk_box_pack_start (GTK_BOX(w),i,FALSE,FALSE,0);
	gtk_box_pack_end(GTK_BOX(w),l,TRUE,TRUE,0);
	gtk_widget_show_all(w);
	return w;
}

void linphone_gtk_call_update_tab_header(LinphoneCall *call,gboolean pause){
	GtkWidget *w=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkNotebook *notebook=GTK_NOTEBOOK(linphone_gtk_get_widget(main_window,"viewswitch"));
	gint call_index=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"call_index"));
	GtkWidget *new_label=gtk_hbox_new (FALSE,0);
	GtkWidget *i=NULL;
	GtkWidget *l;
	gchar *text;
	
	if(pause){
		i=gtk_image_new_from_stock(GTK_STOCK_MEDIA_PAUSE,GTK_ICON_SIZE_SMALL_TOOLBAR);
	} else {
		i=create_pixmap ("startcall-small.png");
	}
	
	text=g_strdup_printf(_("Call #%i"),call_index);
	l=gtk_label_new (text);
	gtk_box_pack_start (GTK_BOX(new_label),i,FALSE,FALSE,0);
	gtk_box_pack_end(GTK_BOX(new_label),l,TRUE,TRUE,0);

	gtk_notebook_set_tab_label(notebook,w,new_label);
	gtk_widget_show_all(new_label);
}

static void linphone_gtk_in_call_set_animation_image(GtkWidget *callview, const char *image_name, gboolean is_stock){
	GtkWidget *container=linphone_gtk_get_widget(callview,"in_call_animation");
	GList *elem=gtk_container_get_children(GTK_CONTAINER(container));
	GtkWidget *image;

	if (!is_stock){
		if (image_name==NULL){
			gtk_widget_hide(container);
		}
		image=create_pixmap(image_name);
	}else
		image=gtk_image_new_from_stock(image_name,GTK_ICON_SIZE_DND);
	if (elem)
		gtk_widget_destroy((GtkWidget*)elem->data);
	gtk_widget_show(image);
	gtk_container_add(GTK_CONTAINER(container),image);
	gtk_widget_show_all(container);
}

static void linphone_gtk_in_call_set_animation_spinner(GtkWidget *callview){
#if GTK_CHECK_VERSION(2,20,0)
	GtkWidget *container=linphone_gtk_get_widget(callview,"in_call_animation");
	GList *elem=gtk_container_get_children(GTK_CONTAINER(container));
	GtkWidget *spinner=gtk_spinner_new();
	if (elem)
		gtk_widget_destroy((GtkWidget*)elem->data);
	gtk_widget_show(spinner);
	gtk_container_add(GTK_CONTAINER(container),spinner);
	gtk_widget_set_size_request(spinner, 20,20);
	gtk_spinner_start(GTK_SPINNER(spinner));
#endif
}

static void linphone_gtk_transfer_call(LinphoneCall *dest_call){
	LinphoneCall *call=linphone_gtk_get_currently_displayed_call(NULL);
	if (call) linphone_core_transfer_call_to_another(linphone_gtk_get_core(),call,dest_call);
}

void transfer_button_clicked(GtkWidget *button, gpointer call_ref){
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
			char *text=g_strdup_printf(_("Transfer to call #%i with %s"),call_index,remote_uri);
			menu_item=gtk_image_menu_item_new_with_label(text);
			ms_free(remote_uri);
			g_free(text);
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_item),create_pixmap("status-green.png"));
			gtk_widget_show(menu_item);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu),menu_item);
			g_signal_connect_swapped(G_OBJECT(menu_item),"activate",(GCallback)linphone_gtk_transfer_call,other_call);
		}
	}
	gtk_menu_popup(GTK_MENU(menu),NULL,NULL,NULL,NULL,0,gtk_get_current_event_time());
	gtk_widget_show(menu);
}

void linphone_gtk_enable_transfer_button(LinphoneCore *lc, gboolean value){
	const MSList *elem=linphone_core_get_calls(lc);
	for(;elem!=NULL;elem=elem->next){
		LinphoneCall *call=(LinphoneCall*)elem->data;
		GtkWidget *call_view=(GtkWidget*)linphone_call_get_user_pointer(call);
		GtkWidget *button=linphone_gtk_get_widget (call_view,"transfer_button");
		if(button != NULL){
			gtk_widget_set_sensitive(button,value);
		}
	}
}

static void conference_button_clicked(GtkWidget *button, gpointer call_ref){
	gtk_widget_set_sensitive(button,FALSE);
	g_object_set_data(G_OBJECT(linphone_gtk_get_main_window()),"conf_frame",NULL);
	linphone_core_add_all_to_conference(linphone_gtk_get_core());
	
}

void linphone_gtk_enable_conference_button(LinphoneCore *lc, gboolean value){
	const MSList *elem=linphone_core_get_calls(lc);
	for(;elem!=NULL;elem=elem->next){
		LinphoneCall *call=(LinphoneCall*)elem->data;
		GtkWidget *call_view=(GtkWidget*)linphone_call_get_user_pointer(call);
		GtkWidget *button=linphone_gtk_get_widget (call_view,"conference_button");
		if (button != NULL){
			gtk_widget_set_sensitive(button,value);
		}
	}
}

static void show_used_codecs(GtkWidget *callstats, LinphoneCall *call){
	const LinphoneCallParams *params=linphone_call_get_current_params(call);
	if (params){
		const PayloadType *acodec=linphone_call_params_get_used_audio_codec(params);
		const PayloadType *vcodec=linphone_call_params_get_used_video_codec(params);
		GtkWidget *acodec_ui=linphone_gtk_get_widget(callstats,"audio_codec");
		GtkWidget *vcodec_ui=linphone_gtk_get_widget(callstats,"video_codec");
		if (acodec){
			char tmp[64]={0};
			snprintf(tmp,sizeof(tmp)-1,"%s/%i/%i",acodec->mime_type,acodec->clock_rate,acodec->channels);
			gtk_label_set_label(GTK_LABEL(acodec_ui),tmp);
		}else gtk_label_set_label(GTK_LABEL(acodec_ui),_("Not used"));
		if (vcodec){
			gtk_label_set_label(GTK_LABEL(vcodec_ui),vcodec->mime_type);
		}else gtk_label_set_label(GTK_LABEL(vcodec_ui),_("Not used"));
	}
}

static const char *ice_state_to_string(LinphoneIceState ice_state){
	switch(ice_state){
		case LinphoneIceStateNotActivated:
			return _("ICE not activated");
		case LinphoneIceStateFailed:
			return _("ICE failed");
		case LinphoneIceStateInProgress:
			return _("ICE in progress");
		case LinphoneIceStateReflexiveConnection:
			return _("Going through one or more NATs");
		case LinphoneIceStateHostConnection:
			return _("Direct");
		case LinphoneIceStateRelayConnection:
			return _("Through a relay server");
	}
	return "invalid";
}

static const char *upnp_state_to_string(LinphoneUpnpState ice_state){
	switch(ice_state){
		case LinphoneUpnpStateIdle:
			return _("uPnP not activated");
		case LinphoneUpnpStatePending:
			return _("uPnP in progress");
		case LinphoneUpnpStateNotAvailable:
			return _("uPnp not available");
		case LinphoneUpnpStateOk:
			return _("uPnP is running");
		case LinphoneUpnpStateKo:
			return _("uPnP failed");
		default:
			break;
	}
	return "invalid";
}

static void _refresh_call_stats(GtkWidget *callstats, LinphoneCall *call){
	const LinphoneCallStats *as=linphone_call_get_audio_stats(call);
	const LinphoneCallStats *vs=linphone_call_get_video_stats(call);
	const char *audio_media_connectivity = _("Direct or through server");
	const char *video_media_connectivity = _("Direct or through server");
	gboolean has_video=linphone_call_params_video_enabled(linphone_call_get_current_params(call));
	gchar *tmp=g_strdup_printf(_("download: %f\nupload: %f (kbit/s)"),
		as->download_bandwidth,as->upload_bandwidth);
	
	gtk_label_set_markup(GTK_LABEL(linphone_gtk_get_widget(callstats,"audio_bandwidth_usage")),tmp);
	g_free(tmp);
	if (has_video)
		tmp=g_strdup_printf(_("download: %f\nupload: %f (kbit/s)"),vs->download_bandwidth,vs->upload_bandwidth);
	else tmp=NULL;
	gtk_label_set_markup(GTK_LABEL(linphone_gtk_get_widget(callstats,"video_bandwidth_usage")),tmp);
	if (tmp) g_free(tmp);
	if(as->upnp_state != LinphoneUpnpStateNotAvailable && as->upnp_state != LinphoneUpnpStateIdle) {
		audio_media_connectivity = upnp_state_to_string(as->upnp_state);
	} else if(as->ice_state != LinphoneIceStateNotActivated) {
		audio_media_connectivity = ice_state_to_string(as->ice_state);
	}
	gtk_label_set_text(GTK_LABEL(linphone_gtk_get_widget(callstats,"audio_media_connectivity")),audio_media_connectivity);
	
	if (has_video){
		if(vs->upnp_state != LinphoneUpnpStateNotAvailable && vs->upnp_state != LinphoneUpnpStateIdle) {
				video_media_connectivity = upnp_state_to_string(vs->upnp_state);
		} else if(vs->ice_state != LinphoneIceStateNotActivated) {
			video_media_connectivity = ice_state_to_string(vs->ice_state);
		}
	}else video_media_connectivity=NULL;
	gtk_label_set_text(GTK_LABEL(linphone_gtk_get_widget(callstats,"video_media_connectivity")),video_media_connectivity);
	
	if (as->round_trip_delay>0){
		tmp=g_strdup_printf(_("%.3f seconds"),as->round_trip_delay);
		gtk_label_set_text(GTK_LABEL(linphone_gtk_get_widget(callstats,"round_trip_time")),tmp);
		g_free(tmp);
	}
}

static gboolean refresh_call_stats(GtkWidget *callstats){
	LinphoneCall *call=(LinphoneCall*)g_object_get_data(G_OBJECT(callstats),"call");
	switch (linphone_call_get_state(call)){
		case LinphoneCallError:
		case LinphoneCallEnd:
		case LinphoneCallReleased:
			gtk_widget_destroy(callstats);
			return FALSE;
		break;
		case LinphoneCallStreamsRunning:
			_refresh_call_stats(callstats,call);
		break;
		default:
		break;
	}
	return TRUE;
}

static void on_call_stats_destroyed(GtkWidget *call_view){
	GtkWidget *call_stats=(GtkWidget*)g_object_get_data(G_OBJECT(call_view),"call_stats");
	LinphoneCall *call=(LinphoneCall*)g_object_get_data(G_OBJECT(call_stats),"call");
	g_source_remove(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(call_stats),"tid")));
	g_object_set_data(G_OBJECT(call_view),"call_stats",NULL);
	linphone_call_unref(call);
}

static void linphone_gtk_show_call_stats(LinphoneCall *call){
	GtkWidget *w=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *call_stats=(GtkWidget*)g_object_get_data(G_OBJECT(w),"call_stats");
	if (call_stats==NULL){
		guint tid;
		call_stats=linphone_gtk_create_window("call_statistics");
		g_object_set_data(G_OBJECT(w),"call_stats",call_stats);
		g_object_set_data(G_OBJECT(call_stats),"call",linphone_call_ref(call));
		tid=g_timeout_add(1000,(GSourceFunc)refresh_call_stats,call_stats);
		g_object_set_data(G_OBJECT(call_stats),"tid",GINT_TO_POINTER(tid));
		g_signal_connect_swapped(G_OBJECT(call_stats),"destroy",(GCallback)on_call_stats_destroyed,(gpointer)w);
		show_used_codecs(call_stats,call);
		refresh_call_stats(call_stats);
		gtk_widget_show(call_stats);
	}

}

void linphone_gtk_enable_video_button(LinphoneCall *call, gboolean sensitive, gboolean holdon){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer (call);
	GtkWidget *button;
	g_return_if_fail(callview!=NULL);
	button=linphone_gtk_get_widget(callview,"video_button");
	gtk_widget_set_sensitive(GTK_WIDGET(button),sensitive);
	gtk_widget_set_visible(GTK_WIDGET(button),sensitive);
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
	linphone_gtk_enable_video_button (call,FALSE,TRUE);
	linphone_gtk_enable_mute_button(
					GTK_BUTTON(linphone_gtk_get_widget(call_view,"incall_mute")),FALSE);

	GtkWidget *transfer = linphone_gtk_get_widget(call_view,"transfer_button");
	gtk_button_set_image(GTK_BUTTON(transfer),gtk_image_new_from_stock
							 (GTK_STOCK_GO_FORWARD,GTK_ICON_SIZE_BUTTON));
	g_signal_connect(G_OBJECT(transfer),"clicked",(GCallback)transfer_button_clicked,call);
	gtk_widget_hide(transfer);

	GtkWidget *conf = linphone_gtk_get_widget(call_view,"conference_button");
	gtk_button_set_image(GTK_BUTTON(conf),gtk_image_new_from_stock (GTK_STOCK_ADD,GTK_ICON_SIZE_BUTTON));
	g_signal_connect(G_OBJECT(conf),"clicked",(GCallback)conference_button_clicked,call);
	gtk_widget_hide(conf);

	GtkWidget *button=linphone_gtk_get_widget(call_view,"terminate_call");
	GtkWidget *image=create_pixmap("stopcall-small.png");
	gtk_button_set_label(GTK_BUTTON(button),_("Hang up"));
	gtk_button_set_image(GTK_BUTTON(button),image);
	gtk_widget_show(image);
	g_signal_connect_swapped(G_OBJECT(linphone_gtk_get_widget(call_view,"quality_indicator")),"button-press-event",(GCallback)linphone_gtk_show_call_stats,call);
}

static void video_button_clicked(GtkWidget *button, LinphoneCall *call){
	gboolean adding=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button),"adding_video"));
	LinphoneCore *lc=linphone_call_get_core(call);
	LinphoneCallParams *params=linphone_call_params_copy(linphone_call_get_current_params(call));
	gtk_widget_set_sensitive(button,FALSE);
	linphone_call_params_enable_video(params,adding);
	linphone_core_update_call(lc,call,params);
	linphone_call_params_destroy(params);
}

void linphone_gtk_update_video_button(LinphoneCall *call){
	GtkWidget *call_view=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *button;
	const LinphoneCallParams *params=linphone_call_get_current_params(call);
	gboolean has_video=linphone_call_params_video_enabled(params);
	if (call_view==NULL) return;
	button=linphone_gtk_get_widget(call_view,"video_button");

	gtk_button_set_image(GTK_BUTTON(button),
	gtk_image_new_from_stock(has_video ? GTK_STOCK_REMOVE : GTK_STOCK_ADD,GTK_ICON_SIZE_BUTTON));
	g_object_set_data(G_OBJECT(button),"adding_video",GINT_TO_POINTER(!has_video));
	if (!linphone_core_video_supported(linphone_call_get_core(call))){
		gtk_widget_set_sensitive(button,FALSE);
		return;
	}
	if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button),"signal_connected"))==0){
		g_signal_connect(G_OBJECT(button),"clicked",(GCallback)video_button_clicked,call);
		g_object_set_data(G_OBJECT(button),"signal_connected",GINT_TO_POINTER(1));
	}
	GtkWidget *conf_frame=(GtkWidget *)g_object_get_data(G_OBJECT(linphone_gtk_get_main_window()),"conf_frame");
	gtk_widget_set_sensitive(button,linphone_call_get_state(call)==LinphoneCallStreamsRunning);
	if(conf_frame!=NULL){
		gtk_widget_set_sensitive(button,FALSE);
	}
}

void linphone_gtk_remove_in_call_view(LinphoneCall *call){
	GtkWidget *w=(GtkWidget*)linphone_call_get_user_pointer (call);
	GtkWidget *main_window=linphone_gtk_get_main_window ();
	GtkWidget *nb=linphone_gtk_get_widget(main_window,"viewswitch");
	gboolean in_conf=linphone_call_params_local_conference_mode(linphone_call_get_current_params(call));
	int idx;
	g_return_if_fail(w!=NULL);
	idx=gtk_notebook_page_num(GTK_NOTEBOOK(nb),w);
	if (in_conf){
		linphone_gtk_unset_from_conference(call);
	}
	linphone_call_set_user_pointer (call,NULL);
	linphone_call_unref(call);
	call=linphone_core_get_current_call(linphone_gtk_get_core());
	if (call==NULL){
		if (linphone_core_is_in_conference(linphone_gtk_get_core())){
			/*show the conference*/
			gtk_notebook_set_current_page(GTK_NOTEBOOK(nb),gtk_notebook_page_num(GTK_NOTEBOOK(nb),
		                            g_object_get_data(G_OBJECT(main_window),"conf_frame")));
		}else gtk_notebook_prev_page(GTK_NOTEBOOK(nb));
	}else{
		/*show the active call*/
		gtk_notebook_set_current_page(GTK_NOTEBOOK(nb),gtk_notebook_page_num(GTK_NOTEBOOK(nb),                                                                     linphone_call_get_user_pointer(call)));
	}
	gtk_notebook_remove_page (GTK_NOTEBOOK(nb),idx);
	gtk_widget_destroy(w);
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

	gtk_label_set_markup(GTK_LABEL(status),_("<b>Calling...</b>"));
	display_peer_name_in_label(callee,linphone_call_get_remote_address (call));

	gtk_label_set_text(GTK_LABEL(duration),_("00::00::00"));
	linphone_gtk_in_call_set_animation_spinner(callview);
}

void linphone_gtk_in_call_view_set_incoming(LinphoneCall *call){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *status=linphone_gtk_get_widget(callview,"in_call_status");
	GtkWidget *callee=linphone_gtk_get_widget(callview,"in_call_uri");
	GtkWidget *answer_button;
	GtkWidget *image;

	gtk_label_set_markup(GTK_LABEL(status),_("<b>Incoming call</b>"));
	gtk_widget_show_all(linphone_gtk_get_widget(callview,"answer_decline_panel"));
	gtk_widget_hide(linphone_gtk_get_widget(callview,"buttons_panel"));
	display_peer_name_in_label(callee,linphone_call_get_remote_address (call));

	answer_button=linphone_gtk_get_widget(callview,"accept_call");
	image=create_pixmap (linphone_gtk_get_ui_config("start_call_icon","startcall-small.png"));
	gtk_button_set_label(GTK_BUTTON(answer_button),_("Answer"));
	gtk_button_set_image(GTK_BUTTON(answer_button),image);
	gtk_widget_show(image);

	image=create_pixmap (linphone_gtk_get_ui_config("stop_call_icon","stopcall-small.png"));
	gtk_button_set_image(GTK_BUTTON(linphone_gtk_get_widget(callview,"decline_call")),image);
	gtk_widget_show(image);

	linphone_gtk_in_call_set_animation_image(callview,GTK_STOCK_DIALOG_INFO,TRUE);
}

static void rating_to_color(float rating, GdkColor *color){
	const char *colorname="grey";
	if (rating>=4.0)
		colorname="green";
	else if (rating>=3.0)
		colorname="white";
	else if (rating>=2.0)
		colorname="yellow";
	else if (rating>=1.0)
		colorname="orange";
	else if (rating>=0)
		colorname="red";
	if (!gdk_color_parse(colorname,color)){
		g_warning("Fail to parse color %s",colorname);
	}
}

static const char *rating_to_text(float rating){
	if (rating>=4.0)
		return _("good");
	if (rating>=3.0)
		return _("average");
	if (rating>=2.0)
		return _("poor");
	if (rating>=1.0)
		return _("very poor");
	if (rating>=0)
		return _("too bad");
	return _("unavailable");
}

static gboolean linphone_gtk_in_call_view_refresh(LinphoneCall *call){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *qi=linphone_gtk_get_widget(callview,"quality_indicator");
	float rating=linphone_call_get_current_quality(call);
	GdkColor color;
	gchar tmp[50];
	linphone_gtk_in_call_view_update_duration(call);
	if (rating>=0){
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(qi),rating/5.0);
		snprintf(tmp,sizeof(tmp),"%.1f (%s)",rating,rating_to_text(rating));
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(qi),tmp);
	}else{
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(qi),0);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(qi),_("unavailable"));
	}
	rating_to_color(rating,&color);
	gtk_widget_modify_bg(qi,GTK_STATE_NORMAL,&color);
	return TRUE;
}

typedef struct _volume_ctx{
	GtkWidget *widget;
	get_volume_t get_volume;
	void *data;
	float last_value;
}volume_ctx_t;

#define UNSIGNIFICANT_VOLUME (-26)
#define SMOOTH 0.15

static gboolean update_audio_meter(volume_ctx_t *ctx){
	float volume_db=ctx->get_volume(ctx->data);
	float frac=(volume_db-UNSIGNIFICANT_VOLUME)/(float)(-UNSIGNIFICANT_VOLUME+3.0);
	if (frac<0) frac=0;
	if (frac>1.0) frac=1.0;
	if (frac<ctx->last_value){
		frac=(frac*SMOOTH)+(ctx->last_value*(1-SMOOTH));
	}
	ctx->last_value=frac;
	//g_message("volume_db=%f, frac=%f",volume_db,frac);
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(ctx->widget),frac);
	return TRUE;
}

static void on_audio_meter_destroy(guint task_id){
	g_source_remove(task_id);
}

void linphone_gtk_init_audio_meter(GtkWidget *w, get_volume_t get_volume, void *data){
	guint task_id=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"task_id"));
	if (task_id==0){
		volume_ctx_t *ctx=g_new(volume_ctx_t,1);
		ctx->widget=w;
		ctx->get_volume=get_volume;
		ctx->data=data;
		ctx->last_value=0;
		g_object_set_data_full(G_OBJECT(w),"ctx",ctx,g_free);
		task_id=g_timeout_add(50,(GSourceFunc)update_audio_meter,ctx);
		g_object_set_data_full(G_OBJECT(w),"task_id",GINT_TO_POINTER(task_id),(GDestroyNotify)on_audio_meter_destroy);
	}
}

void linphone_gtk_uninit_audio_meter(GtkWidget *w){
	guint task_id=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"task_id"));
	if (task_id!=0){
		g_object_set_data(G_OBJECT(w),"ctx",NULL);
		g_object_set_data(G_OBJECT(w),"task_id",NULL);
	}
}

void linphone_gtk_in_call_view_enable_audio_view(LinphoneCall *call, gboolean val){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *audio_view=linphone_gtk_get_widget(callview,"incall_audioview");
	GtkWidget *mic=linphone_gtk_get_widget(callview,"incall_mic_icon");
	GtkWidget *spk=linphone_gtk_get_widget(callview,"incall_spk_icon");
	GtkWidget *mic_level=linphone_gtk_get_widget(callview,"mic_audiolevel");
	GtkWidget *spk_level=linphone_gtk_get_widget(callview,"spk_audiolevel");
	GdkPixbuf *pbuf;

	gtk_image_set_from_pixbuf(GTK_IMAGE(mic),(pbuf=create_pixbuf("mic_active.png")));
	g_object_unref(pbuf);
	if (val){
		gtk_image_set_from_pixbuf(GTK_IMAGE(spk),(pbuf=create_pixbuf("speaker.png")));
		g_object_unref(pbuf);
		linphone_gtk_init_audio_meter(mic_level,(get_volume_t)linphone_call_get_record_volume,call);
		linphone_gtk_init_audio_meter(spk_level,(get_volume_t)linphone_call_get_play_volume,call);
		gtk_widget_show_all(audio_view);
	}else{
		linphone_gtk_uninit_audio_meter(mic_level);
		linphone_gtk_uninit_audio_meter(spk_level);
		gtk_widget_hide(audio_view);
	}
}

void linphone_gtk_auth_token_verified_clicked(GtkButton *button){
	LinphoneCall *call=linphone_gtk_get_currently_displayed_call(NULL);
	if (call){
		linphone_call_set_authentication_token_verified(call,!linphone_call_get_authentication_token_verified(call));
	}
}

void linphone_gtk_in_call_view_show_encryption(LinphoneCall *call){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *encryption_box=linphone_gtk_get_widget(callview,"encryption_box");
	GtkWidget *label=linphone_gtk_get_widget(callview,"encryption_label");
	GtkWidget *status_icon=linphone_gtk_get_widget(callview,"encryption_status_icon");
	GtkWidget *verify_button=linphone_gtk_get_widget(callview,"encryption_verify_button");
	LinphoneMediaEncryption me=linphone_call_params_get_media_encryption(linphone_call_get_current_params(call));
	bool_t verified=linphone_call_get_authentication_token_verified(call);
	switch(me){
		case LinphoneMediaEncryptionSRTP:
			gtk_widget_show_all(encryption_box);
			gtk_label_set_markup(GTK_LABEL(label),_("Secured by SRTP"));
			gtk_widget_hide(status_icon);
			gtk_widget_hide(verify_button);
		break;
		case LinphoneMediaEncryptionZRTP:
		{
			gchar *text=g_strdup_printf(_("Secured by ZRTP - [auth token: %s]"),linphone_call_get_authentication_token(call));
			gtk_label_set_markup(GTK_LABEL(label),text);
			g_free(text);
			gtk_image_set_from_stock(GTK_IMAGE(status_icon),
			                          verified ? GTK_STOCK_APPLY : GTK_STOCK_DIALOG_WARNING,GTK_ICON_SIZE_MENU);
			gtk_button_set_label(GTK_BUTTON(verify_button),
			                     verified ? _("Set unverified") : _("Set verified"));
			gtk_widget_show_all(encryption_box);
		}
		break;
		default:
			gtk_widget_hide_all(encryption_box);
	}
}

void linphone_gtk_in_call_view_set_in_call(LinphoneCall *call){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *status=linphone_gtk_get_widget(callview,"in_call_status");
	GtkWidget *callee=linphone_gtk_get_widget(callview,"in_call_uri");
	GtkWidget *duration=linphone_gtk_get_widget(callview,"in_call_duration");
	guint taskid=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(callview),"taskid"));
	gboolean in_conf=linphone_call_params_local_conference_mode(linphone_call_get_current_params(call));
	GtkWidget *call_stats=(GtkWidget*)g_object_get_data(G_OBJECT(callview),"call_stats");

	display_peer_name_in_label(callee,linphone_call_get_remote_address (call));
	
	gtk_widget_hide(linphone_gtk_get_widget(callview,"answer_decline_panel"));
	gtk_label_set_markup(GTK_LABEL(status),in_conf ? _("In conference") : _("<b>In call</b>"));
	
	gtk_widget_set_sensitive(linphone_gtk_get_widget(callview,"conference_button"),!in_conf);
	gtk_widget_set_sensitive(linphone_gtk_get_widget(callview,"transfer_button"),!in_conf);

	gtk_label_set_text(GTK_LABEL(duration),_("00::00::00"));
	linphone_gtk_in_call_set_animation_image(callview,GTK_STOCK_MEDIA_PLAY,TRUE);
	linphone_gtk_call_update_tab_header(call,FALSE);
	linphone_gtk_enable_mute_button(
					GTK_BUTTON(linphone_gtk_get_widget(callview,"incall_mute")),TRUE);
	
	if (taskid==0){
		taskid=g_timeout_add(250,(GSourceFunc)linphone_gtk_in_call_view_refresh,call);
		g_object_set_data(G_OBJECT(callview),"taskid",GINT_TO_POINTER(taskid));
	}
	linphone_gtk_in_call_view_enable_audio_view(call, !in_conf);
	linphone_gtk_in_call_view_show_encryption(call);
	if (in_conf){
		linphone_gtk_set_in_conference(call);
		gtk_widget_set_sensitive(linphone_gtk_get_widget(callview,"incall_mute"),FALSE);
		gtk_widget_set_sensitive(linphone_gtk_get_widget(callview,"hold_call"),FALSE);
	}else{
		linphone_gtk_unset_from_conference(call); /*in case it was previously*/
		gtk_widget_set_sensitive(linphone_gtk_get_widget(callview,"incall_mute"),TRUE);
		gtk_widget_set_sensitive(linphone_gtk_get_widget(callview,"hold_call"),TRUE);
	}
	gtk_widget_show_all(linphone_gtk_get_widget(callview,"buttons_panel"));
	if (!in_conf) gtk_widget_show_all(linphone_gtk_get_widget(callview,"record_hbox"));
	else gtk_widget_hide(linphone_gtk_get_widget(callview,"record_hbox"));
	if (call_stats) show_used_codecs(call_stats,call);
}

void linphone_gtk_in_call_view_set_paused(LinphoneCall *call){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
	GtkWidget *status=linphone_gtk_get_widget(callview,"in_call_status");
	gtk_widget_hide(linphone_gtk_get_widget(callview,"answer_decline_panel"));
	gtk_label_set_markup(GTK_LABEL(status),_("<b>Paused call</b>"));

	linphone_gtk_in_call_set_animation_image(callview,GTK_STOCK_MEDIA_PAUSE,TRUE);
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
	if(callview==NULL) return;
	GtkWidget *status=linphone_gtk_get_widget(callview,"in_call_status");
	guint taskid=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(callview),"taskid"));
	gboolean in_conf=linphone_call_params_local_conference_mode(linphone_call_get_current_params(call));

	if (status==NULL) return;
	if (error_msg==NULL)
		gtk_label_set_markup(GTK_LABEL(status),_("<b>Call ended.</b>"));
	else{
		char *msg=g_markup_printf_escaped("<span color=\"red\"><b>%s</b></span>",error_msg);
		gtk_label_set_markup(GTK_LABEL(status),msg);
		g_free(msg);
	}
	linphone_gtk_in_call_set_animation_image(callview,
	           linphone_gtk_get_ui_config("stop_call_icon","stopcall-red.png"),FALSE);

	gtk_widget_hide(linphone_gtk_get_widget(callview,"answer_decline_panel"));
	gtk_widget_hide(linphone_gtk_get_widget(callview,"record_hbox"));
	gtk_widget_hide(linphone_gtk_get_widget(callview,"buttons_panel"));
	gtk_widget_hide(linphone_gtk_get_widget(callview,"incall_audioview"));
	gtk_widget_hide(linphone_gtk_get_widget(callview,"quality_indicator"));
	linphone_gtk_enable_mute_button(
		GTK_BUTTON(linphone_gtk_get_widget(callview,"incall_mute")),FALSE);
	linphone_gtk_enable_hold_button(call,FALSE,TRUE);
	
	if (taskid!=0) g_source_remove(taskid);
	g_timeout_add_seconds(2,(GSourceFunc)in_call_view_terminated,call);
	if (in_conf)
		linphone_gtk_terminate_conference_participant(call);
}

void linphone_gtk_in_call_view_set_transfer_status(LinphoneCall *call,LinphoneCallState cstate){
	GtkWidget *callview=(GtkWidget*)linphone_call_get_user_pointer(call);
	if (callview){
		GtkWidget *duration=linphone_gtk_get_widget(callview,"in_call_duration");
		const char *transfer_status="unknown";
		switch(cstate){
			case LinphoneCallOutgoingProgress:
				transfer_status=_("Transfer in progress");
			break;
			case LinphoneCallConnected:
				transfer_status=_("Transfer done.");
			break;
			case LinphoneCallError:
				transfer_status=_("Transfer failed.");
			break;
			default:
			break;
		}
		gtk_label_set_text(GTK_LABEL(duration),transfer_status);
	}
}

void linphone_gtk_draw_mute_button(GtkButton *button, gboolean active){
	g_object_set_data(G_OBJECT(button),"active",GINT_TO_POINTER(active));
	if (active){
		GtkWidget *image=create_pixmap("mic_muted.png");
		/*gtk_button_set_label(GTK_BUTTON(button),_("Unmute"));*/
		if (image!=NULL) {
			gtk_button_set_image(GTK_BUTTON(button),image);
			gtk_widget_show(image);
		}
	}else{
		GtkWidget *image=create_pixmap("mic_active.png");
		/*gtk_button_set_label(GTK_BUTTON(button),_("Mute"));*/
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

void linphone_gtk_enable_mute_button(GtkButton *button, gboolean sensitive){
	/*gtk_widget_set_sensitive(GTK_WIDGET(button),sensitive);*/
	gtk_widget_set_visible(GTK_WIDGET(button),sensitive);
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
	LinphoneCall *call=linphone_gtk_get_currently_displayed_call(NULL);
	linphone_gtk_call_update_tab_header(call,active);
	if (!call) return;
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
	linphone_gtk_call_update_tab_header(call,!holdon);
	button=linphone_gtk_get_widget(callview,"hold_call");
	gtk_widget_set_sensitive(GTK_WIDGET(button),sensitive);
	gtk_widget_set_visible(GTK_WIDGET(button),sensitive);
	linphone_gtk_draw_hold_button(GTK_BUTTON(button),!holdon);
}

void linphone_gtk_call_statistics_closed(GtkWidget *call_stats){
	gtk_widget_destroy(call_stats);
}

void linphone_gtk_record_call_toggled(GtkWidget *button){
	gboolean active=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
	gboolean is_conf=FALSE;
	const char *filepath;
	gchar *message;
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneCall *call=linphone_gtk_get_currently_displayed_call(&is_conf);
	GtkWidget *callview;
	GtkWidget *label;
	if (call){
		callview=(GtkWidget*)linphone_call_get_user_pointer (call);
		const LinphoneCallParams *params=linphone_call_get_current_params(call);
		filepath=linphone_call_params_get_record_file(params);
		label=linphone_gtk_get_widget(callview,"record_status");
	}else if (is_conf){
		GtkWidget *mw=linphone_gtk_get_main_window();
		callview=(GtkWidget *)g_object_get_data(G_OBJECT(linphone_gtk_get_main_window()),"conf_frame");
		label=linphone_gtk_get_widget(callview,"conf_record_status");
		filepath=(const char*)g_object_get_data(G_OBJECT(mw),"conf_record_path");
		if (filepath==NULL){
			filepath=linphone_gtk_get_record_path(NULL,TRUE);
			g_object_set_data_full(G_OBJECT(mw),"conf_record_path",(char*)filepath,g_free);
		}
	}else{
		g_warning("linphone_gtk_record_call_toggled(): bug.");
		return;
	}
	message=g_strdup_printf(_("<small><i>Recording into\n%s %s</i></small>"),filepath,active ? "" : _("(Paused)"));
	
	if (active){
		if (call)
			linphone_call_start_recording(call);
		else
			linphone_core_start_conference_recording(lc,filepath);
	}else {
		if (call)
			linphone_call_stop_recording(call);
		else
			linphone_core_stop_conference_recording(lc);
		
	}
	gtk_label_set_markup(GTK_LABEL(label),message);
	g_free(message);
}

