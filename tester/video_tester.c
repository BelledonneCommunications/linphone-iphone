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

#ifdef VIDEO_ENABLED

#include "linphone/core.h"
#include "liblinphone_tester.h"
#include "tester_utils.h"
#include "linphone/lpconfig.h"

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
			linphone_call_update(call, NULL);
		}
		BC_ASSERT_STRING_EQUAL(newCamId,ms_web_cam_get_string_id(_linphone_call_get_video_device(call)));
		linphone_call_enable_camera(call,FALSE);
		linphone_core_iterate(marie->lc);
		linphone_call_enable_camera(call,TRUE);
		BC_ASSERT_STRING_EQUAL(newCamId,ms_web_cam_get_string_id(_linphone_call_get_video_device(call)));
	}

	linphone_core_terminate_all_calls(pauline->lc);
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&pauline->stat.number_of_LinphoneCallEnd,1));
	BC_ASSERT_TRUE(wait_for(pauline->lc,marie->lc,&marie->stat.number_of_LinphoneCallEnd,1));
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

test_t video_tests[] = {
	TEST_NO_TAG("Enable/disable camera after camera switches", enable_disable_camera_after_camera_switches)
};

test_suite_t video_test_suite = {
	"Video", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(video_tests) / sizeof(video_tests[0]), video_tests
};

#endif // ifdef VIDEO_ENABLED
