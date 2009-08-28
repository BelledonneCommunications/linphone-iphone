/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006-2009  Simon MORLAT (simon.morlat@linphone.org)

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

msspeexec.h : interface of the speex echo canceler integration in mediastreamer2

*/

#ifndef msspeexec_h
#define msspeexec_h

#include <mediastreamer2/msfilter.h>

/** sets the tail length in milliseconds*/
#define MS_SPEEX_EC_SET_TAIL_LENGTH	MS_FILTER_METHOD(MS_SPEEX_EC_ID,0,int)

/** sets the minimum delay of the echo if known. This optimizes the convergence*/
#define MS_SPEEX_EC_SET_DELAY		MS_FILTER_METHOD(MS_SPEEX_EC_ID,1,int)

/** sets the frame size for the AU-MDF algorithm, in number of fft points*/
#define MS_SPEEX_EC_SET_FRAME_SIZE	MS_FILTER_METHOD(MS_SPEEX_EC_ID,2,int)


#endif
