/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
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

extension String {
	func capitalizingFirstLetter() -> String {
		return prefix(1).capitalized + dropFirst()
	}

	mutating func capitalizeFirstLetter() {
		self = self.capitalizingFirstLetter()
	}
}

class MeetingsListItemModel {
	let model: MeetingModel? // if NIL, consider that we are using the fake TodayModel
	var monthStr: String = ""
	var weekStr: String = ""
	var weekDayStr: String = ""
	var dayStr: String = ""
	
	var isToday = true
	
	init(meetingModel: MeetingModel?) {
		model = meetingModel
		if let mod = meetingModel {
			monthStr = createMonthString(date: mod.meetingDate)
			weekStr = createWeekString(date: mod.meetingDate)
			weekDayStr = createWeekDayString(date: mod.meetingDate)
			dayStr = createDayString(date: mod.meetingDate)
			isToday = false
		} else {
			monthStr = createMonthString(date: Date.now)
			weekStr = createWeekString(date: Date.now)
			weekDayStr = createWeekDayString(date: Date.now)
			dayStr = createDayString(date: Date.now)
		}
	}
	
	func createMonthString(date: Date) -> String {
		return "\(date.formatted(Date.FormatStyle().month(.wide))) \(date.formatted(Date.FormatStyle().year()))".capitalized
	}
	
	func createWeekString(date: Date) -> String {
		let calendar = Calendar.current
		let firstDayOfWeekIdx = calendar.firstWeekday
		let dateIndex = calendar.component(.weekday, from: date)
		let weekStartDate = calendar.date(byAdding: .day, value: -(dateIndex - firstDayOfWeekIdx % 7), to: date)!
		let	weekFirstDay = weekStartDate.formatted(Date.FormatStyle().day(.twoDigits))
		let firstMonth = weekStartDate.formatted(Date.FormatStyle().month(.wide)).capitalizingFirstLetter()
		
		let weekEndDate = calendar.date(byAdding: .day, value: 6, to: weekStartDate)!
		let weekEndDay = weekEndDate.formatted(Date.FormatStyle().day(.twoDigits))
		
		let isDifferentMonth =  calendar.component(.month, from: weekStartDate) != calendar.component(.month, from: weekEndDate)
		if isDifferentMonth {
			let lastMonth = weekEndDate.formatted(Date.FormatStyle().month(.wide)).capitalizingFirstLetter()
			return "\(weekFirstDay) \(firstMonth) - \(weekEndDay) \(lastMonth)"
		} else {
			return "\(weekFirstDay) - \(weekEndDay) \(firstMonth)"
		}
	}
	
	func createWeekDayString(date: Date) -> String {
		return date.formatted(Date.FormatStyle().weekday(.abbreviated)).capitalized
	}
	
	func createDayString(date: Date) -> String {
		return date.formatted(Date.FormatStyle().day(.twoDigits))
	}
}
