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
	let isOutgoing = MutableLiveData<Bool>()
	let isIncoming = MutableLiveData<Bool>()
	let callState = MutableLiveData<Call.State>()
	let iFrameReceived = MutableLiveData(false)
	let outgoingEarlyMedia =  MutableLiveData<Bool>()
	let enteredDTMF = MutableLiveData(" ")
	
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
		update()
		initChatRoom()
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
		let conference = call.conference
		isInRemoteConference.value = conference != nil
		if (conference != nil) {
			remoteConferenceSubject.value = conference?.subject != nil && (conference?.subject.count)! > 0 ?  conference!.subject : VoipTexts.conference_default_title
		}
		isOutgoing.value = isOutGoing()
		isIncoming.value = isInComing()
		if (call.mediaInProgress()) {
			DispatchQueue.main.asyncAfter(deadline: .now() + .seconds(1)) {
				self.update()
			}
		}
		outgoingEarlyMedia.value = callState.value == .OutgoingEarlyMedia
		isRecording.value = call.params?.isRecording
		callState.value = call.state		
	}
	
	private func initChatRoom() {
		
		return // V1 work around
		
		let localSipUri = Core.get().defaultAccount?.params?.identityAddress?.asStringUriOnly()
		let remoteSipUri = call.remoteAddress?.asStringUriOnly()
		let conference = call.conference

		
		guard
			let localSipUri = Core.get().defaultAccount?.params?.identityAddress?.asStringUriOnly(),
			let remoteSipUri = call.remoteAddress?.asStringUriOnly(),
			let localAddress = try?Factory.Instance.createAddress(addr: localSipUri),
			let remoteSipAddress = try?Factory.Instance.createAddress(addr: remoteSipUri)
		else {
			Log.e("[Call] Failed to get either local \(localSipUri.logable) or remote \(remoteSipUri.logable) SIP address!")
			return
		}
		do {
			if let conferenceInfo = Core.get().findConferenceInformationFromUri(uri: call.remoteAddress!), let params = try?Core.get().createDefaultChatRoomParams() {
				params.subject = conferenceInfo.subject
				params.backend = ChatRoomBackend.FlexisipChat
				params.groupEnabled = true
				chatRoom = Core.get().searchChatRoom(params: params, localAddr: localAddress, remoteAddr: nil, participants: conferenceInfo.participants)
			} else {
				chatRoom = Core.get().searchChatRoom(params: nil, localAddr: localAddress, remoteAddr: remoteSipAddress, participants: [])
				if (chatRoom == nil) {
					chatRoom = Core.get().searchChatRoom(params: nil, localAddr: localAddress, remoteAddr: nil, participants: [remoteSipAddress])
				}
			}
			
			if (chatRoom == nil) {
				let chatRoomParams = try Core.get().createDefaultChatRoomParams()
				if let conferenceInfo = Core.get().findConferenceInformationFromUri(uri: call.remoteAddress!) {
					Log.w("[Call] Failed to find existing chat room with same subject & participants, creating it")
					chatRoomParams.backend = ChatRoomBackend.FlexisipChat
					chatRoomParams.groupEnabled = true
					chatRoomParams.subject = conferenceInfo.subject
					chatRoom = try?Core.get().createChatRoom(params: chatRoomParams, localAddr: localAddress, participants: conferenceInfo.participants)
				} else {
					Log.w("[Call] Failed to find existing chat room with same participants, creating it")
					// TODO: configure chat room params
					chatRoom = try?Core.get().createChatRoom(params: chatRoomParams, localAddr: localAddress, participants: [remoteSipAddress])
				}
			}
			
			if (chatRoom == nil) {
				Log.e("[Call] Failed to create a chat room for local address \(localSipUri) and remote address \(remoteSipUri)!")
			}
		} catch {
			Log.e("[Call] Exception caught initiating a chat room for local address \(localSipUri) and remote address \(remoteSipUri) Error : \(error)!")
		}
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
			try?call.resume()
		} else {
			try?call.pause()
		}
		isPaused.value = isCallPaused()
	}
	
}
