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

// swiftlint:disable type_body_length
class MeetingViewModel: ObservableObject {
	static let TAG = "[MeetingViewModel]"
	let eventStore: EKEventStore = EKEventStore()
	
	@Published var isBroadcastSelected: Bool = false
	@Published var showBroadcastHelp: Bool = false
	@Published var subject: String = ""
	@Published var description: String = ""
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
	private var mSchedulerDelegate: ConferenceSchedulerDelegate?
	@Published var myself: SelectedAddressModel?
	@Published var fromDate: Date
	@Published var toDate: Date
	@Published var errorMsg: String = ""
	
	init(isShowScheduleMeetingFragmentSubject: String? = nil, isShowScheduleMeetingFragmentParticipants: [SelectedAddressModel]? = nil) {
		self.subject = isShowScheduleMeetingFragmentSubject ?? ""
		self.participants = isShowScheduleMeetingFragmentParticipants ?? []
		
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
		
		if let displayedMeeting = SharedMainViewModel.shared.displayedMeeting {
			self.loadExistingMeeting(meeting: displayedMeeting)
		}
	}
	
	func resetViewModelData() {
		isBroadcastSelected = false
		showBroadcastHelp = false
		subject = ""
		description = ""
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
		return "\(formatter.string(from: fromDate)) | \(fromTime) - \(toTime)"
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
	
	private func sendIcsInvitation(core: Core) {
		if let chatRoomParams = try? core.createDefaultChatRoomParams() {
			chatRoomParams.groupEnabled = false
			chatRoomParams.backend = ChatRoom.Backend.FlexisipChat
			chatRoomParams.encryptionEnabled = true
			chatRoomParams.subject = "Meeting ics"
			
			if self.conferenceScheduler == nil {
				Log.info("\(MeetingViewModel.TAG) ConferenceScheduler is nil, resetting...")
				self.resetConferenceSchedulerAndListeners(core: core)
			}
			
			guard let scheduler = self.conferenceScheduler else {
				Log.error("\(MeetingViewModel.TAG) ConferenceScheduler still nil after reset")
				return
			}
			
			scheduler.sendInvitations(chatRoomParams: chatRoomParams)
		} else {
			Log.error("\(MeetingViewModel.TAG) Failed to create default chatroom parameters. This should not happen")
		}
	}
	
	private func resetConferenceSchedulerAndListeners(core: Core) {
		self.mSchedulerDelegate = nil
		self.conferenceScheduler = LinphoneUtils.createConferenceScheduler(core: core)
		
		guard let scheduler = self.conferenceScheduler else {
			Log.info("\(MeetingViewModel.TAG) ConferenceScheduler is nil after reset, nothing to cancel")
			return
		}
		self.mSchedulerDelegate = ConferenceSchedulerDelegateStub(onStateChanged: { (_: ConferenceScheduler, state: ConferenceScheduler.State) in
			Log.info("\(MeetingViewModel.TAG) Conference state changed \(state)")
			if state == ConferenceScheduler.State.Error {
				DispatchQueue.main.async {
					self.operationInProgress = false
					self.errorMsg = (SharedMainViewModel.shared.displayedMeeting != nil) ? "Could not edit conference" : "Could not create conference"
					ToastViewModel.shared.toastMessage = (SharedMainViewModel.shared.displayedMeeting != nil) ? "meeting_failed_to_edit_toast" : "meeting_failed_to_schedule_toast"
					ToastViewModel.shared.displayToast = true
				}
			} else if state == ConferenceScheduler.State.Ready {
				if let confInfo = scheduler.info, let conferenceAddress = confInfo.uri {
					Log.info("\(MeetingViewModel.TAG) Conference info created, address will be \(conferenceAddress.asStringUriOnly())")
				}
				
				DispatchQueue.main.async {
					ToastViewModel.shared.toastMessage = "Success_meeting_info_created_toast"
					ToastViewModel.shared.displayToast = true
				}
				
				if SharedMainViewModel.shared.displayedMeeting != nil {
					DispatchQueue.main.async {
						NotificationCenter.default.post(
							name: NSNotification.Name("DisplayedMeetingUpdated"),
							object: nil
						)
					}
				}
                
				if self.sendInvitations && !CorePreferences.disableChatFeature {
					Log.info("\(MeetingViewModel.TAG) User asked for invitations to be sent, let's do it")
					self.sendIcsInvitation(core: core)
				} else {
					Log.info("\(MeetingViewModel.TAG) User didn't asked for invitations to be sent")
					DispatchQueue.main.async {
						self.operationInProgress = false
						self.conferenceCreatedEvent = true
					}
				}
			} else if state == ConferenceScheduler.State.Updating {
				self.sendIcsInvitation(core: core)
			}
		}, onInvitationsSent: { (_: ConferenceScheduler, failedInvitations: [Address]) in
			
			if failedInvitations.isEmpty {
				Log.info("\(MeetingViewModel.TAG) All invitations have been sent")
			} else if failedInvitations.count == self.participants.count {
				Log.error("\(MeetingViewModel.TAG) No invitation sent!")
				DispatchQueue.main.async {
					ToastViewModel.shared.toastMessage = "meeting_failed_to_send_invites_toast"
					ToastViewModel.shared.displayToast = true
				}
			} else {
				var failInvList = ""
				for failInv in failedInvitations {
					if !failInvList.isEmpty {
						failInvList += ", "
					}
					failInvList.append(failInv.asStringUriOnly())
				}
				Log.warn("\(MeetingViewModel.TAG) \(failedInvitations.count) invitations couldn't have been sent to: \(failInvList)")
				DispatchQueue.main.async {
					ToastViewModel.shared.toastMessage = "meeting_failed_to_send_part_of_invites_toast"
					ToastViewModel.shared.displayToast = true
				}
			}
			
			DispatchQueue.main.async {
				self.operationInProgress = false
				self.conferenceCreatedEvent = true
			}
		})
		scheduler.addDelegate(delegate: self.mSchedulerDelegate!)
	}
	
	func schedule() {
		guard !subject.isEmpty && !participants.isEmpty else {
			Log.error("\(MeetingViewModel.TAG) Either no subject was set or no participant was selected, can't schedule meeting.")
			DispatchQueue.main.async {
				ToastViewModel.shared.toastMessage = "Failed_no_subject_or_participant"
				ToastViewModel.shared.displayToast = true
			}
			return
		}
		
		guard CoreContext.shared.networkStatusIsConnected else {
			DispatchQueue.main.async {
				ToastViewModel.shared.toastMessage = "Unavailable_network"
				ToastViewModel.shared.displayToast = true
			}
			return
		}
		
		operationInProgress = true
		CoreContext.shared.doOnCoreQueue { core in
			Log.info("\(MeetingViewModel.TAG) Scheduling \(self.isBroadcastSelected ? "broadcast" : "meeting")")
			
			let conferenceInfo: ConferenceInfo?
			if let displayedMeeting = SharedMainViewModel.shared.displayedMeeting {
				conferenceInfo = displayedMeeting.confInfo
			} else {
				conferenceInfo = try? Factory.Instance.createConferenceInfo()
			}
			
			guard let confInfo = conferenceInfo else {
				Log.error("\(MeetingViewModel.TAG) Failed to create conference info")
				return
			}
			
			guard let localAccount = core.defaultAccount else {
				Log.error("\(MeetingViewModel.TAG) Default account is nil")
				return
			}
			
			guard let organizer = localAccount.params?.identityAddress else {
				Log.error("\(MeetingViewModel.TAG) Account params or identityAddress is nil")
				return
			}

			confInfo.organizer = organizer
			
			confInfo.setCapability(streamType: .Text, enable: true)
			
			// Enable end-to-end encryption if client supports it
			//if isEndToEndEncryptedChatAvailable(core: core) {
			if false {
				Log.info("\(MeetingViewModel.TAG) Requesting EndToEnd security level for conference")
				confInfo.securityLevel = .EndToEnd
			} else {
				Log.info("\(MeetingViewModel.TAG) Requesting PointToPoint security level for conference")
				confInfo.securityLevel = .PointToPoint
			}
			
			if self.conferenceScheduler == nil {
				Log.info("\(MeetingViewModel.TAG) ConferenceScheduler is nil, resetting...")
				self.resetConferenceSchedulerAndListeners(core: core)
			}
			
			guard let scheduler = self.conferenceScheduler else {
				Log.error("\(MeetingViewModel.TAG) ConferenceScheduler still nil after reset")
				return
			}
			
			self.fillConferenceInfo(confInfo: confInfo)
			scheduler.account = localAccount
			scheduler.info = confInfo
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
	}
	
	func cancelMeetingWithNotifications(meeting: MeetingModel) {
		CoreContext.shared.doOnCoreQueue { core in
			self.resetConferenceSchedulerAndListeners(core: core)
			
			guard let scheduler = self.conferenceScheduler else {
				Log.info("\(MeetingViewModel.TAG) ConferenceScheduler is nil after reset, nothing to cancel")
				return
			}
			
			scheduler.cancelConference(conferenceInfo: meeting.confInfo)
		}
	}
	
	func createMeetingEKEvent() -> EKEvent {
		let event: EKEvent = EKEvent(eventStore: eventStore)
		event.title = subject
		event.startDate = fromDate
		event.endDate = toDate
		event.notes = description
		event.calendar = eventStore.defaultCalendarForNewEvents
		event.location = "Linphone video meeting"
		return event
	}
	// For iOS 16 and below
	func addMeetingToCalendar() {
		eventStore.requestAccess(to: .event, completion: { (granted: Bool, error: (any Error)?) in
			if !granted {
				Log.error("\(MeetingViewModel.TAG) Failed to add meeting to calendar: access not granted")
			} else if error == nil {
				let event = self.createMeetingEKEvent()
				do {
					try self.eventStore.save(event, span: .thisEvent)
					Log.info("\(MeetingViewModel.TAG) Meeting '\(self.subject)' added to calendar")
					ToastViewModel.shared.toastMessage = "Meeting_added_to_calendar"
					ToastViewModel.shared.displayToast = true
				} catch let error as NSError {
					Log.error("\(MeetingViewModel.TAG) Failed to add meeting to calendar: \(error)")
					ToastViewModel.shared.toastMessage = "Error: \(error)"
					ToastViewModel.shared.displayToast = true
				}
			} else {
				Log.error("\(MeetingViewModel.TAG) Failed to add meeting to calendar: \(error?.localizedDescription ?? "")")
			}
		})
	}
	
	func joinMeeting(addressUri: String) {
		CoreContext.shared.doOnCoreQueue { _ in
			if let address = try? Factory.Instance.createAddress(addr: addressUri) {
				TelecomManager.shared.doCallOrJoinConf(address: address)
			}
		}
	}
}

// swiftlint:enable type_body_length
