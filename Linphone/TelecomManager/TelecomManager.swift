/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
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
// swiftlint:disable cyclomatic_complexity
// swiftlint:disable line_length
// swiftlint:disable type_body_length

import Foundation
import linphonesw
import UserNotifications
import os
import CallKit
import AVFoundation
import SwiftUI

class CallAppData: NSObject {
	var batteryWarningShown = false
	var videoRequested = false /*set when user has requested for video*/
	var isConference = false
	
}

class TelecomManager: ObservableObject {
	static let shared = TelecomManager()
	static var uuidReplacedCall: String?
	
	let providerDelegate: ProviderDelegate // to support callkit
	let callController: CXCallController // to support callkit
	
	@Published var callInProgress: Bool = false
	@Published var callDisplayed: Bool = true
	@Published var callStarted: Bool = false
	@Published var isNotVerifiedCounter: Int = 0
	@Published var outgoingCallStarted: Bool = false
	@Published var remoteConfVideo: Bool = false
	@Published var isRecordingByRemote: Bool = false
	@Published var isPausedByRemote: Bool = false
	@Published var refreshCallViewModel: Bool = false
	@Published var remainingCall: Bool = false
	@Published var callConnected: Bool = false
	@Published var meetingWaitingRoomDisplayed: Bool = false
	@Published var meetingWaitingRoomSelected: Address?
	@Published var meetingWaitingRoomName: String = ""
	
	var actionToFulFill: CXCallAction?
	var callkitAudioSessionActivated: Bool?
	var nextCallIsTransfer: Bool = false
	var speakerBeforePause: Bool = false
	var endCallkit: Bool = false
	var endCallKitReplacedCall: Bool = true
	
	var backgroundContextCall: Call?
	var backgroundContextCameraIsEnabled: Bool = false
	
	var referedFromCall: String?
	var referedToCall: String?
	var actionsToPerformOnceWhenCoreIsOn: [(() -> Void)] = []
	
	private init() {
		providerDelegate = ProviderDelegate()
		callController = CXCallController()
	}
	
	func addAllToLocalConference(core: Core) {
		// TODO
	}
	
	static func getAppData(sCall: Call) -> CallAppData? {
		if sCall.userData == nil {
			return nil
		}
		return Unmanaged<CallAppData>.fromOpaque(sCall.userData!).takeUnretainedValue()
	}
	static func setAppData(sCall: Call, appData: CallAppData?) {
		if sCall.userData != nil {
			Unmanaged<CallAppData>.fromOpaque(sCall.userData!).release()
		}
		if appData == nil {
			sCall.userData = nil
		} else {
			sCall.userData = UnsafeMutableRawPointer(Unmanaged.passRetained(appData!).toOpaque())
		}
	}
	
	func startCallCallKit(core: Core, addr: Address?, isSas: Bool, isVideo: Bool, isConference: Bool = false) throws {
		if addr == nil {
			Log.info("Can not start a call with null address!")
			return
		}
		
		if TelecomManager.callKitEnabled(core: core) {// && !nextCallIsTransfer != true {
			let uuid = UUID()
			let name = addr?.asStringUriOnly() ?? "Unknown"
			let handle = CXHandle(type: .generic, value: addr?.asStringUriOnly() ?? "")
			let startCallAction = CXStartCallAction(call: uuid, handle: handle)
			let transaction = CXTransaction(action: startCallAction)
			
			let callInfo = CallInfo.newOutgoingCallInfo(addr: addr!, isSas: isSas, displayName: name, isVideo: isVideo, isConference: isConference)
			providerDelegate.callInfos.updateValue(callInfo, forKey: uuid)
			providerDelegate.uuids.updateValue(uuid, forKey: "")
			
			setHeldOtherCalls(core: core, exceptCallid: "")
			requestTransaction(transaction, action: "startCall")
			DispatchQueue.main.async {
				withAnimation {
					self.callDisplayed = true
				}
			}
		} else {
			try doCall(core: core, addr: addr!, isSas: isSas, isVideo: isVideo, isConference: isConference)
		}
	}
	
	func setHeldOtherCalls(core: Core, exceptCallid: String) {
		for call in core.calls {
			if call.callLog?.callId != exceptCallid && call.state != .Paused && call.state != .Pausing && call.state != .PausedByRemote {
				setHeld(call: call, hold: true)
			} else if call.callLog?.callId == exceptCallid && (call.state == .Paused || call.state == .Pausing || call.state == .PausedByRemote) {
				setHeld(call: call, hold: true)
			}
		}
	}
	
	func setHeld(call: Call, hold: Bool) {
		
#if targetEnvironment(simulator)
		if hold {
			try?call.pause()
		} else {
			try?call.resume()
		}
#else
		let callid = call.callLog?.callId ?? ""
		let uuid = providerDelegate.uuids["\(callid)"]
		if uuid == nil {
			Log.error("Can not find correspondant call to set held.")
			return
		}
		let setHeldAction = CXSetHeldCallAction(call: uuid!, onHold: hold)
		let transaction = CXTransaction(action: setHeldAction)
		requestTransaction(transaction, action: "setHeld")
#endif
	}
	
	func startCall(core: Core, addr: String, isSas: Bool = false, isVideo: Bool, isConference: Bool = false) {
		do {
			let address = try Factory.Instance.createAddress(addr: addr)
			try startCallCallKit(core: core, addr: address, isSas: isSas, isVideo: isVideo, isConference: isConference)
		} catch {
			Log.error("[TelecomManager] unable to create address for a new outgoing call : \(addr) \(error) ")
		}
	}
	
	func doCallOrJoinConf(address: Address, isVideo: Bool = false, isConference: Bool = false) {
		if address.asStringUriOnly().hasPrefix("sip:conference-focus@sip.linphone.org") {
			do {
				let meetingAddress = try Factory.Instance.createAddress(addr: address.asStringUriOnly())
				
				DispatchQueue.main.async {
					withAnimation {
						self.meetingWaitingRoomDisplayed = true
						self.meetingWaitingRoomSelected = meetingAddress
					}
				}
			} catch {}
		} else {
			doCallWithCore(
				addr: address, isVideo: isVideo, isConference: isConference
			)
		}
	}
	
	func doCallWithCore(addr: Address, isVideo: Bool, isConference: Bool) {
		CoreContext.shared.doOnCoreQueue { core in
			do {
				try self.startCallCallKit(core: core, addr: addr, isSas: false, isVideo: isVideo, isConference: isConference)
			} catch {
				Log.error("[TelecomManager] unable to create address for a new outgoing call : \(addr) \(error) ")
			}
		}
	}
	
	private func makeRecordFilePath() -> String {
		var filePath = "recording_"
		let now = Date()
		let dateFormat = DateFormatter()
		dateFormat.dateFormat = "E-d-MMM-yyyy-HH-mm-ss"
		let date = dateFormat.string(from: now)
		filePath = filePath.appending("\(date).mkv")
		
		let paths = NSSearchPathForDirectoriesInDomains(.cachesDirectory, .userDomainMask, true)
		let writablePath = paths[0]
		return writablePath.appending("/\(filePath)")
	}
	
	func doCall(core: Core, addr: Address, isSas: Bool, isVideo: Bool, isConference: Bool = false) throws {
		// let displayName = FastAddressBook.displayName(for: addr.getCobject)
		
		let lcallParams = try core.createCallParams(call: nil)
		/*
		 if ConfigManager.instance().lpConfigBoolForKey(key: "edge_opt_preference") && AppManager.network() == .network_2g {
		 Log.directLog(BCTBX_LOG_MESSAGE, text: "Enabling low bandwidth mode")
		 lcallParams.lowBandwidthEnabled = true
		 }
		 
		 if (displayName != nil) {
		 try addr.setDisplayname(newValue: displayName!)
		 }
		 
		 if(ConfigManager.instance().lpConfigBoolForKey(key: "override_domain_with_default_one")) {
		 try addr.setDomain(newValue: ConfigManager.instance().lpConfigStringForKey(key: "domain", section: "assistant"))
		 }
		 */
		
		if nextCallIsTransfer {
			let call = core.currentCall
			try call?.transferTo(referTo: addr)
			nextCallIsTransfer = false
		} else {
			// We set the record file name here because we can't do it after the call is started.
			// let writablePath = AppManager.recordingFilePathFromCall(address: addr.username! )
			// Log.directLog(BCTBX_LOG_DEBUG, text: "record file path: \(writablePath)")
			// lcallParams.recordFile = writablePath
			
			lcallParams.recordFile = makeRecordFilePath()
			
			if isSas {
				lcallParams.mediaEncryption = .ZRTP
			}
			
			if isConference {
				lcallParams.videoEnabled = true
				lcallParams.videoDirection = isVideo && core.videoPreviewEnabled ? MediaDirection.SendRecv : MediaDirection.RecvOnly
				/*		if (ConferenceWaitingRoomViewModel.sharedModel.joinLayout.value! != .AudioOnly) {
				 lcallParams.videoEnabled = true
				 lcallParams.videoDirection = ConferenceWaitingRoomViewModel.sharedModel.isVideoEnabled.value == true ? .SendRecv : .RecvOnly
				 lcallParams.conferenceVideoLayout = ConferenceWaitingRoomViewModel.sharedModel.joinLayout.value! == .Grid ? .Grid : .ActiveSpeaker
				 } else {
				 lcallParams.videoEnabled = false
				 }*/
			} else {
				lcallParams.videoEnabled = true
				lcallParams.videoDirection = isVideo ? MediaDirection.SendRecv : MediaDirection.Inactive
			}
			
			if let call = core.inviteAddressWithParams(addr: addr, params: lcallParams) {
				// The LinphoneCallAppData object should be set on call creation with callback
				// - (void)onCall:StateChanged:withMessage:. If not, we are in big trouble and expect it to crash
				// We are NOT responsible for creating the AppData.
				if let data = TelecomManager.getAppData(sCall: call) {
					data.isConference = isConference
					data.videoRequested = lcallParams.videoEnabled
					TelecomManager.setAppData(sCall: call, appData: data)
				} else {
					Log.error("New call instanciated but app data was not set. Expect it to crash.")
					/* will be used later to notify user if video was not activated because of the linphone core*/
				}
			}
			
			DispatchQueue.main.async {
				self.outgoingCallStarted = true
				self.callStarted = true
				self.isNotVerifiedCounter = 0
				if self.callInProgress == false {
					withAnimation {
						self.callInProgress = true
						self.callDisplayed = true
					}
				}
			}
		}
	}
	
	func acceptCall(core: Core, call: Call, hasVideo: Bool) {
		do {
			let callParams = try core.createCallParams(call: call)
			callParams.recordFile = makeRecordFilePath()
			callParams.videoEnabled = hasVideo
			/*if (ConfigManager.instance().lpConfigBoolForKey(key: "edge_opt_preference")) {
			 let low_bandwidth = (AppManager.network() == .network_2g)
			 if (low_bandwidth) {
			 Log.directLog(BCTBX_LOG_MESSAGE, text: "Low bandwidth mode")
			 }
			 callParams.lowBandwidthEnabled = low_bandwidth
			 }*/
			
			// We set the record file name here because we can't do it after the call is started.
			// let address = call.callLog?.fromAddress
			// let writablePath = AppManager.recordingFilePathFromCall(address: address?.username ?? "")
			// Log.directLog(BCTBX_LOG_MESSAGE, text: "Record file path: \(String(describing: writablePath))")
			// callParams.recordFile = writablePath
			
			/*
			 if let chatView : ChatConversationView = PhoneMainView.instance().VIEW(ChatConversationView.compositeViewDescription()), chatView.isVoiceRecording {
			 Log.directLog(BCTBX_LOG_MESSAGE, text: "Voice recording in progress, stopping it befoce accepting the call.")
			 chatView.stopVoiceRecording()
			 }*/
			
			if call.callLog?.wasConference() == true {
				// Prevent incoming group call to start in audio only layout
				// Do the same as the conference waiting room
				callParams.videoEnabled = true
				callParams.videoDirection = core.videoActivationPolicy?.automaticallyInitiate == true ? .SendRecv : .RecvOnly
				Log.info("[Context] Enabling video on call params to prevent audio-only layout when answering")
			}
			
			try call.acceptWithParams(params: callParams)
			
			DispatchQueue.main.async {
				self.callStarted = true
				self.isNotVerifiedCounter = 0
				if self.callDisplayed {
					self.callDisplayed = core.calls.count <= 1
				}
			}
			
			if core.calls.count > 1 {
				DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
					self.callDisplayed = true
				}
			}
		} catch {
			Log.error("accept call failed \(error)")
		}
	}
	
	func terminateCall(call: Call) {
		do {
			try call.terminate()
			Log.info("Call terminated")
		} catch {
			Log.error("Failed to terminate call failed because \(error)")
		}
	}
	
	func displayIncomingCall(call: Call?, handle: String, hasVideo: Bool, callId: String, displayName: String) {
		let uuid = UUID()
		let callInfo = CallInfo.newIncomingCallInfo(callId: callId)
		
		providerDelegate.callInfos.updateValue(callInfo, forKey: uuid)
		providerDelegate.uuids.updateValue(uuid, forKey: callId)
		providerDelegate.reportIncomingCall(call: call, uuid: uuid, handle: handle, hasVideo: hasVideo, displayName: displayName)
	}
	
	func incomingDisplayName(call: Call, completion: @escaping (String) -> Void) {
		CoreContext.shared.doOnCoreQueue { _ in
			ContactsManager.shared.getFriendWithAddressInCoreQueue(address: call.remoteAddress!) { friendResult in
				if call.remoteAddress != nil {
					if friendResult != nil && friendResult!.address != nil && friendResult!.address!.displayName != nil {
						completion(friendResult!.address!.displayName!)
					} else {
						if call.remoteAddress!.displayName != nil {
							completion(call.remoteAddress!.displayName!)
						} else if call.remoteAddress!.username != nil {
							completion(call.remoteAddress!.username!)
						}
					}
					
				} else {
					completion("IncomingDisplayName")
				}
			}
		}
	}
	
	static func callKitEnabled(core: Core) -> Bool {
#if !targetEnvironment(simulator)
		return core.callkitEnabled
#else
		return false
#endif
	}
	
	func requestTransaction(_ transaction: CXTransaction, action: String) {
		callController.request(transaction) { error in
			if let error = error {
				Log.error("CallKit: Requested transaction \(action) failed because: \(error)")
			} else {
				Log.info("CallKit: Requested transaction \(action) successfully")
			}
		}
	}
	
	func onAccountRegistrationStateChanged(core: Core, account: Account, state: RegistrationState, message: String) {
		if core.accountList.count == 1 && (state == .Failed || state == .Cleared) {
			// terminate callkit immediately when registration failed or cleared, supporting single account configuration
			for call in providerDelegate.uuids {
				let callId = providerDelegate.callInfos[call.value]?.callId
				if callId != nil {
					let call = core.getCallByCallid(callId: callId!)
					if call != nil && call?.state != .PushIncomingReceived {
						// sometimes (for example) due to network, registration failed, in this case, keep the call
						continue
					}
				}
				providerDelegate.endCall(uuid: call.value)
			}
			endCallkit = true
		} else {
			endCallkit = false
		}
	}
	
	func updateRemoteConfVideo(remConfVideoEnabled: Bool) {
		if self.remoteConfVideo != remConfVideoEnabled {
			DispatchQueue.main.async {
				self.remoteConfVideo.toggle()
				Log.info("[Call] Remote video is \(remConfVideoEnabled ? "activated" : "not activated")")
			}
		}
	}
	
	func onCallStateChanged(core: Core, call: Call, state cstate: Call.State, message: String) {
		let callLog = call.callLog
		let callId = callLog?.callId ?? ""
		if cstate == .PushIncomingReceived {
			Log.info("PushIncomingReceived in core delegate, display callkit call")
			TelecomManager.shared.displayIncomingCall(call: call, handle: "Calling", hasVideo: false, callId: callId, displayName: "Calling")
		} else {
			// let oldRemoteConfVideo = self.remoteConfVideo
			
			if call.conference != nil {
				if call.conference!.activeSpeakerParticipantDevice != nil {
					let direction = call.conference?.activeSpeakerParticipantDevice!.getStreamCapability(streamType: StreamType.Video)
					updateRemoteConfVideo(remConfVideoEnabled: direction == .SendRecv || direction == .SendOnly)
				} else if call.conference!.participantList.first != nil && call.conference!.participantDeviceList.first != nil
							&& call.conference!.participantList.first?.address != nil
							&& call.conference!.participantList.first!.address!.clone()!.equal(address2: (call.conference!.me?.address)!) {
					let direction = call.conference!.participantDeviceList.first!.getStreamCapability(streamType: StreamType.Video)
					updateRemoteConfVideo(remConfVideoEnabled: direction == .SendRecv || direction == .SendOnly)
				} else if call.conference!.participantList.last != nil && call.conference!.participantDeviceList.last != nil
							&& call.conference!.participantList.last?.address != nil {
					let direction = call.conference!.participantDeviceList.last!.getStreamCapability(streamType: StreamType.Video)
					updateRemoteConfVideo(remConfVideoEnabled: direction == .SendRecv || direction == .SendOnly)
				} else {
					updateRemoteConfVideo(remConfVideoEnabled: false)
				}
			} else {
				var remConfVideoEnabled = false
				if call.currentParams != nil {
					remConfVideoEnabled = call.currentParams!.videoEnabled && call.currentParams!.videoDirection == .SendRecv || call.currentParams!.videoDirection == .RecvOnly
				}
				updateRemoteConfVideo(remConfVideoEnabled: remConfVideoEnabled)
			}
						
			if self.remoteConfVideo {
				Log.info("[Call] Remote video is activated")
			}
			
			let isRecordingByRemoteTmp = call.remoteParams?.isRecording ?? false
			
			if isRecordingByRemoteTmp && ToastViewModel.shared.toastMessage.isEmpty {
				
				var displayName = ""
				let friend = ContactsManager.shared.getFriendWithAddress(address: call.remoteAddress!)
				if friend != nil && friend!.address != nil && friend!.address!.displayName != nil {
					displayName = friend!.address!.displayName!
				} else {
					if call.remoteAddress!.displayName != nil {
						displayName = call.remoteAddress!.displayName!
					} else if call.remoteAddress!.username != nil {
						displayName = call.remoteAddress!.username!
					}
				}
				
				DispatchQueue.main.async {
					self.isRecordingByRemote = isRecordingByRemoteTmp
					ToastViewModel.shared.toastMessage = "\(displayName) is recording"
					ToastViewModel.shared.displayToast = true
				}
				
				Log.info("[Call] Call is recording by \(call.remoteAddress!.asStringUriOnly())")
			}
			
			if !isRecordingByRemoteTmp && ToastViewModel.shared.toastMessage.contains("is recording") {
				DispatchQueue.main.async {
					self.isRecordingByRemote = isRecordingByRemoteTmp
					ToastViewModel.shared.toastMessage = ""
					ToastViewModel.shared.displayToast = false
				}
				Log.info("[Call] Recording is stopped by \(call.remoteAddress!.asStringUriOnly())")
			}
			
			if cstate == Call.State.PausedByRemote {
				DispatchQueue.main.async {
					self.isPausedByRemote = true
				}
			} else {
				DispatchQueue.main.async {
					self.isPausedByRemote = false
				}
			}
			
			if cstate == Call.State.Connected {
				DispatchQueue.main.async {
					self.callConnected = true
					self.meetingWaitingRoomSelected = nil
					self.meetingWaitingRoomDisplayed = false
				}
			}
			
			if call.userData == nil {
				let appData = CallAppData()
				TelecomManager.setAppData(sCall: call, appData: appData)
			}
			
			switch cstate {
			case .IncomingReceived:
				let addr = call.remoteAddress
				incomingDisplayName(call: call) { displayNameResult in
					let displayName = displayNameResult
	#if targetEnvironment(simulator)
					DispatchQueue.main.async {
						self.outgoingCallStarted = false
						self.callStarted = false
						if self.callInProgress == false {
							withAnimation {
								self.callInProgress = true
								self.callDisplayed = true
							}
						}
					}
	#endif
					if TelecomManager.callKitEnabled(core: core) {
						let uuid = self.providerDelegate.uuids["\(callId)"]
						TelecomManager.uuidReplacedCall = callId
						
						if uuid != nil {
							// Tha app is now registered, updated the call already existed.
							self.providerDelegate.updateCall(uuid: uuid!, handle: addr!.asStringUriOnly(), hasVideo: self.remoteConfVideo, displayName: displayName)
						} else {
							let videoEnabled = call.remoteParams?.videoEnabled ?? false
							let isConference = call.callLog?.wasConference() ?? false
							let videoDir = call.remoteParams?.videoDirection != MediaDirection.Inactive
							self.displayIncomingCall(call: call, handle: addr!.asStringUriOnly(), hasVideo: videoEnabled && videoDir && !isConference, callId: callId, displayName: displayName)
						}
					}
				}
			case .StreamsRunning:
				if TelecomManager.callKitEnabled(core: core) {
					
					DispatchQueue.main.async {
						self.outgoingCallStarted = false
					}
					
					let uuid = providerDelegate.uuids["\(callId)"]
					if uuid != nil {
						let callInfo = providerDelegate.callInfos[uuid!]
						if callInfo != nil && callInfo!.isOutgoing && !callInfo!.connected {
							Log.info("CallKit: outgoing call connected with uuid \(uuid!) and callId \(callId)")
							providerDelegate.reportOutgoingCallConnected(uuid: uuid!)
							callInfo!.connected = true
							providerDelegate.callInfos.updateValue(callInfo!, forKey: uuid!)
						}
					}
				}
				
				actionToFulFill?.fulfill()
				actionToFulFill = nil
			case .Paused:
				actionToFulFill?.fulfill()
				actionToFulFill = nil
			case .OutgoingInit,
					.OutgoingProgress,
					.OutgoingRinging,
					.OutgoingEarlyMedia:
				
				if TelecomManager.callKitEnabled(core: core) {
					let uuid = providerDelegate.uuids[""]
					if  uuid != nil {
						let callInfo = providerDelegate.callInfos[uuid!]
						callInfo!.callId = callId
						providerDelegate.callInfos.updateValue(callInfo!, forKey: uuid!)
						providerDelegate.uuids.removeValue(forKey: "")
						providerDelegate.uuids.updateValue(uuid!, forKey: callId)
						
						Log.info("CallKit: outgoing call started connecting with uuid \(uuid!) and callId \(callId)")
						providerDelegate.reportOutgoingCallStartedConnecting(uuid: uuid!)
					} else {
						referedToCall = callId
					}
				}
			case .End,
					.Error:
				
				UIDevice.current.isProximityMonitoringEnabled = false
				if core.callsNb == 0 {
					core.outputAudioDevice = core.defaultOutputAudioDevice
				}
				
				// if core.callsNb == 0 {
				self.incomingDisplayName(call: call) { displayNameResult in
					var displayName = "Unknown"
					if call.dir == .Incoming {
						displayName = displayNameResult
					} else { // if let addr = call.remoteAddress, let contactName = FastAddressBook.displayName(for: addr.getCobject) {
						displayName = "TODOContactName"
					}
					DispatchQueue.main.async {
						if core.callsNb == 0 {
							do {
								try core.setVideodevice(newValue: "AV Capture: com.apple.avfoundation.avcapturedevice.built-in_video:1")
							} catch _ {
								
							}
							withAnimation {
								self.outgoingCallStarted = false
								self.callInProgress = false
								self.callDisplayed = false
								self.callStarted = false
								self.callConnected = false
							}
						} else {
							if core.calls.last != nil {
								self.setHeld(call: core.calls.last!, hold: false)
								
								DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
									self.remainingCall = true
									DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
										self.remainingCall = false
									}
								}
							}
						}
						
						if UIApplication.shared.applicationState != .active && (callLog == nil || callLog?.status == .Missed || callLog?.status == .Aborted || callLog?.status == .EarlyAborted) {
							// Configure the notification's payload.
							let content = UNMutableNotificationContent()
							content.title = NSString.localizedUserNotificationString(forKey: NSLocalizedString("Missed call", comment: ""), arguments: nil)
							content.body = NSString.localizedUserNotificationString(forKey: displayName, arguments: nil)
							
							// Deliver the notification.
							let request = UNNotificationRequest(identifier: "call_request", content: content, trigger: nil) // Schedule the notification.
							let center = UNUserNotificationCenter.current()
							center.add(request) { (error: Error?) in
								if error != nil {
									Log.info("Error while adding notification request : \(error!.localizedDescription)")
								}
							}
						}
					}
				}
				// }
				
				if TelecomManager.callKitEnabled(core: core) {
					var uuid = providerDelegate.uuids["\(callId)"]
					if callId == referedToCall {
						// refered call ended before connecting
						Log.info("Callkit: end refered to call: \(String(describing: referedToCall))")
						referedFromCall = nil
						referedToCall = nil
					}
					if uuid == nil {
						// the call not yet connected
						uuid = providerDelegate.uuids[""]
					}
					if uuid != nil {
						if callId == referedFromCall {
							Log.info("Callkit: end refered from call: \(String(describing: referedFromCall))")
							referedFromCall = nil
							let callInfo = providerDelegate.callInfos[uuid!]
							callInfo!.callId = referedToCall ?? ""
							providerDelegate.callInfos.updateValue(callInfo!, forKey: uuid!)
							providerDelegate.uuids.removeValue(forKey: callId)
							providerDelegate.uuids.updateValue(uuid!, forKey: callInfo!.callId)
							referedToCall = nil
							break
						}
						if endCallKitReplacedCall {
							let transaction = CXTransaction(action: CXEndCallAction(call: uuid!))
							requestTransaction(transaction, action: "endCall")
						} else {
							endCallKitReplacedCall = true
						}
						
					}
				}
			case .Released:
				TelecomManager.setAppData(sCall: call, appData: nil)
			case .Referred:
				referedFromCall = call.callLog?.callId
			default:
				break
			}
		}
		// post Notification kLinphoneCallUpdate
		NotificationCenter.default.post(name: Notification.Name("LinphoneCallUpdate"), object: self, userInfo: [
			AnyHashable("call"): NSValue.init(pointer: UnsafeRawPointer(call.getCobject)),
			AnyHashable("state"): NSNumber(value: cstate.rawValue),
			AnyHashable("message"): message
		])
	}
}

// swiftlint:enable type_body_length
// swiftlint:enable cyclomatic_complexity
// swiftlint:enable line_length
