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

#include <CoreFoundation/CoreFoundation.h>

#include <belr/grammarbuilder.h>

#include "linphone/utils/general.h"
#include "linphone/utils/utils.h"

#include "logger/logger.h"
#include "platform-helpers.h"

// TODO: Remove me
#include "private.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

class IosPlatformHelpers : public PlatformHelpers {
public:
	IosPlatformHelpers (LinphoneCore *lc, void *system_context);
	~IosPlatformHelpers () = default;

	void setDnsServers () override {}
	void acquireWifiLock () override {}
	void releaseWifiLock () override {}
	void acquireMcastLock () override {}
	void releaseMcastLock () override {}
	void acquireCpuLock () override;
	void releaseCpuLock () override;
	string getDataPath () override {return Utils::getEmptyConstRefObject<string>();}
	string getConfigPath () override {return Utils::getEmptyConstRefObject<string>();}

private:
	void bgTaskTimeout ();
	static void sBgTaskTimeout (void *data);
	static std::string directoryForResource (CFStringRef framework, CFStringRef resource);

	long int mCpuLockTaskId;
	int mCpuLockCount;
};

// =============================================================================

IosPlatformHelpers::IosPlatformHelpers (LinphoneCore *lc, void *system_context) : PlatformHelpers(lc) {
	mCpuLockCount = 0;
	mCpuLockTaskId = 0;

	std::string cpimPath = directoryForResource(CFSTR("org.linphone.linphone"), CFSTR("cpim_grammar"));
	std::string vcardPath = directoryForResource(CFSTR("org.linphone.belcard"), CFSTR("vcard_grammar"));
	if (!cpimPath.empty())
		belr::GrammarLoader::get().addPath(cpimPath);
	else
		lError() << "IosPlatformHelpers did not find cpim grammar resource directory...";

	if (!vcardPath.empty())
		belr::GrammarLoader::get().addPath(vcardPath);
	else
		lError() << "IosPlatformHelpers did not find vcard grammar resource directory...";

	lInfo() << "IosPlatformHelpers is fully initialised.";
}

// -----------------------------------------------------------------------------

void IosPlatformHelpers::bgTaskTimeout () {
	lError() << "IosPlatformHelpers: the system requests that the cpu lock is released now.";
	if (mCpuLockTaskId != 0) {
		belle_sip_end_background_task(static_cast<unsigned long>(mCpuLockTaskId));
		mCpuLockTaskId = 0;
	}
}

void IosPlatformHelpers::sBgTaskTimeout (void *data) {
	IosPlatformHelpers *zis = static_cast<IosPlatformHelpers *>(data);
	zis->bgTaskTimeout();
}

// -----------------------------------------------------------------------------

void IosPlatformHelpers::acquireCpuLock () {
	// on iOS, cpu lock is implemented by a long running task and it is abstracted by belle-sip, so let's use belle-sip directly.
	if (mCpuLockCount == 0)
		mCpuLockTaskId = static_cast<long>(belle_sip_begin_background_task("Liblinphone cpu lock", sBgTaskTimeout, this));

	mCpuLockCount++;
}

void IosPlatformHelpers::releaseCpuLock () {
	mCpuLockCount--;
	if (mCpuLockCount != 0)
		return;

	if (mCpuLockTaskId == 0) {
		lError() << "IosPlatformHelpers::releaseCpuLock(): too late, the lock has been released already by the system.";
		return;
	}

	belle_sip_end_background_task(static_cast<unsigned long>(mCpuLockTaskId));
	mCpuLockTaskId = 0;
}

std::string IosPlatformHelpers::directoryForResource (CFStringRef framework, CFStringRef resource) {
	CFBundleRef bundle = CFBundleGetBundleWithIdentifier(framework);
	CFURLRef grammarUrl = CFBundleCopyResourceURL(bundle, resource, nullptr, nullptr);
	CFURLRef grammarUrlDirectory = CFURLCreateCopyDeletingLastPathComponent(nullptr, grammarUrl);
	CFStringRef grammarPath = CFURLCopyFileSystemPath(grammarUrlDirectory, kCFURLPOSIXPathStyle);
	CFStringEncoding encodingMethod = CFStringGetSystemEncoding();
	std::string path(CFStringGetCStringPtr(grammarPath, encodingMethod));
	CFRelease(grammarUrl);
	CFRelease(grammarPath);
	CFRelease(grammarUrlDirectory);
	return path;
}

// -----------------------------------------------------------------------------

PlatformHelpers *createIosPlatformHelpers (LinphoneCore *lc, void *system_context) {
	return new IosPlatformHelpers(lc, system_context);
}

LINPHONE_END_NAMESPACE

#endif
