/*
 * paths-linux.cpp
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

#include "logger/logger.h"

#include "paths-linux.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

static string getBaseDirectory () {
	static string base;
	if (base.empty()) {
		char *dir = getenv("HOME");
		if (!dir)
			lFatal() << "Unable to get home directory.";
		base = dir;
	}
	return base;
}

string SysPaths::getDataPath (PlatformHelpers *) {
	static string dataPath = getBaseDirectory() + "/.local/share/linphone/";
	return dataPath;
}

string SysPaths::getConfigPath (PlatformHelpers *) {
	static string configPath = getBaseDirectory() + "/.config/linphone/";
	return configPath;
}

LINPHONE_END_NAMESPACE
