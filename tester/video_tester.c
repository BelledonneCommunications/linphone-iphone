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

#if defined(VIDEO_ENABLED) && defined(HAVE_GTK)

#include <stdio.h>
#include "CUnit/Basic.h"
#include "linphonecore.h"
#include "liblinphone_tester.h"
#include "lpconfig.h"


#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#elif defined(WIN32)
#include <gdk/gdkwin32.h>
#elif defined(__APPLE__)
extern void *gdk_quartz_window_get_nswindow(GdkWindow      *window);
extern void *gdk_quartz_window_get_nsview(GdkWindow      *window);
#endif

#include <gdk/gdkkeysyms.h>


static unsigned long get_native_handle(GdkWindow *gdkw) {
#ifdef GDK_WINDOWING_X11
	return (unsigned long)GDK_WINDOW_XID(gdkw);
#elif defined(WIN32)
	return (unsigned long)GDK_WINDOW_HWND(gdkw);
#elif defined(__APPLE__)
	return (unsigned long)gdk_quartz_window_get_nsview(gdkw);
#endif
	g_warning("No way to get the native handle from gdk window");
	return 0;
}

static GtkWidget *create_video_window(LinphoneCall *call) {
	GtkWidget *video_window;
	GdkDisplay *display;
	GdkColor color;
	MSVideoSize vsize = MS_VIDEO_SIZE_CIF;
	
	video_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
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
	return video_window;
}

static void show_video_window(LinphoneCall *call) {
	GtkWidget *video_window = (GtkWidget *)linphone_call_get_user_data(call);
	if (video_window == NULL) {
		video_window = create_video_window(call);
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
		case LinphoneCallConnected:
			show_video_window(call);
			break;
		case LinphoneCallEnd:
			hide_video_video(call);
			break;
		default:
			break;
	}
}

static bool_t video_call_with_params(LinphoneCoreManager* caller_mgr, LinphoneCoreManager* callee_mgr, const LinphoneCallParams *caller_params, const LinphoneCallParams *callee_params) {
	int retry = 0;
	stats initial_caller = caller_mgr->stat;
	stats initial_callee = callee_mgr->stat;
	bool_t result = FALSE;
	bool_t did_received_call;

	CU_ASSERT_PTR_NOT_NULL(linphone_core_invite_address_with_params(caller_mgr->lc, callee_mgr->identity, caller_params));
	did_received_call = wait_for(callee_mgr->lc, caller_mgr->lc, &callee_mgr->stat.number_of_LinphoneCallIncomingReceived, initial_callee.number_of_LinphoneCallIncomingReceived + 1);
	if (!did_received_call) return 0;

	CU_ASSERT_TRUE(linphone_core_inc_invite_pending(callee_mgr->lc));
	CU_ASSERT_EQUAL(caller_mgr->stat.number_of_LinphoneCallOutgoingProgress, initial_caller.number_of_LinphoneCallOutgoingProgress + 1);

	while (caller_mgr->stat.number_of_LinphoneCallOutgoingRinging != (initial_caller.number_of_LinphoneCallOutgoingRinging + 1)
		&& caller_mgr->stat.number_of_LinphoneCallOutgoingEarlyMedia != (initial_caller.number_of_LinphoneCallOutgoingEarlyMedia + 1)
		&& retry++ < 20) {
		linphone_core_iterate(caller_mgr->lc);
		linphone_core_iterate(callee_mgr->lc);
		ms_usleep(100000);
	}

	CU_ASSERT_TRUE((caller_mgr->stat.number_of_LinphoneCallOutgoingRinging == initial_caller.number_of_LinphoneCallOutgoingRinging + 1)
		|| (caller_mgr->stat.number_of_LinphoneCallOutgoingEarlyMedia == initial_caller.number_of_LinphoneCallOutgoingEarlyMedia + 1));

	CU_ASSERT_PTR_NOT_NULL(linphone_core_get_current_call_remote_address(callee_mgr->lc));
	if(!linphone_core_get_current_call(caller_mgr->lc) || !linphone_core_get_current_call(callee_mgr->lc) || !linphone_core_get_current_call_remote_address(callee_mgr->lc)) {
		return 0;
	}

	linphone_core_accept_call_with_params(callee_mgr->lc, linphone_core_get_current_call(callee_mgr->lc), callee_params);

	CU_ASSERT_TRUE(wait_for(callee_mgr->lc, caller_mgr->lc, &callee_mgr->stat.number_of_LinphoneCallConnected, initial_callee.number_of_LinphoneCallConnected + 1));
	CU_ASSERT_TRUE(wait_for(callee_mgr->lc, caller_mgr->lc, &caller_mgr->stat.number_of_LinphoneCallConnected, initial_callee.number_of_LinphoneCallConnected + 1));
	result = wait_for(callee_mgr->lc, caller_mgr->lc, &caller_mgr->stat.number_of_LinphoneCallStreamsRunning, initial_caller.number_of_LinphoneCallStreamsRunning + 1)
		&& wait_for(callee_mgr->lc, caller_mgr->lc, &callee_mgr->stat.number_of_LinphoneCallStreamsRunning, initial_callee.number_of_LinphoneCallStreamsRunning + 1);
	return result;
}


static void early_media_video_during_video_call_test(void) {
	LinphoneCoreManager *marie;
	LinphoneCoreManager *pauline;
	LinphoneCallParams *marie_params;
	LinphoneCallParams *pauline_params;
	LinphoneCoreVTable *marie_vtable;
	LinphoneCoreVTable *pauline_vtable;
	int dummy = 0;

	marie = linphone_core_manager_new( "marie_rc");
	pauline = linphone_core_manager_new( "pauline_rc");
	marie_vtable = linphone_core_v_table_new();
	marie_vtable->call_state_changed = video_call_state_changed;
	linphone_core_add_listener(marie->lc, marie_vtable);
	linphone_core_set_video_device(marie->lc, "StaticImage: Static picture");
	//linphone_core_set_video_device(marie->lc, "V4L2: /dev/video0");
	linphone_core_enable_video_capture(marie->lc, TRUE);
	linphone_core_enable_video_display(marie->lc, TRUE);
	linphone_core_set_avpf_mode(marie->lc, LinphoneAVPFEnabled);
	marie_params = linphone_core_create_default_call_parameters(marie->lc);
	linphone_call_params_enable_video(marie_params, TRUE);
	disable_all_video_codecs_except_one(marie->lc, "VP8");
	pauline_vtable = linphone_core_v_table_new();
	pauline_vtable->call_state_changed = video_call_state_changed;
	linphone_core_add_listener(pauline->lc, pauline_vtable);
	linphone_core_set_video_device(pauline->lc, "StaticImage: Static picture");
	linphone_core_enable_video_capture(pauline->lc, TRUE);
	linphone_core_enable_video_display(pauline->lc, TRUE);
	pauline_params = linphone_core_create_default_call_parameters(pauline->lc);
	linphone_call_params_enable_video(pauline_params, TRUE);
	disable_all_video_codecs_except_one(pauline->lc, "VP8");
	
	CU_ASSERT_TRUE(video_call_with_params(marie, pauline, marie_params, pauline_params));

	/* Wait for 3s. */
	wait_for_until(marie->lc, pauline->lc, &dummy, 1, 3000);

	linphone_core_terminate_all_calls(marie->lc);
	CU_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallEnd, 1));
	CU_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallEnd, 1));
	CU_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &marie->stat.number_of_LinphoneCallReleased, 1));
	CU_ASSERT_TRUE(wait_for(marie->lc, pauline->lc, &pauline->stat.number_of_LinphoneCallReleased, 1));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

test_t video_tests[] = {
	{ "Early-media video during video call", early_media_video_during_video_call_test }
};

test_suite_t video_test_suite = {
	"Video",
	NULL,
	NULL,
	sizeof(video_tests) / sizeof(video_tests[0]),
	video_tests
};

#endif /* VIDEO_ENABLED */
