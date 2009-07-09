/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2009  Simon MORLAT (simon.morlat@linphone.org)

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

#include <mediastreamer2/msfilter.h>


typedef struct _EqualizerState{
	int rate;
	int nfft; //number of fft points in time
	int16_t *fft_cpx;
	int fir_len;
	int16_t *fir;
} EqualizerState;

static EqualizerState * equalizer_state_new(){
	EqualizerState *s=ms_new0(EqualizerState,1);
	s->rate=8000;
	s->nfft=128;
	s->fft_cpx=ms_new0(int16_t,s->nfft);
	s->fir_len=s->nfft;
	s->fir=ms_new(int16_t,s->fir_len);
}

static int equalizer_state_hz_to_index(EqualizerState *s, int hz){
	int ret;
	if (hz<0){
		ms_error("Bad frequency value %i",hz);
		return -1;
	}
	if (hz>(s->rate/2)){
		hz=(s->rate/2);
	}
	ret=hz*s->nfft/s->rate;
	if (ret==s->nfft/2) ret--;
	return ret;
}

static float gain_float(int16_t val){
	return (float)val/22000.0;
}

static int16_t gain_int16(float val){
	int ret=(int)(val*22000.0);
	if (ret>=32767) ret=32767;
	return (int16_t)ret;
}

static float equalizer_get(EqualizerState *s, int freqhz){
	int idx=equalizer_state_hz_to_index(s,freqhz);
	if (idx>=0) return gain_float(s->fft_cpx[idx*2]);
	return 0;
}

/* return the frequency band width we want to control around hz*/
static void equalizer_get_band(EqualizerState *s, int hz, int *low_index, int *high_index){
	int half_band=(int)((float)hz*0.1);
	*low_index=equalizer_state_hz_to_index(s,hz-half_band);
	*high_index=equalizer_state_hz_to_index(s,hz+half_band);
}

static int16_t equalizer_set(EqualizerState *s, int freqhz, float gain){
	int low,high;
	int i;
	equalizer_get_band(s,freqhz,&low,&high);
	for(i=low;i<=high;++i){
		ms_message("Setting gain %f for freq_index %i (freqhz=%i)",gain,i,freqhz);
		s->fft_cpx[i*2]=gain_int16(gain);
	}
}
