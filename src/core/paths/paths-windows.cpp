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

#include <algorithm>
#include <comutil.h>
#include <ShlObj_core.h>

#pragma comment(lib, "comsuppw.lib")
#pragma comment(lib, "kernel32.lib")

#include "core/platform-helpers/platform-helpers.h"

#include "paths-windows.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

static string getPath (const GUID &id) {
	string strPath;

	LPWSTR path;
	if (SHGetKnownFolderPath(id, KF_FLAG_DONT_VERIFY, 0, &path) == S_OK) {
		strPath = _bstr_t(path);
		replace(strPath.begin(), strPath.end(), '\\', '/');
		CoTaskMemFree(path);
	}

	return strPath.append("/linphone/");
}


string SysPaths::getDataPath (PlatformHelpers *) {
	static string dataPath = getPath(FOLDERID_LocalAppData);
	return dataPath;
}

string SysPaths::getConfigPath (PlatformHelpers *platformHelpers) {
	// Yes, same path.
	return getDataPath(platformHelpers);
}

LINPHONE_END_NAMESPACE
