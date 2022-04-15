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
	let conferenceCreationPending = MutableLiveData<Bool>()
	let conferenceParticipants = MutableLiveData<[ConferenceParticipantData]>()
	let conferenceParticipantDevices = MutableLiveData<[ConferenceParticipantDeviceData]>()
	let conferenceDisplayMode = MutableLiveData<ConferenceLayout>()
	
	let isRecording = MutableLiveData<Bool>()
	let isRemotelyRecorded = MutableLiveData<Bool>()
	
	let participantAdminStatusChangedEvent = MutableLiveData<ConferenceParticipantData>()
	
	let maxParticipantsForMosaicLayout = ConfigManager.instance().lpConfigIntForKey(key: "max_conf_part_mosaic_layout",defaultValue: 6)
	
	let speakingParticipant = MutableLiveData<ConferenceParticipantDeviceData>()
	
	private var conferenceDelegate :  ConferenceDelegateStub?
	private var coreDelegate :  CoreDelegateStub?
	
	init ()  {
		conferenceDelegate = ConferenceDelegateStub(
			onParticipantAdded: { (conference: Conference, participant: Participant) in
				Log.i("[Conference] \(conference) Participant \(participant) added")
				self.updateParticipantsList(conference)
				let count = self.conferenceParticipantDevices.value!.count
				if (count > self.maxParticipantsForMosaicLayout) {
					Log.w("[Conference] \(conference) More than \(self.maxParticipantsForMosaicLayout) participants \(count), forcing active speaker layout")
					self.conferenceDisplayMode.value = .ActiveSpeaker
				}
			},
			onParticipantRemoved: {(conference: Conference, participant: Participant) in
				Log.i("[Conference] \(conference) \(participant) Participant removed")
				self.updateParticipantsList(conference)
			},
			onParticipantDeviceAdded: {(conference: Conference, participantDevice: ParticipantDevice) in
				Log.i("[Conference] \(conference) Participant device \(participantDevice) added")
				self.addParticipantDevice(device: participantDevice)
				
			},
			onParticipantDeviceRemoved: { (conference: Conference, participantDevice: ParticipantDevice) in
				Log.i("[Conference] \(conference) Participant device \(participantDevice) removed")
				self.removeParticipantDevice(device: participantDevice)
			},
			onParticipantAdminStatusChanged: { (conference: Conference, participant: Participant) in
				Log.i("[Conference] \(conference) Participant admin status changed")
				self.isMeAdmin.value = conference.me?.isAdmin
				self.updateParticipantsList(conference)
				if let participantData = self.conferenceParticipants.value?.filter ({$0.participant.address!.weakEqual(address2: participant.address!)}).first {
					self.participantAdminStatusChangedEvent.value = participantData
				} else {
					Log.w("[Conference] Failed to find participant [\(participant.address!.asStringUriOnly())] in conferenceParticipants list")
				}
			},
			onParticipantDeviceLeft: { (conference: Conference, device: ParticipantDevice) in
				if (conference.isMe(uri: device.address!)) {
					Log.i("[Conference] Left conference")
					self.isConferenceLocallyPaused.value = true
				}
			},
			onParticipantDeviceJoined: { (conference: Conference, device: ParticipantDevice) in
				if (conference.isMe(uri: device.address!)) {
					Log.i("[Conference] Joined conference")
					self.isConferenceLocallyPaused.value = false
				}
			},
			onStateChanged: { (conference: Conference, state: Conference.State) in
				Log.i("[Conference] State changed: \(state)")
				self.isVideoConference.value = conference.currentParams?.isVideoEnabled
				if (state == .Created) {
					self.configureConference(conference)
					self.conferenceCreationPending.value = false
				}
				if (state == .TerminationPending) {
					self.terminateConference(conference)
				}
			},
			onSubjectChanged: { (conference: Conference, subject: String) in
				self.subject.value = subject
			},
			onParticipantDeviceIsSpeakingChanged: { (conference: Conference, participantDevice: ParticipantDevice, isSpeaking:Bool) in
				Log.i("[Conference] Participant [\(participantDevice.address!.asStringUriOnly())] is speaking = \(isSpeaking)")
				if (isSpeaking) {
					if let device = self.conferenceParticipantDevices.value?.filter ({
						$0.participantDevice.address!.weakEqual(address2: participantDevice.address!)
					}).first {
						self.speakingParticipant.value = device
					} else {
						Log.w("[Conference] Participant device [\((participantDevice.address?.asStringUriOnly()).orNil)] is speaking but couldn't find it in devices list")
					}
					
				}
			}
		)
		
		coreDelegate = CoreDelegateStub(
			onConferenceStateChanged: { (core, conference, state) in
				Log.i("[Conference] \(conference) Conference state changed: \(state)")
				if (state == Conference.State.Instantiated) {
					self.conferenceCreationPending.value = true
					self.initConference(conference)
					
				}
			}
		)
		
		Core.get().addDelegate(delegate: coreDelegate!)
		conferenceParticipants.value = []
		conferenceParticipantDevices.value = []
		conferenceDisplayMode.value = .Grid
		subject.value = VoipTexts.conference_default_title
		
		if let conference = core.conference != nil ? core.conference : core.currentCall?.conference {
			Log.i("[Conference] Found an existing conference: \(conference) in state \(conference.state)")
			
			if (conference.state != .TerminationPending && conference.state != .Terminated) {
				initConference(conference)
				if (conference.state == Conference.State.Created) {
					configureConference(conference)
				} else {
					conferenceCreationPending.value = true
				}
			}
		}
	}
	
	func pauseConference() {
		Log.i("[Conference] Leaving conference with address \(conference) temporarily")
		let _ = conference.value?.leave()
	}
	
	func resumeConference() {
		Log.i("[Conference] entering conference with address \(conference)")
		let _ = conference.value?.enter()
	}
	
	func toggleRecording() {
		if (conference.value?.isRecording == true) {
			Log.i("[Conference] Stopping conference recording")
			let _ = conference.value?.stopRecording()
		} else {
			let writablePath = AppManager.recordingFilePathFromCall(address: (conference.value?.conferenceAddress!.asString())!)
			Log.i("[Conference] Starting recording in file $path")
			let _ = conference.value?.startRecording(path: writablePath)
		}
		isRecording.value = conference.value?.isRecording
	}
	
	func initConference(_ conference: Conference) {
		conferenceExists.value = true
		self.conference.value = conference
		conference.addDelegate(delegate: self.conferenceDelegate!)
		isRecording.value = conference.isRecording
		updateConferenceLayout(conference: conference)
		
		if let call = core.currentCall, CallManager.getAppData(call: call.getCobject!)?.isConference == true { // Apply waiting room preference
			if (ConferenceWaitingRoomViewModel.sharedModel.isSpeakerSelected.value == true) {
				ControlsViewModel.shared.forceSpeakerAudioRoute()
			} else {
				ControlsViewModel.shared.forceEarpieceAudioRoute()
				ControlsViewModel.shared.updateUI()
			}
			Core.get().micEnabled = ConferenceWaitingRoomViewModel.sharedModel.isMicrophoneMuted.value != true
			conference.layout = ConferenceWaitingRoomViewModel.sharedModel.joinLayout.value!
			updateConferenceLayout(conference: conference)
		}
		
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
		if (conf.isIn) {
			Log.i("[Conference] Conference was paused, resuming it")
			let _ = conf.enter()
		}
	}
	
	
	func changeLayout(layout: ConferenceLayout) {
		Log.i("[Conference] Trying to change conference layout to $layout")
		if let conference = conference.value {
			conference.layout = layout
			updateConferenceLayout(conference: conference)
		} else {
			Log.e("[Conference] Conference is null in ConferenceViewModel")
		}
	}
	
	private func updateConferenceLayout(conference: Conference) {
		conferenceDisplayMode.value = conference.layout == .Legacy ? .Grid : conference.layout
		let list = sortDevicesDataList(devices: conferenceParticipantDevices.value!)
		conferenceParticipantDevices.value = list
		Log.i("[Conference] Conference current layout is: \(conference.layout)")
	}
	
	
	
	func terminateConference(_ conference: Conference) {
		conferenceExists.value = false
		isVideoConference.value = false
		
		conference.removeDelegate(delegate: conferenceDelegate!)
		
		self.conferenceParticipants.value?.forEach{ $0.destroy()}
		self.conferenceParticipantDevices.value?.forEach{ $0.destroy()}
		conferenceParticipants.value = []
		conferenceParticipantDevices.value = []
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
	
	private func addParticipantDevice(device: ParticipantDevice) {
		var devices :[ConferenceParticipantDeviceData] = []
		conferenceParticipantDevices.value?.forEach{devices.append($0)}
		
		if let deviceAddress = device.address,  let _ = devices.filter({ $0.participantDevice.address!.weakEqual(address2: deviceAddress)}).first {
			Log.e("[Conference] Participant is already in devices list: \(device.name) (\((device.address?.asStringUriOnly()) ?? "nil")")
			return
		}
		
		Log.i("[Conference] New participant device found: \(device.name) (\((device.address?.asStringUriOnly()).orNil)")
		let deviceData = ConferenceParticipantDeviceData(participantDevice: device, isMe: false)
		devices.append(deviceData)
		
		let sortedDevices = sortDevicesDataList(devices: devices)
		
		if (speakingParticipant.value == nil) {
			speakingParticipant.value = deviceData
		}
		
		conferenceParticipantDevices.value = sortedDevices
	}
	
	private func removeParticipantDevice(device: ParticipantDevice) {
		let devices = conferenceParticipantDevices.value?.filter {
			$0.participantDevice.address?.asStringUriOnly() != device.address?.asStringUriOnly()
		}
		if (devices?.count == conferenceParticipantDevices.value?.count) {
			Log.e("[Conference] Failed to remove participant device: \(device.name) (\((device.address?.asStringUriOnly()).orNil)")
		} else {
			Log.i("[Conference] Participant device removed: \(device.name) (\((device.address?.asStringUriOnly()).orNil)")
		}
		
		conferenceParticipantDevices.value = devices
	}
	
	
	private func sortDevicesDataList(devices: [ConferenceParticipantDeviceData]) -> [ConferenceParticipantDeviceData] {
		if let meDeviceData = devices.filter({$0.isMe}).first {
			var devicesWithoutMe = devices.filter { !$0.isMe }
			if (conferenceDisplayMode.value == .ActiveSpeaker) {
				devicesWithoutMe.insert(meDeviceData, at: 0)
			} else {
				devicesWithoutMe.append(meDeviceData)
			}
			return devicesWithoutMe
		}
		return devices
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
	
	// Review below (dynamic add/remove)
	
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
					Log.w("[Conference Participants] Participant \((participant.address?.asStringUriOnly()).orNil) will be removed from conference")
					try conference.removeParticipant(participant: participant)
				}
			}
		} catch {
			Log.e("[Conference Participants] Error updating participant lists \(error)")
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
