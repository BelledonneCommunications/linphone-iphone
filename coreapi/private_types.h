/*
 * private_types.h
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _PRIVATE_TYPES_H_
#define _PRIVATE_TYPES_H_


typedef struct _CallCallbackObj CallCallbackObj;

typedef struct StunCandidate StunCandidate;

typedef struct _PortConfig PortConfig;

typedef struct _LinphoneFriendPresence LinphoneFriendPresence;

typedef struct _LinphoneFriendPhoneNumberSipUri LinphoneFriendPhoneNumberSipUri;

typedef struct sip_config sip_config_t;

typedef struct rtp_config rtp_config_t;

typedef struct net_config net_config_t;

typedef struct sound_config sound_config_t;

typedef struct codecs_config codecs_config_t;

typedef struct video_config video_config_t;

typedef struct text_config text_config_t;

typedef struct ui_config ui_config_t;

typedef struct autoreplier_config autoreplier_config_t;

typedef struct _LinphoneToneDescription LinphoneToneDescription;

typedef struct _LinphoneTaskList LinphoneTaskList;

typedef struct _LCCallbackObj LCCallbackObj;

typedef struct _EcCalibrator EcCalibrator;

typedef struct _xmlparsing_context xmlparsing_context_t;

typedef struct _VTableReference  VTableReference;

typedef struct _EchoTester EchoTester;

typedef struct _LinphoneXmlRpcArg LinphoneXmlRpcArg;

#endif /* _PRIVATE_TYPES_H_ */
