/*
 * Copyright (c) 2010-2021 Belledonne Communications SARL.
 *
 * This file is part of linphone-android
 * (see https://www.linphone.org).
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


struct TimeZoneData : Comparable {
	let timeZone: TimeZone

	static func == (lhs: TimeZoneData, rhs: TimeZoneData) -> Bool {
		return lhs.timeZone.identifier == rhs.timeZone.identifier
	}
	
	static func < (lhs: TimeZoneData, rhs: TimeZoneData) -> Bool {
		return lhs.timeZone.secondsFromGMT() < rhs.timeZone.secondsFromGMT()

	}
	
	func descWithOffset() -> String {
		return "\(timeZone.identifier) - GMT\(timeZone.offsetInHours())"
	}
}

extension TimeZone {

	func offsetFromUTC() -> String
	{
		let localTimeZoneFormatter = DateFormatter()
		localTimeZoneFormatter.timeZone = self
		localTimeZoneFormatter.dateFormat = "Z"
		return localTimeZoneFormatter.string(from: Date())
	}

	func offsetInHours() -> String
	{
	
		let hours = secondsFromGMT()/3600
		let minutes = abs(secondsFromGMT()/60) % 60
		let tz_hr = String(format: "%+.2d:%.2d", hours, minutes) // "+hh:mm"
		return tz_hr
	}
}
