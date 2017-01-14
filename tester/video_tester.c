/*
	liblinphone_tester - liblinphone test suite
	Copyright (C) 2013  Belledonne Communications SARL

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "private.h"

#if defined(VIDEO_ENABLED)


#include "linphone/core.h"
#include "liblinphone_tester.h"
#include "linphone/lpconfig.h"

#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic push
#endif
#ifndef _MSC_VER
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
#endif

#if HAVE_GTK
#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#elif defined(_WIN32)
#include <gdk/gdkwin32.h>
#elif defined(__APPLE__)
extern void *gdk_quartz_window_get_nswindow(GdkWindow      *window);
extern void *gdk_quartz_window_get_nsview(GdkWindow      *window);
#endif

#include <gdk/gdkkeysyms.h>

#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic pop
#endif


static void *get_native_handle(GdkWindow *gdkw) {
#ifdef GDK_WINDOWING_X11
	return (void *)GDK_WINDOW_XID(gdkw);
#elif defined(_WIN32)
	return (void *)GDK_WINDOW_HWND(gdkw);
#elif defined(__APPLE__)
	return (void *)gdk_quartz_window_get_nsview(gdkw);
#endif
	g_warning("No way to get the native handle from gdk window");
	return 0;
}

static GtkWidget *create_video_window(LinphoneCall *call, LinphoneCallState cstate) {
	GtkWidget *video_window;
	GdkDisplay *display;
	GdkColor color;
	MSVideoSize vsize = { MS_VIDEO_SIZE_CIF_W, MS_VIDEO_SIZE_CIF_H };
	const char *cstate_str;
	char *title;
	stats* counters = get_stats(call->core);

	cstate_str = linphone_call_state_to_string(cstate);
	title = g_strdup_printf("%s", cstate_str);
	video_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(video_window), title);
	g_free(title);
	gtk_window_resize(GTK_WINDOW(video_window), vsize.width, vsize.height);
	gdk_color_parse("black", &color);
	gtk_widget_modify_bg(video_window, GTK_STATE_NORMAL, &color);
	gtk_widget_show(video_window);
	g_object_set_data(G_OBJECT(video_window), "call", call);
#if GTK_CHECK_VERSION(2,24,0)
	display = gdk_window_get_display(gtk_widget_get_window(video_window));
#else // backward compatibility with Debian 6 and Centos 6
	display = gdk_drawable_get_display(gtk_widget_get_window(video_window));
#endif
	gdk_display_flush(display);
	counters->number_of_video_windows_created++;
	return video_window;
}

static void show_video_window(LinphoneCall *call, LinphoneCallState cstate) {
	GtkWidget *video_window = (GtkWidget *)linphone_call_get_user_data(call);
	if (video_window == NULL) {
		video_window = create_video_window(call, cstate);
		linphone_call_set_user_data(call, video_window);
		linphone_call_set_native_video_window_id(call, get_native_handle(gtk_widget_get_window(video_window)));
	}
}

static void hide_video_video(LinphoneCall *call) {
	GtkWidget *video_window = (GtkWidget *)linphone_call_get_user_data(call);
	if (video_window != NULL) {
		gtk_widget_destroy(video_window);
		linphone_call_set_user_data(call, NULL);
		linphone_call_set_native_video_window_id(call, 0);
	}
}

static void video_call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg) {
	switch (cstate) {
		case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallOutgoingEarlyMedia:
		case LinphoneCallConnected:
			show_video_window(call, cstate);
			break;
		case LinphoneCallEnd:
			hide_video_video(call);
			break;
		default:
			break;
	}
}

static void early_media_video_call_state_changed(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg) {
	LinphoneCallParams *params;

	video_call_state_changed(lc, call, cstate, msg);
	switch (cstate) {
		case LinphoneCallIncomingReceived:
			params = linphone_core_create_call_params(lc, call);
			linphone_call_params_enable_video(params, TRUE);
			linphone_call_params_set_audio_direction(params, LinphoneMediaDirectionSendOnly);
			linphone_call_params_set_video_direction(params, LinphoneMediaDirectionRecvOnly);
			linphone_core_accept_early_media_with_params(lc, call, params);
			linphone_call_params_unref(params);
			break;
		default:
			break;
	}
}

static void early_media_video_call_state_changed_with_inactive_audio(LinphoneCore *lc, LinphoneCall *call, LinphoneCallState cstate, const char *msg) {
	LinphoneCallParams *params;

	video_call_state_changed(lc, call, cstate, msg);
	switch (cstate) {
		case LinphoneCallIncomingReceived:
			params = linphone_core_create_call_params(lc, call);
			linphone_call_params_enable_video(params, TRUE);
			linphone_call_params_set_audio_direction(params, LinphoneMediaDirectionInactive);
			linphone_call_params_set_video_direction(params, LinphoneMediaDirectionRecvOnly);
			linphone_core_accept_early_media_with_params(lc, call, params);
			linphone_call_params_unref(params);
			break;
		default:
			break;
	}
}

bool_t wait_for_three_cores(LinphoneCore *lc1, LinphoneCore *lc2, LinphoneCore *lc3, int timeout) {
	bctbx_list_t *lcs = NULL;
	bool_t result;
	int dummy = 0;
	if (lc1) lcs = bctbx_list_append(lcs, lc1);
	if (lc2) lcs = bctbx_list_append(lcs, lc2);
	if (lc3) lcs = bctbx_list_append(lcs, lc3);
	result = wait_for_list(lcs, &dummy, 1, timeout);
	bctbx_list_free(lcs);
	return result;
}

static bool_t video_call_with_params(LinphoneCoreManager* caller_mgr, LinphoneCoreManager* callee_mgr, const LinphoneCallParams *caller_params, const LinphoneCallParams *callee_params, bool_t automatically_accept) {
	int retry = 0;
	stats initial_caller = caller_mgr->stat;
	stats initial_callee = callee_mgr->stat;
	bool_t result = TRUE;
	bool_t did_received_call;

	BC_ASSERT_PTR_NOT_NULL(linphone_core_invite_address_with_params(caller_mgr->lc, callee_mgr->identity, caller_params));
	did_received_call = wait_for(callee_mgr->lc, caller_mgr->lc, &callee_mgr->stat.number_of_LinphoneCallIncomingReceived, initial_callee.number_of_LinphoneCallIncomingReceived + 1);
	if (!did_received_call) return 0;

	BC_ASSERT_EQUAL(caller_mgr->stat.number_of_LinphoneCallOutgoingProgress, initial_caller.number_of_LinphoneCallOutgoingProgress + 1, int, "%d");

	while (caller_mgr->stat.number_of_LinphoneCallOutgoingRinging != (initial_caller.number_of_LinphoneCallOutgoingRinging + 1)
		&& caller_mgr->stat.number_of_LinphoneCallOutgoingEarlyMedia != (initial_caller.number_of_LinphoneCallOutgoingEarlyMedia + 1)
		&& retry++ < 200) {
		linphone_core_iterate(caller_mgr->lc);
		linphone_core_iterate(callee_mgr->lc);
		ms_usleep(10000);
	}

	BC_ASSERT_TRUE((caller_mgr->stat.number_of_LinphoneCallOutgoingRinging == initial_caller.number_of_LinphoneCallOutgoingRinging + 1)
		|| (caller_mgr->stat.number_of_LinphoneCallOutgoingEarlyMedia == initial_caller.number_of_LinphoneCallOutgoingEarlyMedia + 1));

	BC_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call_remote_address(callee_mgr->lc));
	if(!linphone_core_get_current_call(caller_mgr->lc) || !linphone_core_get_current_call(callee_mgr->lc) || !linphone_core_get_current_call_remote_address(callee_mgr->lc)) {
		return 0;
	}

	if (automatically_accept == TRUE) {
		linphone_core_accept_call_with_params(callee_mgr->lc, linphone_core_get_current_call(callee_mgr->lc), callee_params);

		BC_ASSERT_TRUE(wait_for(callee_mgr->lc, caller_mgr->lc, &callee_mgr->stat.number_of_LinphoneCallConnected, initial_callee.number_of_LinphoneCallConnected + 1));
		BC_ASSERT_TRUE(wait_for(callee_mgr->lc, caller_mgr->lc, &caller_mgr->stat.number_of_LinphoneCallConnected, initial_callee.number_of_LinphoneCallConnected + 1));
		result = wait_for(callee_mgr->lc, caller_mgr->lc, &caller_mgr->stat.number_of_LinphoneCallStreamsRunning, initial_caller.number_of_LinphoneCallStreamsRunning + 1)
			&& wait_for(callee_mgr->lc, caller_mgr->lc, &callee_mgr->stat.number_of_LinphoneCallStreamsRunning, initial_callee.number_of_LinphoneCallStreamsRunning + 1);
	}
	return result;
}

static LinphoneCallParams * _configure_for_video(LinphoneCoreManager *manager, LinphoneCoreCallStateChangedCb cb) {
	LinphoneCallParams *params;
	LinphoneCoreVTable *vtable = linphone_core_v_table_new();
	vtable->call_state_changed = cb;
	linphone_core_add_listener(manager->lc, vtable);
	linphone_core_set_video_device(manager->lc, "StaticImage: Static picture");
	linphone_core_enable_video_capture(manager->lc, TRUE);
	linphone_core_enable_video_display(manager->lc, TRUE);
	params = linphone_core_create_call_params(manager->lc, NULL);
	linphone_call_params_enable_video(params, TRUE);
	if (linphone_core_find_payload_type(manager->lc,"VP8", 90000, -1)!=NULL){
		disable_all_video_codecs_except_one(manager->lc, "VP8");
	}else{
		ms_warning("VP8 codec not available, will use MP4V-ES instead");
		disable_all_video_codecs_except_one(manager->lc, "MP4V-ES");
	}
	return params;
}

static LinphoneCallParams * configure_for_video(LinphoneCoreManager *manager) {
	return _configure_for_video(manager, video_call_state_changed);
}

static LinphoneCallParams * configure_for_early_media_video_sending(LinphoneCoreManager *manager) {
	LinphoneCallParams *params = _configure_for_video(manager, video_call_state_changed);
	linphone_call_params_enable_early_media_sending(params, TRUE);
	return params;
}

static LinphoneCallParams * configure_for_early_media_video_receiving(LinphoneCoreManager *manager) {
	return _configure_for_video(manager, early_media_video_call_state_changed);
}

static LinphoneCallParams * configure_for_early_media_video_receiving_with_inactive_audio(LinphoneCoreManager *manager) {
	return _configure_for_video(manager, early_media_video_call_state_changed_with_inactive_audio);
}


static void early_media_video_during_video_call_test(void) {
	LinphoneCoreManager *marie;
	LinphoneCoreManager *pauline;
	LinphoneCoreManager *laure;
	LinphoneCallParams *marie_params;
	LinphoneCallParams *pauline_params;
	LinphoneCallParams *laure_params;

	marie = linphone_core_manager_new("marie_rc");
	pauline = linphone_core_manager_new("pauline_tcp_rc");
	laure = linphone_core_manager_new("laure_rc_udp");
	marie_params = configure_for_early_media_video_receiving(marie);
	pauline_params = configure_for_video(pauline);
	laure_params = configure_for_early_media_video_sending(laure);

	/* Normal automatically accepted video call from marie to pauline. */
	BC_ASSERT_TRUE(video_call_with_params(marie, pauline, marie_params, pauline_params, TRUE));

	/* Wait for 2s. */
	wait_for_three_cores(marie->lc, pauline->lc, NULL, 2000);

	/* Early media video call from laure to marie. */
	BC_ASSERT_TRUE(video_call_with_params(laure, marie, laure_params, NULL, FALSE));

	/* Wait for 2s. */
	wait_for_three_cores(marie->lc, pauline->lc, laure->lc, 2000);

	linphone_core_terminate_all_calls(marie->lc);
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallEnd, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallEnd, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallReleased, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallReleased, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, laure->lc, &marie->stat.number_of_LinphoneCallEnd, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, laure->lc, &laure->stat.number_of_LinphoneCallEnd, 1));

	BC_ASSERT_EQUAL(marie->stat.number_of_video_windows_created, 2, int, "%d");
	BC_ASSERT_EQUAL(pauline->stat.number_of_video_windows_created, 1, int, "%d");
	BC_ASSERT_EQUAL(laure->stat.number_of_video_windows_created, 1, int, "%d");

	linphone_call_params_unref(marie_params);
	linphone_call_params_unref(pauline_params);
	linphone_call_params_unref(laure_params);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
}

static void two_incoming_early_media_video_calls_test(void) {
	char *ringback_path;
	LinphoneCoreManager *marie;
	LinphoneCoreManager *pauline;
	LinphoneCoreManager *laure;
	LinphoneCallParams *marie_params;
	LinphoneCallParams *pauline_params;
	LinphoneCallParams *laure_params;
	LinphoneCall *call;
	const bctbx_list_t *calls_list;

	marie = linphone_core_manager_new("marie_rc");
	pauline = linphone_core_manager_new("pauline_tcp_rc");
	laure = linphone_core_manager_new("laure_rc_udp");
	marie_params = configure_for_early_media_video_receiving(marie);
	pauline_params = configure_for_early_media_video_sending(pauline);
	laure_params = configure_for_early_media_video_sending(laure);

	/* Configure early media audio to play ring during early-media and send remote ring back tone. */
	linphone_core_set_ring_during_incoming_early_media(marie->lc, TRUE);
	ringback_path = bc_tester_res("sounds/ringback.wav");
	linphone_core_set_remote_ringback_tone(marie->lc, ringback_path);
	ms_free(ringback_path);

	/* Early media video call from pauline to marie. */
	BC_ASSERT_TRUE(video_call_with_params(pauline, marie, pauline_params, NULL, FALSE));

	/* Wait for 2s. */
	wait_for_three_cores(marie->lc, pauline->lc, NULL, 2000);

	/* Early media video call from laure to marie. */
	BC_ASSERT_TRUE(video_call_with_params(laure, marie, laure_params, NULL, FALSE));

	/* Wait for 2s. */
	wait_for_three_cores(marie->lc, pauline->lc, laure->lc, 2000);

	BC_ASSERT_EQUAL(linphone_core_get_calls_nb(marie->lc), 2, int, "%d");
	if (linphone_core_get_calls_nb(marie->lc) == 2) {
		calls_list = linphone_core_get_calls(marie->lc);
		call = (LinphoneCall *)bctbx_list_nth_data(calls_list, 0);
		BC_ASSERT_PTR_NOT_NULL(call);
		if (call != NULL) {
			LinphoneCallParams *params = linphone_call_params_copy(linphone_call_get_current_params(call));
			linphone_call_params_set_audio_direction(params, LinphoneMediaDirectionSendRecv);
			linphone_call_params_set_video_direction(params, LinphoneMediaDirectionSendRecv);
			linphone_core_accept_call_with_params(marie->lc, call, params);
			linphone_call_params_unref(params);

			/* Wait for 5s. */
			wait_for_three_cores(marie->lc, pauline->lc, laure->lc, 5000);
		}
	}

	linphone_core_terminate_all_calls(marie->lc);
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallEnd, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallEnd, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallReleased, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallReleased, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, laure->lc, &marie->stat.number_of_LinphoneCallEnd, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, laure->lc, &laure->stat.number_of_LinphoneCallEnd, 1));

	BC_ASSERT_EQUAL(marie->stat.number_of_video_windows_created, 2, int, "%d");
	BC_ASSERT_EQUAL(pauline->stat.number_of_video_windows_created, 1, int, "%d");
	BC_ASSERT_EQUAL(laure->stat.number_of_video_windows_created, 1, int, "%d");

	linphone_call_params_unref(marie_params);
	linphone_call_params_unref(pauline_params);
	linphone_call_params_unref(laure_params);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
	linphone_core_manager_destroy(laure);
}

static void early_media_video_with_inactive_audio(void) {
	LinphoneCoreManager *marie;
	LinphoneCoreManager *pauline;
	LinphoneCallParams *marie_params;
	LinphoneCallParams *pauline_params;

	marie = linphone_core_manager_new("marie_rc");
	pauline = linphone_core_manager_new("pauline_tcp_rc");
	marie_params = configure_for_early_media_video_receiving_with_inactive_audio(marie);
	pauline_params = configure_for_early_media_video_sending(pauline);

	/* Early media video call from pauline to marie. */
	BC_ASSERT_TRUE(video_call_with_params(pauline, marie, pauline_params, NULL, FALSE));

	/* Wait for 2s. */
	wait_for_three_cores(marie->lc, pauline->lc, NULL, 2000);

	/* Check that we are in LinphoneCallOutgoingEarlyMedia state and that the ringstream is present meaning we are playing the ringback tone. */
	BC_ASSERT_EQUAL(linphone_call_get_state(linphone_core_get_current_call(pauline->lc)), LinphoneCallOutgoingEarlyMedia, int, "%d");
	BC_ASSERT_PTR_NOT_NULL(pauline->lc->ringstream);

	linphone_core_terminate_all_calls(marie->lc);
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallEnd, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallEnd, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallReleased, 1));
	BC_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallReleased, 1));

	BC_ASSERT_EQUAL(marie->stat.number_of_video_windows_created, 1, int, "%d");
	BC_ASSERT_EQUAL(pauline->stat.number_of_video_windows_created, 1, int, "%d");

	linphone_call_params_unref(marie_params);
	linphone_call_params_unref(pauline_params);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void forked_outgoing_early_media_video_call_with_inactive_audio_test(void) {
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_tcp_rc");
	LinphoneCoreManager *marie1 = linphone_core_manager_new("marie_early_rc");
	LinphoneCoreManager *marie2 = linphone_core_manager_new("marie_early_rc");
	bctbx_list_t *lcs = NULL;
	LinphoneCallParams *pauline_params;
	LinphoneCallParams *marie1_params;
	LinphoneCallParams *marie2_params;
	LinphoneVideoPolicy pol;
	LinphoneCall *marie1_call;
	LinphoneCall *marie2_call;
	LinphoneCall *pauline_call;
	LinphoneInfoMessage *info;
	int dummy = 0;
	pol.automatically_accept = 1;
	pol.automatically_initiate = 1;

	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, TRUE);
	linphone_core_enable_video_capture(marie1->lc, TRUE);
	linphone_core_enable_video_display(marie1->lc, TRUE);
	linphone_core_set_video_policy(marie1->lc, &pol);
	linphone_core_enable_video_capture(marie2->lc, TRUE);
	linphone_core_enable_video_display(marie2->lc, TRUE);
	linphone_core_set_video_policy(marie2->lc, &pol);
	linphone_core_set_audio_port_range(marie2->lc, 40200, 40300);
	linphone_core_set_video_port_range(marie2->lc, 40400, 40500);

	lcs = bctbx_list_append(lcs,marie1->lc);
	lcs = bctbx_list_append(lcs,marie2->lc);
	lcs = bctbx_list_append(lcs,pauline->lc);

	pauline_params = linphone_core_create_call_params(pauline->lc, NULL);
	linphone_call_params_enable_early_media_sending(pauline_params, TRUE);
	linphone_call_params_enable_video(pauline_params, TRUE);
	marie1_params = configure_for_early_media_video_receiving_with_inactive_audio(marie1);
	marie2_params = configure_for_early_media_video_receiving_with_inactive_audio(marie2);

	linphone_core_invite_address_with_params(pauline->lc, marie1->identity, pauline_params);
	linphone_call_params_unref(pauline_params);

	BC_ASSERT_TRUE(wait_for_list(lcs, &marie1->stat.number_of_LinphoneCallIncomingReceived, 1, 3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie2->stat.number_of_LinphoneCallIncomingReceived, 1, 3000));

	marie1_call = linphone_core_get_current_call(marie1->lc);
	marie2_call = linphone_core_get_current_call(marie2->lc);

	if (marie1_call){
		linphone_call_set_next_video_frame_decoded_callback(marie1_call, linphone_call_iframe_decoded_cb, marie1->lc);
	}
	if (marie2_call){
		linphone_call_set_next_video_frame_decoded_callback(marie2_call, linphone_call_iframe_decoded_cb, marie2->lc);
	}

	BC_ASSERT_TRUE(wait_for_list(lcs, &marie1->stat.number_of_LinphoneCallIncomingEarlyMedia, 1, 3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie2->stat.number_of_LinphoneCallIncomingEarlyMedia, 1, 3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallOutgoingEarlyMedia, 1, 3000));

	pauline_call = linphone_core_get_current_call(pauline->lc);

	BC_ASSERT_PTR_NOT_NULL(pauline_call);
	BC_ASSERT_PTR_NOT_NULL(marie1_call);
	BC_ASSERT_PTR_NOT_NULL(marie2_call);

	if (pauline_call && marie1_call && marie2_call) {
		linphone_call_set_next_video_frame_decoded_callback(pauline_call, linphone_call_iframe_decoded_cb, pauline->lc);

		/* wait a bit that streams are established */
		wait_for_list(lcs, &dummy, 1, 3000);
		BC_ASSERT_EQUAL(linphone_call_get_audio_stats(pauline_call)->download_bandwidth, 0, float, "%f");
		BC_ASSERT_EQUAL(linphone_call_get_audio_stats(marie1_call)->download_bandwidth, 0, float, "%f");
		BC_ASSERT_EQUAL(linphone_call_get_audio_stats(marie2_call)->download_bandwidth, 0, float, "%f");
		BC_ASSERT_LOWER(linphone_call_get_video_stats(pauline_call)->download_bandwidth, 11, float, "%f"); /* because of stun packets*/
		BC_ASSERT_GREATER(linphone_call_get_video_stats(marie1_call)->download_bandwidth, 0, float, "%f");
		BC_ASSERT_GREATER(linphone_call_get_video_stats(marie2_call)->download_bandwidth, 0, float, "%f");
		BC_ASSERT_GREATER(marie1->stat.number_of_IframeDecoded, 1, int, "%i");
		BC_ASSERT_GREATER(marie2->stat.number_of_IframeDecoded, 1, int, "%i");

		linphone_call_params_set_audio_direction(marie1_params, LinphoneMediaDirectionSendRecv);
		linphone_core_accept_call_with_params(marie1->lc, linphone_core_get_current_call(marie1->lc), marie1_params);
		BC_ASSERT_TRUE(wait_for_list(lcs, &marie1->stat.number_of_LinphoneCallStreamsRunning, 1, 3000));
		BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallStreamsRunning, 1, 3000));

		/* marie2 should get her call terminated */
		BC_ASSERT_TRUE(wait_for_list(lcs, &marie2->stat.number_of_LinphoneCallEnd, 1, 1000));

		/*wait a bit that streams are established*/
		wait_for_list(lcs, &dummy, 1, 3000);
		BC_ASSERT_GREATER(linphone_call_get_audio_stats(pauline_call)->download_bandwidth, 71, float, "%f");
		BC_ASSERT_GREATER(linphone_call_get_audio_stats(marie1_call)->download_bandwidth, 71, float, "%f");
		BC_ASSERT_GREATER(linphone_call_get_video_stats(pauline_call)->download_bandwidth, 0, float, "%f");
		BC_ASSERT_GREATER(linphone_call_get_video_stats(marie1_call)->download_bandwidth, 0, float, "%f");
		BC_ASSERT_GREATER(pauline->stat.number_of_IframeDecoded, 1, int, "%i");

		/* send an INFO in reverse side to check that dialogs are properly established */
		info = linphone_core_create_info_message(marie1->lc);
		linphone_call_send_info_message(marie1_call, info);
		BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_inforeceived, 1, 3000));
	}

	linphone_core_terminate_all_calls(pauline->lc);
	BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphoneCallEnd, 1, 3000));
	BC_ASSERT_TRUE(wait_for_list(lcs, &marie1->stat.number_of_LinphoneCallEnd, 1, 3000));

	linphone_call_params_unref(marie1_params);
	linphone_call_params_unref(marie2_params);
	bctbx_list_free(lcs);
	linphone_core_manager_destroy(marie1);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(pauline);
}
#endif /*HAVE_GTK*/

static void enable_disable_camera_after_camera_switches(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	const char *currentCamId = (char*)linphone_core_get_video_device(marie->lc);
	const char **cameras=linphone_core_get_video_devices(marie->lc);
	const char *newCamId=NULL;
	int i;


	video_call_base_2(marie,pauline,FALSE,LinphoneMediaEncryptionNone,TRUE,TRUE);

	for (i=0;cameras[i]!=NULL;++i){
		if (strcmp(cameras[i],currentCamId)!=0){
			newCamId=cameras[i];
			break;
		}
	}
	if (newCamId){
		LinphoneCall *call = linphone_core_get_current_call(marie->lc);
		ms_message("Switching from [%s] to [%s]", currentCamId, newCamId);
		linphone_core_set_video_device(marie->lc, newCamId);
		if(call != NULL) {
			linphone_core_update_call(marie->lc, call, NULL);
		}
		BC_ASSERT_STRING_EQUAL(newCamId,ms_web_cam_get_string_id(linphone_call_get_video_device(call)));
		linphone_call_enable_camera(call,FALSE);
		linphone_core_iterate(marie->lc);
		linphone_call_enable_camera(call,TRUE);
		BC_ASSERT_STRING_EQUAL(newCamId,ms_web_cam_get_string_id(linphone_call_get_video_device(call)));
	}


	linphone_core_terminate_all_calls(pauline->lc);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}
test_t video_tests[] = {
#if HAVE_GTK
	TEST_NO_TAG("Early-media video during video call", early_media_video_during_video_call_test),
	TEST_NO_TAG("Two incoming early-media video calls", two_incoming_early_media_video_calls_test),
	TEST_NO_TAG("Early-media video with inactive audio", early_media_video_with_inactive_audio),
	TEST_NO_TAG("Forked outgoing early-media video call with inactive audio", forked_outgoing_early_media_video_call_with_inactive_audio_test),
#endif /*HAVE_GTK*/
	TEST_NO_TAG("Enable/disable camera after camera switches", enable_disable_camera_after_camera_switches)

};

test_suite_t video_test_suite = {"Video", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
								 sizeof(video_tests) / sizeof(video_tests[0]), video_tests};

#endif /* VIDEO_ENABLED */
