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
	
	let core = Core.get()
	static let shared = ConferenceViewModel()
	
	
	let isConferencePaused = MutableLiveData<Bool>()
	let canResumeConference = MutableLiveData<Bool>()
	
	let isMeConferenceFocus = MutableLiveData<Bool>()
	let isMeAdmin = MutableLiveData<Bool>()
	
	let conferenceAddress = MutableLiveData<Address>()
	
	let conferenceParticipants = MutableLiveData<[ConferenceParticipantData]>()
	let conferenceParticipantDevices = MutableLiveData<[ConferenceParticipantDeviceData]>()
	let conferenceDisplayMode = MutableLiveData<ConferenceLayout>()
		
	let isInConference = MutableLiveData<Bool>()
	
	let isVideoConference = MutableLiveData<Bool>()
	
	let isRecording = MutableLiveData<Bool>()
	let isRemotelyRecorded = MutableLiveData<Bool>()
	
	let subject = MutableLiveData<String>()
	
	let conference = MutableLiveData<Conference>()
	
	let maxParticipantsForMosaicLayout = ConfigManager.instance().lpConfigIntForKey(key: "max_conf_part_mosaic_layout",defaultValue: 6)
	
	private var conferenceDelegate :  ConferenceDelegateStub?
	private var coreDelegate :  CoreDelegateStub?
	
	init ()  {
		conferenceDelegate = ConferenceDelegateStub(onParticipantAdded: { (conference: Conference, participant: Participant)in
			if (conference.isMe(uri: participant.address!)) {
				Log.i("[Conference] \(conference) Entered conference")
				self.isConferencePaused.value = false
			} else {
				Log.i("[Conference] \(conference) Participant \(participant) added")
			}
			self.updateParticipantsList(conference)
			self.updateParticipantsDevicesList(conference)
			
			let count = self.conferenceParticipantDevices.value!.count
			if (count > self.maxParticipantsForMosaicLayout) {
				Log.w("[Conference] \(conference) More than \(self.maxParticipantsForMosaicLayout) participants \(count), forcing active speaker layout")
				self.conferenceDisplayMode.value = .ActiveSpeaker
			}
		}, onParticipantRemoved: {(conference: Conference, participant: Participant) in
			if (conference.isMe(uri: participant.address!)) {
				Log.i("[Conference] \(conference) \(participant) Left conference")
				self.isConferencePaused.value = true
			} else {
				Log.i("[Conference] \(conference) \(participant) Participant removed")
			}
			self.updateParticipantsList(conference)
			self.updateParticipantsDevicesList(conference)
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
		}
		)
		
		coreDelegate = CoreDelegateStub(
			onConferenceStateChanged: { (core, conference, state) in
				Log.i("[Conference] \(conference) Conference state changed: \(state)")
				self.isConferencePaused.value = !conference.isIn
				self.canResumeConference.value = true // TODO: How can this value be false?
				self.isVideoConference.value = conference.currentParams?.isVideoEnabled == true
				
				if (state == Conference.State.Instantiated) {
					self.conference.value = conference
					self.isInConference.value = true
					conference.addDelegate(delegate: self.conferenceDelegate!)
				} else if (state == Conference.State.Created) {
					self.updateParticipantsList(conference)
					self.updateParticipantsDevicesList(conference)
					self.isMeConferenceFocus.value = conference.me?.isFocus == true
					self.isMeAdmin.value = conference.me?.isAdmin == true
					self.conferenceAddress.value = conference.conferenceAddress
					self.subject.value =   conference.subject.isEmpty ? (
						conference.me?.isFocus == true ? (
							VoipTexts.conference_local_title
						) : (
							VoipTexts.conference_default_title
						)
					) : (
						conference.subject
					)
				} else if (state == Conference.State.Terminated || state == Conference.State.TerminationFailed) {
					self.isInConference.value = false
					self.isVideoConference.value = false
					conference.removeDelegate(delegate: self.conferenceDelegate!)
					self.conferenceParticipants.value?.forEach{ $0.destroy()}
					self.conferenceParticipantDevices.value?.forEach{ $0.destroy()}
					self.conferenceParticipants.value = []
					self.conferenceParticipantDevices.value = []
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
			self.conference.value = conference
			conference.addDelegate(delegate: self.conferenceDelegate!)
			
			
			isInConference.value = true
			isConferencePaused.value = !conference.isIn
			isMeConferenceFocus.value = conference.me?.isFocus == true
			isMeAdmin.value = conference.me?.isAdmin == true
			isVideoConference.value = conference.currentParams?.isVideoEnabled == true
			conferenceAddress.value = conference.conferenceAddress
			if (!conference.subject.isEmpty)  {
				subject.value = conference.subject
			}
			
			let layout = conference.layout == .None ? .Grid : conference.layout
			conferenceDisplayMode.value = layout 
			Log.i("[Conference] \(conference) Conference current layout is: \(layout)")
			
			updateParticipantsList(conference)
			updateParticipantsDevicesList(conference)
		}
		
		
	}
	
	
	func destroy() {
		core.removeDelegate(delegate: self.coreDelegate!)
		self.conferenceParticipants.value?.forEach{ $0.destroy()}
		self.conferenceParticipantDevices.value?.forEach{ $0.destroy()}
	}
	
	
	func pauseConference() {
		let defaultProxyConfig = core.defaultProxyConfig
		let localAddress = defaultProxyConfig?.identityAddress
		let participants : [Address] = []
		let remoteConference = core.searchConference(params: nil, localAddr: localAddress, remoteAddr: conferenceAddress.value, participants: participants)
		let localConference = core.searchConference(params: nil, localAddr: conferenceAddress.value, remoteAddr: conferenceAddress.value, participants: participants)
		let conference = remoteConference != nil ? remoteConference : localConference
		
		if (conference != nil) {
			Log.i("[Conference] Leaving conference with address \(conference) temporarily")
			conference!.leave()
		} else {
			Log.w("[Conference] Unable to find conference with address \(conference)")
		}
	}
	
	func resumeConference() {
		let defaultProxyConfig = core.defaultProxyConfig
		let localAddress = defaultProxyConfig?.identityAddress
		let participants : [Address] = []
		let remoteConference = core.searchConference(params: nil, localAddr: localAddress, remoteAddr: conferenceAddress.value, participants: participants)
		let localConference = core.searchConference(params: nil, localAddr: conferenceAddress.value, remoteAddr: conferenceAddress.value, participants: participants)
		
		if let conference = remoteConference != nil ? remoteConference : localConference {
			Log.i("[Conference] Entering again conference with address \(conference)")
			conference.enter()
		} else {
			Log.w("[Conference] Unable to find conference with address \(conference)")
		}
	}
	
	func togglePlayPause () {
		if (isConferencePaused.value == true) {
			resumeConference()
			isConferencePaused.value = false
		} else {
			pauseConference()
			isConferencePaused.value = true
		}
	}
	
	func toggleRecording() {
		guard let conference = core.conference else {
			Log.e("[Conference] Failed to find conference!")
			return
		}
				
		if (conference.isRecording == true) {
			conference.stopRecording()
		} else {
			let path = AppManager.recordingFilePathFromCall(address: conference.conferenceAddress?.asStringUriOnly() ?? "")
			Log.i("[Conference] Starting recording \(conference) in file \(path)")
			conference.startRecording(path: path)
		}
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
	
}

enum FlexDirection {
	case ROW
	case ROW_REVERSE
	case COLUMN
	case COLUMN_REVERSE
}
