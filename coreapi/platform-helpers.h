/*
linphone
Copyright (C) 2017 Belledonne Communications SARL

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

#ifndef platform_helpers_h
#define platform_helpers_h


namespace LinphonePrivate{

/**
 * This interface aims at abstracting some features offered by the platform, most often mobile platforms.
 * A per platform implementation is to be made to implement these features, if available on the platform
 */
class PlatformHelpers{
	public:
		//This method shall retrieve DNS server list from the platform and assign it to the core.
		virtual void setDnsServers() = 0;
		virtual void acquireWifiLock() = 0;
		virtual void releaseWifiLock() = 0;
		virtual void acquireMcastLock() = 0;
		virtual void releaseMcastLock() = 0;
		virtual void acquireCpuLock() = 0;
		virtual void releaseCpuLock() = 0;
		virtual ~PlatformHelpers();
	protected:
		PlatformHelpers(LinphoneCore *lc) : mCore(lc){
		}
		LinphoneCore *mCore;
};

class StubbedPlatformHelpers : public PlatformHelpers{
public:
	StubbedPlatformHelpers(LinphoneCore *lc);
	virtual void setDnsServers();
	virtual void acquireWifiLock();
	virtual void releaseWifiLock();
	virtual void acquireMcastLock();
	virtual void releaseMcastLock();
	virtual void acquireCpuLock();
	virtual void releaseCpuLock();
	virtual ~StubbedPlatformHelpers();
};

PlatformHelpers *createAndroidPlatformHelpers(LinphoneCore *lc, void *system_context);
PlatformHelpers *createIosPlatformHelpers(LinphoneCore *lc, void *system_context);

}//end of namespace

#endif
