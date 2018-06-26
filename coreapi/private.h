/***************************************************************************
 *            private.h
 *
 *  Mon Jun 13 14:23:23 2005
 *  Copyright  2005  Simon Morlat
 *  Email simon dot morlat at linphone dot org
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _PRIVATE_H
#define _PRIVATE_H

#include <memory>

#include "linphone/core.h"
#include "linphone/friend.h"
#include "linphone/friendlist.h"
#include "linphone/tunnel.h"

#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic push
#endif
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#else
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#include "linphone/core_utils.h"
#if __clang__ || ((__GNUC__ == 4 && __GNUC_MINOR__ >= 6) || __GNUC__ > 4)
#pragma GCC diagnostic pop
#endif

#include "linphone/conference.h"

#include "address/address.h"
#include "c-wrapper/internal/c-sal.h"
#include "sal/call-op.h"
#include "sal/event-op.h"
#include "sal/message-op.h"
#include "sal/presence-op.h"
#include "sal/register-op.h"

#ifdef __cplusplus
#include "core/platform-helpers/platform-helpers.h"
#endif

#include "linphone/sipsetup.h"
#include "quality_reporting.h"
#include "linphone/ringtoneplayer.h"
#include "vcard_private.h"
#include "carddav.h"
#include "linphone/player.h"
#include "account_creator_private.h"
#include "tester_utils.h"

#include "bctoolbox/port.h"
#include "bctoolbox/map.h"
#include "bctoolbox/vfs.h"
#include "belle-sip/belle-sip.h" /*we need this include for all http operations*/


#include <ctype.h>


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "mediastreamer2/ice.h"
#include "mediastreamer2/mediastream.h"
#include "mediastreamer2/msconference.h"


#ifndef LIBLINPHONE_VERSION
#define LIBLINPHONE_VERSION LINPHONE_VERSION
#endif

#ifdef __ANDROID__
#include <jni.h>
#endif

#ifdef _WIN32
#if defined(__MINGW32__) || !defined(WINAPI_FAMILY_PARTITION) || !defined(WINAPI_PARTITION_DESKTOP)
#define LINPHONE_WINDOWS_DESKTOP 1
#elif defined(WINAPI_FAMILY_PARTITION)
#if defined(WINAPI_PARTITION_DESKTOP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define LINPHONE_WINDOWS_DESKTOP 1
#elif defined(WINAPI_PARTITION_PHONE_APP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE_APP)
#define LINPHONE_WINDOWS_PHONE 1
#elif defined(WINAPI_PARTITION_APP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#define LINPHONE_WINDOWS_UNIVERSAL 1
#endif
#endif
#endif
#ifdef _MSC_VER
#if (_MSC_VER >= 1900)
#define LINPHONE_MSC_VER_GREATER_19
#endif
#endif


#ifdef SQLITE_STORAGE_ENABLED
#include <sqlite3.h>
#endif


#include "private_structs.h"
#include "private_functions.h"
#include "core_private.h"


#define STRING_RESET(field)	if (field) bctbx_free(field); (field) = NULL
#define STRING_SET(field, value)	do{ if (field){bctbx_free(field);field=NULL;}; field=bctbx_strdup(value); }while(0)
#define STRING_TRANSFER(field, newvalue)	do{ if (field){bctbx_free(field);field=NULL;}; field=newvalue; }while(0)

#ifdef __cplusplus
#define getPlatformHelpers(lc) static_cast<LinphonePrivate::PlatformHelpers*>(lc->platform_helper)
#endif


#define HOLD_OFF	(0)
#define HOLD_ON		(1)

#ifndef NB_MAX_CALLS
#define NB_MAX_CALLS	(10)
#endif

#define LINPHONE_MAX_CALL_HISTORY_UNLIMITED (-1)
#ifndef LINPHONE_MAX_CALL_HISTORY_SIZE
	#ifdef SQLITE_STORAGE_ENABLED
		#define LINPHONE_MAX_CALL_HISTORY_SIZE LINPHONE_MAX_CALL_HISTORY_UNLIMITED
	#else
		#define LINPHONE_MAX_CALL_HISTORY_SIZE 30
	#endif
#endif

#define LINPHONE_SQLITE3_VFS "sqlite3bctbx_vfs"

#endif /* _PRIVATE_H */
