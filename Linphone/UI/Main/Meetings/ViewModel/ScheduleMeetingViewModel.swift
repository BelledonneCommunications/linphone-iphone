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

class SelectedAddressModel: ObservableObject {
	var address: Address
	var avatarModel: ContactAvatarModel
	
	init (addr: Address, avModel: ContactAvatarModel) {
		address = addr
		avatarModel = avModel
	}
}

class ScheduleMeetingViewModel: ObservableObject {
	static let TAG = "[ScheduleMeetingViewModel]"
	
	@Published var isBroadcastSelected: Bool = false
	@Published var showBroadcastHelp: Bool = false
	@Published var subject: String = ""
	@Published var description: String = ""
	@Published var allDayMeeting: Bool = false
	@Published var fromDateStr: String = ""
	@Published var fromTime: String = ""
	@Published var toDateStr: String = ""
	@Published var toTime: String = ""
	@Published var timezone: String = ""
	@Published var sendInvitations: Bool = true
	@Published var participantsToAdd: [SelectedAddressModel] = []
	@Published var participants: [SelectedAddressModel] = []
	@Published var operationInProgress: Bool = false
	@Published var conferenceCreatedEvent: Bool = false
	
	@Published var searchField: String = ""
	
	var conferenceScheduler: ConferenceScheduler?
	private var mSchedulerSubscriptions = Set<AnyCancellable?>()
	var conferenceInfoToEdit: ConferenceInfo?
	
	@Published var fromDate: Date
	@Published var toDate: Date
	
	init() {
		fromDate = Calendar.current.date(byAdding: .hour, value: 1, to: Date.now)!
		toDate = Calendar.current.date(byAdding: .hour, value: 2, to: Date.now)!
		computeDateLabels()
		computeTimeLabels()
		updateTimezone()
	}
	
	func resetViewModelData() {
		isBroadcastSelected = false
		showBroadcastHelp = false
		subject = ""
		description = ""
		allDayMeeting = false
		timezone = ""
		sendInvitations = true
		participantsToAdd = []
		participants = []
		operationInProgress = false
		conferenceCreatedEvent = false
		searchField = ""
		
		fromDate = Calendar.current.date(byAdding: .hour, value: 1, to: Date.now)!
		toDate = Calendar.current.date(byAdding: .hour, value: 2, to: Date.now)!
		computeDateLabels()
		computeTimeLabels()
		updateTimezone()
	}
	
	func computeDateLabels() {
		var day = fromDate.formatted(Date.FormatStyle().weekday(.wide))
		var dayNumber = fromDate.formatted(Date.FormatStyle().day(.twoDigits))
		var month = fromDate.formatted(Date.FormatStyle().month(.wide))
		fromDateStr = "\(day) \(dayNumber), \(month)"
		Log.info("\(ScheduleMeetingViewModel.TAG) computed start date is \(fromDateStr)")
		
		day = toDate.formatted(Date.FormatStyle().weekday(.wide))
		dayNumber = toDate.formatted(Date.FormatStyle().day(.twoDigits))
		month = toDate.formatted(Date.FormatStyle().month(.wide))
		toDateStr = "\(day) \(dayNumber), \(month)"
		Log.info("\(ScheduleMeetingViewModel.TAG)) computed end date is \(toDateStr)")
	}
	
	func computeTimeLabels() {
		let formatter = DateFormatter()
		formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
		fromTime = formatter.string(from: fromDate)
		toTime = formatter.string(from: toDate)
	}
	
	private func updateTimezone() {
		// TODO
	}
	
	func selectParticipant(addr: Address) {
		if let idx = participantsToAdd.firstIndex(where: {$0.address.weakEqual(address2: addr)}) {
			participantsToAdd.remove(at: idx)
		} else {
			participantsToAdd.append(SelectedAddressModel(addr: addr, avModel: ContactAvatarModel.getAvatarModelFromAddress(address: addr)))
		}
	}
	func addParticipants() {
		var list = participants
		for selectedAddr in participantsToAdd {
			if let found = list.first(where: { $0.address.weakEqual(address2: selectedAddr.address) }) {
				Log.info("\(ScheduleMeetingViewModel.TAG) Participant \(found.address.asStringUriOnly()) already in list, skipping")
				continue
			}
			
			list.append(selectedAddr)
			Log.info("\(ScheduleMeetingViewModel.TAG) Added participant \(selectedAddr.address.asStringUriOnly())")
		}
		Log.info("\(ScheduleMeetingViewModel.TAG) [\(list.count - participants.count) participants added, now there are \(list.count) participants in list")

		participants = list
		participantsToAdd = []
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
				Log.error("\(ScheduleMeetingViewModel.TAG) Failed to create Participant Info from address \(participant.address.asStringUriOnly())")
			}
		}
		confInfo.participantInfos = participantsInfoList
	}
	
	private func initConferenceSchedulerAndListeners(core: Core) {
		self.conferenceScheduler = try? core.createConferenceScheduler()
		
		self.mSchedulerSubscriptions.insert(self.conferenceScheduler?.publisher?.onStateChanged?.postOnCoreQueue { (cbVal: (conferenceScheduler: ConferenceScheduler, state: ConferenceScheduler.State)) in
			
			Log.info("\(ScheduleMeetingViewModel.TAG) Conference state changed \(cbVal.state)")
			if cbVal.state == ConferenceScheduler.State.Error {
				DispatchQueue.main.async {
					self.operationInProgress = false
					// TODO: show error toast
				}
			} else if cbVal.state == ConferenceScheduler.State.Ready {
				let conferenceAddress = self.conferenceScheduler?.info?.uri
				if let confInfoToEdit = self.conferenceInfoToEdit {
					Log.info("\(ScheduleMeetingViewModel.TAG) Conference info \(confInfoToEdit.uri?.asStringUriOnly() ?? "'nil'") has been updated")
				} else {
					Log.info("\(ScheduleMeetingViewModel.TAG) Conference info created, address will be \(conferenceAddress?.asStringUriOnly() ?? "'nil'")")
				}
				
				if self.sendInvitations {
					Log.info("\(ScheduleMeetingViewModel.TAG) User asked for invitations to be sent, let's do it")
					if let chatRoomParams = try? core.createDefaultChatRoomParams() {
						chatRoomParams.groupEnabled = false
						chatRoomParams.backend = ChatRoom.Backend.FlexisipChat
						chatRoomParams.encryptionEnabled = true
						chatRoomParams.subject = "Meeting invitation" // Won't be used
						self.conferenceScheduler?.sendInvitations(chatRoomParams: chatRoomParams)
					} else {
						Log.error("\(ScheduleMeetingViewModel.TAG) Failed to create default chatroom parameters. This should not happen")
					}
				} else {
					Log.info("\(ScheduleMeetingViewModel.TAG) User didn't asked for invitations to be sent")
					DispatchQueue.main.async {
						self.operationInProgress = false
						self.conferenceCreatedEvent = true
					}
				}
			}
		})
		
		self.mSchedulerSubscriptions.insert(self.conferenceScheduler?.publisher?.onInvitationsSent?.postOnCoreQueue { (cbVal: (conferenceScheduler: ConferenceScheduler, failedInvitations: [Address])) in
			
			if cbVal.failedInvitations.isEmpty {
				Log.info("\(ScheduleMeetingViewModel.TAG) All invitations have been sent")
			} else if cbVal.failedInvitations.count == self.participants.count {
				Log.error("\(ScheduleMeetingViewModel.TAG) No invitation sent!")
				// TODO: show error toast
			} else {
				Log.warn("\(ScheduleMeetingViewModel.TAG) \(cbVal.failedInvitations.count) invitations couldn't have been sent for:")
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
			Log.error("\(ScheduleMeetingViewModel.TAG) Either no subject was set or no participant was selected, can't schedule meeting.")
			// TODO: show red toast
			return
		}
		operationInProgress = true
		
		CoreContext.shared.doOnCoreQueue { core in
			Log.info("\(ScheduleMeetingViewModel.TAG) Scheduling \(self.isBroadcastSelected ? "broadcast" : "meeting")")
			
			let localAccount = core.defaultAccount
			let localAddress = localAccount?.params?.identityAddress
			
			if let conferenceInfo = try? Factory.Instance.createConferenceInfo() {
				conferenceInfo.organizer = localAddress
				self.fillConferenceInfo(confInfo: conferenceInfo)
				if self.conferenceScheduler == nil {
					self.initConferenceSchedulerAndListeners(core: core)
				}
				self.conferenceScheduler?.account = localAccount
				// Will trigger the conference creation automatically
				self.conferenceScheduler?.info = conferenceInfo
			}
		}
	}
	
	func update() {
		self.operationInProgress = true
		CoreContext.shared.doOnCoreQueue { core in
			Log.info("\(ScheduleMeetingViewModel.TAG) Updating \(self.isBroadcastSelected ? "broadcast" : "meeting")")
			
			if let conferenceInfo = self.conferenceInfoToEdit {
				self.fillConferenceInfo(confInfo: conferenceInfo)
				if self.conferenceScheduler == nil {
					self.initConferenceSchedulerAndListeners(core: core)
				}
				
				// Will trigger the conference update automatically
				self.conferenceScheduler?.info = conferenceInfo
			} else {
				Log.error("No conference info to edit found!")
				return
			}
		}
	}
	
	func loadExistingConferenceInfoFromUri(conferenceUri: String) {
		CoreContext.shared.doOnCoreQueue { core in
			if let conferenceAddress = core.interpretUrl(url: conferenceUri, applyInternationalPrefix: false) {
				if let conferenceInfo = core.findConferenceInformationFromUri(uri: conferenceAddress) {
					
					self.conferenceInfoToEdit = conferenceInfo
					Log.info("\(ScheduleMeetingViewModel.TAG)  Found conference info matching URI \(conferenceInfo.uri?.asString()) with subject \(conferenceInfo.subject)")
					
					self.fromDate = Date(timeIntervalSince1970: TimeInterval(conferenceInfo.dateTime))
					self.toDate = Calendar.current.date(byAdding: .minute, value: Int(conferenceInfo.duration), to: self.fromDate)!
					
					var list: [SelectedAddressModel] = []
					for partInfo in conferenceInfo.participantInfos {
						if let addr = partInfo.address {
							let avatarModel = ContactAvatarModel.getAvatarModelFromAddress(address: addr)
							list.append(SelectedAddressModel(addr: addr, avModel: avatarModel))
							Log.info("\(ScheduleMeetingViewModel.TAG) Loaded participant \(addr.asStringUriOnly())")
						}
					}
					Log.info("\(ScheduleMeetingViewModel.TAG) \(list.count) participants loaded from found conference info")
					
					DispatchQueue.main.async {
						self.subject = conferenceInfo.subject ?? ""
						self.description = conferenceInfo.description ?? ""
						self.isBroadcastSelected = false // TODO FIXME
						self.computeDateLabels()
						self.computeTimeLabels()
						self.updateTimezone()
						self.participants = list
					}
					
				} else {
					Log.error("\(ScheduleMeetingViewModel.TAG) Failed to find a conference info matching URI [${conferenceAddress.asString()}], abort")
				}
			} else {
				Log.error("\(ScheduleMeetingViewModel.TAG) Failed to parse conference URI [$conferenceUri], abort")
			}
			
		}
	}
	
}
