/*
 * Copyright (c) 2010-2021 Belledonne Communications SARL.
 *
 * This file is part of linphone-android
 * (see https://www.linphone.org).
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
 * aDouble with this program. If not, see <http://www.gnu.org/licenses/>.
 */


import Foundation
import linphonesw

class ConferenceSchedulingViewModel  {
	
	var core : Core { get { Core.get() } }
	static let shared = ConferenceSchedulingViewModel()
	
	let subject = MutableLiveData<String>()
	let description = MutableLiveData<String>()
	
	let scheduleForLater = MutableLiveData<Bool>()
	let scheduledDate = MutableLiveData<Date>()
	let scheduledTime = MutableLiveData<Date>()

	var scheduledTimeZone = MutableLiveData<Int>()
	static let timeZones: [TimeZoneData] = computeTimeZonesList()
	
	var scheduledDuration = MutableLiveData<Int>()
	static let durationList: [Duration] = computeDurationList()
	
	let isEncrypted = MutableLiveData<Bool>()
	
	let sendInviteViaChat = MutableLiveData<Bool>()
	let sendInviteViaEmail = MutableLiveData<Bool>()
	
	let address = MutableLiveData<Address>()
	
	let conferenceCreationInProgress = MutableLiveData<Bool>()
	
	let conferenceCreationCompletedEvent: MutableLiveData<Pair<String?,String?>> = MutableLiveData()
	let onErrorEvent = MutableLiveData<String>()
	
	let continueEnabled: MutableLiveData<Bool> = MutableLiveData()
	
	let selectedAddresses = MutableLiveData<[Address]>([])
	
	private var conferenceScheduler: ConferenceScheduler? = nil
	
	
	private var hour: Int = 0
	private var minutes: Int = 0
	
	private var chatRooomDelegate : ChatRoomDelegate? = nil
	private var conferenceSchedulerDelegate : ConferenceSchedulerDelegateStub? = nil
	
	var existingConfInfo:MutableLiveData<ConferenceInfo?> = MutableLiveData()
	
	init () {
		
		conferenceSchedulerDelegate = ConferenceSchedulerDelegateStub(
			onStateChanged: { scheduler, state in
				Log.i("[Conference Creation] Conference scheduler state is \(state)")
				if (state == .Ready) {
					Log.i("[Conference Creation] Conference info created, address will be \(scheduler.info?.uri?.asStringUriOnly())")
					guard let conferenceAddress = scheduler.info?.uri else {
						Log.e("[Conference Creation] conference address is null")
						return
					}
					self.address.value = conferenceAddress
					
					if (self.scheduleForLater.value == true && self.sendInviteViaChat.value == true) {
						// Send conference info even when conf is not scheduled for later
						// as the conference server doesn't invite participants automatically
						if let chatRoomParams = self.getConferenceInvitationsChatRoomParams() {
							scheduler.sendInvitations(chatRoomParams: chatRoomParams)
						}
					} else {
						self.conferenceCreationInProgress.value = false
						self.conferenceCreationCompletedEvent.value = Pair(conferenceAddress.asStringUriOnly(),self.conferenceScheduler?.info?.subject)
					}
				}
			}, onInvitationsSent: { conferenceScheduler, failedInvitations in
				Log.i("[Conference Creation] Conference information successfully sent to all participants")
				self.conferenceCreationInProgress.value = false
				
				if (failedInvitations.count > 0) {
					failedInvitations.forEach { address in
						Log.e("[Conference Creation] Conference information wasn't sent to participant \(address.asStringUriOnly())")
						self.onErrorEvent.value = VoipTexts.conference_schedule_info_not_sent_to_participant+" (\(address.username))"
					}
				}
				
				guard let conferenceAddress = conferenceScheduler.info?.uri else {
					Log.e("[Conference Creation] conference address is null")
					return
				}
				self.conferenceCreationCompletedEvent.value = Pair(conferenceAddress.asStringUriOnly(),self.conferenceScheduler?.info?.subject)
			}
		)
				
		chatRooomDelegate = ChatRoomDelegateStub(
			onStateChanged : { (room: ChatRoom, state: ChatRoom.State) -> Void in
				if (state == ChatRoom.State.Created) {
					Log.i("[Conference Creation] Chat room created")
					room.removeDelegate(delegate: self.chatRooomDelegate!)
				} else if (state == ChatRoom.State.CreationFailed) {
					Log.e("[Conference Creation] Group chat room creation has failed !")
					room.removeDelegate(delegate: self.chatRooomDelegate!)
				}
			}
		)
		
		reset()
		
		subject.observe { _ in
			self.continueEnabled.value = self.allMandatoryFieldsFilled()
		}
		scheduleForLater.observe { _ in
			self.continueEnabled.value = self.allMandatoryFieldsFilled()
		}
		scheduledDate.observe { _ in
			self.continueEnabled.value = self.allMandatoryFieldsFilled()
		}
		scheduledTime.observe { _ in
			self.continueEnabled.value = self.allMandatoryFieldsFilled()
		}
		
		
	}
	
	func isEndToEndEncryptedChatAvailable() -> Bool {
		let core = Core.get()
		return core.limeX3DhEnabled &&
		((core.limeX3DhServerUrl != nil && core.limeX3DhServerUrl.count > 0) || core.defaultAccount?.params?.limeServerUrl != nil) &&
					 core.defaultAccount?.params?.conferenceFactoryUri != nil
	 }
	
	func reset() {
		
		subject.value = ""
		scheduleForLater.value = false
		isEncrypted.value = false
		sendInviteViaChat.value = true
		sendInviteViaEmail.value = false
		scheduledDate.value = nil
		scheduledTime.value = nil

		scheduledTimeZone.value = ConferenceSchedulingViewModel.timeZones.indices.filter {
			ConferenceSchedulingViewModel.timeZones[$0].timeZone.identifier == NSTimeZone.default.identifier
		}.first
		
		scheduledDuration.value = ConferenceSchedulingViewModel.durationList.indices.filter {
			ConferenceSchedulingViewModel.durationList[$0].value == 60
		}.first
		continueEnabled.value = false
    selectedAddresses.value = []
		existingConfInfo.value = nil
		description.value = ""
    
	}
	
	
	
	func destroy() {
		conferenceScheduler?.removeDelegate(delegate: conferenceSchedulerDelegate!)
	}
	
	
	func gotoChatRoom() {
		
	}
		
	func createConference() {
		
		if (selectedAddresses.value?.count == 0) {
			Log.e("[Conference Creation] Couldn't create conference without any participant!")
			return
		}
		
		do {
			conferenceCreationInProgress.value = true
			guard let localAccount = core.defaultAccount, let localAddress = localAccount.params?.identityAddress else {
				Log.e("[Conference Creation] Couldn't get local address from default account!")
				return
			}
			
			conferenceScheduler = try? Core.get().createConferenceScheduler()
			conferenceScheduler?.addDelegate(delegate: conferenceSchedulerDelegate!)
		
			guard let conferenceInfo = existingConfInfo.value != nil ? existingConfInfo.value??.clone() : try Factory.Instance.createConferenceInfo() else {
				Log.e("[Conference Creation/Update] Failed, unable to get conf info.")
				return
			}
			conferenceInfo.organizer = localAddress
			subject.value.map { conferenceInfo.subject = $0}
			description.value.map { conferenceInfo.description = $0}
			conferenceInfo.participants = selectedAddresses.value!
			if (scheduleForLater.value == true) {
				let timestamp = getConferenceStartTimestamp()
				conferenceInfo.dateTime = time_t(timestamp)
				scheduledDuration.value.map { conferenceInfo.duration = UInt(ConferenceSchedulingViewModel.durationList[$0].value) }
			}
      conferenceScheduler?.account = localAccount
			conferenceScheduler?.info = conferenceInfo // Will trigger the conference creation automatically
			existingConfInfo.value = conferenceInfo

		} catch {
			Log.e("[Conference Creation] Failed \(error)")
		}
	}
	
	
	
	private func allMandatoryFieldsFilled() -> Bool {
		return subject.value != nil && subject.value!.count > 0 && (scheduleForLater.value != true || (scheduledDate.value != nil && scheduledTime.value != nil) );
	}
	

	private func getConferenceStartTimestamp() -> Double {
		let days = Int32(scheduledDate.value!.timeIntervalSince1970)/86400
		let time = Int32(scheduledTime.value!.timeIntervalSince1970)%86400

		return scheduleForLater.value == true ? TimeInterval(days * 86400 + time) + Double(ConferenceSchedulingViewModel.timeZones[scheduledTimeZone.value!].timeZone.secondsFromGMT()-TimeZone.current.secondsFromGMT())
		: Date().timeIntervalSince1970
	}
	
	
	private static func computeTimeZonesList() -> [TimeZoneData] {
		return TimeZone.knownTimeZoneIdentifiers.map {
			(ident) in TimeZoneData(timeZone:TimeZone(identifier:ident)!)
		}.sorted()
	}
	
	private static func computeDurationList() -> [Duration] {
		return [Duration(value: 30, display: "30min"), Duration(value: 60, display: "1h"), Duration(value: 120, display: "2h")]
	}
	
	func getConferenceInvitationsChatRoomParams() -> ChatRoomParams? {
		if let chatRoomParams = try?self.core.createDefaultChatRoomParams() {
			chatRoomParams.encryptionEnabled = self.isEndToEndEncryptedChatAvailable()
			chatRoomParams.groupEnabled = false
			chatRoomParams.backend = chatRoomParams.encryptionEnabled ? .FlexisipChat : .Basic
			chatRoomParams.subject = self.subject.value!
			return chatRoomParams
		}
		return nil
	}
	
	
}
