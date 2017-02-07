/*
defs.h
Copyright (C) 2010-2017 Belledonne Communications SARL

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

#ifndef LINPHONE_DEFS_H_
#define LINPHONE_DEFS_H_


#include "mediastreamer2/mscommon.h"


#define LINPHONE_IPADDR_SIZE 64
#define LINPHONE_HOSTNAME_SIZE 128

#ifndef LINPHONE_PUBLIC
#if defined(_MSC_VER)
#ifdef LINPHONE_STATIC
#define LINPHONE_PUBLIC
#else
#ifdef LINPHONE_EXPORTS
#define LINPHONE_PUBLIC	__declspec(dllexport)
#else
#define LINPHONE_PUBLIC	__declspec(dllimport)
#endif
#endif
#else
#define LINPHONE_PUBLIC
#endif
#endif


#ifndef LINPHONE_DEPRECATED
#define LINPHONE_DEPRECATED MS2_DEPRECATED
#endif


#endif /* LINPHONE_DEFS_H_ */
