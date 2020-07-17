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

import Foundation
import linphonesw
import UserNotifications
import os
import CallKit
import AVFoundation

@objc class CallAppData: NSObject {
	@objc var batteryWarningShown = false
	@objc var videoRequested = false /*set when user has requested for video*/
}

/*
* CallManager is a class that manages application calls and supports callkit.
* There is only one CallManager by calling CallManager.instance().
*/
@objc class CallManager: NSObject {
	static var theCallManager: CallManager?
	let providerDelegate: ProviderDelegate! // to support callkit
	let callController: CXCallController! // to support callkit
	let manager: CoreManagerDelegate! // callbacks of the linphonecore
	var lc: Core?
	@objc var speakerBeforePause : Bool = false
	@objc var speakerEnabled : Bool = false
	@objc var bluetoothEnabled : Bool = false
	@objc var nextCallIsTransfer: Bool = false
	@objc var alreadyRegisteredForNotification: Bool = false
	var referedFromCall: String?
	var referedToCall: String?


	fileprivate override init() {
		providerDelegate = ProviderDelegate()
		callController = CXCallController()
		manager = CoreManagerDelegate()
	}

	@objc static func instance() -> CallManager {
		if (theCallManager == nil) {
			theCallManager = CallManager()
		}
		return theCallManager!
	}

	@objc func setCore(core: OpaquePointer) {
		lc = Core.getSwiftObject(cObject: core)
		lc?.addDelegate(delegate: manager)
	}

	@objc static func getAppData(call: OpaquePointer) -> CallAppData? {
		let sCall = Call.getSwiftObject(cObject: call)
		return getAppData(sCall: sCall)
	}
	
	static func getAppData(sCall:Call) -> CallAppData? {
		if (sCall.userData == nil) {
			return nil
		}
		return Unmanaged<CallAppData>.fromOpaque(sCall.userData!).takeUnretainedValue()
	}

	@objc static func setAppData(call:OpaquePointer, appData: CallAppData) {
		let sCall = Call.getSwiftObject(cObject: call)
		setAppData(sCall: sCall, appData: appData)
	}
	
	static func setAppData(sCall:Call, appData:CallAppData?) {
		if (sCall.userData != nil) {
			Unmanaged<CallAppData>.fromOpaque(sCall.userData!).release()
		}
		if (appData == nil) {
			sCall.userData = nil
		} else {
			sCall.userData = UnsafeMutableRawPointer(Unmanaged.passRetained(appData!).toOpaque())
		}
	}

	@objc func findCall(callId: String?) -> OpaquePointer? {
		let call = callByCallId(callId: callId)
		return call?.getCobject
	}

	func callByCallId(callId: String?) -> Call? {
		if (callId == nil) {
			return nil
		}
		let calls = lc?.calls
		if let callTmp = calls?.first(where: { $0.callLog?.callId == callId }) {
			return callTmp
		}
		return nil
	}

	@objc static func callKitEnabled() -> Bool {
		#if !targetEnvironment(simulator)
		if ConfigManager.instance().lpConfigBoolForKey(key: "use_callkit", section: "app") {
			return true
		}
		#endif
		return false
	}

	@objc func allowSpeaker() -> Bool {
		if (UIDevice.current.userInterfaceIdiom == .pad) {
			// For now, ipad support only speaker.
			return true
		}

		var allow = true
		let newRoute = AVAudioSession.sharedInstance().currentRoute
		if (newRoute.outputs.count > 0) {
			let route = newRoute.outputs[0].portType
			allow = !( route == .lineOut || route == .headphones || (AudioHelper.bluetoothRoutes() as Array).contains(where: {($0 as! AVAudioSession.Port) == route}))
		}

		return allow
	}

	@objc func enableSpeaker(enable: Bool) {
		speakerEnabled = enable
		do {
			if (enable && allowSpeaker()) {
				try AVAudioSession.sharedInstance().overrideOutputAudioPort(.speaker)
				UIDevice.current.isProximityMonitoringEnabled = false
				bluetoothEnabled = false
			} else {
				try AVAudioSession.sharedInstance().overrideOutputAudioPort(.none)
				let buildinPort = AudioHelper.builtinAudioDevice()
				try AVAudioSession.sharedInstance().setPreferredInput(buildinPort)
				UIDevice.current.isProximityMonitoringEnabled = (lc!.callsNb > 0)
			}
		} catch {
			Log.directLog(BCTBX_LOG_ERROR, text: "Failed to change audio route: err \(error)")
		}
	}

	func requestTransaction(_ transaction: CXTransaction, action: String) {
		callController.request(transaction) { error in
			if let error = error {
				Log.directLog(BCTBX_LOG_ERROR, text: "CallKit: Requested transaction \(action) failed because: \(error)")
			} else {
				Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: Requested transaction \(action) successfully")
			}
		}
	}

	// From ios13, display the callkit view when the notification is received.
	@objc func displayIncomingCall(callId: String) {
		let uuid = CallManager.instance().providerDelegate.uuids["\(callId)"]
		if (uuid != nil) {
			let callInfo = providerDelegate.callInfos[uuid!]
			if (callInfo?.declined ?? false) {
				// This call was declined.
				providerDelegate.reportIncomingCall(call:nil, uuid: uuid!, handle: "Calling", hasVideo: true)
				providerDelegate.endCall(uuid: uuid!)
			}
			return
		}

		let call = CallManager.instance().callByCallId(callId: callId)
		if (call != nil) {
			let addr = FastAddressBook.displayName(for: call?.remoteAddress?.getCobject) ?? "Unknow"
			let video = UIApplication.shared.applicationState == .active && (lc!.videoActivationPolicy?.automaticallyAccept ?? false) && (call!.remoteParams?.videoEnabled ?? false)
			displayIncomingCall(call: call, handle: addr, hasVideo: video, callId: callId)
		} else {
			displayIncomingCall(call: nil, handle: "Calling", hasVideo: true, callId: callId)
		}
	}

	func displayIncomingCall(call:Call?, handle: String, hasVideo: Bool, callId: String) {
		let uuid = UUID()
		let callInfo = CallInfo.newIncomingCallInfo(callId: callId)

		providerDelegate.callInfos.updateValue(callInfo, forKey: uuid)
		providerDelegate.uuids.updateValue(uuid, forKey: callId)
		providerDelegate.reportIncomingCall(call:call, uuid: uuid, handle: handle, hasVideo: hasVideo)
	}

	@objc func acceptCall(call: OpaquePointer?, hasVideo:Bool) {
		if (call == nil) {
			Log.directLog(BCTBX_LOG_ERROR, text: "Can not accept null call!")
			return
		}
		let call = Call.getSwiftObject(cObject: call!)
		acceptCall(call: call, hasVideo: hasVideo)
	}

	func acceptCall(call: Call, hasVideo:Bool) {
		do {
			let callParams = try lc!.createCallParams(call: call)
			callParams.videoEnabled = hasVideo
			if (ConfigManager.instance().lpConfigBoolForKey(key: "edge_opt_preference")) {
				let low_bandwidth = (AppManager.network() == .network_2g)
				if (low_bandwidth) {
					Log.directLog(BCTBX_LOG_MESSAGE, text: "Low bandwidth mode")
				}
				callParams.lowBandwidthEnabled = low_bandwidth
			}

			//We set the record file name here because we can't do it after the call is started.
			let address = call.callLog?.fromAddress
			let writablePath = AppManager.recordingFilePathFromCall(address: address?.username ?? "")
			Log.directLog(BCTBX_LOG_MESSAGE, text: "Record file path: \(String(describing: writablePath))")
			callParams.recordFile = writablePath

			try call.acceptWithParams(params: callParams)
		} catch {
			Log.directLog(BCTBX_LOG_ERROR, text: "accept call failed \(error)")
		}
	}

	// Used when addressBook updated
	@objc func updateIncomingCall() {
		let call = lc?.currentCall
		if (call == nil || call!.state != .IncomingReceived) {
			return
		}
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: LinphoneAddressBookUpdated, updating callkit view")
		let callId = call!.callLog!.callId
		let uuid = CallManager.instance().providerDelegate.uuids["\(callId)"]
		if (uuid != nil) {
			let address = FastAddressBook.displayName(for: call!.remoteAddress?.getCobject) ?? "Unknow"
			let video = UIApplication.shared.applicationState == .active && (lc?.videoActivationPolicy?.automaticallyAccept ?? false) && (call!.remoteParams?.videoEnabled ?? false)
			CallManager.instance().providerDelegate.updateCall(uuid: uuid!, handle: address, hasVideo: video)
		}
	}

	// for outgoing call. There is not yet callId
	@objc func startCall(addr: OpaquePointer?, isSas: Bool) {
		if (addr == nil) {
			print("Can not start a call with null address!")
			return
		}

		let sAddr = Address.getSwiftObject(cObject: addr!)
		if (CallManager.callKitEnabled() && !CallManager.instance().nextCallIsTransfer) {
			let uuid = UUID()
			let name = FastAddressBook.displayName(for: addr) ?? "unknow"
			let handle = CXHandle(type: .generic, value: name)
			let startCallAction = CXStartCallAction(call: uuid, handle: handle)
			let transaction = CXTransaction(action: startCallAction)

			let callInfo = CallInfo.newOutgoingCallInfo(addr: sAddr, isSas: isSas)
			providerDelegate.callInfos.updateValue(callInfo, forKey: uuid)
			providerDelegate.uuids.updateValue(uuid, forKey: "")

			requestTransaction(transaction, action: "startCall")
		}else {
			try? doCall(addr: sAddr, isSas: isSas)
		}
	}

	func doCall(addr: Address, isSas: Bool) throws {
		let displayName = FastAddressBook.displayName(for: addr.getCobject)

		let lcallParams = try CallManager.instance().lc!.createCallParams(call: nil)
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

		if (CallManager.instance().nextCallIsTransfer) {
			let call = CallManager.instance().lc!.currentCall
			try call?.transfer(referTo: addr.asString())
			CallManager.instance().nextCallIsTransfer = false
		} else {
			//We set the record file name here because we can't do it after the call is started.
			let writablePath = AppManager.recordingFilePathFromCall(address: addr.username )
			Log.directLog(BCTBX_LOG_DEBUG, text: "record file path: \(writablePath)")
			lcallParams.recordFile = writablePath
			if (isSas) {
				lcallParams.mediaEncryption = .ZRTP
			}
			let call = CallManager.instance().lc!.inviteAddressWithParams(addr: addr, params: lcallParams)
			if (call != nil) {
				// The LinphoneCallAppData object should be set on call creation with callback
				// - (void)onCall:StateChanged:withMessage:. If not, we are in big trouble and expect it to crash
				// We are NOT responsible for creating the AppData.
				let data = CallManager.getAppData(sCall: call!)
				if (data == nil) {
					Log.directLog(BCTBX_LOG_ERROR, text: "New call instanciated but app data was not set. Expect it to crash.")
					/* will be used later to notify user if video was not activated because of the linphone core*/
				} else {
					data!.videoRequested = lcallParams.videoEnabled
					CallManager.setAppData(sCall: call!, appData: data)
				}
			}
		}
	}

	@objc func groupCall() {
		if (CallManager.callKitEnabled()) {
			let calls = lc?.calls
			if (calls == nil || calls!.isEmpty) {
				return
			}
			let firstCall = calls!.first?.callLog?.callId ?? ""
			let lastCall = (calls!.count > 1) ? calls!.last?.callLog?.callId ?? "" : ""

			let currentUuid = CallManager.instance().providerDelegate.uuids["\(firstCall)"]
			if (currentUuid == nil) {
				Log.directLog(BCTBX_LOG_ERROR, text: "Can not find correspondant call to group.")
				return
			}

			let newUuid = CallManager.instance().providerDelegate.uuids["\(lastCall)"]
			let groupAction = CXSetGroupCallAction(call: currentUuid!, callUUIDToGroupWith: newUuid)
			let transcation = CXTransaction(action: groupAction)
			requestTransaction(transcation, action: "groupCall")

			// To simulate the real group call action
			let heldAction = CXSetHeldCallAction(call: currentUuid!, onHold: false)
			let otherTransacation = CXTransaction(action: heldAction)
			requestTransaction(otherTransacation, action: "heldCall")
		} else {
			try? lc?.addAllToConference()
		}
	}

	@objc func removeAllCallInfos() {
		providerDelegate.callInfos.removeAll()
		providerDelegate.uuids.removeAll()
	}

	// To be removed.
	static func configAudioSession(audioSession: AVAudioSession) {
		do {
			try audioSession.setCategory(AVAudioSession.Category.playAndRecord, mode: AVAudioSession.Mode.voiceChat, options: AVAudioSession.CategoryOptions(rawValue: AVAudioSession.CategoryOptions.allowBluetooth.rawValue | AVAudioSession.CategoryOptions.allowBluetoothA2DP.rawValue))
			try audioSession.setMode(AVAudioSession.Mode.voiceChat)
			try audioSession.setPreferredSampleRate(48000.0)
			try AVAudioSession.sharedInstance().setActive(true, options: [])
		} catch {
			Log.directLog(BCTBX_LOG_WARNING, text: "CallKit: Unable to config audio session because : \(error)")
		}
	}

	@objc func terminateCall(call: OpaquePointer?) {
		if (call == nil) {
			Log.directLog(BCTBX_LOG_ERROR, text: "Can not terminate null call!")
			return
		}
		let call = Call.getSwiftObject(cObject: call!)
		do {
			try call.terminate()
			Log.directLog(BCTBX_LOG_DEBUG, text: "Call terminated")
		} catch {
			Log.directLog(BCTBX_LOG_ERROR, text: "Failed to terminate call failed because \(error)")
		}
		if (UIApplication.shared.applicationState == .background) {
			CoreManager.instance().stopLinphoneCore()
		}
	}

	@objc func markCallAsDeclined(callId: String) {
		if !CallManager.callKitEnabled() {
			return
		}

		let uuid = providerDelegate.uuids["\(callId)"]
		if (uuid == nil) {
			Log.directLog(BCTBX_LOG_MESSAGE, text: "Mark call \(callId) as declined.")
			let uuid = UUID()
			providerDelegate.uuids.updateValue(uuid, forKey: callId)
			let callInfo = CallInfo.newIncomingCallInfo(callId: callId)
			callInfo.declined = true
			providerDelegate.callInfos.updateValue(callInfo, forKey: uuid)
		} else {
			// end call
			providerDelegate.endCall(uuid: uuid!)
		}
	}

	@objc func setHeld(call: OpaquePointer, hold: Bool) {
		let sCall = Call.getSwiftObject(cObject: call)
		let callid = sCall.callLog?.callId ?? ""
		let uuid = providerDelegate.uuids["\(callid)"]

		if (uuid == nil) {
			Log.directLog(BCTBX_LOG_ERROR, text: "Can not find correspondant call to group.")
			return
		}
		let setHeldAction = CXSetHeldCallAction(call: uuid!, onHold: hold)
		let transaction = CXTransaction(action: setHeldAction)

		requestTransaction(transaction, action: "setHeld")
	}
}

class CoreManagerDelegate: CoreDelegate {
	static var speaker_already_enabled : Bool = false

	override func onCallStateChanged(lc: Core, call: Call, cstate: Call.State, message: String) {
		let addr = call.remoteAddress;
		let address = FastAddressBook.displayName(for: addr?.getCobject) ?? "Unknow"
		let callLog = call.callLog
		let callId = callLog?.callId
		let video = UIApplication.shared.applicationState == .active && (lc.videoActivationPolicy?.automaticallyAccept ?? false) && (call.remoteParams?.videoEnabled ?? false)
		// we keep the speaker auto-enabled state in this static so that we don't
		// force-enable it on ICE re-invite if the user disabled it.
		CoreManagerDelegate.speaker_already_enabled = false

		if (call.userData == nil) {
			let appData = CallAppData()
			CallManager.setAppData(sCall: call, appData: appData)
		}


		switch cstate {
			case .IncomingReceived:
				if (CallManager.callKitEnabled()) {
					let uuid = CallManager.instance().providerDelegate.uuids["\(callId!)"]
					if (uuid != nil) {
						// Tha app is now registered, updated the call already existed.
						CallManager.instance().providerDelegate.updateCall(uuid: uuid!, handle: address, hasVideo: video)
						let callInfo = CallManager.instance().providerDelegate.callInfos[uuid!]
						if (callInfo?.declined ?? false) {
							// The call is already declined.
							try? call.decline(reason: Reason.Unknown)
						} else if (callInfo?.accepted ?? false) {
							// The call is already answered.
							CallManager.instance().acceptCall(call: call, hasVideo: video)
						}
					} else {
						CallManager.instance().displayIncomingCall(call: call, handle: address, hasVideo: video, callId: callId!)
					}
				} else if (UIApplication.shared.applicationState != .active) {
					// not support callkit , use notif
					let content = UNMutableNotificationContent()
					content.title = NSLocalizedString("Incoming call", comment: "")
					content.body = address
					content.sound = UNNotificationSound.init(named: UNNotificationSoundName.init("notes_of_the_optimistic.caf"))
					content.categoryIdentifier = "call_cat"
					content.userInfo = ["CallId" : callId!]
					let req = UNNotificationRequest.init(identifier: "call_request", content: content, trigger: nil)
						UNUserNotificationCenter.current().add(req, withCompletionHandler: nil)
				}
				break
			case .StreamsRunning:
				if (CallManager.callKitEnabled()) {
					let uuid = CallManager.instance().providerDelegate.uuids["\(callId!)"]
					if (uuid != nil) {
						let callInfo = CallManager.instance().providerDelegate.callInfos[uuid!]
						if (callInfo != nil && callInfo!.isOutgoing && !callInfo!.connected) {
							Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: outgoing call connected with uuid \(uuid!) and callId \(callId!)")
							CallManager.instance().providerDelegate.reportOutgoingCallConnected(uuid: uuid!)
							callInfo!.connected = true
							CallManager.instance().providerDelegate.callInfos.updateValue(callInfo!, forKey: uuid!)
						}
					}
				}

				if (CallManager.instance().speakerBeforePause) {
					CallManager.instance().speakerBeforePause = false
					CallManager.instance().enableSpeaker(enable: true)
					CoreManagerDelegate.speaker_already_enabled = true
				}
				break
			case .OutgoingRinging:
				if (CallManager.callKitEnabled()) {
					let uuid = CallManager.instance().providerDelegate.uuids[""]
					if (uuid != nil) {
						let callInfo = CallManager.instance().providerDelegate.callInfos[uuid!]
						callInfo!.callId = callId!
						CallManager.instance().providerDelegate.callInfos.updateValue(callInfo!, forKey: uuid!)
						CallManager.instance().providerDelegate.uuids.removeValue(forKey: "")
						CallManager.instance().providerDelegate.uuids.updateValue(uuid!, forKey: callId!)

						Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: outgoing call started connecting with uuid \(uuid!) and callId \(callId!)")
						CallManager.instance().providerDelegate.reportOutgoingCallStartedConnecting(uuid: uuid!)
					} else {
						CallManager.instance().referedToCall = callId
					}
				}
				break
			case .End,
				 .Error:
				UIDevice.current.isProximityMonitoringEnabled = false
				CoreManagerDelegate.speaker_already_enabled = false
				if (CallManager.instance().lc!.callsNb == 0) {
					CallManager.instance().enableSpeaker(enable: false)
					// disable this because I don't find anygood reason for it: _bluetoothAvailable = FALSE;
					// furthermore it introduces a bug when calling multiple times since route may not be
					// reconfigured between cause leading to bluetooth being disabled while it should not
					CallManager.instance().bluetoothEnabled = false
				}

				if UIApplication.shared.applicationState != .active && (callLog == nil || callLog?.status == .Missed || callLog?.status == .Aborted || callLog?.status == .EarlyAborted)  {
					// Configure the notification's payload.
					let content = UNMutableNotificationContent()
					content.title = NSString.localizedUserNotificationString(forKey: NSLocalizedString("Missed call", comment: ""), arguments: nil)
					content.body = NSString.localizedUserNotificationString(forKey: address, arguments: nil)

					// Deliver the notification.
					let request = UNNotificationRequest(identifier: "call_request", content: content, trigger: nil) // Schedule the notification.
					let center = UNUserNotificationCenter.current()
					center.add(request) { (error : Error?) in
						if error != nil {
						Log.directLog(BCTBX_LOG_ERROR, text: "Error while adding notification request : \(error!.localizedDescription)")
						}
					}
				}

				if (CallManager.callKitEnabled()) {
					var uuid = CallManager.instance().providerDelegate.uuids["\(callId!)"]
					if (callId == CallManager.instance().referedToCall) {
						// refered call ended before connecting
						Log.directLog(BCTBX_LOG_MESSAGE, text: "Callkit: end refered to call :  \(String(describing: CallManager.instance().referedToCall))")
						CallManager.instance().referedFromCall = nil
						CallManager.instance().referedToCall = nil
					}
					if uuid == nil {
						// the call not yet connected
						uuid = CallManager.instance().providerDelegate.uuids[""]
					}
					if (uuid != nil) {
						if (callId == CallManager.instance().referedFromCall) {
							Log.directLog(BCTBX_LOG_MESSAGE, text: "Callkit: end refered from call : \(String(describing: CallManager.instance().referedFromCall))")
							CallManager.instance().referedFromCall = nil
							let callInfo = CallManager.instance().providerDelegate.callInfos[uuid!]
							callInfo!.callId = CallManager.instance().referedToCall ?? ""
							CallManager.instance().providerDelegate.callInfos.updateValue(callInfo!, forKey: uuid!)
							CallManager.instance().providerDelegate.uuids.removeValue(forKey: callId!)
							CallManager.instance().providerDelegate.uuids.updateValue(uuid!, forKey: callInfo!.callId)
							CallManager.instance().referedToCall = nil
							break
						}

						let transaction = CXTransaction(action:
						CXEndCallAction(call: uuid!))
						CallManager.instance().requestTransaction(transaction, action: "endCall")
					}
				}
				break
			case .Released:
				call.userData = nil
				break
			case .Referred:
				CallManager.instance().referedFromCall = call.callLog?.callId
				break
			default:
				break
		}

		if (cstate == .IncomingReceived || cstate == .OutgoingInit || cstate == .Connected || cstate == .StreamsRunning) {
			if ((call.currentParams?.videoEnabled ?? false) && !CoreManagerDelegate.speaker_already_enabled && !CallManager.instance().bluetoothEnabled) {
				CallManager.instance().enableSpeaker(enable: true)
				CoreManagerDelegate.speaker_already_enabled = true
			}
		}

		// post Notification kLinphoneCallUpdate
		NotificationCenter.default.post(name: Notification.Name("LinphoneCallUpdate"), object: self, userInfo: [
			AnyHashable("call"): NSValue.init(pointer:UnsafeRawPointer(call.getCobject)),
			AnyHashable("state"): NSNumber(value: cstate.rawValue),
			AnyHashable("message"): message
		])
	}
}


