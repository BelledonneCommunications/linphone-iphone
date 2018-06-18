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

#include <mediastreamer2/msqrcodereader.h>

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

typedef struct struct_qrcode_callback_data {
	int qrcode_found;
	char *text;
}qrcode_callback_data;


static void qrcode_found_cb(LinphoneCore *lc, const char *result) {
	LinphoneCoreCbs *cbs = linphone_core_get_current_callbacks(lc);
	qrcode_callback_data *found = (qrcode_callback_data *)linphone_core_cbs_get_user_data(cbs);
	found->qrcode_found = TRUE;
	if (result) {
		if (found->text) ms_free(found->text);
		found->text = ms_strdup(result);
	}
}

typedef struct struct_image_rect {
	int x;
	int y;
	int w;
	int h;
}image_rect;

static void _decode_qrcode(const char* image_path, image_rect *rect) {
	qrcode_callback_data qrcode_data;
	char *qrcode_image;
	LinphoneCoreManager* lcm = NULL;
	MSFactory* factory = NULL;
	factory = ms_factory_new_with_voip();
	if (!ms_factory_lookup_filter_by_name(factory, "MSQRCodeReader")) {
		ms_error("QRCode support is not built-in");
		goto end;
	}

	lcm =linphone_core_manager_create("empty_rc");
	LinphoneCoreCbs* cbs = NULL;
	qrcode_data.qrcode_found = FALSE;
	qrcode_data.text = NULL;
	linphone_core_manager_start(lcm, FALSE);

	qrcode_image = bc_tester_res(image_path);

	linphone_core_set_video_device(lcm->lc, liblinphone_tester_static_image_id);
	linphone_core_set_static_picture(lcm->lc, qrcode_image);

	linphone_core_enable_qrcode_video_preview(lcm->lc, TRUE);
	cbs = linphone_core_get_current_callbacks(lcm->lc);
	linphone_core_cbs_set_qrcode_found(cbs, qrcode_found_cb);
	linphone_core_cbs_set_user_data(cbs, &qrcode_data);
	if (rect) {
		linphone_core_set_qrcode_decode_rect(lcm->lc, rect->x, rect->y, rect->w, rect->h);
	}
	linphone_core_enable_video_preview(lcm->lc, TRUE);

	BC_ASSERT_TRUE(wait_for_until(lcm->lc, NULL, &qrcode_data.qrcode_found, TRUE, 2000));
	if (qrcode_data.qrcode_found) {
		if (BC_ASSERT_PTR_NOT_NULL(qrcode_data.text)) {
			ms_message("QRCode decode: %s", qrcode_data.text);
			BC_ASSERT_STRING_EQUAL(qrcode_data.text, "https://www.linphone.org/");
		}
	}

	if (qrcode_data.text) ms_free(qrcode_data.text);
	if (qrcode_image) ms_free(qrcode_image);

	linphone_core_enable_video_preview(lcm->lc, FALSE);
	linphone_core_manager_destroy(lcm);
end:
	ms_factory_destroy(factory);
}

static void decode_qrcode_from_image(void) {
	_decode_qrcode("images/linphonesiteqr.jpg", NULL);
}

static void decode_qrcode_from_zone(void) {
	image_rect rect;
	rect.x = 332;
	rect.y = 470;
	rect.w = 268;
	rect.h = 262;
	_decode_qrcode("images/linphonesiteqr_captured.jpg", &rect);
}

test_t video_tests[] = {
	TEST_NO_TAG("Enable/disable camera after camera switches", enable_disable_camera_after_camera_switches),
	TEST_ONE_TAG("Decode QRCode from image", decode_qrcode_from_image, "QRCode"),
	TEST_ONE_TAG("Decode QRCode from zone", decode_qrcode_from_zone, "QRCode")
};

test_suite_t video_test_suite = {
	"Video", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(video_tests) / sizeof(video_tests[0]), video_tests
};

#endif // ifdef VIDEO_ENABLED
