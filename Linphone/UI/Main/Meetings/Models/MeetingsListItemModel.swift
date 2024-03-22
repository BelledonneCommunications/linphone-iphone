//
//  MeetingsListItemModel.swift
//  Linphone
//
//  Created by QuentinArguillere on 19/03/2024.
//
import Foundation

class MeetingsListItemModel {
	let model: MeetingModel? // if NIL, consider that we are using the fake TodayModel
	var month: String = Date.now.formatted(Date.FormatStyle().month(.wide))
	var isToday = true
	
	init(meetingModel: MeetingModel?) {
		model = meetingModel
		if let mod = meetingModel {
			month = mod.month
			isToday = false
		}
	}
}
