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
import linphonesw
import Combine

class ScheduleMeetingViewModel: ObservableObject {
	private let TAG = "[ScheduleMeetingViewModel]"
	
	@Published var isBroadcastSelected: Bool = false
	@Published var showBroadcastHelp: Bool = false
	@Published var subject: String = ""
	@Published var description: String = ""
	@Published var allDayMeeting: Bool = false
	@Published var fromDateStr: String = ""
	@Published var fromTime: String = ""
	@Published var toDateStr: String = ""
	@Published var  toTime: String = ""
	@Published var  timezone: String = ""
	@Published var  sendInvitations: Bool = true
	// var participants = MutableLiveData<ArrayList<SelectedAddressModel>>()
	@Published var  operationInProgress: Bool = false
	@Published var  conferenceCreatedEvent: Bool = false
	
	var conferenceScheduler: ConferenceScheduler?
	var conferenceInfoToEdit: ConferenceScheduler?
	
	private var fromDate: Date
	private var toDate: Date
	
	init() {
		fromDate = Calendar.current.date(byAdding: .hour, value: 1, to: Date.now)!
		toDate = Calendar.current.date(byAdding: .hour, value: 1, to: fromDate)!
		
		computeDateLabels()
		computeTimeLabels()
		updateTimezone()
	}
	
	private func computeDateLabels() {
		var day = fromDate.formatted(Date.FormatStyle().weekday(.wide))
		var dayNumber = fromDate.formatted(Date.FormatStyle().day(.twoDigits))
		var month = fromDate.formatted(Date.FormatStyle().month(.wide))
		fromDateStr = "\(day) \(dayNumber), \(month)"
		Log.info("\(TAG) computed start date is \(fromDateStr)")
		
		day = toDate.formatted(Date.FormatStyle().weekday(.wide))
		dayNumber = toDate.formatted(Date.FormatStyle().day(.twoDigits))
		month = toDate.formatted(Date.FormatStyle().month(.wide))
		toDateStr = "\(day) \(dayNumber), \(month)"
		Log.info("\(TAG) computed end date is \(toDateStr)")
	}
	
	private func computeTimeLabels() {
		let formatter = DateFormatter()
		formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
		fromTime = formatter.string(from: fromDate)
		toTime = formatter.string(from: toDate)
	}
	
	private func updateTimezone() {
		// TODO
	}
}
