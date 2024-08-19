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
import EventKit

// swiftlint:disable line_length
class MeetingViewModel: ObservableObject {
	static let TAG = "[MeetingViewModel]"
	let eventStore : EKEventStore = EKEventStore()
	
	@Published var isBroadcastSelected: Bool = false
	@Published var showBroadcastHelp: Bool = false
	@Published var subject: String = ""
	@Published var description: String = ""
	@Published var allDayMeeting: Bool = false
	@Published var fromDateStr: String = ""
	@Published var fromTime: String = ""
	@Published var toDateStr: String = ""
	@Published var toTime: String = ""
	@Published var sendInvitations: Bool = true
	@Published var participants: [SelectedAddressModel] = []
	@Published var operationInProgress: Bool = false
	@Published var conferenceCreatedEvent: Bool = false
	@Published var conferenceUri: String = ""
	
	@Published var selectedTimezoneIdx = 0
	var selectedTimezone = TimeZone.current
	var knownTimezones: [String] = []
	
	var conferenceScheduler: ConferenceScheduler?
	private var mSchedulerSubscriptions = Set<AnyCancellable?>()
	var conferenceInfoToEdit: ConferenceInfo?
	@Published var displayedMeeting: MeetingModel? // if nil, then we are currently creating a new meeting
	@Published var myself: SelectedAddressModel?
	@Published var fromDate: Date
	@Published var toDate: Date
	@Published var errorMsg: String = ""
	
	init() {
		fromDate = Calendar.current.date(byAdding: .hour, value: 1, to: Date.now)!
		toDate = Calendar.current.date(byAdding: .hour, value: 2, to: Date.now)!
		
		var tzIds = TimeZone.knownTimeZoneIdentifiers
		tzIds.sort(by: {
			let gmtOffset0 = TimeZone(identifier: $0)!.secondsFromGMT()
			let gmtOffset1 = TimeZone(identifier: $1)!.secondsFromGMT()
			if gmtOffset0 == gmtOffset1 {
				return $0 < $1 // sort by name if same GMT offset
			} else {
				return gmtOffset0 < gmtOffset1
			}
		})
		knownTimezones = tzIds
		selectedTimezoneIdx = knownTimezones.firstIndex(where: {$0 == selectedTimezone.identifier}) ?? 0
		computeDateLabels()
		computeTimeLabels()
	}
	
	func resetViewModelData() {
		isBroadcastSelected = false
		showBroadcastHelp = false
		subject = ""
		description = ""
		allDayMeeting = false
		sendInvitations = true
		participants = []
		operationInProgress = false
		conferenceCreatedEvent = false
		conferenceUri = ""
		fromDate = Calendar.current.date(byAdding: .hour, value: 1, to: Date.now)!
		toDate = Calendar.current.date(byAdding: .hour, value: 2, to: Date.now)!
		selectedTimezone = TimeZone.current
		selectedTimezoneIdx = knownTimezones.firstIndex(where: {$0 == selectedTimezone.identifier}) ?? 0
		computeDateLabels()
		computeTimeLabels()
	}
	
	func updateTimezone(timeZone: TimeZone) {
		selectedTimezone = timeZone
		computeDateLabels()
		computeTimeLabels()
	}
	
	func computeDateLabels() {
		let formatter = DateFormatter()
		formatter.timeZone = selectedTimezone
		formatter.dateFormat = "EEEE d MMM"
		
		fromDateStr = formatter.string(from: fromDate)
		Log.info("\(MeetingViewModel.TAG) computed start date is \(fromDateStr)")
		toDateStr = formatter.string(from: toDate)
		Log.info("\(MeetingViewModel.TAG)) computed end date is \(toDateStr)")
	}
	
	func computeTimeLabels() {
		let formatter = DateFormatter()
		formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
		formatter.timeZone = selectedTimezone
		fromTime = formatter.string(from: fromDate)
		toTime = formatter.string(from: toDate)
	}
	
	func getFullDateString() -> String {
		let formatter = DateFormatter()
		formatter.dateFormat = "EEE d MMM yyyy"
		return "\(formatter.string(from: fromDate)) | \(allDayMeeting ? "All day" : "\(fromTime) - \(toTime)")"
	}
	
	func addParticipants(participantsToAdd: [SelectedAddressModel]) {
		var list = participants
		for selectedAddr in participantsToAdd {
			if let found = list.first(where: { $0.address.weakEqual(address2: selectedAddr.address) }) {
				Log.info("\(MeetingViewModel.TAG) Participant \(found.address.asStringUriOnly()) already in list, skipping")
				continue
			}
			
			list.append(selectedAddr)
			Log.info("\(MeetingViewModel.TAG) Added participant \(selectedAddr.address.asStringUriOnly())")
		}
		Log.info("\(MeetingViewModel.TAG) [\(list.count - participants.count) participants added, now there are \(list.count) participants in list")

		participants = list
	}
	
	private func fillConferenceInfo(confInfo: ConferenceInfo) {
		confInfo.subject = self.subject
		confInfo.description = self.description
		confInfo.dateTime = time_t(self.fromDate.timeIntervalSince1970)
		confInfo.duration = UInt(self.fromDate.distance(to: self.toDate) / 60)
		
		let participantsList = self.participants
		var participantsInfoList: [ParticipantInfo] = []
		for participant in participantsList {
			if let info = try? Factory.Instance.createParticipantInfo(address: participant.address) {
				// For meetings, all participants must have Speaker role
				info.role = Participant.Role.Speaker
				participantsInfoList.append(info)
			} else {
				Log.error("\(MeetingViewModel.TAG) Failed to create Participant Info from address \(participant.address.asStringUriOnly())")
			}
		}
		confInfo.participantInfos = participantsInfoList
	}
	
	private func resetConferenceSchedulerAndListeners(core: Core) {
		self.mSchedulerSubscriptions.removeAll()
		self.conferenceScheduler = try? core.createConferenceScheduler()
		
		self.mSchedulerSubscriptions.insert(self.conferenceScheduler?.publisher?.onStateChanged?.postOnCoreQueue { (cbVal: (conferenceScheduler: ConferenceScheduler, state: ConferenceScheduler.State)) in
			
			Log.info("\(MeetingViewModel.TAG) Conference state changed \(cbVal.state)")
			if cbVal.state == ConferenceScheduler.State.Error {
				DispatchQueue.main.async {
					self.operationInProgress = false
					
					self.errorMsg = (self.displayedMeeting != nil) ? "Could not edit conference" : "Could not create conference"
					// TODO: show error toast
				}
			} else if cbVal.state == ConferenceScheduler.State.Ready {
				let conferenceAddress = self.conferenceScheduler?.info?.uri
				if let confInfoToEdit = self.conferenceInfoToEdit {
					Log.info("\(MeetingViewModel.TAG) Conference info \(confInfoToEdit.uri?.asStringUriOnly() ?? "'nil'") has been updated")
				} else {
					Log.info("\(MeetingViewModel.TAG) Conference info created, address will be \(conferenceAddress?.asStringUriOnly() ?? "'nil'")")
				}
				
				if self.sendInvitations {
					Log.info("\(MeetingViewModel.TAG) User asked for invitations to be sent, let's do it")
					if let chatRoomParams = try? core.createDefaultChatRoomParams() {
						chatRoomParams.groupEnabled = false
						chatRoomParams.backend = ChatRoom.Backend.FlexisipChat
						chatRoomParams.encryptionEnabled = true
						chatRoomParams.subject = "Meeting invitation" // Won't be used
						self.conferenceScheduler?.sendInvitations(chatRoomParams: chatRoomParams)
					} else {
						Log.error("\(MeetingViewModel.TAG) Failed to create default chatroom parameters. This should not happen")
					}
				} else {
					Log.info("\(MeetingViewModel.TAG) User didn't asked for invitations to be sent")
					DispatchQueue.main.async {
						self.operationInProgress = false
						self.conferenceCreatedEvent = true
					}
				}
			}
		})
		
		self.mSchedulerSubscriptions.insert(self.conferenceScheduler?.publisher?.onInvitationsSent?.postOnCoreQueue { (cbVal: (conferenceScheduler: ConferenceScheduler, failedInvitations: [Address])) in
			
			if cbVal.failedInvitations.isEmpty {
				Log.info("\(MeetingViewModel.TAG) All invitations have been sent")
			} else if cbVal.failedInvitations.count == self.participants.count {
				Log.error("\(MeetingViewModel.TAG) No invitation sent!")
				// TODO: show error toast
			} else {
				Log.warn("\(MeetingViewModel.TAG) \(cbVal.failedInvitations.count) invitations couldn't have been sent for:")
				for failInv in cbVal.failedInvitations {
					Log.warn(failInv.asStringUriOnly())
				}
				// TODO: show error toast
			}
			
			DispatchQueue.main.async {
				self.operationInProgress = false
				self.conferenceCreatedEvent = true
			}
		})
	}
	
	func schedule() {
		if subject.isEmpty || participants.isEmpty {
			Log.error("\(MeetingViewModel.TAG) Either no subject was set or no participant was selected, can't schedule meeting.")
			// TODO: show red toast
			return
		}
		operationInProgress = true
		
		CoreContext.shared.doOnCoreQueue { core in
			Log.info("\(MeetingViewModel.TAG) Scheduling \(self.isBroadcastSelected ? "broadcast" : "meeting")")
			
			if let conferenceInfo = (self.displayedMeeting != nil ? self.displayedMeeting!.confInfo : try? Factory.Instance.createConferenceInfo()) {
				let localAccount = core.defaultAccount
				conferenceInfo.organizer = localAccount?.params?.identityAddress
				self.fillConferenceInfo(confInfo: conferenceInfo)
				self.resetConferenceSchedulerAndListeners(core: core)
				self.conferenceScheduler?.account = localAccount
				// Will trigger the conference creation automatically
				self.conferenceScheduler?.info = conferenceInfo
			}
		}
	}
	
	func update() {
		self.operationInProgress = true
		CoreContext.shared.doOnCoreQueue { core in
			Log.info("\(MeetingViewModel.TAG) Updating \(self.isBroadcastSelected ? "broadcast" : "meeting")")
			
			if let conferenceInfo = self.conferenceInfoToEdit {
				self.fillConferenceInfo(confInfo: conferenceInfo)
				self.resetConferenceSchedulerAndListeners(core: core)
				
				// Will trigger the conference update automatically
				self.conferenceScheduler?.info = conferenceInfo
			} else {
				Log.error("No conference info to edit found!")
				return
			}
		}
	}
	
	// Warning: must be called from core queue. Removed the dispatchQueue.main.async in order to have the animation properly trigger.
	func loadExistingMeeting(meeting: MeetingModel) {
			self.resetViewModelData()
			self.subject = meeting.confInfo.subject ?? ""
			self.description = meeting.confInfo.description ?? ""
			self.fromDate = meeting.meetingDate
			self.toDate = meeting.endDate
			self.participants = []
						
			CoreContext.shared.doOnCoreQueue { core in
				let organizer = meeting.confInfo.organizer
				var organizerFound = false
				
				if let myAddr = core.defaultAccount?.contactAddress {
					let isOrganizer = (organizer != nil) ? myAddr.weakEqual(address2: organizer!) : false
					organizerFound = organizerFound || isOrganizer
					ContactAvatarModel.getAvatarModelFromAddress(address: myAddr) { avatarResult in
						DispatchQueue.main.async {
							self.myself = SelectedAddressModel(addr: myAddr, avModel: avatarResult, isOrg: isOrganizer)
						}
					}
				}
				
				for pInfo in meeting.confInfo.participantInfos {
					if let addr = pInfo.address {
						let isOrganizer = (organizer != nil) ? addr.weakEqual(address2: organizer!) : false
						organizerFound = organizerFound || isOrganizer
						ContactAvatarModel.getAvatarModelFromAddress(address: addr) { avatarResult in
							DispatchQueue.main.async {
								self.participants.append(SelectedAddressModel(addr: addr, avModel: avatarResult, isOrg: isOrganizer))
							}
						}
					}
				}
				
				// if didn't find organizer, add him
				if !organizerFound, let org = organizer {
					ContactAvatarModel.getAvatarModelFromAddress(address: org) { avatarResult in
						DispatchQueue.main.async {
							self.participants.append(SelectedAddressModel(addr: org, avModel: avatarResult, isOrg: true))
						}
					}
				}
			}
			
			self.conferenceUri = meeting.confInfo.uri?.asStringUriOnly() ?? ""
			self.computeDateLabels()
			self.computeTimeLabels()
			self.displayedMeeting = meeting
	}
	
	func sendMeetingCancelledNotifications(meeting: MeetingModel) {
		CoreContext.shared.doOnCoreQueue { core in
			self.conferenceScheduler = try? core.createConferenceScheduler()
			self.conferenceScheduler?.cancelConference(conferenceInfo: meeting.confInfo)
		}
	}
	
	func addMeetingToCalendar() {
		if #available(iOS 17.0, *) {
			eventStore.requestAccess(to: .event) { granted, error in
				guard let meeting = self.displayedMeeting else {
					Log.error("\(MeetingViewModel.TAG) Failed to add meeting to calendar: no meeting selected")
				}
				if !granted {
					Log.error("\(MeetingViewModel.TAG) Failed to add meeting to calendar: access not granted")
				} else if error == nil {
					
					let event:EKEvent = EKEvent(eventStore: self.eventStore)
					event.title = self.subject
					event.startDate = self.fromDate
					event.endDate = self.toDate
					event.notes = self.description
					event.calendar = self.eventStore.defaultCalendarForNewEvents
					do {
						try self.eventStore.save(event, span: .thisEvent)
					} catch let error as NSError {
						Log.error("\(MeetingViewModel.TAG) Failed to add meeting to calendar: \(error)")
					}
					Log.info("\(MeetingViewModel.TAG) Meeting '\(meeting.subject)': \(error ?? "")")
				} else {
					Log.error("\(MeetingViewModel.TAG) Failed to add meeting to calendar: \(error ?? "")")
				}
			}
		}
	}
	// eventStore.requestAccess(to: EKEntityType.event) { granted, error in
}

// swiftlint:enable line_length
