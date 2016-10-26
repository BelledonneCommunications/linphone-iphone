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
#include "audio_bypass_wav_header.h" // This is a copy of mediastreamer2/src/audiofilters/wav_header.h

/**********************************************************************
 * This is a (simpler) copy of msfileplay filter in mediastreamer2   *
 *********************************************************************/

struct _PlayerData{
	int fd;
	MSPlayerState state;
	int rate;
	int nchannels;
	int hsize;
	int loop_after;
	int pause_time;
	int count;
	int samplesize;
	char *mime;
	uint32_t ts;
	bool_t swap;
	bool_t is_raw;
};

typedef struct _PlayerData PlayerData;

static void audio_bypass_snd_read_init(MSFilter *f) {
	PlayerData *d=ms_new0(PlayerData,1);
	d->fd=-1;
	d->state=MSPlayerClosed;
	d->swap=TRUE;
	d->rate=44100;
	d->nchannels=1;
	d->samplesize=2;
	d->mime = "L16";
	d->hsize=0;
	d->loop_after=-1; /*by default, don't loop*/
	d->pause_time=0;
	d->count=0;
	d->ts=0;
	d->is_raw=TRUE;
	f->data=d;
}

int audio_bypass_read_wav_header_from_fd(wave_header_t *header,int fd){
	int count;
	int skip;
	int hsize=0;
	riff_t *riff_chunk=&header->riff_chunk;
	format_t *format_chunk=&header->format_chunk;
	data_t *data_chunk=&header->data_chunk;

	unsigned long len=0;

	len = read(fd, (char*)riff_chunk, sizeof(riff_t)) ;
	if (len != sizeof(riff_t)){
		goto not_a_wav;
	}

	if (0!=strncmp(riff_chunk->riff, "RIFF", 4) || 0!=strncmp(riff_chunk->wave, "WAVE", 4)){
		goto not_a_wav;
	}

	len = read(fd, (char*)format_chunk, sizeof(format_t)) ;
	if (len != sizeof(format_t)){
		ms_warning("Wrong wav header: cannot read file");
		goto not_a_wav;
	}

	if ((skip=le_uint32(format_chunk->len)-0x10)>0)
	{
		lseek(fd,skip,SEEK_CUR);
	}
	hsize=sizeof(wave_header_t)-0x10+le_uint32(format_chunk->len);

	count=0;
	do{
		len = read(fd, data_chunk, sizeof(data_t)) ;
		if (len != sizeof(data_t)){
			ms_warning("Wrong wav header: cannot read file");
			goto not_a_wav;
		}
		if (strncmp(data_chunk->data, "data", 4)!=0){
			ms_warning("skipping chunk=%c%c%c%c len=%i", data_chunk->data[0],data_chunk->data[1],data_chunk->data[2],data_chunk->data[3], data_chunk->len);
			lseek(fd,le_uint32(data_chunk->len),SEEK_CUR);
			count++;
			hsize+=len+le_uint32(data_chunk->len);
		}else{
			hsize+=len;
			break;
		}
	}while(count<30);
	return hsize;

	not_a_wav:
		/*rewind*/
		lseek(fd,0,SEEK_SET);
		return -1;
}

static int read_wav_header(PlayerData *d){
	wave_header_t header;
	format_t *format_chunk=&header.format_chunk;
	int ret=audio_bypass_read_wav_header_from_fd(&header,d->fd);

	d->samplesize=le_uint16(format_chunk->blockalign)/d->nchannels;
	d->hsize=ret;

	#ifdef WORDS_BIGENDIAN
	if (le_uint16(format_chunk->blockalign)==le_uint16(format_chunk->channel) * 2)
		d->swap=TRUE;
	#endif
	d->is_raw=FALSE;
	return 0;
}

static void audio_bypass_snd_read_preprocess(MSFilter *f) {
	PlayerData *d=(PlayerData*)f->data;
	int fd;
	const char *file=bc_tester_res("sounds/hello44100.wav");

	if ((fd=open(file,O_RDONLY|O_BINARY))==-1){
		ms_warning("MSFilePlayer[%p]: failed to open %s: %s",f,file,strerror(errno));
		return;
	}

	d->state=MSPlayerPaused;
	d->fd=fd;
	d->ts=0;
	if (read_wav_header(d)!=0 && strstr(file,".wav")){
		ms_warning("File %s has .wav extension but wav header could be found.",file);
	}
	ms_filter_notify_no_arg(f,MS_FILTER_OUTPUT_FMT_CHANGED);
	ms_message("MSFilePlayer[%p]: %s opened: rate=%i,channel=%i",f,file,d->rate,d->nchannels);

	if (d->state==MSPlayerPaused)
		d->state=MSPlayerPlaying;
	return;
}

static void swap_bytes(unsigned char *bytes, int len){
	int i;
	unsigned char tmp;
	for(i=0;i<len;i+=2){
		tmp=bytes[i];
		bytes[i]=bytes[i+1];
		bytes[i+1]=tmp;
	}
}

static void audio_bypass_snd_read_process(MSFilter *f) {
	PlayerData *d=(PlayerData*)f->data;
	int nsamples=(f->ticker->interval*d->rate*d->nchannels)/1000;
	int bytes;
	/*send an even number of samples each tick. At 22050Hz the number of samples per 10 ms chunk is odd.
	Odd size buffer of samples cause troubles to alsa. Fixing in alsa is difficult, so workaround here.
	*/
	if (nsamples & 0x1 ) { //odd number of samples
		if (d->count & 0x1 )
			nsamples++;
		else
			nsamples--;
	}
	bytes=nsamples*d->samplesize;
	d->count++;
	ms_filter_lock(f);
	if (d->state==MSPlayerPlaying){
		{
			int err;
			mblk_t *om=allocb(bytes,0);
			if (d->pause_time>0){
				err=bytes;
				memset(om->b_wptr,0,bytes);
				d->pause_time-=f->ticker->interval;
			}else{
				err=read(d->fd,om->b_wptr,bytes);
				if (d->swap) swap_bytes(om->b_wptr,bytes);
			}
			if (err>=0){
				if (err!=0){
					if (err<bytes)
						memset(om->b_wptr+err,0,bytes-err);
					om->b_wptr+=bytes;
					mblk_set_timestamp_info(om,d->ts);
					d->ts+=nsamples;
					ms_queue_put(f->outputs[0],om);
				}else freemsg(om);
				if (err<bytes){
					ms_filter_notify_no_arg(f,MS_PLAYER_EOF);
					/*for compatibility:*/
					lseek(d->fd,d->hsize,SEEK_SET);

					/* special value for playing file only once */
					if (d->loop_after<0)
					{
						d->state=MSPlayerPaused;
						ms_filter_unlock(f);
						return;
					}

					if (d->loop_after>=0){
						d->pause_time=d->loop_after;
					}
				}
			}else{
				ms_warning("Fail to read %i bytes: %s",bytes,strerror(errno));
			}
		}
	}
	ms_filter_unlock(f);
}

static void audio_bypass_snd_read_postprocess(MSFilter *f) {
	PlayerData *d=(PlayerData*)f->data;
	ms_filter_lock(f);
	if (d->state!=MSPlayerClosed){
		d->state=MSPlayerPaused;
		lseek(d->fd,d->hsize,SEEK_SET);
	}
	ms_filter_unlock(f);
	if (d->fd!=-1)	close(d->fd);
	d->fd=-1;
	d->state=MSPlayerClosed;
}

static void audio_bypass_snd_read_uninit(MSFilter *f) {
	PlayerData *d=(PlayerData*)f->data;
	ms_free(d);
}

static int audio_bypass_snd_read_set_sample_rate(MSFilter *f, void *arg) { // This is to prevent ms2 to put a resampler between this filter and the rtpsend
	return 0;
}

static int audio_bypass_snd_read_set_nchannels(MSFilter *f, void *arg) { // This is to prevent ms2 to put a resampler between this filter and the rtpsend
	return 0;
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
	{ MS_FILTER_SET_SAMPLE_RATE, audio_bypass_snd_read_set_sample_rate },
	{ MS_FILTER_SET_NCHANNELS, audio_bypass_snd_read_set_nchannels },
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
	audio_bypass_snd_read_methods,
	0
};

static void audio_bypass_snd_write_init(MSFilter *f) {

}

static void audio_bypass_snd_write_preprocess(MSFilter *f) {

}

static void audio_bypass_snd_write_process(MSFilter *f) {
	mblk_t *m = ms_queue_get(f->inputs[0]);
	ms_free(m);
}

static void audio_bypass_snd_write_postprocess(MSFilter *f) {

}

static void audio_bypass_snd_write_uninit(MSFilter *f) {

}

static int audio_bypass_snd_write_set_sample_rate(MSFilter *f, void *arg) { // This is to prevent ms2 to put a resampler between this filter and the rtprecv
	return 0;
}

static int audio_bypass_snd_write_set_nchannels(MSFilter *f, void *arg) { // This is to prevent ms2 to put a resampler between this filter and the rtprecv
	return 0;
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
	{ MS_FILTER_SET_SAMPLE_RATE, audio_bypass_snd_write_set_sample_rate },
	{ MS_FILTER_SET_NCHANNELS, audio_bypass_snd_write_set_nchannels },
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
	audio_bypass_snd_write_methods,
	0
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
	NULL,
	NULL,
	NULL,
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

static void only_enable_payload(LinphoneCore *lc, const char *mime, int rate, int channels) {
	const MSList *elem = linphone_core_get_audio_codecs(lc);
	PayloadType *pt;

	for(; elem != NULL; elem = elem->next) {
		pt = (PayloadType *)elem->data;
		linphone_core_enable_payload_type(lc, pt, FALSE);
	}
	pt = linphone_core_find_payload_type(lc, mime, rate, channels);
	if (BC_ASSERT_PTR_NOT_NULL(pt)) {
		linphone_core_enable_payload_type(lc, pt, TRUE);
	}
}

static void audio_bypass(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCore *marie_lc = marie->lc;
	MSFactory *marie_factory = linphone_core_get_ms_factory(marie_lc);
	MSSndCardManager *marie_sndcard_manager = ms_factory_get_snd_card_manager(marie_factory);

	LinphoneCoreManager *pauline = linphone_core_manager_new("pauline_rc");
	LinphoneCore *pauline_lc = pauline->lc;
	MSFactory *pauline_factory = linphone_core_get_ms_factory(pauline_lc);
	MSSndCardManager *pauline_sndcard_manager = ms_factory_get_snd_card_manager(pauline_factory);

	bool_t call_ok;
	char *hellopath = bc_tester_res("sounds/hello44100.wav");
	char *recordpath = bc_tester_file("audiobypass-record.wav");
	double similar=1;
	const double threshold = 0.85;

	lp_config_set_string(marie_lc->config, "sound", "features", "None");
	lp_config_set_string(pauline_lc->config, "sound", "features", "None");

	/*make sure the record file doesn't already exists, otherwise this test will append new samples to it*/
	unlink(recordpath);

	// Enable L16 audio codec
	only_enable_payload(marie_lc, "L16", 44100, 1);
	only_enable_payload(pauline_lc, "L16", 44100, 1);

	// Add our custom sound card
	ms_snd_card_manager_register_desc(marie_sndcard_manager, &audio_bypass_snd_card_desc);
	ms_snd_card_manager_register_desc(pauline_sndcard_manager, &audio_bypass_snd_card_desc);
	linphone_core_reload_sound_devices(marie_lc);
	linphone_core_reload_sound_devices(pauline_lc);
	linphone_core_set_playback_device(marie_lc, AUDIO_BYPASS_SOUNDCARD);
	linphone_core_set_playback_device(pauline_lc, AUDIO_BYPASS_SOUNDCARD);
	linphone_core_set_capture_device(marie_lc, AUDIO_BYPASS_SOUNDCARD);
	linphone_core_set_capture_device(pauline_lc, AUDIO_BYPASS_SOUNDCARD);
	BC_ASSERT_STRING_EQUAL(linphone_core_get_capture_device(marie_lc), AUDIO_BYPASS_SOUNDCARD);
	BC_ASSERT_STRING_EQUAL(linphone_core_get_capture_device(pauline_lc), AUDIO_BYPASS_SOUNDCARD);
	BC_ASSERT_STRING_EQUAL(linphone_core_get_playback_device(marie_lc), AUDIO_BYPASS_SOUNDCARD);
	BC_ASSERT_STRING_EQUAL(linphone_core_get_playback_device(pauline_lc), AUDIO_BYPASS_SOUNDCARD);

	linphone_core_use_files(pauline_lc, TRUE);
	linphone_core_set_play_file(pauline_lc, NULL);
	linphone_core_set_record_file(pauline_lc, recordpath);

	call_ok = call(marie, pauline);
	BC_ASSERT_TRUE(call_ok);
	if (!call_ok) goto end;

	BC_ASSERT_STRING_EQUAL(linphone_call_params_get_used_audio_codec(linphone_call_get_current_params(linphone_core_get_current_call(marie_lc)))->mime_type, "L16");

	wait_for_until(pauline_lc, marie_lc, NULL, 0, 4000); //hello44100.wav is 4 seconds long
	end_call(marie, pauline);

	BC_ASSERT_EQUAL(ms_audio_diff(hellopath, recordpath, &similar, &audio_cmp_params, NULL, NULL), 0, int, "%d");
	BC_ASSERT_GREATER(similar, threshold, double, "%g");
	BC_ASSERT_LOWER(similar, 1.0, double, "%g");

end:
	bc_free(recordpath);
	bc_free(hellopath);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

test_t audio_bypass_tests[] = {
	TEST_NO_TAG("Audio Bypass", audio_bypass)
};

test_suite_t audio_bypass_suite = { "Audio Bypass", NULL, NULL,
	liblinphone_tester_before_each, liblinphone_tester_after_each,
	sizeof(audio_bypass_tests) / sizeof(audio_bypass_tests[0]), audio_bypass_tests };
