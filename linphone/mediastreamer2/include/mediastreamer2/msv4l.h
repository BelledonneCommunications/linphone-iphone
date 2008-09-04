/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

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

#ifndef msv4l_h
#define msv4l_h

#include "msfilter.h"

#define	MS_V4L_START		MS_FILTER_METHOD_NO_ARG(MS_V4L_ID,0)
#define	MS_V4L_STOP			MS_FILTER_METHOD_NO_ARG(MS_V4L_ID,1)
#define	MS_V4L_SET_DEVICE	MS_FILTER_METHOD(MS_V4L_ID,2,int)
#define	MS_V4L_SET_IMAGE	MS_FILTER_METHOD(MS_V4L_ID,3,char)

#endif
