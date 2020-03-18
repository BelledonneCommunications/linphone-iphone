/*
* Copyright (c) 2010-2019 Belledonne Communications SARL.
*
* This file is part of linphone-iphone
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

import Foundation
import linphonesw

@objc class CoreManager: NSObject {
	static var theCoreManager: CoreManager?
	var lc: Core?
	private var mIterateTimer: Timer?

	@objc static func instance() -> CoreManager {
		if (theCoreManager == nil) {
			theCoreManager = CoreManager()
		}
		return theCoreManager!
	}

	@objc func setCore(core: OpaquePointer) {
		lc = Core.getSwiftObject(cObject: core)
	}

	@objc private func iterate() {
		var coreIterateTaskId = UIBackgroundTaskIdentifier(rawValue: 0)
		coreIterateTaskId = UIApplication.shared.beginBackgroundTask(expirationHandler: {
			Log.directLog(BCTBX_LOG_WARNING, text: "Background task for core iteration launching expired.")
			UIApplication.shared.endBackgroundTask(coreIterateTaskId)
		})
		lc?.iterate()
		if (coreIterateTaskId != UIBackgroundTaskIdentifier.invalid) {
			UIApplication.shared.endBackgroundTask(coreIterateTaskId)
		}
	}

	@objc func startIterateTimer() {
		mIterateTimer = Timer.scheduledTimer(timeInterval: 0.02, target: self, selector: #selector(self.iterate), userInfo: nil, repeats: true)
		Log.directLog(BCTBX_LOG_DEBUG, text: "start iterate timer")

	}

	@objc func stopIterateTimer() {
		if let timer = mIterateTimer {
			timer.invalidate()
			Log.directLog(BCTBX_LOG_DEBUG, text: "stop iterate timer")
		}
	}
}
