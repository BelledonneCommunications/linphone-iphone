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

import linphonesw
import SwiftUI

class MeetingModel: ObservableObject {

	var confInfo: ConferenceInfo
	var id: String
	var meetingDate: Date
	var endDate: Date
	var isToday: Bool
	var isAfterToday: Bool
	
	private let startTime: String
	private let endTime: String
	var time: String // "$startTime - $endTime"
	var day: String
	var dayNumber: String

	@Published var isBroadcast: Bool
	@Published var subject: String
	@Published var address: String
	
	init(conferenceInfo: ConferenceInfo) {
		confInfo = conferenceInfo
		id = confInfo.uri?.asStringUriOnly() ?? ""
		meetingDate = Date(timeIntervalSince1970: TimeInterval(confInfo.dateTime))
		
		let formatter = DateFormatter()
		formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
		startTime = formatter.string(from: meetingDate)
		endDate = Calendar.current.date(byAdding: .minute, value: Int(confInfo.duration), to: meetingDate)!
		endTime = formatter.string(from: endDate)
		time = "\(startTime) - \(endTime)"
		
		day = meetingDate.formatted(Date.FormatStyle().weekday(.abbreviated))
		dayNumber = meetingDate.formatted(Date.FormatStyle().day(.twoDigits))
		
		isToday = Calendar.current.isDateInToday(meetingDate)
		if isToday {
			isAfterToday = false
		} else {
			isAfterToday = meetingDate > Date.now
		}
		
		// If at least one participant is listener, we are in broadcast mode
		isBroadcast = confInfo.participantInfos.firstIndex(where: {$0.role == Participant.Role.Listener}) != nil
		
		subject = confInfo.subject ?? ""
		
		address = confInfo.uri?.asStringUriOnly() ?? ""
	}
}
