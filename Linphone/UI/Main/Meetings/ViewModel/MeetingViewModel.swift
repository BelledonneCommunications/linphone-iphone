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

// TODO: à merger avec le ParticipantModel de la branche de Benoit
class ParticipantModel: ObservableObject {
	
	static let TAG = "[Participant Model]"
	
	let address: Address
	@Published var sipUri: String
	@Published var name: String
	@Published var avatarModel: ContactAvatarModel
	
	init(address: Address) {
		self.address = address
		
		self.sipUri = address.asStringUriOnly()

		if let addressFriend = ContactsManager.shared.getFriendWithAddress(address: self.address) {
			self.name = addressFriend.name!
		} else {
			self.name = address.displayName != nil ? address.displayName! : address.username!
		}
		
		self.avatarModel = ContactAvatarModel.getAvatarModelFromAddress(address: self.address)
	}
}

class MeetingViewModel: ObservableObject {
	static let TAG = "[Meeting ViewModel]"
	
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
	
	var conferenceInfo: ConferenceInfo?
	
	init() {
		
	}
	
	func findConferenceInfo(uri: String) {
		coreContext.doOnCoreQueue { core in
			var confInfoFound = false
			if let address = try? Factory.Instance.createAddress(addr: uri) {
				let foundConfInfo = core.findConferenceInformationFromUri(uri: address)
				if foundConfInfo != nil {
					Log.info("\(MeetingViewModel.TAG) Conference info with SIP URI \(uri) was found")
					self.conferenceInfo = foundConfInfo
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
		if let confInfo = self.conferenceInfo {
			
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
			
			if let organizerAddress = confInfo.organizer {
				let localAccount = core.accountList.first(where: {
					if let address = $0.params?.identityAddress {
						return organizerAddress.weakEqual(address2: address)
					} else {
						return false
					}
				})
				
				isEditable = localAccount != nil
			} else {
				Log.error("\(MeetingViewModel.TAG) No organizer SIP URI found for: \(confInfo.uri?.asStringUriOnly() ?? "(empty)")")
			}
			
			let startDate = Date(timeIntervalSince1970: TimeInterval(confInfo.dateTime))
			let endDate = Calendar.current.date(byAdding: .minute, value: Int(confInfo.duration), to: startDate)!
			
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
			let startTime = formatter.string(from: startDate)
			let endTime = formatter.string(from: endDate)
			let dateTime = "\(startTime) - \(endTime)"
			
			DispatchQueue.main.sync {
				self.subject = confInfo.subject ?? ""
				self.sipUri = confInfo.uri?.asStringUriOnly() ?? ""
				self.description = confInfo.description
				self.startDate = startDate
				self.endDate = endDate
				self.dateTime = dateTime
				self.isEditable = isEditable
			}
			
			self.computeParticipantsList(core: core, confInfo: confInfo)
		}
	}
	
	private func computeParticipantsList(core: Core, confInfo: ConferenceInfo) {
		var speakersList: [ParticipantModel] = []
		var participantsList: [ParticipantModel] = []
		var allSpeaker = true
		let organizer = confInfo.organizer
		var organizerFound = false
		for pInfo in confInfo.participantInfos {
			if let participantAddress = pInfo.address {
				let isOrganizer = organizer != nil && organizer!.weakEqual(address2: participantAddress)
				
				Log.info("\(MeetingViewModel.TAG) Conference \(confInfo.subject)[${conferenceInfo.subject}] \(isOrganizer ? "organizer: " : "participant: ") \(participantAddress.asStringUriOnly()) is a \(pInfo.role)")
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
		
		DispatchQueue.main.sync {
			self.isBroadcast = !allSpeaker
			speakers = speakersList
			participants = participantsList
		}
	}
}
