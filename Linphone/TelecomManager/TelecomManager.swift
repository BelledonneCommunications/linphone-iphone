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
    @Published var callStarted: Bool = false
	
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
			let name = "outgoingTODO" // FastAddressBook.displayName(for: addr) ?? "unknow"
			let handle = CXHandle(type: .generic, value: addr?.asStringUriOnly() ?? "")
			let startCallAction = CXStartCallAction(call: uuid, handle: handle)
			let transaction = CXTransaction(action: startCallAction)
			
			let callInfo = CallInfo.newOutgoingCallInfo(addr: addr!, isSas: isSas, displayName: name, isVideo: isVideo, isConference: isConference)
			providerDelegate.callInfos.updateValue(callInfo, forKey: uuid)
			providerDelegate.uuids.updateValue(uuid, forKey: "")
			
			// setHeldOtherCalls(core: core, exceptCallid: "")
			requestTransaction(transaction, action: "startCall")
		} else {
			try doCall(core: core, addr: addr!, isSas: isSas, isVideo: isVideo, isConference: isConference)
		}
	}
	
	func startCall(core: Core, addr: String, isSas: Bool = false, isVideo: Bool, isConference: Bool = false) {
		do {
			let address = try Factory.Instance.createAddress(addr: addr)
			try startCallCallKit(core: core, addr: address, isSas: isSas, isVideo: isVideo, isConference: isConference)
		} catch {
			Log.error("[TelecomManager] unable to create address for a new outgoing call : \(addr) \(error) ")
		}
	}
	
    func doCallWithCore(addr: Address) {
        CoreContext.shared.doOnCoreQueue { core in
			do {
				try self.startCallCallKit(core: core, addr: addr, isSas: false, isVideo: false, isConference: false)
			} catch {
				Log.error("[TelecomManager] unable to create address for a new outgoing call : \(addr) \(error) ")
			}
        }
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
			if isSas {
				lcallParams.mediaEncryption = .ZRTP
			}
			if isConference {
		/*		if (ConferenceWaitingRoomViewModel.sharedModel.joinLayout.value! != .AudioOnly) {
					lcallParams.videoEnabled = true
					lcallParams.videoDirection = ConferenceWaitingRoomViewModel.sharedModel.isVideoEnabled.value == true ? .SendRecv : .RecvOnly
					lcallParams.conferenceVideoLayout = ConferenceWaitingRoomViewModel.sharedModel.joinLayout.value! == .Grid ? .Grid : .ActiveSpeaker
				} else {
					lcallParams.videoEnabled = false
				}*/
			} else {
				lcallParams.videoEnabled = isVideo
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
				self.callStarted = true
				withAnimation {
					self.callInProgress = true
				}
			}
		}
	}
	
	func acceptCall(core: Core, call: Call, hasVideo: Bool) {
		do {
			let callParams = try core.createCallParams(call: call)
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
				callParams.videoDirection = core.videoActivationPolicy?.automaticallyInitiate == true ?  .SendRecv : .RecvOnly
				Log.info("[Context] Enabling video on call params to prevent audio-only layout when answering")
			}
			
			try call.acceptWithParams(params: callParams)
			
            DispatchQueue.main.async {
                self.callStarted = true
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
	
	func incomingDisplayName(call: Call) -> String {
		if call.remoteAddress != nil {
			let friend = ContactsManager.shared.getFriendWithAddress(address: call.remoteAddress!)
			if friend != nil && friend!.address != nil && friend!.address!.displayName != nil {
				return friend!.address!.displayName!
			} else {
				if call.remoteAddress!.displayName != nil {
					return call.remoteAddress!.displayName!
				} else if call.remoteAddress!.username != nil {
					return call.remoteAddress!.username!
				}
			}
		}
		return "IncomingDisplayName"
	}
	
	static func callKitEnabled(core: Core) -> Bool {
#if !targetEnvironment(simulator)
		return core.callkitEnabled
#endif
		return false
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
	
	func onCallStateChanged(core: Core, call: Call, state cstate: Call.State, message: String) {
		let callLog = call.callLog
		let callId = callLog?.callId ?? ""
		if cstate == .PushIncomingReceived {
			displayIncomingCall(call: call, handle: "Calling", hasVideo: false, callId: callId, displayName: "Calling")
		} else {
			let video = (core.videoActivationPolicy?.automaticallyAccept ?? false) && (call.remoteParams?.videoEnabled ?? false)
			
			if call.userData == nil {
				let appData = CallAppData()
				TelecomManager.setAppData(sCall: call, appData: appData)
			}
			/*
			if let conference = call.conference, ConferenceViewModel.shared.conference.value == nil {
				Log.info("[Call] Found conference attached to call and no conference in dedicated view model, init & configure it")
				ConferenceViewModel.shared.initConference(conference)
				ConferenceViewModel.shared.configureConference(conference)
			}
			*/
			switch cstate {
			case .IncomingReceived:
				let addr = call.remoteAddress
				let displayName = incomingDisplayName(call: call)
				
			#if targetEnvironment(simulator)
				DispatchQueue.main.async {
					withAnimation {
						TelecomManager.shared.callInProgress = true
					}
				}
			#endif
				
				if call.replacedCall != nil {
					endCallKitReplacedCall = false
					
					let uuid = providerDelegate.uuids["\(TelecomManager.uuidReplacedCall ?? "")"]
					let callInfo = providerDelegate.callInfos[uuid!]
					callInfo!.callId = referedToCall ?? ""
					if callInfo != nil && uuid != nil && addr != nil {
						providerDelegate.callInfos.updateValue(callInfo!, forKey: uuid!)
						providerDelegate.uuids.removeValue(forKey: callId)
						providerDelegate.uuids.updateValue(uuid!, forKey: callInfo!.callId)
						providerDelegate.updateCall(uuid: uuid!, handle: addr!.asStringUriOnly(), hasVideo: video, displayName: displayName)
					}
				} else if TelecomManager.callKitEnabled(core: core) {
					/*
					 let isConference = isConferenceCall(call: call)
					let isEarlyConference = isConference && CallsViewModel.shared.currentCallData.value??.isConferenceCall.value != true // Conference info not be received yet.
					if (isEarlyConference) {
						CallsViewModel.shared.currentCallData.readCurrentAndObserve { _ in
							let uuid = providerDelegate.uuids["\(callId)"]
							if (uuid != nil) {
								displayName = "\(VoipTexts.conference_incoming_title):  \(CallsViewModel.shared.currentCallData.value??.remoteConferenceSubject.value ?? "") (\(CallsViewModel.shared.currentCallData.value??.conferenceParticipantsCountLabel.value ?? ""))"
								providerDelegate.updateCall(uuid: uuid!, handle: addr!.asStringUriOnly(), hasVideo: video, displayName: displayName)
							}
						}
					}
					*/
					let uuid = providerDelegate.uuids["\(callId)"]
					if call.replacedCall == nil {
						TelecomManager.uuidReplacedCall = callId
					}
					
					if uuid != nil {
						// Tha app is now registered, updated the call already existed.
						providerDelegate.updateCall(uuid: uuid!, handle: addr!.asStringUriOnly(), hasVideo: video, displayName: displayName)
					} else {
						displayIncomingCall(call: call, handle: addr!.asStringUriOnly(), hasVideo: video, callId: callId, displayName: displayName)
					}
				} /* else if UIApplication.shared.applicationState != .active {
					// not support callkit , use notif
					let content = UNMutableNotificationContent()
					content.title = NSLocalizedString("Incoming call", comment: "")
					content.body = displayName
					content.sound = UNNotificationSound.init(named: UNNotificationSoundName.init("notes_of_the_optimistic.caf"))
					content.categoryIdentifier = "call_cat"
					content.userInfo = ["CallId": callId]
					let req = UNNotificationRequest.init(identifier: "call_request", content: content, trigger: nil)
					UNUserNotificationCenter.current().add(req, withCompletionHandler: nil)
				} */
			case .StreamsRunning:
				if TelecomManager.callKitEnabled(core: core) {
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
				
				/*
				if speakerBeforePause {
					speakerBeforePause = false
					AudioRouteUtils.routeAudioToSpeaker(core: core)
				}
				 */
				
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
					if  uuid != nil && callId.isEmpty {
						let callInfo = providerDelegate.callInfos[uuid!]
						callInfo!.callId = callId
						providerDelegate.callInfos.updateValue(callInfo!, forKey: uuid!)
						providerDelegate.uuids.removeValue(forKey: "")
						providerDelegate.uuids.updateValue(uuid!, forKey: callId)
						
						Log.info("CallKit: outgoing call started connecting with uuid \(uuid!) and callId \(callId)")
						providerDelegate.reportOutgoingCallStartedConnecting(uuid: uuid!)
					} else {
						if false { /* isConferenceCall(call: call) {
							let uuid = UUID()
							let callInfo = CallInfo.newOutgoingCallInfo(addr: call.remoteAddress!, isSas: call.params?.mediaEncryption == .ZRTP, displayName: VoipTexts.conference_default_title, isVideo: call.params?.videoEnabled == true, isConference:true)
							providerDelegate.callInfos.updateValue(callInfo, forKey: uuid)
							providerDelegate.uuids.updateValue(uuid, forKey: "")
							providerDelegate.reportOutgoingCallStartedConnecting(uuid: uuid)
							Core.get().activateAudioSession(actived: true) */
						} else {
							referedToCall = callId
						}
					}
				}
			case .End,
					.Error:
				
				DispatchQueue.main.async {
					withAnimation {
						self.callInProgress = false
						self.callStarted = false
					}
				}
				var displayName = "Unknown"
				if call.dir == .Incoming {
					displayName = incomingDisplayName(call: call)
				} else { // if let addr = call.remoteAddress, let contactName = FastAddressBook.displayName(for: addr.getCobject) {
					displayName = "TODOContactName"
				}
				
				UIDevice.current.isProximityMonitoringEnabled = false
				if core.callsNb == 0 {
					core.outputAudioDevice = core.defaultOutputAudioDevice
					// disable this because I don't find anygood reason for it: _bluetoothAvailable = FALSE;
					// furthermore it introduces a bug when calling multiple times since route may not be
					// reconfigured between cause leading to bluetooth being disabled while it should not
					// bluetoothEnabled = false
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
			
			// AudioRouteUtils.isBluetoothAvailable(core: core)
			// AudioRouteUtils.isHeadsetAudioRouteAvailable(core: core)
			// AudioRouteUtils.isBluetoothAudioRouteAvailable(core: core)
			
			/*
			let readyForRoutechange = callkitAudioSessionActivated == nil || (callkitAudioSessionActivated == true)
			if readyForRoutechange && (cstate == .IncomingReceived || cstate == .OutgoingInit || cstate == .Connected || cstate == .StreamsRunning) {
				if (call.currentParams?.videoEnabled ?? false) && AudioRouteUtils.isReceiverEnabled(core: core) && call.conference == nil {
					AudioRouteUtils.routeAudioToSpeaker(core: core, call: call)
				} else if AudioRouteUtils.isBluetoothAvailable(core: core) {
					// Use bluetooth device by default if one is available
					AudioRouteUtils.routeAudioToBluetooth(core: core, call: call)
				}
			}
			 */
		}
		// post Notification kLinphoneCallUpdate
		NotificationCenter.default.post(name: Notification.Name("LinphoneCallUpdate"), object: self, userInfo: [
			AnyHashable("call"): NSValue.init(pointer: UnsafeRawPointer(call.getCobject)),
			AnyHashable("state"): NSNumber(value: cstate.rawValue),
			AnyHashable("message"): message
		])
	}
}

// swiftlint:enable cyclomatic_complexity
