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
	
	let core = Core.get()
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
	
	let conferenceCreationCompletedEvent: MutableLiveData<Bool> = MediatorLiveData()
	let onErrorEvent = MutableLiveData<String>()
	
	let continueEnabled: MediatorLiveData<Bool> = MediatorLiveData()
	
	let selectedAddresses = MutableLiveData<[Address]>([])
	
	
	private var hour: Int = 0
	private var minutes: Int = 0
	
	private var coreDelegate : CoreDelegateStub? = nil
	private var chatRooomDelegate : ChatRoomDelegate? = nil
	
	
	init () {
		coreDelegate = CoreDelegateStub(
			onConferenceStateChanged : { (core: Core, conference: Conference, state: Conference.State?) -> Void in
				Log.i("[Conference Creation] Conference state changed: \(state)")
				if (state == .CreationPending) {
					Log.i("[Conference Creation] Conference address will be \(conference.conferenceAddress?.asStringUriOnly())")
					self.address.value = conference.conferenceAddress
					
					if (self.scheduleForLater.value == true) {
						self.sendConferenceInfo()
					} else {
						self.conferenceCreationInProgress.value = false
						self.conferenceCreationCompletedEvent.value = true
					}
				}
			},
			onConferenceInfoOnSent : { (core: Core, conferenceInfo:ConferenceInfo) -> Void in
				Log.i("[Conference Creation] Conference information successfully sent to all participants")
				self.conferenceCreationInProgress.value = false
				self.conferenceCreationCompletedEvent.value = true
			},
			onConferenceInfoOnParticipantError : { (core: Core, conferenceInfo: ConferenceInfo, participant: Address, error: ConferenceInfoError?) -> Void in
				Log.e("[Conference Creation] Conference information wasn't sent to participant \(participant.asStringUriOnly())")
				self.onErrorEvent.value = VoipTexts.conference_schedule_info_not_sent_to_participant
				self.conferenceCreationInProgress.value = false
			}
		)
		Core.get().addDelegate(delegate: coreDelegate!)
		
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
	
	func reset() {
		
		subject.value = ""
		scheduleForLater.value = false
		isEncrypted.value = false
		sendInviteViaChat.value = true
		sendInviteViaEmail.value = false
		let now = Date()
		scheduledTime.value = Calendar.current.date(from: Calendar.current.dateComponents([.hour, .minute, .second], from: now))
		scheduledDate.value = Calendar.current.date(from: Calendar.current.dateComponents([.year, .month, .day], from: now))

		scheduledTimeZone.value = ConferenceSchedulingViewModel.timeZones.indices.filter {
			ConferenceSchedulingViewModel.timeZones[$0].timeZone.identifier == NSTimeZone.default.identifier
		}.first
		
		scheduledDuration.value = ConferenceSchedulingViewModel.durationList.indices.filter {
			ConferenceSchedulingViewModel.durationList[$0].value == 60
		}.first
		
		continueEnabled.value = false
	}
	
	
	
	func destroy() {
		core.removeDelegate(delegate: coreDelegate!)
	}
	
	
	
	func createConference() {
		
		if (selectedAddresses.value?.count == 0) {
			Log.e("[Conference Creation] Couldn't create conference without any participant!")
			return
		}
		
		do {
			conferenceCreationInProgress.value = true
			let localAddress = core.defaultAccount?.params?.identityAddress
			
			// TODO: Temporary workaround for chat room, to be removed once we can get matching chat room from conference
			let chatRoomParams = try core.createDefaultChatRoomParams()
			chatRoomParams.backend = ChatRoomBackend.FlexisipChat
			chatRoomParams.groupEnabled = true
			chatRoomParams.subject = subject.value!
			let chatRoom = try core.createChatRoom(params: chatRoomParams, localAddr: localAddress, participants: selectedAddresses.value!)
			Log.i("[Conference Creation] Creating chat room with same subject [\(subject.value)] & participants as for conference")
			chatRoom.addDelegate(delegate: chatRooomDelegate!)
			let params = try core.createConferenceParams()
			params.videoEnabled = true // TODO: Keep this to true ?
			params.subject = subject.value!
			let startTime = getConferenceStartTimestamp()
			params.startTime = time_t(startTime)
			
			scheduledDuration.value.map {
				params.endTime = params.startTime + $0
			}
			try core.createConferenceOnServer(params: params, localAddr: localAddress, participants: selectedAddresses.value!)
		} catch {
			Log.e("[Conference Creation] Failed \(error)")
		}
	}
	
	
	
	private func allMandatoryFieldsFilled() -> Bool {
		return subject.value != nil && subject.value!.count > 0 && (scheduleForLater.value != true || (scheduledDate.value != nil && scheduledTime.value != nil));
	}
	
	private func sendConferenceInfo() {
		let participants :[Address] = []
		
		do {
			let conferenceInfo = try Factory.Instance.createConferenceInfo()
			conferenceInfo.uri = try Factory.Instance.createAddress(addr: "sip:video-conference-0@sip.linphone.org") // TODO: use address.value
			conferenceInfo.participants = participants
			conferenceInfo.organizer = core.defaultAccount?.params?.identityAddress
			subject.value.map { conferenceInfo.subject = $0}
			description.value.map { conferenceInfo.description = $0}
			scheduledDuration.value.map {conferenceInfo.duration = $0 }
			let timestamp = getConferenceStartTimestamp()
			conferenceInfo.dateTime = time_t(timestamp)
			
			Log.i("[Conference Creation] Conference date & time set to ${TimestampUtils.dateToString(timestamp)} ${TimestampUtils.timeToString(timestamp)}, duration = ${conferenceInfo.duration}")
			core.sendConferenceInformation(conferenceInformation: conferenceInfo, text: "")
			
			conferenceCreationInProgress.value = false
			conferenceCreationCompletedEvent.value = true
		} catch {
			Log.e("[Conference Creation] unable to create conference \(error)")
		}
	}
	
	private func getConferenceStartTimestamp() -> Double {
		return scheduleForLater.value == true ? scheduledDate.value!.timeIntervalSince1970 + scheduledTime.value!.timeIntervalSince1970 : Date().timeIntervalSince1970
		
	}
	
	
	private static func computeTimeZonesList() -> [TimeZoneData] {
		return TimeZone.knownTimeZoneIdentifiers.map {
			(ident) in TimeZoneData(timeZone:TimeZone(identifier:ident)!)
		}.sorted()
	}
	
	private static func computeDurationList() -> [Duration] {
		return [Duration(value: 30, display: "30min"), Duration(value: 60, display: "1h"), Duration(value: 120, display: "2h")]
	}
	
	
}
