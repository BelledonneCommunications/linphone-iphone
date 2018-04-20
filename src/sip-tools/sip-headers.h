/*
 * sip-headers.h
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

#ifndef _L_SIP_HEADERS_H_
#define _L_SIP_HEADERS_H_

#include "linphone/utils/general.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

namespace PriorityHeader {
	constexpr char HeaderName[] = "Priority";

	// Values
	constexpr char NonUrgent[] = "non-urgent";
	constexpr char Urgent[] = "urgent";
	constexpr char Emergency[] = "emergency";
	constexpr char Normal[] = "normal";
}

LINPHONE_END_NAMESPACE

#endif // _L_SIP_HEADERS_H_
