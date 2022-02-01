/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linhome
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
import AVFoundation

class ConferenceViewModel {
	
	var core : Core { get { Core.get() } }
	static let shared = ConferenceViewModel()
	
	let conferenceExists = MutableLiveData<Bool>()
	let subject = MutableLiveData<String>()
	let isConferenceLocallyPaused = MutableLiveData<Bool>()
	let isVideoConference = MutableLiveData<Bool>()
	let isMeAdmin = MutableLiveData<Bool>()

	let conference = MutableLiveData<Conference>()
	let conferenceParticipants = MutableLiveData<[ConferenceParticipantData]>()
	let conferenceParticipantDevices = MutableLiveData<[ConferenceParticipantDeviceData]>()
	let conferenceDisplayMode = MutableLiveData<ConferenceLayout>()
	let isRecording = MutableLiveData<Bool>()
	let isRemotelyRecorded = MutableLiveData<Bool>()
	let maxParticipantsForMosaicLayout = ConfigManager.instance().lpConfigIntForKey(key: "max_conf_part_mosaic_layout",defaultValue: 6)
	
	
	private var conferenceDelegate :  ConferenceDelegateStub?
	private var coreDelegate :  CoreDelegateStub?
	
	init ()  {
		conferenceDelegate = ConferenceDelegateStub(onParticipantAdded: { (conference: Conference, participant: Participant) in
			Log.i("[Conference] \(conference) Participant \(participant) added")
			self.updateParticipantsList(conference)
			let count = self.conferenceParticipantDevices.value!.count
			if (count > self.maxParticipantsForMosaicLayout) {
				Log.w("[Conference] \(conference) More than \(self.maxParticipantsForMosaicLayout) participants \(count), forcing active speaker layout")
				self.conferenceDisplayMode.value = .ActiveSpeaker
			}
		}, onParticipantRemoved: {(conference: Conference, participant: Participant) in
			Log.i("[Conference] \(conference) \(participant) Participant removed")
			self.updateParticipantsList(conference)
		}, onParticipantDeviceAdded: {(conference: Conference, participantDevice: ParticipantDevice) in
			Log.i("[Conference] \(conference) Participant device \(participantDevice) added")
			self.updateParticipantsDevicesList(conference)
		}, onParticipantDeviceRemoved: { (conference: Conference, participantDevice: ParticipantDevice) in
			Log.i("[Conference] \(conference) Participant device \(participantDevice) removed")
			self.updateParticipantsDevicesList(conference)
		}, onParticipantAdminStatusChanged: { (conference: Conference, participant: Participant) in
			Log.i("[Conference] \(conference) Participant admin status changed")
			self.isMeAdmin.value = conference.me?.isAdmin
			self.updateParticipantsList(conference)
		}, onParticipantDeviceLeft: { (conference: Conference, device: ParticipantDevice) in
			Log.i("[Conference] onParticipantDeviceJoined Entered conference")
			self.isConferenceLocallyPaused.value = true
		}, onParticipantDeviceJoined: { (conference: Conference, device: ParticipantDevice) in
			Log.i("[Conference] onParticipantDeviceJoined Entered conference")
			self.isConferenceLocallyPaused.value = false
		}, onSubjectChanged: { (conference: Conference, subject: String) in
			self.subject.value = subject
		}
		)
		
		coreDelegate = CoreDelegateStub(
			onConferenceStateChanged: { (core, conference, state) in
				Log.i("[Conference] \(conference) Conference state changed: \(state)")
				self.isVideoConference.value = conference.currentParams?.videoEnabled == true
				
				if (state == Conference.State.Instantiated) {
					self.initConference(conference)
				} else if (state == Conference.State.Created) {
					self.initConference(conference)
					self.configureConference(conference)
				} else if (state == Conference.State.Terminated || state == Conference.State.TerminationFailed) {
					self.terminateConference(conference)
				}
				
				let layout = conference.layout == .None ? .Grid : conference.layout
				self.conferenceDisplayMode.value = layout
				Log.i("[Conference] \(conference) Conference current layout is: \(layout)")
			}
		)
		
		Core.get().addDelegate(delegate: coreDelegate!)
		conferenceParticipants.value = []
		conferenceParticipantDevices.value = []
		conferenceDisplayMode.value = .Grid
		subject.value = VoipTexts.conference_default_title
		
		if let conference = core.conference != nil ? core.conference : core.currentCall?.conference {
			Log.i("[Conference] Found an existing conference: \(conference)")
			initConference(conference)
			configureConference(conference)
		}
		
		
	}
	
	func initConference(_ conference: Conference) {
		 conferenceExists.value = true
		self.conference.value = conference
		conference.addDelegate(delegate: self.conferenceDelegate!)
		 isRecording.value = conference.isRecording
	 }
	
	func terminateConference(_ conference: Conference) {
		conferenceExists.value = false
		isVideoConference.value = false
		self.conferenceParticipants.value?.forEach{ $0.destroy()}
		self.conferenceParticipantDevices.value?.forEach{ $0.destroy()}
		conferenceParticipants.value = []
		conferenceParticipantDevices.value = []
	}
	
	func configureConference(_ conference: Conference) {
		self.updateParticipantsList(conference)
		self.updateParticipantsDevicesList(conference)
		
		isConferenceLocallyPaused.value = !conference.isIn
		self.isMeAdmin.value = conference.me?.isAdmin == true
		isVideoConference.value = conference.currentParams?.videoEnabled == true

		self.subject.value =   conference.subject.isEmpty ? (
			conference.me?.isFocus == true ? (
				VoipTexts.conference_local_title
			) : (
				VoipTexts.conference_default_title
			)
		) : (
			conference.subject
		)
	}
	
	
	
	func pauseConference() {
		Log.i("[Conference] Leaving conference with address \(conference) temporarily")
		conference.value?.leave()
	}
	
	func resumeConference() {
		Log.i("[Conference] entering conference with address \(conference)")
		conference.value?.enter()
	}
	
	func togglePlayPause () {
		if (isConferenceLocallyPaused.value == true) {
			resumeConference()
			isConferenceLocallyPaused.value = false
		} else {
			pauseConference()
			isConferenceLocallyPaused.value = true
		}
	}
	
	func toggleRecording() {
		guard let conference = conference.value else {
			Log.e("[Conference] Failed to find conference!")
			return
		}
		/* frogtrust has is own recording method
		if (conference.isRecording == true) {
			conference.stopRecording()
		} else {
			let path = AppManager.recordingFilePathFromCall(address: conference.conferenceAddress?.asStringUriOnly() ?? "")
			Log.i("[Conference] Starting recording \(conference) in file \(path)")
			conference.startRecording(path: path)
		}*/
		
		isRecording.value = conference.isRecording
	}
	
	private func updateParticipantsList(_ conference: Conference) {
		self.conferenceParticipants.value?.forEach{ $0.destroy()}
		var participants :[ConferenceParticipantData] = []
		
		let participantsList = conference.participantList
		Log.i("[Conference] \(conference) Conference has \(participantsList.count) participants")
		
		participantsList.forEach { (participant) in
			let participantDevices = participant.devices
			Log.i("[Conference] \(conference) Participant found: \(participant) with \(participantDevices.count) device(s)")
			let participantData = ConferenceParticipantData(conference: conference, participant: participant)
			participants.append(participantData)
		}
		
		conferenceParticipants.value = participants
	}
	
	private func updateParticipantsDevicesList(_ conference: Conference) {
		self.conferenceParticipantDevices.value?.forEach{ $0.destroy()}
		var devices :[ConferenceParticipantDeviceData] = []
		
		let participantsList = conference.participantList
		Log.i("[Conference] \(conference) Conference has \(participantsList.count) participants")
		
		participantsList.forEach { (participant) in
			let participantDevices = participant.devices
			Log.i("[Conference] \(conference) Participant found: \(participant) with \(participantDevices.count) device(s)")
			
			participantDevices.forEach { (device) in
				Log.i("[Conference] \(conference) Participant device found: \(device.name) (\(device.address!.asStringUriOnly()))")
				let deviceData = ConferenceParticipantDeviceData(participantDevice: device, isMe: false)
				devices.append(deviceData)
			}
			
		}
		conference.me?.devices.forEach { (device) in
			Log.i("[Conference] \(conference) Participant device for myself found: \(device.name)  (\(device.address!.asStringUriOnly()))")
			let deviceData = ConferenceParticipantDeviceData(participantDevice: device, isMe: true)
			devices.append(deviceData)
		}
		
		
		conferenceParticipantDevices.value = devices
	}
	
	func updateParticipants(addresses:[Address]) {
		guard let conference = conference.value else {
			Log.w("[Conference Participants] conference not set, can't update participants")
			return
		}
		do {
			// Adding new participants first, because if we remove all of them (or all of them except one)
			// It will terminate the conference first and we won't be able to add new participants after
			try addresses.forEach { address in
				let participant = conference.participantList.filter { $0.address?.asStringUriOnly() == address.asStringUriOnly() }.first
				if (participant == nil) {
					Log.i("[Conference Participants] Participant \(address.asStringUriOnly()) will be added to group")
					try conference.addParticipant(uri: address)
				}
			}
			
			// Removing participants
			try conference.participantList.forEach { participant in
				let member = addresses.filter { $0.asStringUriOnly() == participant.address?.asStringUriOnly() }.first
				if (member == nil) {
					Log.w("[Conference Participants] Participant \(participant.address?.asStringUriOnly()) will be removed from conference")
					try conference.removeParticipant(participant: participant)
				}
			}
		} catch {
			Log.e("[Conference Participants] Error updating participant lists \(error)")
		}
	}
	
	func addCallsToConference() {
		Log.i("[Conference] Trying to merge all calls into existing conference")
		guard let conf = conference.value else {
			return
		}
		core.calls.forEach { call in
			if (call.conference == nil) {
				try? conf.addParticipant(call: call)
			}
		}
	}

	
}

@objc class ConferenceViewModelBridge : NSObject {	
	@objc static func updateParticipantsList(addresses:[String]) {
		do {
			try ConferenceViewModel.shared.updateParticipants(addresses: addresses.map { try Factory.Instance.createAddress(addr: $0)} )
		} catch {
			Log.e("[ParticipantsListView] unable to update participants list \(error)")
		}
	}
}





enum FlexDirection {
	case ROW
	case ROW_REVERSE
	case COLUMN
	case COLUMN_REVERSE
}
