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
import CoreTelephony

enum NetworkType: Int {
	case network_none = 0
	case network_2g = 1
	case network_3g = 2
	case network_4g = 3
	case network_lte = 4
	case network_wifi = 5
}

/*
* AppManager is a class that includes some useful functions.
*/
@objc class AppManager: NSObject {
	static func network() -> NetworkType {
		let info = CTTelephonyNetworkInfo()
		let currentRadio = info.currentRadioAccessTechnology
		if (currentRadio == CTRadioAccessTechnologyEdge) {
			return NetworkType.network_2g
		} else if (currentRadio == CTRadioAccessTechnologyLTE) {
			return NetworkType.network_4g
		}
		return NetworkType.network_3g
	}

	@objc static func recordingFilePathFromCall(address: String) -> String {
		var filePath = "recording_"
		filePath = filePath.appending(address.isEmpty ? "unknow" : address)
		let now = Date()
		let dateFormat = DateFormatter()
		dateFormat.dateFormat = "E-d-MMM-yyyy-HH-mm-ss"
		let date = dateFormat.string(from: now)
		
		filePath = filePath.appending("_\(date).mkv")
		
		let paths = NSSearchPathForDirectoriesInDomains(.cachesDirectory, .userDomainMask, true)
		var writablePath = paths[0]
		writablePath = writablePath.appending("/\(filePath)")
		let message:String = "file path is \(writablePath)"
		Log.directLog(BCTBX_LOG_MESSAGE, text: message)
		return writablePath
		//file name is recording_contact-name_dayName-day-monthName-year-hour-minutes-seconds
		//The recording prefix is used to identify recordings in the cache directory.
		//We will use name_dayName-day-monthName-year to separate recordings by days, then hour-minutes-seconds to order them in each day.
	}
}
