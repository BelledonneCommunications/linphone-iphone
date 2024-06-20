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

class MeetingViewModel: ObservableObject {
	static let TAG = "[Meeting ViewModel]"
	/*
	private var coreContext = CoreContext.shared
	
	@Published var showBackbutton: Bool = false
	@Published var isBroadcast: Bool = false
	@Published var isEditable: Bool = false
	@Published var subject: String = ""
	@Published var sipUri: String = ""
	@Published var description: String?
	@Published var timezone: String = ""
	@Published var startDate: Date?
	@Published var endDate: Date?
	@Published var dateTime: String = ""
	
	@Published var speakers: [ParticipantModel] = []
	@Published var participants: [ParticipantModel] = []
	@Published var conferenceInfoFoundEvent: Bool = false
	
	var meetingModel: MeetingModel
	
	init(model: MeetingModel) {
		meetingModel = model
	}
	
	func findConferenceInfo(uri: String) {
		coreContext.doOnCoreQueue { core in
			var confInfoFound = false
			if let address = try? Factory.Instance.createAddress(addr: uri) {
				
				if let confInfo = core.findConferenceInformationFromUri(uri: address) {
					Log.info("\(MeetingViewModel.TAG) Conference info with SIP URI \(uri) was found")
					self.meetingModel.confInfo = confInfo
					self.configureConferenceInfo(core: core)
					confInfoFound = true
				} else {
					Log.error("\(MeetingViewModel.TAG) Conference info with SIP URI \(uri) couldn't be found!")
					confInfoFound = false
				}
			} else {
				Log.error("\(MeetingViewModel.TAG) Failed to parse SIP URI \(uri) as Address!")
				confInfoFound = false
			}
			DispatchQueue.main.sync {
				self.conferenceInfoFoundEvent = confInfoFound
			}
		}
	}
	
	private func configureConferenceInfo(core: Core) {
		/*
		 timezone.postValue(
		 AppUtils.getFormattedString(
		 R.string.meeting_schedule_timezone_title,
		 TimeZone.getDefault().displayName
		 )
		 .replaceFirstChar { if (it.isLowerCase()) it.titlecase(Locale.getDefault()) else it.toString() }
		 )
		 */
		
		var isEditable = false
		
		if let organizerAddress = meetingModel.confInfo.organizer {
			let localAccount = core.accountList.first(where: {
				if let address = $0.params?.identityAddress {
					return organizerAddress.weakEqual(address2: address)
				} else {
					return false
				}
			})
			
			isEditable = localAccount != nil
		} else {
			Log.error("\(MeetingViewModel.TAG) No organizer SIP URI found for: \(meetingModel.confInfo.uri?.asStringUriOnly() ?? "(empty)")")
		}
		
		let startDate = Date(timeIntervalSince1970: TimeInterval(meetingModel.confInfo.dateTime))
		let endDate = Calendar.current.date(byAdding: .minute, value: Int(meetingModel.confInfo.duration), to: startDate)!
		
		let formatter = DateFormatter()
		formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
		let startTime = formatter.string(from: startDate)
		let endTime = formatter.string(from: endDate)
		let dateTime = "\(startTime) - \(endTime)"
		
		DispatchQueue.main.async {
			self.subject = self.meetingModel.confInfo.subject ?? ""
			self.sipUri = self.meetingModel.confInfo.uri?.asStringUriOnly() ?? ""
			self.description = self.meetingModel.confInfo.description
			self.startDate = startDate
			self.endDate = endDate
			self.dateTime = dateTime
			self.isEditable = isEditable
		}
		
		self.computeParticipantsList()
	}
	
	private func computeParticipantsList() {
		var speakersList: [ParticipantModel] = []
		var participantsList: [ParticipantModel] = []
		var allSpeaker = true
		let organizer = meetingModel.confInfo.organizer
		var organizerFound = false
		for pInfo in meetingModel.confInfo.participantInfos {
			if let participantAddress = pInfo.address {
				let isOrganizer = organizer != nil && organizer!.weakEqual(address2: participantAddress)
				
				Log.info("\(MeetingViewModel.TAG) Conference \(meetingModel.confInfo.subject)[${conferenceInfo.subject}] \(isOrganizer ? "organizer: " : "participant: ") \(participantAddress.asStringUriOnly()) is a \(pInfo.role)")
				if isOrganizer {
					organizerFound = true
				}
				
				if pInfo.role == Participant.Role.Listener {
					allSpeaker = false
					participantsList.append(ParticipantModel(address: participantAddress))
				} else {
					speakersList.append(ParticipantModel(address: participantAddress))
				}
			}
		}
		
		if allSpeaker {
			Log.info("$TAG All participants have Speaker role, considering it is a meeting")
			participantsList = speakersList
		}
		
		if !organizerFound, let organizerAddress = organizer {
			Log.info("$TAG Organizer not found in participants list, adding it to participants list")
			participantsList.append(ParticipantModel(address: organizerAddress))
		}
		
		DispatchQueue.main.async {
			self.isBroadcast = !allSpeaker
			self.speakers = speakersList
			self.participants = participantsList
		}
	}
	*/
}
