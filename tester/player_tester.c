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

#include "liblinphone_tester.h"
#include <mediastreamer2/mediastream.h>

static bool_t wait_for_eof(bool_t *eof, int *time,int time_refresh, int timeout) {
	while(*time < timeout && !*eof) {
		ms_usleep(time_refresh * 1000U);
		*time += time_refresh;
	}
	return *time < timeout;
}

static void eof_callback(LinphonePlayer *player, void *user_data) {
	bool_t *eof = (bool_t *)user_data;
	*eof = TRUE;
}

static void play_file(const char *filename, bool_t unsupported_format, const char *audio_mime, const char *video_mime) {
	LinphoneCoreManager *lc_manager;
	LinphonePlayer *player;
	int res, time = 0;
	bool_t eof = FALSE;

	lc_manager = linphone_core_manager_new("marie_rc");
	CU_ASSERT_PTR_NOT_NULL(lc_manager);
	if(lc_manager == NULL) return;

	player = linphone_core_create_local_player(lc_manager->lc, ms_snd_card_manager_get_default_card(ms_snd_card_manager_get()), video_stream_get_default_video_renderer(), 0);
	CU_ASSERT_PTR_NOT_NULL(player);
	if(player == NULL) goto fail;

	res = linphone_player_open(player, filename, eof_callback, &eof);
	if(unsupported_format
			|| (audio_mime == NULL && video_mime == NULL)
			|| (video_mime == NULL && audio_mime && !ms_filter_codec_supported(audio_mime))
			|| (audio_mime == NULL && video_mime && !ms_filter_codec_supported(video_mime))) {
		CU_ASSERT_EQUAL(res, -1);
	} else {
		CU_ASSERT_EQUAL(res, 0);
	}
	if(res == -1) goto fail;

	CU_ASSERT_EQUAL((res = linphone_player_start(player)), 0);
	if(res == -1) goto fail;

	CU_ASSERT_TRUE(wait_for_eof(&eof, &time, 100, 13000));

	linphone_player_close(player);

	fail:
	if(player) linphone_player_destroy(player);
	if(lc_manager) linphone_core_manager_destroy(lc_manager);
}

static void playing_test(void) {
	char *filename = ms_strdup_printf("%s/sounds/hello_opus_h264.mkv", liblinphone_tester_file_prefix);
	const char *audio_mime = "opus";
	const char *video_mime = "h264";
	play_file(filename, !linphone_local_player_matroska_supported(), audio_mime, video_mime);
	ms_free(filename);
}

test_t player_tests[] = {
	{	"Local MKV file"	,	playing_test	}
};

test_suite_t player_test_suite = {
	"Player",
	NULL,
	NULL,
	sizeof(player_tests) / sizeof(test_t),
	player_tests
};
