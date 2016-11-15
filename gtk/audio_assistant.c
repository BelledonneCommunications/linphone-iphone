/*
linphone, gtk-glade interface.
Copyright (C) 2008  Simon MORLAT (simon.morlat@linphone.org)

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <glib/gstdio.h>

#include "linphone.h"
#include "linphone/core_utils.h"
#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/msvolume.h"

static GtkWidget *audio_assistant=NULL;
static void prepare(GtkAssistant *w);

GtkWidget *get_widget_from_assistant(const char *name){
	return (GtkWidget *)g_object_get_data(G_OBJECT(audio_assistant),name);
}

static void set_widget_to_assistant(const char *name,GtkWidget *w){
	g_object_set_data(G_OBJECT(audio_assistant),name,w);
}

static void update_record_button(gboolean is_visible){
	GtkWidget *rec_button = get_widget_from_assistant("rec_button");
	gtk_widget_set_sensitive(rec_button,is_visible);
}

#if 0
static void activate_record_button(gboolean is_active){
	GtkWidget *rec_button = get_widget_from_assistant("rec_button");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rec_button),is_active);
}
#endif

static void update_play_button(gboolean is_visible){
	GtkWidget *play_button = get_widget_from_assistant("play_button");
	gtk_widget_set_sensitive(play_button,is_visible);
}

static void activate_play_button(gboolean is_active){
	GtkWidget *play_button = get_widget_from_assistant("play_button");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(play_button),is_active);
}

static gboolean deactivate_play_button(void){
	activate_play_button(FALSE);
	return FALSE;
}

static gchar *get_record_file(void){
	char filename[256]={0};
	char date[64]={0};
	time_t curtime=time(NULL);
	struct tm loctime;

	#ifdef _WIN32
		loctime=*localtime(&curtime);
	#else
		localtime_r(&curtime,&loctime);
	#endif
		snprintf(date,sizeof(date)-1,"%i%02i%02i-%02i%02i%2i",loctime.tm_year+1900,loctime.tm_mon+1,loctime.tm_mday, loctime.tm_hour, loctime.tm_min, loctime.tm_sec);

	snprintf(filename,sizeof(filename)-1,"record-%s.wav",date);
	return g_build_path(G_DIR_SEPARATOR_S,g_get_tmp_dir(),filename,NULL);;
}

static float audio_stream_get_record_volume(AudioStream *st){
	if (st && st->volsend){
		float vol=0;
		ms_filter_call_method(st->volsend,MS_VOLUME_GET,&vol);
		return vol;
	}
	return LINPHONE_VOLUME_DB_LOWEST;
}

static float audio_stream_get_max_volume(AudioStream *st){
	if (st && st->volsend){
		float vol=0;
		ms_filter_call_method(st->volsend,MS_VOLUME_GET_MAX,&vol);
		return vol;
	}
	return LINPHONE_VOLUME_DB_LOWEST;
}

static gboolean update_audio_label(volume_ctx_t *ctx){
	float volume_db=ctx->get_volume(ctx->data);
	gchar *result;
	if (volume_db < -20) result = _("No voice detected");
	else if (volume_db <= -10) result = _("Too low");
	else if (volume_db < -6) result = _("Good");
	else result = _("Too loud");
	g_message("volume_max_db=%f, text=%s",volume_db,result);
	gtk_label_set_text(GTK_LABEL(ctx->widget),result);
	return TRUE;
}

static void on_audio_label_destroy(guint task_id){
	g_source_remove(task_id);
}

void linphone_gtk_init_audio_label(GtkWidget *w, get_volume_t get_volume, void *data){
	guint task_id=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"task_id_t"));
	if (task_id==0){
		volume_ctx_t *ctx=g_new(volume_ctx_t,1);
		ctx->widget=w;
		ctx->get_volume=get_volume;
		ctx->data=data;
		ctx->last_value=0;
		g_object_set_data_full(G_OBJECT(w),"ctx_t",ctx,g_free);
		task_id=g_timeout_add(200,(GSourceFunc)update_audio_label,ctx);
		g_object_set_data_full(G_OBJECT(w),"task_id_t",GINT_TO_POINTER(task_id),(GDestroyNotify)on_audio_label_destroy);
	}
}

void linphone_gtk_uninit_audio_label(GtkWidget *w){
	guint task_id=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"task_id_t"));
	if (task_id!=0){
		g_object_set_data(G_OBJECT(w),"ctx_t",NULL);
		g_object_set_data(G_OBJECT(w),"task_id_t",NULL);
	}
}

static void playback_device_changed(GtkWidget *w){
	gchar *sel=gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	linphone_core_set_playback_device(linphone_gtk_get_core(),sel);
	g_free(sel);
}

static void capture_device_changed(GtkWidget *capture_device){
	gchar *sel;
	GtkWidget *mic_audiolevel;
	GtkWidget *label_audiolevel;
	GtkWidget *assistant=gtk_widget_get_toplevel(capture_device);
	AudioStream *audio_stream;

	mic_audiolevel = get_widget_from_assistant("mic_audiolevel");
	label_audiolevel = get_widget_from_assistant("label_audiolevel");
	audio_stream = (AudioStream *) g_object_get_data(G_OBJECT(assistant),"stream");
	sel = gtk_combo_box_get_active_text(GTK_COMBO_BOX(capture_device));
	linphone_core_set_capture_device(linphone_gtk_get_core(),sel);
	linphone_gtk_uninit_audio_meter(mic_audiolevel);
	linphone_gtk_uninit_audio_label(label_audiolevel);
	audio_stream_stop(audio_stream);
	g_free(sel);
	/*now restart the audio stream*/
	prepare(GTK_ASSISTANT(assistant));
}

static void dialog_click(GtkWidget *dialog, guint response_id, GtkWidget *page){
	switch(response_id){
		case GTK_RESPONSE_YES:
			 gtk_assistant_set_page_complete(GTK_ASSISTANT(audio_assistant),page,TRUE);
		break;
		default:
			break;
	}
	gtk_widget_destroy(dialog);
}

static void calibration_finished(LinphoneCore *lc, LinphoneEcCalibratorStatus status, int delay, void *data){
	GtkWidget * dialog;
	GtkWidget *speaker_page;
	ms_message("echo calibration finished %s.",status==LinphoneEcCalibratorDone ? "successfully" : "with faillure");
	if (status==LinphoneEcCalibratorDone) ms_message("Measured delay is %i",delay);

	speaker_page = get_widget_from_assistant("speaker_page");

	dialog = gtk_message_dialog_new (
			GTK_WINDOW(audio_assistant),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO,
			"%s",_("Did you hear three beeps ?"));

	g_signal_connect(G_OBJECT (dialog), "response",
			G_CALLBACK (dialog_click),speaker_page);
	gtk_widget_show(dialog);
}

void linphone_gtk_start_sound(GtkWidget *w){
	LinphoneCore *lc = linphone_gtk_get_core();
	linphone_core_start_echo_calibration(lc,calibration_finished,NULL,NULL,NULL);
}

static gboolean linphone_gtk_stop_record(gpointer data){
	AudioStream *stream = (AudioStream *)g_object_get_data(G_OBJECT(audio_assistant),"record_stream");
	if(stream != NULL){
		audio_stream_stop(stream);
		g_object_set_data(G_OBJECT(audio_assistant),"record_stream",NULL);
	}
	update_record_button(FALSE);
	update_play_button(TRUE);
	return FALSE;
}


void linphone_gtk_start_record_sound(GtkWidget *w, gpointer data){
	LinphoneCore *lc = linphone_gtk_get_core();
	MSFactory *factory = linphone_core_get_ms_factory(lc);
	AudioStream *stream = NULL;
	MSSndCardManager *manager = ms_factory_get_snd_card_manager(factory);
	gboolean active=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	gint timeout_id;

	if(active){
		gchar *path = get_record_file();
		stream=audio_stream_new(factory, 8888, 8889, FALSE);
		if(stream != NULL){
			audio_stream_start_full(stream,&av_profile,"127.0.0.1",8888,"127.0.0.1",8889,0,0,NULL,
				path,NULL,ms_snd_card_manager_get_card(manager,linphone_core_get_capture_device(lc)),FALSE);
			g_object_set_data(G_OBJECT(audio_assistant),"record_stream",stream);
		}
		timeout_id = gtk_timeout_add(6000,(GtkFunction)linphone_gtk_stop_record,NULL);
		g_object_set_data(G_OBJECT(audio_assistant),"timeout_id",GINT_TO_POINTER(timeout_id));
		g_object_set_data(G_OBJECT(audio_assistant),"path",path);
	} else {
		stream = (AudioStream *)g_object_get_data(G_OBJECT(audio_assistant),"record_stream");
		timeout_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(audio_assistant),"timeout_id"));
		gtk_timeout_remove(timeout_id);
		if(stream != NULL){
			audio_stream_stop(stream);
			g_object_set_data(G_OBJECT(audio_assistant),"record_stream",NULL);
		}
		update_record_button(FALSE);
		update_play_button(TRUE);
	}
}

static void endoffile_cb(void *ud, MSFilter *f, unsigned int ev,void * arg){
	switch (ev) {
		case MS_PLAYER_EOF: {
			ms_message("EndOfFile received");
			/*workaround for a mediastreamer2 bug. Don't deactivate the play button, because it will stop the graph from the end of file callback,
			 * which is sometimes crashing. On master branch it is fixed in mediastreamer2, the workaround is only valid in 3.8.x branch*/
			g_timeout_add(0, (GSourceFunc)deactivate_play_button, NULL);
			break;
		}
		break;
	}
}

void linphone_gtk_start_play_record_sound(GtkWidget *w,gpointer data){
	LinphoneCore *lc = linphone_gtk_get_core();
	MSFactory *factory = linphone_core_get_ms_factory(lc);
	gboolean active=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	AudioStream *stream = NULL;
	MSSndCardManager *manager = ms_factory_get_snd_card_manager(factory);

	if(active){
		gchar *path = g_object_get_data(G_OBJECT(audio_assistant),"path");
		stream=audio_stream_new(factory, 8888, 8889, FALSE);
		if(path != NULL){
			audio_stream_start_full(stream,&av_profile,"127.0.0.1",8888,"127.0.0.1",8889,0,0,path,
				NULL,ms_snd_card_manager_get_card(manager,linphone_core_get_playback_device(lc)),NULL,FALSE);
			ms_filter_add_notify_callback(stream->soundread,endoffile_cb,stream,FALSE);
			g_object_set_data(G_OBJECT(audio_assistant),"play_stream",stream);
		}
	} else {
		stream = (AudioStream *)g_object_get_data(G_OBJECT(audio_assistant),"play_stream");
		if(stream != NULL){
			audio_stream_stop(stream);
			g_object_set_data(G_OBJECT(audio_assistant),"play_stream",NULL);
		}
	}
}

void display_popup(GtkMessageType type,const gchar *message){
	GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(audio_assistant),
                                  GTK_DIALOG_DESTROY_WITH_PARENT,
                                  type,
                                  GTK_BUTTONS_CLOSE,
                                  "%s",
                                  (const gchar*)message);
	/* Destroy the dialog when the user responds to it (e.g. clicks a button) */
	g_signal_connect_swapped (G_OBJECT (dialog), "response",
                   G_CALLBACK (gtk_widget_destroy),
                   G_OBJECT (dialog));
	gtk_widget_show(dialog);
}

static void open_mixer(void){
	GError *error = NULL;

#ifdef _WIN32
	if(!g_spawn_command_line_async("control mmsys.cpl",&error)){
		display_popup(GTK_MESSAGE_WARNING,_("Sound preferences not found "));
		g_error_free(error);
	}
#elif __APPLE__
	if(!g_spawn_command_line_async("open /System/Library/PreferencePanes/Sound.prefPane",&error)){
		display_popup(GTK_MESSAGE_WARNING,_("Sound preferences not found "));
		g_error_free(error);
	}
#else
	if(!g_spawn_command_line_async("gnome-volume-control",&error)){
		if(!g_spawn_command_line_async("gnome-control-center sound",&error)){
			if(!g_spawn_command_line_async("kmix",&error)){
				if(!g_spawn_command_line_async("mate-volume-control",&error)){
					if(!g_spawn_command_line_async("xterm alsamixer",&error)){
						display_popup(GTK_MESSAGE_WARNING,_("Cannot launch system sound control "));
						g_error_free(error);
					}
				}
			}
		}
	}
#endif
}

static GtkWidget *create_intro(void){
	GtkWidget *vbox=gtk_vbox_new(FALSE,2);
	GtkWidget *label=gtk_label_new(_("Welcome!\nThis assistant will help you to configure audio settings for Linphone"));
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 2);
	gtk_widget_show_all(vbox);
	return vbox;
}

static GtkWidget *create_mic_page(void){
	GtkWidget *vbox=gtk_table_new(3,2,FALSE);
	LinphoneCore *lc=linphone_gtk_get_core();
	const char **sound_devices;
	GtkWidget *labelMicChoice=gtk_label_new(_("Capture device"));
	GtkWidget *labelMicLevel=gtk_label_new(_("Recorded volume"));
	GtkWidget *mic_audiolevel=gtk_progress_bar_new();
	GtkWidget *capture_device=gtk_combo_box_new();
	GtkWidget *box = gtk_vbox_new(FALSE,0);
	GtkWidget *label_audiolevel=gtk_label_new(_("No voice"));
	GtkWidget *mixer_button=gtk_button_new_with_label(_("System sound preferences"));
	GtkWidget *image;

	image=gtk_image_new_from_stock(GTK_STOCK_PREFERENCES,GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(mixer_button),image);

	gtk_box_pack_start(GTK_BOX(box),mic_audiolevel,TRUE,TRUE,1);
	gtk_box_pack_start(GTK_BOX(box),label_audiolevel,FALSE,FALSE,1);

	gtk_table_attach_defaults(GTK_TABLE(vbox), labelMicChoice, 0, 1, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(vbox), capture_device, 1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelMicLevel, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(vbox), box, 1, 2, 1, 2);
	gtk_table_attach(GTK_TABLE(vbox),  mixer_button, 0, 2, 2, 3, GTK_SHRINK, GTK_SHRINK, 0,0);

	gtk_table_set_row_spacings(GTK_TABLE(vbox),10);

	set_widget_to_assistant("mic_audiolevel",mic_audiolevel);
	set_widget_to_assistant("label_audiolevel",label_audiolevel);

	sound_devices=linphone_core_get_sound_devices(lc);
	linphone_gtk_fill_combo_box(capture_device, sound_devices,
					linphone_core_get_capture_device(lc), CAP_CAPTURE);
	gtk_widget_show_all(vbox);

	g_signal_connect(G_OBJECT(capture_device),"changed",(GCallback)capture_device_changed,capture_device);
	g_signal_connect(G_OBJECT(mixer_button),"clicked",(GCallback)open_mixer,vbox);

	return vbox;
}

static GtkWidget *create_speaker_page(void){
	GtkWidget *vbox=gtk_table_new(3,2,FALSE);
	LinphoneCore *lc=linphone_gtk_get_core();

	GtkWidget *labelSpeakerChoice=gtk_label_new(_("Playback device"));
	GtkWidget *labelSpeakerLevel=gtk_label_new(_("Play three beeps"));
	GtkWidget *spk_button=gtk_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
	GtkWidget *playback_device=gtk_combo_box_new();
	GtkWidget *mixer_button=gtk_button_new_with_label(_("System sound preferences"));
	GtkWidget *image;
	const char **sound_devices;

	image=gtk_image_new_from_stock(GTK_STOCK_PREFERENCES,GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(mixer_button),image);

	gtk_table_attach_defaults(GTK_TABLE(vbox), labelSpeakerChoice, 0, 1, 0, 1);
	gtk_table_attach(GTK_TABLE(vbox), playback_device, 1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0,0);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelSpeakerLevel, 0, 1, 1, 2);
	gtk_table_attach(GTK_TABLE(vbox), spk_button, 1, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 0,0);
	gtk_table_attach(GTK_TABLE(vbox), mixer_button, 0, 2, 2, 3, GTK_SHRINK, GTK_SHRINK, 0,0);

	gtk_table_set_row_spacings(GTK_TABLE(vbox),10);

	sound_devices=linphone_core_get_sound_devices(lc);
	linphone_gtk_fill_combo_box(playback_device, sound_devices,
					linphone_core_get_playback_device(lc),CAP_PLAYBACK);
	gtk_widget_show_all(vbox);

	set_widget_to_assistant("speaker_page",vbox);
	g_signal_connect(G_OBJECT(playback_device),"changed",(GCallback)playback_device_changed,playback_device);
	g_signal_connect(G_OBJECT(spk_button),"clicked",(GCallback)linphone_gtk_start_sound,vbox);
	g_signal_connect(G_OBJECT(mixer_button),"clicked",(GCallback)open_mixer,vbox);

	return vbox;
}

static GtkWidget *create_play_record_page(void){
	GtkWidget *vbox=gtk_table_new(2,2,FALSE);
	GtkWidget *labelRecord=gtk_label_new(_("Press the record button and say some words"));
	GtkWidget *labelPlay=gtk_label_new(_("Listen to your record voice"));
	GtkWidget *rec_button=gtk_toggle_button_new_with_label(_("Record"));
	GtkWidget *play_button=gtk_toggle_button_new_with_label(_("Play"));
	GtkWidget *image;

	image=gtk_image_new_from_stock(GTK_STOCK_MEDIA_RECORD,GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(rec_button),image);

	image=gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY,GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(play_button),image);
	gtk_widget_set_sensitive(play_button,FALSE);

	gtk_table_attach_defaults(GTK_TABLE(vbox), labelRecord, 0, 1, 0, 1);
	gtk_table_attach(GTK_TABLE(vbox), rec_button, 1, 2, 0, 1, GTK_SHRINK, GTK_SHRINK, 0,0);
	gtk_table_attach_defaults(GTK_TABLE(vbox), labelPlay, 0, 1, 1, 2);
	gtk_table_attach(GTK_TABLE(vbox),  play_button, 1, 2, 1, 2, GTK_SHRINK, GTK_SHRINK, 0,0);

	gtk_widget_show_all(vbox);

	set_widget_to_assistant("rec_button",rec_button);
	set_widget_to_assistant("play_button",play_button);
	g_signal_connect(G_OBJECT(rec_button),"toggled",(GCallback)linphone_gtk_start_record_sound,vbox);
	g_signal_connect(G_OBJECT(play_button),"toggled",(GCallback)linphone_gtk_start_play_record_sound,vbox);

	return vbox;
}

static GtkWidget *create_end_page(void){
	GtkWidget *vbox=gtk_vbox_new(FALSE,2);
	GtkWidget *label=gtk_label_new(_("Let's start Linphone now"));
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 2);
	gtk_widget_show_all(vbox);
	return vbox;
}

static void prepare(GtkAssistant *w){
	AudioStream *audio_stream = NULL;
	LinphoneCore *lc=linphone_gtk_get_core();
	MSFactory *factory = linphone_core_get_ms_factory(lc);
	int page = gtk_assistant_get_current_page(w);
	GtkWidget *mic_audiolevel = get_widget_from_assistant("mic_audiolevel");
	GtkWidget *label_audiolevel = get_widget_from_assistant("label_audiolevel");

	//Speaker page
	if(page == 1){
		MSSndCardManager *manager =  ms_factory_get_snd_card_manager(factory);
		audio_stream = audio_stream_start_with_sndcards(factory, &av_profile,9898,"127.0.0.1",19898,0,0,ms_snd_card_manager_get_card(manager,linphone_core_get_playback_device(lc)),ms_snd_card_manager_get_card(manager,linphone_core_get_capture_device(lc)),FALSE);
		if (mic_audiolevel != NULL && audio_stream != NULL){
			g_object_set_data(G_OBJECT(audio_assistant),"stream",audio_stream);
			linphone_gtk_init_audio_meter(mic_audiolevel,(get_volume_t)audio_stream_get_record_volume,audio_stream);
			linphone_gtk_init_audio_label(label_audiolevel,(get_volume_t)audio_stream_get_max_volume,audio_stream);
		}
	} else if(page == 2 || page == 0){
		if(mic_audiolevel != NULL && label_audiolevel != NULL){
			audio_stream = (AudioStream *)g_object_get_data(G_OBJECT(audio_assistant),"stream");
			if(audio_stream != NULL){
				linphone_gtk_uninit_audio_meter(mic_audiolevel);
				linphone_gtk_uninit_audio_label(label_audiolevel);
				audio_stream_stop(audio_stream);
				g_object_set_data(G_OBJECT(audio_assistant),"stream",NULL);
			}
		}
	}
}

void linphone_gtk_close_audio_assistant(GtkWidget *w){
	gchar *path;
	AudioStream *stream;

	path = g_object_get_data(G_OBJECT(audio_assistant),"path");
	if(path != NULL){
		g_unlink(path);
	}
	stream = (AudioStream *)g_object_get_data(G_OBJECT(audio_assistant), "stream");
	if(stream) {
		audio_stream_stop(stream);
	}
	gtk_widget_destroy(w);
	if(linphone_gtk_get_audio_assistant_option()){
		gtk_main_quit();
	}
	audio_assistant = NULL;
}

void linphone_gtk_audio_assistant_apply(GtkWidget *w){
	linphone_gtk_close_audio_assistant(w);
}

void linphone_gtk_show_audio_assistant(void){
	GtkWidget *w;
	GtkWidget *welcome;
	GtkWidget *mic_page;
	GtkWidget *speaker_page;
	GtkWidget *play_record_page;
	GtkWidget *end_page;
	if(audio_assistant!=NULL)
		return;
	w=audio_assistant=linphone_gtk_create_window("audio_assistant", linphone_gtk_get_main_window());

	gtk_window_set_resizable (GTK_WINDOW(w), FALSE);
	gtk_window_set_title(GTK_WINDOW(w),_("Audio Assistant"));

	welcome=create_intro();
	mic_page=create_mic_page();
	speaker_page=create_speaker_page();
	play_record_page=create_play_record_page();
	end_page=create_end_page();

	gtk_assistant_append_page(GTK_ASSISTANT(w),welcome);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),welcome,GTK_ASSISTANT_PAGE_INTRO);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),welcome,_("Audio assistant"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w),welcome,TRUE);

	gtk_assistant_append_page(GTK_ASSISTANT(w),mic_page);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),mic_page,GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),mic_page,_("Mic Gain calibration"));
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w),mic_page,TRUE);

	gtk_assistant_append_page(GTK_ASSISTANT(w),speaker_page);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),speaker_page,GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w),speaker_page,FALSE);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),speaker_page,_("Speaker volume calibration"));

	gtk_assistant_append_page(GTK_ASSISTANT(w),play_record_page);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),play_record_page,GTK_ASSISTANT_PAGE_CONTENT);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w),play_record_page,TRUE);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),play_record_page,_("Record and Play"));

	gtk_assistant_append_page(GTK_ASSISTANT(w),end_page);
	gtk_assistant_set_page_type(GTK_ASSISTANT(w),end_page,GTK_ASSISTANT_PAGE_SUMMARY);
	gtk_assistant_set_page_complete(GTK_ASSISTANT(w),end_page,TRUE);
	gtk_assistant_set_page_title(GTK_ASSISTANT(w),end_page,_("Terminating"));

 	g_signal_connect(G_OBJECT(w),"close",(GCallback)linphone_gtk_close_audio_assistant,w);
 	g_signal_connect(G_OBJECT(w),"cancel",(GCallback)linphone_gtk_close_audio_assistant,w);
 	g_signal_connect(G_OBJECT(w),"prepare",(GCallback)prepare,NULL);

	gtk_widget_show(w);
}
