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
#include "private.h"

static void audio_bypass_snd_read_init(MSFilter *f) {
	
}

static void audio_bypass_snd_read_preprocess(MSFilter *f) {
	
}

static void audio_bypass_snd_read_process(MSFilter *f) {
	mblk_t *m = allocb(10, 0);
	memset(m->b_wptr, 0, 10);
	m->b_wptr += 10;
	ms_queue_put(f->outputs[0], m);
}

static void audio_bypass_snd_read_postprocess(MSFilter *f) {
	
}

static void audio_bypass_snd_read_uninit(MSFilter *f) {
	
}

static int audio_bypass_snd_read_get_sample_rate(MSFilter *f, void *arg) {
	int *sample_rate = (int *)arg;
	*sample_rate = 44100;
	return 0;
}

static int audio_bypass_snd_read_get_nchannels(MSFilter *f, void *arg) {
	int *nchannels = (int *)arg;
	*nchannels = 1;
	return 0;
}

static int audio_bypass_snd_read_get_fmt(MSFilter *f, void *arg) {
	MSPinFormat *pinFmt = (MSPinFormat *)arg;
	pinFmt->fmt = ms_factory_get_audio_format(f->factory, "L16", 44100, 1, NULL);
	return 0;
}

static MSFilterMethod audio_bypass_snd_read_methods[] = {
	{ MS_FILTER_GET_SAMPLE_RATE, audio_bypass_snd_read_get_sample_rate },
	{ MS_FILTER_GET_NCHANNELS, audio_bypass_snd_read_get_nchannels },
	{ MS_FILTER_GET_OUTPUT_FMT, audio_bypass_snd_read_get_fmt },
	{ 0, NULL }
};

MSFilterDesc audio_bypass_snd_read_desc = {
	MS_FILTER_PLUGIN_ID,
	"AudioBypassReader",
	"audio bypass source",
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	audio_bypass_snd_read_init,
	audio_bypass_snd_read_preprocess,
	audio_bypass_snd_read_process,
	audio_bypass_snd_read_postprocess,
	audio_bypass_snd_read_uninit,
	audio_bypass_snd_read_methods
};

static void audio_bypass_snd_write_init(MSFilter *f) {
	
}

static void audio_bypass_snd_write_preprocess(MSFilter *f) {
	
}

static void audio_bypass_snd_write_process(MSFilter *f) {
	ms_queue_get(f->inputs[0]);	
}

static void audio_bypass_snd_write_postprocess(MSFilter *f) {
	
}

static void audio_bypass_snd_write_uninit(MSFilter *f) {
	
}

static int audio_bypass_snd_write_get_sample_rate(MSFilter *f, void *arg) {
	int *sample_rate = (int*)arg;
	*sample_rate = 44100;
	return 0;
}

static int audio_bypass_snd_write_get_nchannels(MSFilter *obj, void *arg) {
	int *nchannels = (int*)arg;
	*nchannels = 1;
	return 0;
}

static int audio_bypass_snd_write_get_fmt(MSFilter *f, void *arg) {
	MSPinFormat *pinFmt = (MSPinFormat *)arg;
	pinFmt->fmt = ms_factory_get_audio_format(f->factory, "L16", 44100, 1, NULL);
	return 0;
}

static MSFilterMethod audio_bypass_snd_write_methods[] = {
	{ MS_FILTER_GET_SAMPLE_RATE, audio_bypass_snd_write_get_sample_rate },
	{ MS_FILTER_GET_NCHANNELS, audio_bypass_snd_write_get_nchannels },
	{ MS_FILTER_GET_OUTPUT_FMT, audio_bypass_snd_write_get_fmt },
	{ 0, NULL }
};

MSFilterDesc audio_bypass_snd_write_desc = {
	MS_FILTER_PLUGIN_ID,
	"AudioBypassWriter",
	"audio bypass output",
	MS_FILTER_OTHER,
	NULL,
	1,
	0,
	audio_bypass_snd_write_init,
	audio_bypass_snd_write_preprocess,
	audio_bypass_snd_write_process,
	audio_bypass_snd_write_postprocess,
	audio_bypass_snd_write_uninit,
	audio_bypass_snd_write_methods
};

static MSFilter* audio_bypass_snd_card_create_reader(MSSndCard *sndcard) {
	MSFactory *factory = ms_snd_card_get_factory(sndcard);
	MSFilter *f = ms_factory_create_filter_from_desc(factory, &audio_bypass_snd_read_desc);
	return f;
}

static MSFilter* audio_bypass_snd_card_create_writer(MSSndCard *sndcard) {
	MSFactory *factory = ms_snd_card_get_factory(sndcard);
	MSFilter *f = ms_factory_create_filter_from_desc(factory, &audio_bypass_snd_write_desc);
	return f;
}

static void audio_bypass_snd_card_detect(MSSndCardManager *m);

MSSndCardDesc audio_bypass_snd_card_desc = {
	"audioBypass",
	audio_bypass_snd_card_detect,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,	
	audio_bypass_snd_card_create_reader,
	audio_bypass_snd_card_create_writer,
	NULL
};

static MSSndCard* create_audio_bypass_snd_card(void) {
	MSSndCard* sndcard;
	sndcard = ms_snd_card_new(&audio_bypass_snd_card_desc);
	sndcard->data = NULL;
	sndcard->name = ms_strdup("audio bypass sound card");
	sndcard->capabilities = MS_SND_CARD_CAP_PLAYBACK | MS_SND_CARD_CAP_CAPTURE;
	sndcard->latency = 0;
	return sndcard;
}

#define AUDIO_BYPASS_SOUNDCARD "audioBypass: audio bypass sound card"

static void audio_bypass_snd_card_detect(MSSndCardManager *m) {
	ms_snd_card_manager_add_card(m, create_audio_bypass_snd_card());
}

static void only_enable_payload(MSList *codecs, const char *mime, int channels) {
	while (codecs) {
		PayloadType *pt = (PayloadType *)codecs->data;
		if (strcmp(pt->mime_type, mime) == 0 && pt->channels == channels) {
			pt->flags |= PAYLOAD_TYPE_ENABLED;
		} else {
			pt->flags &= PAYLOAD_TYPE_ENABLED;
		}
		codecs = ms_list_next(codecs);
	}
}

static void audio_bypass(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc_audio_bypass");
	LinphoneCore *marie_lc = marie->lc;
	MSFactory *marie_factory = linphone_core_get_ms_factory(marie_lc);
	MSSndCardManager *marie_sndcard_manager = ms_factory_get_snd_card_manager(marie_factory);
	
	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_rc_audio_bypass");
	LinphoneCore *pauline_lc = pauline->lc;
	MSFactory *pauline_factory = linphone_core_get_ms_factory(pauline_lc);
	MSSndCardManager *pauline_sndcard_manager = ms_factory_get_snd_card_manager(pauline_factory);
	
	bool_t call_ok;
	MSList *marie_audio_codecs = marie_lc->codecs_conf.audio_codecs;
	MSList *pauline_audio_codecs = pauline_lc->codecs_conf.audio_codecs;
	
	// Enable L16 audio codec
	only_enable_payload(marie_audio_codecs, "L16", 1);
	only_enable_payload(pauline_audio_codecs, "L16", 1);
	
	// Add our custom sound card
	ms_snd_card_manager_register_desc(marie_sndcard_manager, &audio_bypass_snd_card_desc);
	ms_snd_card_manager_register_desc(pauline_sndcard_manager, &audio_bypass_snd_card_desc);
	linphone_core_reload_sound_devices(marie->lc);
	linphone_core_reload_sound_devices(pauline->lc);
	linphone_core_set_playback_device(marie->lc, AUDIO_BYPASS_SOUNDCARD);
	linphone_core_set_playback_device(pauline->lc, AUDIO_BYPASS_SOUNDCARD);
	linphone_core_set_capture_device(marie->lc, AUDIO_BYPASS_SOUNDCARD);
	linphone_core_set_capture_device(pauline->lc, AUDIO_BYPASS_SOUNDCARD);
	BC_ASSERT_STRING_EQUAL(linphone_core_get_capture_device(marie->lc), AUDIO_BYPASS_SOUNDCARD);
	BC_ASSERT_STRING_EQUAL(linphone_core_get_capture_device(pauline->lc), AUDIO_BYPASS_SOUNDCARD);
	BC_ASSERT_STRING_EQUAL(linphone_core_get_playback_device(marie->lc), AUDIO_BYPASS_SOUNDCARD);
	BC_ASSERT_STRING_EQUAL(linphone_core_get_playback_device(pauline->lc), AUDIO_BYPASS_SOUNDCARD);

	call_ok = call(marie, pauline);
	BC_ASSERT_TRUE(call_ok);
	if (!call_ok) goto end;
	
	BC_ASSERT_STRING_EQUAL(linphone_call_params_get_used_audio_codec(linphone_call_get_current_params(linphone_core_get_current_call(marie_lc)))->mime_type, "L16");
	
	wait_for_until(pauline->lc, marie->lc, NULL, 0, 10000);
	end_call(marie, pauline);
	
end:
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

test_t audio_bypass_tests[] = {
	TEST_NO_TAG("Audio Bypass", audio_bypass)
};

test_suite_t audio_bypass_suite = { "Audio Bypass", NULL, NULL, 
	liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(audio_bypass_tests) / sizeof(audio_bypass_tests[0]), audio_bypass_tests };
