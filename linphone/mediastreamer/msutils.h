/*
  The mediastreamer library aims at providing modular media processing and I/O
	for linphone, but also for any telephony application.
  Copyright (C) 2001  Simon MORLAT simon.morlat@linphone.org

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef MSUTILS_H
#define MSUTILS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GLIB
#include <glib.h>
#else
#include <uglib.h>
#endif
#include <errno.h>

#ifndef ENODATA
/* this is for freeBSD .*/
#define ENODATA EWOULDBLOCK	
#endif

#ifdef MS_DEBUG

#define ms_trace g_message

#else

#define ms_trace(...)
#endif

#define ms_warning g_warning
#define ms_error g_error

#define VIDEO_SIZE_CIF_W 352
#define VIDEO_SIZE_CIF_H 288
#define VIDEO_SIZE_QCIF_W 176
#define VIDEO_SIZE_QCIF_H 144
#define VIDEO_SIZE_4CIF_W 704
#define VIDEO_SIZE_4CIF_H 576
#define VIDEO_SIZE_MAX_W VIDEO_SIZE_4CIF_W
#define VIDEO_SIZE_MAX_H VIDEO_SIZE_4CIF_H


#endif
