#include "private.h"
#include <mediastreamer2/fileplayer.h>
#include <mediastreamer2/mssndcard.h>

static int file_player_open(LinphonePlayer *obj, const char *filename);
static int file_player_start(LinphonePlayer *obj);
static int file_player_pause(LinphonePlayer *obj);
static int file_player_seek(LinphonePlayer *obj, int time_ms);
static MSPlayerState file_player_get_state(LinphonePlayer *obj);
static void file_player_close(LinphonePlayer *obj);
static void file_player_eof_callback(void *user_data);

LinphonePlayer *linphone_core_create_file_player(LinphoneCore *lc, MSSndCard *snd_card, const char *video_out) {
	LinphonePlayer *obj = ms_new0(LinphonePlayer, 1);
	if(snd_card == NULL) snd_card = lc->sound_conf.play_sndcard;
	if(video_out == NULL) video_out = linphone_core_get_video_display_filter(lc);
	obj->impl = ms_file_player_new(snd_card, video_out);
	obj->open = file_player_open;
	obj->start = file_player_start;
	obj->pause = file_player_pause;
	obj->seek = file_player_seek;
	obj->get_state = file_player_get_state;
	obj->close = file_player_close;
	ms_file_player_set_eof_callback((MSFilePlayer *)obj->impl, file_player_eof_callback, obj);
	return obj;
}

void linphone_file_player_destroy(LinphonePlayer *obj) {
	ms_file_player_free((MSFilePlayer *)obj->impl);
	ms_free(obj);
}

bool_t linphone_file_player_matroska_supported(void) {
	return ms_file_player_matroska_supported();
}

static int file_player_open(LinphonePlayer *obj, const char *filename) {
	return ms_file_player_open((MSFilePlayer *)obj->impl, filename) ? 0 : -1;
}

static int file_player_start(LinphonePlayer *obj) {
	return ms_file_player_start((MSFilePlayer *)obj->impl) ? 0 : -1;
}

static int file_player_pause(LinphonePlayer *obj) {
	ms_file_player_pause((MSFilePlayer *)obj->impl);
	return 0;
}

static int file_player_seek(LinphonePlayer *obj, int time_ms) {
	return ms_file_player_seek((MSFilePlayer *)obj->impl, time_ms) ? 0 : -1;
}

static MSPlayerState file_player_get_state(LinphonePlayer *obj) {
	return ms_file_player_get_state((MSFilePlayer *)obj->impl);
}

static void file_player_close(LinphonePlayer *obj) {
	ms_file_player_close((MSFilePlayer *)obj->impl);
}

static void file_player_eof_callback(void *user_data) {
	LinphonePlayer *obj = (LinphonePlayer *)user_data;
	obj->cb(obj, obj->user_data);
}
