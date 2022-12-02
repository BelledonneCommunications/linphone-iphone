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

import linphonesw
import Foundation

class CallData  {
	
	var call : Call
	let address :String?
	
	let isPaused = MutableLiveData<Bool>()
	let isRemotelyPaused = MutableLiveData<Bool>()
	let canBePaused = MutableLiveData<Bool>()
	let isRecording = MutableLiveData<Bool>()
	let isRemotelyRecorded = MutableLiveData<Bool>()
	let isInRemoteConference = MutableLiveData<Bool>()
	let remoteConferenceSubject = MutableLiveData<String>()
	let isConferenceCall = MediatorLiveData<Bool>()
	let conferenceParticipants = MutableLiveData<[Address]>()
	let conferenceParticipantsCountLabel = MutableLiveData<String>()
	let callKitConferenceLabel = MutableLiveData<String>()

	let isOutgoing = MutableLiveData<Bool>()
	let isIncoming = MutableLiveData<Bool>()
	let callState = MutableLiveData<Call.State>()
	let iFrameReceived = MutableLiveData(false)
	let outgoingEarlyMedia =  MutableLiveData<Bool>()
	let enteredDTMF = MutableLiveData("")
	
	var chatRoom: ChatRoom? = nil
	
	private var callDelegate :  CallDelegateStub?
	
	init (call:Call)  {
		self.call = call
		address = call.remoteAddress?.asStringUriOnly()
		callDelegate = CallDelegateStub(
			onStateChanged : { (call: linphonesw.Call, state: linphonesw.Call.State, message: String) -> Void in
				self.update()
			},
			onNextVideoFrameDecoded : { (call: linphonesw.Call) -> Void in
				self.iFrameReceived.value = true
			},
			onRemoteRecording: {  (call: linphonesw.Call, recording:Bool) -> Void in
				self.isRemotelyRecorded.value = recording
			}
		)
		call.addDelegate(delegate: callDelegate!)
		
		remoteConferenceSubject.readCurrentAndObserve { _ in
			self.isConferenceCall.value = self.remoteConferenceSubject.value?.count ?? 0 > 0  || self.conferenceParticipants.value?.count ?? 0 > 0
		}
		conferenceParticipants.readCurrentAndObserve { _ in
			self.isConferenceCall.value = self.remoteConferenceSubject.value?.count ?? 0 > 0  || self.conferenceParticipants.value?.count ?? 0 > 0
		}
		
		update()
	}
	
	
	private func isCallPaused() -> Bool {
		return [Call.State.Paused, Call.State.Pausing].contains(call.state)
	}
	
	private func isCallRemotelyPaused() -> Bool {
		return [Call.State.PausedByRemote].contains(call.state)
	}
	
	private func isOutGoing() -> Bool {
		return [Call.State.OutgoingInit, Call.State.OutgoingEarlyMedia, Call.State.OutgoingProgress, Call.State.OutgoingRinging].contains(call.state)
	}
	
	private func isInComing() -> Bool {
		return [Call.State.IncomingReceived, Call.State.IncomingEarlyMedia].contains(call.state)
	}
	
	private func canCallBePaused() -> Bool {
		return !call.mediaInProgress() && [Call.State.StreamsRunning, Call.State.PausedByRemote].contains(call.state)
	}
	
	private func update() {
		isPaused.value = isCallPaused()
		isRemotelyPaused.value = isCallRemotelyPaused()
		canBePaused.value = canCallBePaused()
		
		updateConferenceInfo()

		let outgoing = isOutGoing()
		if (outgoing != isOutgoing.value) {
			isOutgoing.value = outgoing
		}
		let incoming = isInComing()
		if (incoming != isIncoming.value) {
			isIncoming.value = incoming
		}
		
		if (call.mediaInProgress()) {
			DispatchQueue.main.asyncAfter(deadline: .now() + .seconds(1)) {
				self.update()
			}
		}
		outgoingEarlyMedia.value = callState.value == .OutgoingEarlyMedia
		isRecording.value = call.params?.isRecording
		callState.value = call.state		
	}
	
	private func updateConferenceInfo() {
		let conference = call.conference
		isInRemoteConference.value = conference != nil
		if (conference != nil) {
			Log.d("[Call] Found conference attached to call")
			remoteConferenceSubject.value = ConferenceViewModel.getConferenceSubject(conference: conference!)
			Log.d("[Call] Found conference related to this call with subject \(remoteConferenceSubject.value)")
			var participantsList:[Address] = []
			conference?.participantList.forEach {
				$0.address.map {participantsList.append($0)}
			}
			conferenceParticipants.value = participantsList
			conferenceParticipantsCountLabel.value = VoipTexts.conference_participants_title.replacingOccurrences(of:"%d",with:String(participantsList.count))
		} else {
			if let conferenceAddress = getConferenceAddress(call: call), let conferenceInfo =  Core.get().findConferenceInformationFromUri(uri:conferenceAddress) {
				Log.d("[Call] Found matching conference info with subject: \(conferenceInfo.subject)")
				remoteConferenceSubject.value = conferenceInfo.subject
				var participantsList:[Address] = []
				conferenceInfo.participants.forEach {
					participantsList.append($0)
				}
				// Add organizer if not in participants list
				if let organizer = conferenceInfo.organizer {
					if (participantsList.filter { $0.weakEqual(address2: organizer) }.first == nil) {
						participantsList.insert(organizer, at:0)
					}
					conferenceParticipants.value = participantsList
					conferenceParticipantsCountLabel.value = VoipTexts.conference_participants_title.replacingOccurrences(of:"%d",with:String(participantsList.count))
				}
			}
		}
	}
	
	func getConferenceAddress(call: Call) -> Address? {
		let remoteContact = call.remoteContact
		return call.dir == .Incoming ? (remoteContact != nil ? Core.get().interpretUrl(url: remoteContact, applyInternationalPrefix: CallManager.instance().applyInternationalPrefix()) : nil) : call.remoteAddress
	}
	
	func sendDTMF(dtmf:String) {
		enteredDTMF.value = enteredDTMF.value! + dtmf
		Core.get().playDtmf(dtmf: dtmf.utf8CString[0], durationMs: 1000)
		try?call.sendDtmf(dtmf:  dtmf.utf8CString[0])
	}
	
	func destroy()  {
		call.removeDelegate(delegate: callDelegate!)
		isPaused.clearObservers()
		isRemotelyPaused.clearObservers()
		canBePaused.clearObservers()
		isRecording.clearObservers()
		isRemotelyRecorded.clearObservers()
		isInRemoteConference.clearObservers()
		remoteConferenceSubject.clearObservers()
		isOutgoing.clearObservers()
		isIncoming.clearObservers()
		callState.clearObservers()
		iFrameReceived.clearObservers()
		outgoingEarlyMedia.clearObservers()
		enteredDTMF.clearObservers()
	}
	
	func toggleRecord() {
		if (call.params?.isRecording == true) {
			call.stopRecording()
		} else {
			call.startRecording()
		}
		isRecording.value = call.params?.isRecording
	}
	
	func togglePause() {
		if (isCallPaused()) {
			CallsViewModel.shared.callsData.value?.forEach {
				if ($0.canCallBePaused()) {
					CallManager.instance().setHeld(call: $0.call, hold: true)
				}
			}
			CallManager.instance().setHeld(call: call, hold: false)
		} else {
			CallManager.instance().setHeld(call: call, hold: true)
		}
		isPaused.value = isCallPaused()
	}
	
	func isOngoingSingleCall() -> Bool {
		return !isOutGoing() && !isInComing() && call.conference == nil && call.callLog?.wasConference() != true
	}
	
	func isOngoingConference() -> Bool {
		return !isOutGoing() && !isInComing() && (call.conference != nil || call.callLog?.wasConference() == true)
	}


}
