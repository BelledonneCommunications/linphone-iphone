#include "liblinphone_tester.h"

static bool_t wait_for_eof(bool_t *eof, int *time,int time_refresh, int timeout) {
	while(*time < timeout && !*eof) {
		usleep(time_refresh * 1000U);
		*time += time_refresh;
	}
	return *time < timeout;
}

static void eof_callback(LinphonePlayer *player, void *user_data) {
	bool_t *eof = (bool_t *)user_data;
	*eof = TRUE;
}

static void playing_test(void) {
	LinphoneCoreManager *lc_manager;
	LinphonePlayer *player;
	const char *filename = "sounds/hello_opus_h264.mkv";
	int res, time = 0;
	bool_t eof = FALSE;

	lc_manager = linphone_core_manager_new("marie_rc");
	CU_ASSERT_PTR_NOT_NULL(lc_manager);
	if(lc_manager == NULL) return;

	player = linphone_core_create_file_player(lc_manager->lc, ms_snd_card_manager_get_default_card(ms_snd_card_manager_get()), video_stream_get_default_video_renderer());
	CU_ASSERT_PTR_NOT_NULL(player);
	if(player == NULL) goto fail;

	CU_ASSERT_EQUAL((res = linphone_player_open(player, filename, eof_callback, &eof)), 0);
	if(res == -1) goto fail;

	CU_ASSERT_EQUAL((res = linphone_player_start(player)), 0);
	if(res == -1) goto fail;

	CU_ASSERT_TRUE(wait_for_eof(&eof, &time, 100, 13000));

	linphone_player_close(player);

	fail:
	if(player) file_player_destroy(player);
	if(lc_manager) linphone_core_manager_destroy(lc_manager);
}

test_t player_tests[] = {
	{	"Playing"	,	playing_test	}
};

test_suite_t player_test_suite = {
	"Player",
	NULL,
	NULL,
	sizeof(player_tests) / sizeof(test_t),
	player_tests
};
