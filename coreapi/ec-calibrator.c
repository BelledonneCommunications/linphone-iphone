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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "private.h"

#include "mediastreamer2/mstonedetector.h"
#include "mediastreamer2/dtmfgen.h"

#include "lpconfig.h"



static void ecc_init_filters(EcCalibrator *ecc){
	unsigned int rate;
	ecc->ticker=ms_ticker_new();

	ecc->sndread=ms_snd_card_create_reader(ecc->play_card);
	ms_filter_call_method(ecc->sndread,MS_FILTER_SET_SAMPLE_RATE,&ecc->rate);
	ecc->det=ms_filter_new(MS_TONE_DETECTOR_ID);
	ms_filter_call_method(ecc->det,MS_FILTER_SET_SAMPLE_RATE,&ecc->rate);
	ecc->rec=ms_filter_new(MS_FILE_REC_ID);

	ms_filter_link(ecc->sndread,0,ecc->det,0);
	ms_filter_link(ecc->det,0,ecc->rec,0);

	ecc->play=ms_filter_new(MS_FILE_PLAYER_ID);
	ecc->gen=ms_filter_new(MS_DTMF_GEN_ID);
	ms_filter_call_method(ecc->gen,MS_FILTER_SET_SAMPLE_RATE,&ecc->rate);
	ecc->resampler=ms_filter_new(MS_RESAMPLE_ID);
	ecc->sndwrite=ms_snd_card_create_writer(ecc->capt_card);

	ms_filter_link(ecc->play,0,ecc->gen,0);
	ms_filter_link(ecc->gen,0,ecc->resampler,0);
	ms_filter_link(ecc->resampler,0,ecc->sndwrite,0);

	ms_filter_call_method(ecc->sndwrite,MS_FILTER_SET_SAMPLE_RATE,&ecc->rate);
	ms_filter_call_method(ecc->sndwrite,MS_FILTER_GET_SAMPLE_RATE,&rate);
	ms_filter_call_method(ecc->resampler,MS_FILTER_SET_SAMPLE_RATE,&ecc->rate);
	ms_filter_call_method(ecc->resampler,MS_FILTER_SET_OUTPUT_SAMPLE_RATE,&rate);

	ms_ticker_attach(ecc->ticker,ecc->play);
	ms_ticker_attach(ecc->ticker,ecc->sndread);
}

static void ecc_deinit_filters(EcCalibrator *ecc){
	ms_ticker_detach(ecc->ticker,ecc->play);
	ms_ticker_detach(ecc->ticker,ecc->sndread);

	ms_filter_unlink(ecc->play,0,ecc->gen,0);
	ms_filter_unlink(ecc->gen,0,ecc->resampler,0);
	ms_filter_unlink(ecc->resampler,0,ecc->sndwrite,0);

	ms_filter_unlink(ecc->sndread,0,ecc->det,0);
	ms_filter_unlink(ecc->det,0,ecc->rec,0);

	ms_filter_destroy(ecc->sndread);
	ms_filter_destroy(ecc->det);
	ms_filter_destroy(ecc->rec);
	ms_filter_destroy(ecc->play);
	ms_filter_destroy(ecc->gen);
	ms_filter_destroy(ecc->resampler);
	ms_filter_destroy(ecc->sndwrite);

	ms_ticker_destroy(ecc->ticker);
}

static void on_tone_sent(void *data, MSFilter *f, unsigned int event_id, void *arg){
	MSDtmfGenEvent *ev=(MSDtmfGenEvent*)arg;
	EcCalibrator *ecc=(EcCalibrator*)data;
	ecc->sent_count++;
	ecc->acc-=ev->tone_start_time;
	ms_message("Sent tone at %u",(unsigned int)ev->tone_start_time);
}

static void on_tone_received(void *data, MSFilter *f, unsigned int event_id, void *arg){
	MSToneDetectorEvent *ev=(MSToneDetectorEvent*)arg;
	EcCalibrator *ecc=(EcCalibrator*)data;
	ecc->recv_count++;
	ecc->acc+=ev->tone_start_time;
	ms_message("Received tone at %u",(unsigned int)ev->tone_start_time);
}

static void ecc_play_tones(EcCalibrator *ecc){
	MSDtmfGenCustomTone tone;
	MSToneDetectorDef expected_tone;

	
	ms_filter_set_notify_callback(ecc->det,on_tone_received,ecc);

	expected_tone.frequency=2000;
	expected_tone.min_duration=40;
	expected_tone.min_amplitude=0.02;

	ms_filter_call_method (ecc->det,MS_TONE_DETECTOR_ADD_SCAN,&expected_tone);
	
	tone.frequency=1000;
	tone.duration=1000;
	tone.amplitude=1.0;

	/*play an initial tone to startup the audio playback/capture*/
	ms_filter_call_method(ecc->gen,MS_DTMF_GEN_PLAY_CUSTOM,&tone);
	ms_sleep(2);

	ms_filter_set_notify_callback(ecc->gen,on_tone_sent,ecc);
	tone.frequency=2000;
	tone.duration=100;

	ms_filter_call_method(ecc->gen,MS_DTMF_GEN_PLAY_CUSTOM,&tone);
	ms_sleep(1);
	ms_filter_call_method(ecc->gen,MS_DTMF_GEN_PLAY_CUSTOM,&tone);
	ms_sleep(1);
	ms_filter_call_method(ecc->gen,MS_DTMF_GEN_PLAY_CUSTOM,&tone);
	ms_sleep(1);

	if (ecc->sent_count==3 && ecc->recv_count==3){
		int delay=ecc->acc/3;
		if (delay<0){
			ms_error("Quite surprising calibration result, delay=%i",delay);
			ecc->status=LinphoneEcCalibratorFailed;
		}else{ms_message("Echo calibration estimated delay to be %i ms",delay);
			ecc->delay=delay;
			ecc->status=LinphoneEcCalibratorDone;
		}
	}else{
		ms_error("Echo calibration failed, tones received = %i",ecc->recv_count);
		ecc->status=LinphoneEcCalibratorFailed;
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

EcCalibrator * ec_calibrator_new(MSSndCard *play_card, MSSndCard *capt_card, unsigned int rate, LinphoneEcCalibrationCallback cb, void *cb_data ){
	EcCalibrator *ecc=ms_new0(EcCalibrator,1);

	ecc->rate=rate;
	ecc->cb=cb;
	ecc->cb_data=cb_data;
	ecc->capt_card=capt_card;
	ecc->play_card=play_card;
	ms_thread_create(&ecc->thread,NULL,ecc_thread,ecc);
	return ecc;
}

LinphoneEcCalibratorStatus ec_calibrator_get_status(EcCalibrator *ecc){
	return ecc->status;
}

void ec_calibrator_destroy(EcCalibrator *ecc){
	ms_thread_join(ecc->thread,NULL);
	ms_free(ecc);
}

int linphone_core_start_echo_calibration(LinphoneCore *lc, LinphoneEcCalibrationCallback cb, void *cb_data){
	if (lc->ecc!=NULL){
		ms_error("Echo calibration is still on going !");
		return -1;
	}
	unsigned int rate = lp_config_get_int(lc->config,"sound","echo_cancellation_rate",8000);
	lc->ecc=ec_calibrator_new(lc->sound_conf.play_sndcard,lc->sound_conf.capt_sndcard,rate,cb,cb_data);
	return 0;
}

