/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
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

class TimestampUtils {

	static func is24Hour() -> Bool {
		let dateFormat = DateFormatter.dateFormat(fromTemplate: "j", options: 0, locale: Locale.current)!
		return dateFormat.firstIndex(of: "a") == nil
	}
	
	static func timeToString(unixTimestamp: Double, timestampInSecs: Bool = true) -> String {
		let date = Date(timeIntervalSince1970: unixTimestamp)
		let dateFormat = DateFormatter()
		dateFormat.dateFormat = is24Hour()  ? "HH'h'mm" : "h:mm a"
		return dateFormat.string(from: date)
	}
	
	static func toString(
		unixTimestamp: Double,
			   onlyDate: Bool = false,
			   timestampInSecs: Bool = true,
			   shortDate: Bool = true
		   ) -> String {
			   let date = Date(timeIntervalSince1970: unixTimestamp)
			   let dateFormatter = DateFormatter()
			   dateFormatter.dateStyle = onlyDate ? .none : .long
			   dateFormatter.timeStyle = shortDate ? .short : .long
			   dateFormatter.doesRelativeDateFormatting = true
			   return dateFormatter.string(from: date)
		   }
	
	static func dateToString(date:Date) -> String {
		let dateFormatter = DateFormatter()
		dateFormatter.dateStyle = .short
		dateFormatter.timeStyle = .none
		return dateFormatter.string(from: date)
	}
	
	static func dateLongToString(date:Date) -> String {
		let dateFormatter = DateFormatter()
		dateFormatter.dateStyle = .long
		dateFormatter.timeStyle = .none
		let dayFormatter = DateFormatter()
		dayFormatter.dateFormat = "EEEE"
		let day = dayFormatter.string(from: date)
		return day.prefix(1).uppercased() + day.dropFirst()+" "+dateFormatter.string(from: date)
	}
	
	static func timeToString(date:Date) -> String {
		let dateFormatter = DateFormatter()
		dateFormatter.dateStyle = .none
		dateFormatter.timeStyle = .short
		return dateFormatter.string(from: date)
	}
	
	
}

