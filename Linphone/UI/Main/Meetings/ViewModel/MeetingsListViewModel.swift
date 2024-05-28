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

class MeetingsListViewModel: ObservableObject {
	static let TAG = "[Meetings ListViewModel]"
	
	private var coreContext = CoreContext.shared
	private var mCoreSuscriptions = Set<AnyCancellable?>()
	var selectedMeeting: ConversationModel?
	
	@Published var meetingsList: [MeetingsListItemModel] = []
	@Published var currentFilter = ""
	
	init() {
		coreContext.doOnCoreQueue { core in
			self.mCoreSuscriptions.insert(core.publisher?.onConferenceInfoReceived?.postOnCoreQueue { (cbVal: (core: Core, conferenceInfo: ConferenceInfo)) in
				Log.info("\(MeetingsListViewModel.TAG) Conference info received [\(cbVal.conferenceInfo.uri?.asStringUriOnly())")
				self.computeMeetingsList()
			})
		}
		computeMeetingsList()
	}
	
	func computeMeetingsList() {
		let filter = self.currentFilter
		
		coreContext.doOnCoreQueue { core in
			var confInfoList: [ConferenceInfo] = []
			
			if let account = core.defaultAccount {
				confInfoList = account.conferenceInformationList
			}
			if confInfoList.isEmpty {
				confInfoList = core.conferenceInformationList
			}
			
			var meetingsListTmp: [MeetingsListItemModel] = []
			var previousModel: MeetingModel?
			var meetingForTodayFound = false
			Log.info("debugtrace -- computeMeetingsList, \(confInfoList.count) conferences found")
			for confInfo in confInfoList {
				if confInfo.duration == 0 { continue }// This isn't a scheduled conference, don't display it
				var add = true
				if !filter.isEmpty {
					let organizerCheck = confInfo.organizer?.asStringUriOnly().range(of: filter, options: .caseInsensitive) != nil
					let subjectCheck = confInfo.subject?.range(of: filter, options: .caseInsensitive) != nil
					let descriptionCheck = confInfo.description?.range(of: filter, options: .caseInsensitive) != nil
					let participantsCheck = confInfo.participantInfos.first(where: {$0.address?.asStringUriOnly().range(of: filter, options: .caseInsensitive) != nil}) != nil
					
					add = organizerCheck || subjectCheck || descriptionCheck || participantsCheck
				}
				
				if add {
					let model = MeetingModel(conferenceInfo: confInfo)
					
					if !meetingForTodayFound {
						if model.isToday {
							meetingForTodayFound = true
						} else if model.isAfterToday {
							// If no meeting was found for today, insert "Today" fake model before the next meeting to come
							meetingsListTmp.append(MeetingsListItemModel(meetingModel: nil))
							meetingForTodayFound = true
						}
					}
					
					meetingsListTmp.append(MeetingsListItemModel(meetingModel: model))
					previousModel = model
				}
			}
			
			Log.info("debugtrace -- computeMeetingsList, previous count = \(self.meetingsList.count), new count = \(meetingsListTmp.count)")
			DispatchQueue.main.sync {
				self.meetingsList = meetingsListTmp
			}
		}
	}
}
