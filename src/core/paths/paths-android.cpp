/*
 * paths-android.cpp
 * Copyright (C) 2010-2017 Belledonne Communications SARL
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

#include <jni.h>

#include "core/platform-helpers/platform-helpers.h"
#include "linphone/utils/utils.h"

#include "paths-android.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

const std::string &SysPaths::getDataPath (PlatformHelper *platformHelper) {
	if (!platformHelper) {
		return Utils::getEmptyConstRefObject<std::string>();
	}
	return platformHelper->getDataPath();
}

const std::string &SysPaths::getConfigPath (PlatformHelper *platformHelper) {
	if (!platformHelper) {
		return Utils::getEmptyConstRefObject<std::string>();
	}
	return platformHelper->getConfigPath();
}

LINPHONE_END_NAMESPACE
