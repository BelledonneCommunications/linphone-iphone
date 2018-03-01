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

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

#if TARGET_OS_IPHONE

#include "private.h"
#include "platform-helpers.h"



namespace LinphonePrivate{

class IosPlatformHelpers : public PlatformHelpers{
public:
	IosPlatformHelpers(LinphoneCore *lc, void *system_context);
	virtual void setDnsServers();
	virtual void acquireWifiLock();
	virtual void releaseWifiLock();
	virtual void acquireMcastLock();
	virtual void releaseMcastLock();
	virtual void acquireCpuLock();
	virtual void releaseCpuLock();
	~IosPlatformHelpers();
private:
	void bgTaskTimeout();
	static void sBgTaskTimeout(void *data);
	long int mCpuLockTaskId;
	int mCpuLockCount;
};


IosPlatformHelpers::IosPlatformHelpers(LinphoneCore *lc, void *system_context) : PlatformHelpers(lc) {
	mCpuLockCount = 0;
	mCpuLockTaskId = 0;
	ms_message("IosPlatformHelpers is fully initialised");
}

IosPlatformHelpers::~IosPlatformHelpers(){
	
}


void IosPlatformHelpers::setDnsServers(){
}

void IosPlatformHelpers::acquireWifiLock(){
}

void IosPlatformHelpers::releaseWifiLock(){
}

void IosPlatformHelpers::acquireMcastLock(){
}

void IosPlatformHelpers::releaseMcastLock(){
}

void IosPlatformHelpers::bgTaskTimeout(){
	ms_error("IosPlatformHelpers: the system requests that the cpu lock is released now.");
	if (mCpuLockTaskId != 0){
		belle_sip_end_background_task(mCpuLockTaskId);
		mCpuLockTaskId = 0;
	}
}

void IosPlatformHelpers::sBgTaskTimeout(void *data){
	IosPlatformHelpers *zis = (IosPlatformHelpers*)data;
	zis->bgTaskTimeout();
}

void IosPlatformHelpers::acquireCpuLock(){
	if (mCpuLockCount == 0){
		/* on iOS, cpu lock is implemented by a long running task (obcj-C) and it is abstracted by belle-sip,
		so let's use belle-sip directly.*/
		mCpuLockTaskId = belle_sip_begin_background_task("Liblinphone cpu lock", sBgTaskTimeout, this);
	}
	mCpuLockCount++;
}

void IosPlatformHelpers::releaseCpuLock(){
	mCpuLockCount--;
	if (mCpuLockCount == 0){
		if (mCpuLockTaskId != 0){
			belle_sip_end_background_task(mCpuLockTaskId);
			mCpuLockTaskId = 0;
		}else{
			ms_error("IosPlatformHelpers::releaseCpuLock(): too late, the lock has been released already by the system.");
		}
	}
}


PlatformHelpers *createIosPlatformHelpers(LinphoneCore *lc, void *system_context){
	return new IosPlatformHelpers(lc, system_context);
}

}//end of namespace

#endif
