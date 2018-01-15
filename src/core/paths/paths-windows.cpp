/*
 * paths-windows.cpp
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

#include <stream>

#include "shlobj.h"

#include "linphone/utils/utils.h"

#include "core/platform-helpers/platform-helpers.h"

#include "paths-windows.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

string SysPaths::getDataPath (PlatformHelpers *platformHelper) {
	TCHAR szPath[MAX_PATH];
	// Get path for each computer, non-user specific and non-roaming data.
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath))) {
		stringstream path;
		path << boost::lexical_cast<std::string>(szPath) << "\\linphone\\";
		string ret = path.str();
		boost::replace_all(ret, "\\", "\\\\");
		return ret;
	}

	return Utils::getEmptyConstRefObject<std::string>();
}

string SysPaths::getConfigPath (PlatformHelpers *platformHelper) {
	// Seems to be the same directory.
	return getDataPath(platformHelper);
}

LINPHONE_END_NAMESPACE
