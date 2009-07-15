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

#ifndef msequalizer_h
#define msequalizer_h

#include <mediastreamer2/msfilter.h>

typedef struct _MSEqualizerGain{
	float frequency; ///< In hz
	float gain; ///< between 0-1.2
}MSEqualizerGain;

#define MS_EQUALIZER_SET_GAIN	MS_FILTER_METHOD(MS_EQUALIZER_ID,0,MSEqualizerGain)
#define MS_EQUALIZER_GET_GAIN	MS_FILTER_METHOD(MS_EQUALIZER_ID,1,MSEqualizerGain)
#define MS_EQUALIZER_SET_ACTIVE	MS_FILTER_METHOD(MS_EQUALIZER_ID,2,int)
/**
 * Sets the coeficient for computing how large is the frequency band around a gain setting.
 * For example, setting this value to 0.4 will result in a frequency range of 800-1200 when setting
 * a gain for frequency 1000 Hz.
**/
#define MS_EQUALIZER_SET_FREQ_WIDTH_COEF MS_FILTER_METHOD(MS_EQUALIZER_ID,3,float)

#endif

