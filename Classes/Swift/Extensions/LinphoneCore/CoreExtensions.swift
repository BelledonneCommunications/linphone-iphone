/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
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

extension Core {
	static func get() -> Core {
		return CallManager.instance().lc!
	}
	
	func showSwitchCameraButton() -> Bool {
		   return videoDevicesList.count > 2 // Count StaticImage camera
	}
	
	func toggleCamera() {
		Log.i("[Core] Current camera device is \(videoDevice)")
		var switched = false
		videoDevicesList.forEach  {
			if (!switched && $0 != videoDevice && $0 != "StaticImage: Static picture") {
				Log.i("[Core] New camera device will be \($0)")
				try?setVideodevice(newValue: $0)
				switched = true
			}
		}
	}
}
