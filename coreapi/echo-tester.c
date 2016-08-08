/*
linphone
Copyright (C) 2016 Belledonne Communications SARL
Author: Erwan CROZE (erwan.croze@belledonne-communications.com)

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

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msticker.h"
#include <ortp/ortp.h>

EchoTester* ec_tester_new(MSFactory *factory, MSSndCard *capture_card, MSSndCard *playback_card, unsigned int rate) {
    EchoTester *ect = ms_new0(EchoTester,1);
    ect->factory = factory;
    ect->capture_card = capture_card;
    ect->playback_card = playback_card;
    ect->rate = rate;

    return ect;
}

static void ect_init_filters(EchoTester *ect) {
    unsigned int rate;
    int channels = 1;
    int ect_channels = 1;
    MSTickerParams params={0};
	params.name="Echo tester";
	params.prio=MS_TICKER_PRIO_HIGH;
	ect->ticker=ms_ticker_new_with_params(&params);

    ect->in = ms_snd_card_create_reader(ect->capture_card);
    ect->out = ms_snd_card_create_writer(ect->playback_card);

    ms_filter_call_method(ect->in,MS_FILTER_SET_SAMPLE_RATE,&ect->rate);
    ms_filter_call_method(ect->in,MS_FILTER_GET_SAMPLE_RATE,&rate);
    ms_filter_call_method(ect->in,MS_FILTER_SET_NCHANNELS,&ect_channels);
    ms_filter_call_method(ect->in,MS_FILTER_GET_NCHANNELS,&channels);

    ms_filter_call_method(ect->out,MS_FILTER_SET_SAMPLE_RATE,&ect->rate);
    ms_filter_call_method(ect->out,MS_FILTER_SET_OUTPUT_SAMPLE_RATE,&rate);
    ms_filter_call_method(ect->out,MS_FILTER_SET_NCHANNELS,&ect_channels);
    ms_filter_call_method(ect->out,MS_FILTER_SET_OUTPUT_NCHANNELS,&channels);

    ms_filter_link(ect->in,0,ect->out,0);

    ms_ticker_attach(ect->ticker,ect->in);
    ms_ticker_attach(ect->ticker,ect->out);
}

static void ect_uninit_filters(EchoTester *ect) {
    ms_ticker_detach(ect->ticker,ect->in);
    ms_ticker_detach(ect->ticker,ect->out);

    ms_filter_unlink(ect->in,0,ect->out,0);

    ms_filter_destroy(ect->in);
    ms_filter_destroy(ect->out);

    ms_ticker_destroy(ect->ticker);
}

void ec_tester_destroy(EchoTester *ect) {
    ms_free(ect);
}

int linphone_core_start_echo_tester(LinphoneCore *lc, unsigned int rate) {
    if (lc->ect != NULL) {
        ms_error("Echo tester is still on going !");
        return -1;
    }
    lc->ect = ec_tester_new(lc->factory, lc->sound_conf.capt_sndcard
        ,lc->sound_conf.play_sndcard, rate);
    ect_init_filters(lc->ect);

    return 1;
}

int linphone_core_stop_echo_tester(LinphoneCore *lc) {
    if (lc->ect == NULL) {
        ms_error("Echo tester is not running !");
        return -1;
    }
    ect_uninit_filters(lc->ect);

    ec_tester_destroy(lc->ect);
    lc->ect = NULL;
    return 1;
}