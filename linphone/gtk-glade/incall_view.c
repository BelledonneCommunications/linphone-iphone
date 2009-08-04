/*
*  C Implementation: incall_frame
*
* Description: 
*
*
* Author: Simon Morlat <simon.morlat@linphone.org>, (C) 2009
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#include "linphone.h"

void linphone_gtk_show_in_call_view(void){
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *idle_frame=linphone_gtk_get_widget(main_window,"idle_frame");
	GtkWidget *in_call_frame=linphone_gtk_get_widget(main_window,"in_call_frame");
	gtk_widget_hide(idle_frame);
	gtk_widget_show(in_call_frame);
}

void linphone_gtk_show_idle_view(void){
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *idle_frame=linphone_gtk_get_widget(main_window,"idle_frame");
	GtkWidget *in_call_frame=linphone_gtk_get_widget(main_window,"in_call_frame");
	gtk_widget_show(idle_frame);
	gtk_widget_hide(in_call_frame);
}

void linphone_gtk_in_call_view_set_calling(const char *uri){
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *status=linphone_gtk_get_widget(main_window,"in_call_status");
	GtkWidget *callee=linphone_gtk_get_widget(main_window,"in_call_uri");
	GtkWidget *duration=linphone_gtk_get_widget(main_window,"in_call_duration");
	GtkWidget *animation=linphone_gtk_get_widget(main_window,"in_call_animation");
	GdkPixbufAnimation *pbuf=create_pixbuf_animation("calling_anim.gif");
	GtkWidget *terminate_button=linphone_gtk_get_widget(main_window,"in_call_terminate");
	char *uri_label;

	gtk_widget_set_sensitive(terminate_button,TRUE);
	gtk_label_set_markup(GTK_LABEL(status),_("<b>Calling...</b>"));
	uri_label=g_markup_printf_escaped("<span size=\"large\"><i>%s</i></span>", uri);
	gtk_label_set_markup(GTK_LABEL(callee),uri_label);
	g_free(uri_label);
	gtk_label_set_text(GTK_LABEL(duration),"00:00:00");
	if (pbuf!=NULL){
		gtk_image_set_from_animation(GTK_IMAGE(animation),pbuf);
		g_object_unref(G_OBJECT(pbuf));
	}else gtk_image_set_from_stock(GTK_IMAGE(animation),GTK_STOCK_INFO,GTK_ICON_SIZE_DIALOG);
}

void linphone_gtk_in_call_view_set_in_call(const char *uri){
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *status=linphone_gtk_get_widget(main_window,"in_call_status");
	GtkWidget *callee=linphone_gtk_get_widget(main_window,"in_call_uri");
	GtkWidget *duration=linphone_gtk_get_widget(main_window,"in_call_duration");
	char *uri_label;
	GtkWidget *animation=linphone_gtk_get_widget(main_window,"in_call_animation");
	GdkPixbufAnimation *pbuf=create_pixbuf_animation("incall_anim.gif");
	GtkWidget *terminate_button=linphone_gtk_get_widget(main_window,"in_call_terminate");

	gtk_widget_set_sensitive(terminate_button,TRUE);
	gtk_label_set_markup(GTK_LABEL(status),_("<b>In call with</b>"));
	uri_label=g_markup_printf_escaped("<span size=\"large\"><i>%s</i></span>", uri);
	gtk_label_set_markup(GTK_LABEL(callee),uri_label);
	g_free(uri_label);
	gtk_label_set_text(GTK_LABEL(duration),_("00::00::00"));
	if (pbuf!=NULL){
		gtk_image_set_from_animation(GTK_IMAGE(animation),pbuf);
		g_object_unref(G_OBJECT(pbuf));
	}else gtk_image_set_from_stock(GTK_IMAGE(animation),GTK_STOCK_INFO,GTK_ICON_SIZE_DIALOG);
}

void linphone_gtk_in_call_view_update_duration(int duration){
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *duration_label=linphone_gtk_get_widget(main_window,"in_call_duration");
	char tmp[256]={0};
	int seconds=duration%60;
	int minutes=(duration/60)%60;
	int hours=duration/3600;
	snprintf(tmp,sizeof(tmp)-1,_("%02i::%02i::%02i"),hours,minutes,seconds);
	gtk_label_set_text(GTK_LABEL(duration_label),tmp);
}

static gboolean in_call_view_terminated(){
	linphone_gtk_show_idle_view();
	return FALSE;
}

void linphone_gtk_in_call_view_terminate(const char *error_msg){
	GtkWidget *main_window=linphone_gtk_get_main_window();
	GtkWidget *status=linphone_gtk_get_widget(main_window,"in_call_status");
	GtkWidget *animation=linphone_gtk_get_widget(main_window,"in_call_animation");
	GtkWidget *terminate_button=linphone_gtk_get_widget(main_window,"in_call_terminate");
	GdkPixbuf *pbuf=create_pixbuf(linphone_gtk_get_ui_config("stop_call_icon","red.png"));

	gtk_widget_set_sensitive(terminate_button,FALSE);
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
	g_timeout_add_seconds(2,(GSourceFunc)in_call_view_terminated,NULL);
}
