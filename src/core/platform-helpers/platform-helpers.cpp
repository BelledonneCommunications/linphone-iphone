/*
 * platform-helpers.cpp
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

#include "platform-helpers.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

StubbedPlatformHelpers::StubbedPlatformHelpers (LinphoneCore *lc) : PlatformHelpers(lc) {}

void StubbedPlatformHelpers::setDnsServers () {}

void StubbedPlatformHelpers::acquireWifiLock () {}

void StubbedPlatformHelpers::releaseWifiLock () {}

void StubbedPlatformHelpers::acquireMcastLock () {}

void StubbedPlatformHelpers::releaseMcastLock () {}

void StubbedPlatformHelpers::acquireCpuLock () {}

void StubbedPlatformHelpers::releaseCpuLock () {}

string StubbedPlatformHelpers::getDataPath () {
	return "";
}

string StubbedPlatformHelpers::getConfigPath () {
	return "";
}

LINPHONE_END_NAMESPACE
