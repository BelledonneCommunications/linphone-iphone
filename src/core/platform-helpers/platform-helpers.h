/*
 * platform-helpers.h
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

#ifndef _L_PLATFORM_HELPERS_H_
#define _L_PLATFORM_HELPERS_H_

#include <string>

#include "linphone/utils/general.h"

// =============================================================================

L_DECL_C_STRUCT(LinphoneCore);

LINPHONE_BEGIN_NAMESPACE

/**
 * This interface aims at abstracting some features offered by the platform, most often mobile platforms.
 * A per platform implementation is to be made to implement these features, if available on the platform.
 */
class PlatformHelpers {
public:
	virtual ~PlatformHelpers () = default;

	// This method shall retrieve DNS server list from the platform and assign it to the core.
	virtual void setDnsServers () = 0;
	virtual void acquireWifiLock () = 0;
	virtual void releaseWifiLock () = 0;
	virtual void acquireMcastLock () = 0;
	virtual void releaseMcastLock () = 0;
	virtual void acquireCpuLock () = 0;
	virtual void releaseCpuLock () = 0;
	virtual std::string getDataPath () = 0;
	virtual std::string getConfigPath () = 0;

protected:
	inline explicit PlatformHelpers (LinphoneCore *lc) : mCore(lc) {}

	LinphoneCore *mCore;
};

class StubbedPlatformHelpers : public PlatformHelpers {
public:
	explicit StubbedPlatformHelpers (LinphoneCore *lc);
	virtual ~StubbedPlatformHelpers () = default;

	void setDnsServers () override;
	void acquireWifiLock () override;
	void releaseWifiLock () override;
	void acquireMcastLock () override;
	void releaseMcastLock () override;
	void acquireCpuLock () override;
	void releaseCpuLock () override;
	std::string getDataPath () override;
	std::string getConfigPath () override;
};

PlatformHelpers *createAndroidPlatformHelpers (LinphoneCore *lc, void *systemContext);
PlatformHelpers *createIosPlatformHelpers (LinphoneCore *lc, void *systemContext);

LINPHONE_END_NAMESPACE

#endif // indef _L_PLATFORM_HELPERS_H_
