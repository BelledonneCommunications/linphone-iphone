//
//  MeetingModel.swift
//  Linphone
//
//  Created by QuentinArguillere on 19/03/2024.
//

import linphonesw

class MeetingModel: ObservableObject {

	private var confInfo: ConferenceInfo
	var id: String
	var meetingDate: Date
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
	@Published var firstMeetingOfTheDay: Bool = false
	
	init(conferenceInfo: ConferenceInfo) {
		confInfo = conferenceInfo
		id = confInfo.uri?.asStringUriOnly() ?? ""
		meetingDate = Date(timeIntervalSince1970: TimeInterval(confInfo.dateTime))
		
		let formatter = DateFormatter()
		formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
		startTime = formatter.string(from: meetingDate)
		let endDate = Calendar.current.date(byAdding: .minute, value: Int(confInfo.duration), to: meetingDate)!
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
