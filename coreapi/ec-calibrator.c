/*
linphone
Copyright (C) 2011 Belledonne Communications SARL
Author: Simon MORLAT (simon.morlat@linphone.org)

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

#include "private.h"

#include "mediastreamer2/mstonedetector.h"
#include "mediastreamer2/dtmfgen.h"

#include "linphone/lpconfig.h"



static void ecc_init_filters(EcCalibrator *ecc){
	unsigned int rate;
	int channels = 1;
	int ecc_channels = 1;
	MSTickerParams params={0};
	params.name="Echo calibrator";
	params.prio=MS_TICKER_PRIO_HIGH;
	ecc->ticker=ms_ticker_new_with_params(&params);

	ecc->sndread=ms_snd_card_create_reader(ecc->capt_card);
	ms_filter_call_method(ecc->sndread,MS_FILTER_SET_SAMPLE_RATE,&ecc->rate);
	ms_filter_call_method(ecc->sndread,MS_FILTER_GET_SAMPLE_RATE,&rate);
	ms_filter_call_method(ecc->sndread,MS_FILTER_SET_NCHANNELS,&ecc_channels);
	ms_filter_call_method(ecc->sndread,MS_FILTER_GET_NCHANNELS,&channels);
	ecc->read_resampler=ms_factory_create_filter(ecc->factory, MS_RESAMPLE_ID);
	ms_filter_call_method(ecc->read_resampler,MS_FILTER_SET_SAMPLE_RATE,&rate);
	ms_filter_call_method(ecc->read_resampler,MS_FILTER_SET_OUTPUT_SAMPLE_RATE,&ecc->rate);
	ms_filter_call_method(ecc->read_resampler,MS_FILTER_SET_NCHANNELS,&ecc_channels);
	ms_filter_call_method(ecc->read_resampler,MS_FILTER_SET_OUTPUT_NCHANNELS,&channels);
	
	
	ecc->det=ms_factory_create_filter(ecc->factory, MS_TONE_DETECTOR_ID);
	ms_filter_call_method(ecc->det,MS_FILTER_SET_SAMPLE_RATE,&ecc->rate);
	ecc->rec=ms_factory_create_filter(ecc->factory, MS_VOID_SINK_ID);

	ms_filter_link(ecc->sndread,0,ecc->read_resampler,0);
	ms_filter_link(ecc->read_resampler,0,ecc->det,0);
	ms_filter_link(ecc->det,0,ecc->rec,0);

	ecc->play=ms_factory_create_filter(ecc->factory, MS_VOID_SOURCE_ID);
	ecc->gen=ms_factory_create_filter(ecc->factory, MS_DTMF_GEN_ID);
	ms_filter_call_method(ecc->gen,MS_FILTER_SET_SAMPLE_RATE,&ecc->rate);
	ecc->write_resampler=ms_factory_create_filter(ecc->factory, MS_RESAMPLE_ID);
	ecc->sndwrite=ms_snd_card_create_writer(ecc->play_card);
	
	ms_filter_call_method(ecc->sndwrite,MS_FILTER_SET_SAMPLE_RATE,&ecc->rate);
	ms_filter_call_method(ecc->sndwrite,MS_FILTER_GET_SAMPLE_RATE,&rate);
	ms_filter_call_method(ecc->sndwrite,MS_FILTER_SET_NCHANNELS,&ecc_channels);
	ms_filter_call_method(ecc->sndwrite,MS_FILTER_GET_NCHANNELS,&channels);
	ms_filter_call_method(ecc->write_resampler,MS_FILTER_SET_SAMPLE_RATE,&ecc->rate);
	ms_filter_call_method(ecc->write_resampler,MS_FILTER_SET_OUTPUT_SAMPLE_RATE,&rate);
	ms_filter_call_method(ecc->write_resampler,MS_FILTER_SET_NCHANNELS,&ecc_channels);
	ms_filter_call_method(ecc->write_resampler,MS_FILTER_SET_OUTPUT_NCHANNELS,&channels);

	ms_filter_link(ecc->play,0,ecc->gen,0);
	ms_filter_link(ecc->gen,0,ecc->write_resampler,0);
	ms_filter_link(ecc->write_resampler,0,ecc->sndwrite,0);

	ms_ticker_attach(ecc->ticker,ecc->sndread);
	ms_ticker_attach(ecc->ticker,ecc->play);

	if (ecc->audio_init_cb != NULL) {
		(*ecc->audio_init_cb)(ecc->cb_data);
	}
}

static void ecc_deinit_filters(EcCalibrator *ecc){
	if (ecc->audio_uninit_cb != NULL) {
		(*ecc->audio_uninit_cb)(ecc->cb_data);
	}

	ms_ticker_detach(ecc->ticker,ecc->sndread);
	ms_ticker_detach(ecc->ticker,ecc->play);

	ms_filter_unlink(ecc->play,0,ecc->gen,0);
	ms_filter_unlink(ecc->gen,0,ecc->write_resampler,0);
	ms_filter_unlink(ecc->write_resampler,0,ecc->sndwrite,0);

	ms_filter_unlink(ecc->sndread,0,ecc->read_resampler,0);
	ms_filter_unlink(ecc->read_resampler,0,ecc->det,0);
	ms_filter_unlink(ecc->det,0,ecc->rec,0);

	ms_filter_destroy(ecc->sndread);
	ms_filter_destroy(ecc->det);
	ms_filter_destroy(ecc->rec);
	ms_filter_destroy(ecc->play);
	ms_filter_destroy(ecc->gen);
	ms_filter_destroy(ecc->read_resampler);
	ms_filter_destroy(ecc->write_resampler);
	ms_filter_destroy(ecc->sndwrite);

	ms_ticker_destroy(ecc->ticker);
}

static void on_tone_sent(void *data, MSFilter *f, unsigned int event_id, void *arg){
	MSDtmfGenEvent *ev=(MSDtmfGenEvent*)arg;
	EcCalibrator *ecc=(EcCalibrator*)data;
	if (ev->tone_name[0] != '\0'){
		ecc->acc-=ev->tone_start_time;
		ms_message("Sent tone at %u",(unsigned int)ev->tone_start_time);
	}
}

static bool_t is_valid_tone(EcCalibrator *ecc, MSToneDetectorEvent *ev){
	bool_t *toneflag=NULL;
	if (strcmp(ev->tone_name,"freq1")==0){
		toneflag=&ecc->freq1;
	}else if (strcmp(ev->tone_name,"freq2")==0){
		toneflag=&ecc->freq2;
	}else if (strcmp(ev->tone_name,"freq3")==0){
		toneflag=&ecc->freq3;
	}else{
		ms_error("Calibrator bug.");
		return FALSE;
	}
	if (*toneflag){
		ms_message("Duplicated tone event, ignored.");
		return FALSE;
	}
	*toneflag=TRUE;
	return TRUE;
}

static void on_tone_received(void *data, MSFilter *f, unsigned int event_id, void *arg){
	MSToneDetectorEvent *ev=(MSToneDetectorEvent*)arg;
	EcCalibrator *ecc=(EcCalibrator*)data;
	if (is_valid_tone(ecc,ev)){
		ecc->acc+=ev->tone_start_time;
		ms_message("Received tone at %u",(unsigned int)ev->tone_start_time);
	}
}

static void ecc_play_tones(EcCalibrator *ecc){
	MSDtmfGenCustomTone tone;
	MSToneDetectorDef expected_tone;
	
	memset(&tone,0,sizeof(tone));
	memset(&expected_tone,0,sizeof(expected_tone));

	ms_filter_add_notify_callback(ecc->det,on_tone_received,ecc,TRUE);

	/* configure the tones to be scanned */
	
	strncpy(expected_tone.tone_name,"freq1",sizeof(expected_tone.tone_name));
	expected_tone.frequency=(int)2349.32;
	expected_tone.min_duration=40;
	expected_tone.min_amplitude=0.1f;

	ms_filter_call_method (ecc->det,MS_TONE_DETECTOR_ADD_SCAN,&expected_tone);
	
	strncpy(expected_tone.tone_name,"freq2",sizeof(expected_tone.tone_name));
	expected_tone.frequency=(int)2637.02;
	expected_tone.min_duration=40;
	expected_tone.min_amplitude=0.1f;

	ms_filter_call_method (ecc->det,MS_TONE_DETECTOR_ADD_SCAN,&expected_tone);
	
	strncpy(expected_tone.tone_name,"freq3",sizeof(expected_tone.tone_name));
	expected_tone.frequency=(int)2093;
	expected_tone.min_duration=40;
	expected_tone.min_amplitude=0.1f;

	ms_filter_call_method (ecc->det,MS_TONE_DETECTOR_ADD_SCAN,&expected_tone);
	
	/*play an initial tone to startup the audio playback/capture*/
	
	tone.frequencies[0]=140;
	tone.duration=1000;
	tone.amplitude=0.5;

	ms_filter_call_method(ecc->gen,MS_DTMF_GEN_PLAY_CUSTOM,&tone);
	ms_sleep(2);

	ms_filter_add_notify_callback(ecc->gen,on_tone_sent,ecc,TRUE);
	
	/* play the three tones*/
	
	
	if (ecc->play_cool_tones){
		strncpy(tone.tone_name, "D", sizeof(tone.tone_name));
		tone.frequencies[0]=(int)2349.32;
		tone.duration=100;
		ms_filter_call_method(ecc->gen,MS_DTMF_GEN_PLAY_CUSTOM,&tone);
		ms_usleep(300000);
		
		strncpy(tone.tone_name, "E", sizeof(tone.tone_name));
		tone.frequencies[0]=(int)2637.02;
		tone.duration=100;
		ms_filter_call_method(ecc->gen,MS_DTMF_GEN_PLAY_CUSTOM,&tone);
		ms_usleep(300000);
		
		strncpy(tone.tone_name, "C", sizeof(tone.tone_name));
		tone.frequencies[0]=(int)2093;
		tone.duration=100;
		ms_filter_call_method(ecc->gen,MS_DTMF_GEN_PLAY_CUSTOM,&tone);
		ms_usleep(300000);
	}else{
		strncpy(tone.tone_name, "C", sizeof(tone.tone_name));
		tone.frequencies[0]=(int)2093;
		tone.duration=100;
		ms_filter_call_method(ecc->gen,MS_DTMF_GEN_PLAY_CUSTOM,&tone);
		ms_usleep(300000);
		
		strncpy(tone.tone_name, "D", sizeof(tone.tone_name));
		tone.frequencies[0]=(int)2349.32;
		tone.duration=100;
		ms_filter_call_method(ecc->gen,MS_DTMF_GEN_PLAY_CUSTOM,&tone);
		ms_usleep(300000);
		
		strncpy(tone.tone_name, "E", sizeof(tone.tone_name));
		tone.frequencies[0]=(int)2637.02;
		tone.duration=100;
		ms_filter_call_method(ecc->gen,MS_DTMF_GEN_PLAY_CUSTOM,&tone);
		ms_usleep(300000);
	}
	
	/*these two next ones are for lyrism*/
	if (ecc->play_cool_tones){
		tone.tone_name[0]='\0';
		tone.frequencies[0]=(int)1046.5;
		tone.duration=400;
		ms_filter_call_method(ecc->gen,MS_DTMF_GEN_PLAY_CUSTOM,&tone);
		ms_usleep(300000);
		
		tone.tone_name[0]='\0';
		tone.frequencies[0]=(int)1567.98;
		tone.duration=400;
		ms_filter_call_method(ecc->gen,MS_DTMF_GEN_PLAY_CUSTOM,&tone);
	}
	
	ms_sleep(1);
	
	if (ecc->freq1 && ecc->freq2 && ecc->freq3) {
		int delay=(int)(ecc->acc/3);
		if (delay<0){
			ms_error("Quite surprising calibration result, delay=%i",delay);
			ecc->status=LinphoneEcCalibratorFailed;
		}else{
			ms_message("Echo calibration estimated delay to be %i ms",delay);
			ecc->delay=delay;
			ecc->status=LinphoneEcCalibratorDone;
		}
	} else if ((ecc->freq1 || ecc->freq2 || ecc->freq3)==FALSE) {
			ms_message("Echo calibration succeeded, no echo has been detected");
			ecc->status = LinphoneEcCalibratorDoneNoEcho;
	} else {
			ecc->status = LinphoneEcCalibratorFailed;
	}

	if (ecc->status == LinphoneEcCalibratorFailed) {
		ms_error("Echo calibration failed.");
	}
}

static void  * ecc_thread(void *p){
	EcCalibrator *ecc=(EcCalibrator*)p;
	
	ecc_init_filters(ecc);
	ecc_play_tones(ecc);
	ecc_deinit_filters(ecc);
	ms_thread_exit(NULL);
	return NULL;
}

EcCalibrator * ec_calibrator_new(MSFactory *factory, MSSndCard *play_card, MSSndCard *capt_card, unsigned int rate, LinphoneEcCalibrationCallback cb,
				 LinphoneEcCalibrationAudioInit audio_init_cb, LinphoneEcCalibrationAudioUninit audio_uninit_cb, void *cb_data){
	EcCalibrator *ecc=ms_new0(EcCalibrator,1);

	ecc->rate=rate;
	ecc->cb=cb;
	ecc->cb_data=cb_data;
	ecc->audio_init_cb=audio_init_cb;
	ecc->audio_uninit_cb=audio_uninit_cb;
	ecc->capt_card=capt_card;
	ecc->play_card=play_card;
	ecc->factory=factory;
	return ecc;
}

void ec_calibrator_start(EcCalibrator *ecc){
	ms_thread_create(&ecc->thread,NULL,ecc_thread,ecc);
}

LinphoneEcCalibratorStatus ec_calibrator_get_status(EcCalibrator *ecc){
	return ecc->status;
}

void ec_calibrator_destroy(EcCalibrator *ecc){
	if (ecc->thread != 0) ms_thread_join(ecc->thread,NULL);
	ms_free(ecc);
}

int linphone_core_start_echo_calibration(LinphoneCore *lc, LinphoneEcCalibrationCallback cb,
					 LinphoneEcCalibrationAudioInit audio_init_cb, LinphoneEcCalibrationAudioUninit audio_uninit_cb, void *cb_data){
	unsigned int rate;

	if (lc->ecc!=NULL){
		ms_error("Echo calibration is still on going !");
		return -1;
	}
	rate = lp_config_get_int(lc->config,"sound","echo_cancellation_rate",8000);
	lc->ecc=ec_calibrator_new(lc->factory, lc->sound_conf.play_sndcard,lc->sound_conf.capt_sndcard,rate,cb,audio_init_cb,audio_uninit_cb,cb_data);
	lc->ecc->play_cool_tones = lp_config_get_int(lc->config, "sound", "ec_calibrator_cool_tones", 0);
	ec_calibrator_start(lc->ecc);
	return 0;
}

