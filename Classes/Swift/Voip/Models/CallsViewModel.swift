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


class CallsViewModel {
	
	let currentCallData = MutableLiveData<CallData?>(nil)
	let callsData = MutableLiveData<[CallData]>([])
	let inactiveCallsCount = MutableLiveData(0)
	let currentCallUnreadChatMessageCount = MutableLiveData(0)
	let chatAndCallsCount = MutableLiveData(0)
	let callConnectedEvent = MutableLiveData<Call>()
	let callUpdateEvent = MutableLiveData<Call>()
	let noMoreCallEvent = MutableLiveData(false)
	var core : Core { get { Core.get() } }

	static let shared = CallsViewModel()
	
	private var coreDelegate :  CoreDelegateStub?
	
	init ()  {
		coreDelegate = CoreDelegateStub(
			onCallStateChanged : { (core: Core, call: Call, state: Call.State, message:String) -> Void in
				Log.i("[Calls] Call state changed: \(call) : \(state)")
				let currentCall = core.currentCall
				if (currentCall != nil && self.currentCallData.value??.call.getCobject != currentCall?.getCobject) {
					self.updateCurrentCallData(currentCall: currentCall)
				} else if (currentCall == nil && core.callsNb > 0) {
					self.updateCurrentCallData(currentCall: currentCall)
				}
				if ([.End,.Released,.Error].contains(state)) {
					self.removeCallFromList(call: call)
				} else if ([.OutgoingInit].contains(state)) {
					self.addCallToList(call:call)
				} else if ([.IncomingReceived].contains(state)) {
					self.addCallToList(call:call)
				} else if (state == .UpdatedByRemote) {
					let remoteVideo = call.remoteParams?.videoEnabled == true
					let localVideo = call.currentParams?.videoEnabled == true
					let autoAccept = call.core?.videoActivationPolicy?.automaticallyAccept == true
					if (remoteVideo && !localVideo && !autoAccept) {
						if (core.videoCaptureEnabled || core.videoDisplayEnabled) {
							try?call.deferUpdate()
							self.callUpdateEvent.value =  call
						} else {
							call.answerVideoUpdateRequest(accept: false)
						}
					}
				}else if (state == Call.State.Connected) {
					self.callConnectedEvent.value = call
				} else if (state == Call.State.StreamsRunning) {
					self.callUpdateEvent.value = call
				}
				self.updateInactiveCallsCount()
				self.callsData.notifyValue()
			},
			
			onMessageReceived : { (core: Core, room: ChatRoom, message: ChatMessage) -> Void in
				self.updateUnreadChatCount()
			},
			onChatRoomRead : { (core: Core, room: ChatRoom) -> Void in
				self.updateUnreadChatCount()
			},
			onLastCallEnded: { (core: Core)  -> Void in
				self.currentCallData.value??.destroy()
				self.currentCallData.value = nil
				self.noMoreCallEvent.value = true
			}
		)
		
		Core.get().addDelegate(delegate: coreDelegate!)
		
		if let currentCall = core.currentCall {
			currentCallData.value??.destroy()
			currentCallData.value = CallData(call:currentCall)
		}
		
		chatAndCallsCount.value = 0
		inactiveCallsCount.readCurrentAndObserve { (_) in
			self.updateCallsAndChatCount()
		}
		currentCallUnreadChatMessageCount.readCurrentAndObserve { (_) in
			self.updateCallsAndChatCount()
		}
		
		initCallList()
		updateInactiveCallsCount()
		updateUnreadChatCount()
		
	}
	
	private func initCallList() {
		core.calls.forEach { addCallToList(call: $0) }
	}
	
	private func removeCallFromList(call: Call) {
		Log.i("[Calls] Removing call \(call) from calls list")
		if let removeCandidate = callsData.value?.filter{$0.call.getCobject == call.getCobject}.first {
			removeCandidate.destroy()
		}

		callsData.value = callsData.value?.filter(){$0.call.getCobject != call.getCobject}
		callsData.notifyValue()
	}
	
	private func addCallToList(call: Call) {
		Log.i("[Calls] Adding call \(call) to calls list")
		callsData.value?.append(CallData(call: call))
		callsData.notifyValue()
	}
	
	private func updateUnreadChatCount() {
		guard let unread = currentCallData.value??.chatRoom?.unreadMessagesCount else {
			currentCallUnreadChatMessageCount.value = 0
			return
		}
		currentCallUnreadChatMessageCount.value = unread
	}
	
	private func updateInactiveCallsCount() {
		inactiveCallsCount.value = core.callsNb - 1
	}
	
	private func updateCallsAndChatCount() {
		var value = 0
		if let calls = inactiveCallsCount.value {
			value += calls
		}
		if let chats = currentCallUnreadChatMessageCount.value {
			value += chats
		}
		chatAndCallsCount.value = value
	}
	
	func mergeCallsIntoLocalConference() {
		CallManager.instance().startLocalConference()
	}
	
	private func updateCurrentCallData(currentCall: Call?) {
		var callToUse = currentCall
		if (currentCall == nil) {
			Log.w("[Calls] Current call is now null")

			let firstCall = core.calls.first
			if (firstCall != nil && currentCallData.value??.call.getCobject != firstCall?.getCobject) {
				Log.i("[Calls] Using first call as \"current\" call")
				callToUse = firstCall
			}
		}

		guard let callToUse = callToUse else {
			Log.w("[Calls] No call found to be used as \"current\"")
			return
		}

		let firstToUse = callsData.value?.filter{$0.call.getCobject != callToUse.getCobject}.first
		if (firstToUse != nil) {
			currentCallData.value = firstToUse
		} else {
			Log.w("[Calls] Call not found in calls data list, shouldn't happen!")
			currentCallData.value = CallData(call: callToUse)
		}

		updateUnreadChatCount()
	}


}
