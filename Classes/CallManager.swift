/*
* Copyright (c) 2010-2019 Belledonne Communications SARL.
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
import CoreTelephony

enum NetworkType: Int {
	case network_none = 0
	case network_2g = 1
	case network_3g = 2
	case network_4g = 3
	case network_lte = 4
	case network_wifi = 5
}

@objc class CallAppData: NSObject {
	@objc public var batteryWarningShown = false
	@objc public var videoRequested = false /*set when user has requested for video*/
}

@objc class CallManager: NSObject {
	static var theCallManager: CallManager?
	@objc static public var nextCallIsTransfer: Bool = false

	let providerDelegate: ProviderDelegate!
	let callController: CXCallController!
	let manager: CoreManager!
	let applicationKey = "app"
	var callAppDatas: [String : CallAppData] = [:]
	@objc var speakerBeforePause : Bool = false
	@objc var speakerEnabled : Bool = false
	@objc var bluetoothEnabled : Bool = false

	var lc: Core?
	var config: Config?

	fileprivate override init() {
		providerDelegate = ProviderDelegate()
		callController = CXCallController()
		manager = CoreManager()
	}

	@objc static func instance() -> CallManager {
		if (theCallManager == nil) {
			theCallManager = CallManager()
		}
		return theCallManager!
	}

	@objc func getAppData (callId : String) -> CallAppData? {
		return CallManager.instance().callAppDatas["\(callId)"]
	}

	@objc func setAppData (callId: String, appData: CallAppData) {
		CallManager.instance().callAppDatas.updateValue(appData, forKey: callId)
	}

	@objc func configCallManager(core: OpaquePointer, db:OpaquePointer) {
		lc = Core.getSwiftObject(cObject: core)
		lc?.addDelegate(delegate: manager)
		config = Config.getSwiftObject(cObject: db)
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
		if CallManager.instance().lpConfigBoolForKey(key: "use_callkit", section: "app") {
			return true
		}
		#endif
		return false
	}

	static func network() -> NetworkType {
		let info = CTTelephonyNetworkInfo()
		let currentRadio = info.currentRadioAccessTechnology
		if (currentRadio == CTRadioAccessTechnologyEdge) {
			return NetworkType.network_2g
		} else if (currentRadio == CTRadioAccessTechnologyLTE) {
			return NetworkType.network_4g
		}
		return NetworkType.network_3g
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
			allow = route != .lineOut || route == .headphones || (AudioHelper.bluetoothRoutes() as Array).contains(where: {($0 as! AVAudioSession.Port) == route})
		}

		return allow
	}

	@objc func setSpeakerEnabled(enable: Bool) {
		speakerEnabled = enable
		do {
			if (enable && allowSpeaker()) {
				try AVAudioSession.sharedInstance().overrideOutputAudioPort(.speaker)
				UIDevice.current.isProximityMonitoringEnabled = false
				bluetoothEnabled = false
			} else {
				let buildinPort = AudioHelper.bluetoothAudioDevice()
				try AVAudioSession.sharedInstance().setPreferredInput(buildinPort)
				UIDevice.current.isProximityMonitoringEnabled = (lc!.callsNb > 0)
			}
		} catch {
			Log.directLog(BCTBX_LOG_ERROR, text: "Failed to change audio route: err \(error)")
		}
	}
	
	@objc static func recordingFilePathFromCall(address: String) -> String {
		var filePath = "recording_"
		filePath = filePath.appending(address.isEmpty ? address : "unknow")
		let now = Date()
		let dateFormat = DateFormatter()
		dateFormat.dateFormat = "E-d-MMM-yyyy-HH-mm-ss"
		let date = dateFormat.string(from: now)
		
		filePath = filePath.appending("_\(date).mkv")
		
		let paths = NSSearchPathForDirectoriesInDomains(.cachesDirectory, .userDomainMask, true)
		var writablePath = paths[0]
		writablePath = writablePath.appending("/\(filePath)")
		Log.directLog(BCTBX_LOG_MESSAGE, text: "file path is \(writablePath)")
		return writablePath
		//file name is recording_contact-name_dayName-day-monthName-year-hour-minutes-seconds
		//The recording prefix is used to identify recordings in the cache directory.
		//We will use name_dayName-day-monthName-year to separate recordings by days, then hour-minutes-seconds to order them in each day.
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
		displayIncomingCall(call: nil, handle: "Calling", hasVideo: false, callId: callId)
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
			if (lpConfigBoolForKey(key: "edge_opt_preference")) {
				let low_bandwidth = (CallManager.network() == .network_2g)
				if (low_bandwidth) {
					Log.directLog(BCTBX_LOG_MESSAGE, text: "Low bandwidth mode")
				}
				callParams.lowBandwidthEnabled = low_bandwidth
			}

			//We set the record file name here because we can't do it after the call is started.
			let address = call.callLog?.fromAddress
			let writablePath = CallManager.recordingFilePathFromCall(address: address?.username ?? "")
			Log.directLog(BCTBX_LOG_MESSAGE, text: "Record file path: \(String(describing: writablePath))")
			callParams.recordFile = writablePath

			try call.acceptWithParams(params: callParams)
		} catch {
			Log.directLog(BCTBX_LOG_ERROR, text: "accept call failed \(error)")
		}
	}

	// for outgoing call. There is not yet callId
	@objc func startCall(addr: OpaquePointer?, isSas: Bool) {
		if (addr == nil) {
			print("Can not start a call with null address!")
			return
		}

		let sAddr = Address.getSwiftObject(cObject: addr!)
		if (CallManager.callKitEnabled()) {
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
		if CallManager.instance().lpConfigBoolForKey(key: "edge_opt_preference") && CallManager.network() == .network_2g {
			Log.directLog(BCTBX_LOG_MESSAGE, text: "Enabling low bandwidth mode")
			lcallParams.lowBandwidthEnabled = true
		}

		if (displayName != nil) {
			try addr.setDisplayname(newValue: displayName!)
		}

		if(CallManager.instance().lpConfigBoolForKey(key: "override_domain_with_default_one")) {
			try addr.setDomain(newValue: CallManager.instance().lpConfigStringForKey(key: "domain", section: "assistant"))
		}

		if (CallManager.nextCallIsTransfer) {
			let call = CallManager.instance().lc!.currentCall
			try call?.transfer(referTo: addr.asString())
			CallManager.nextCallIsTransfer = false
		} else {
			//We set the record file name here because we can't do it after the call is started.
			let writablePath = CallManager.recordingFilePathFromCall(address: addr.username )
			Log.directLog(BCTBX_LOG_DEBUG, text: "record file path: \(writablePath)")
			lcallParams.recordFile = writablePath
			if (isSas) {
				lcallParams.mediaEncryption = .ZRTP
			}
			let call = CallManager.instance().lc!.inviteAddressWithParams(addr: addr, params: lcallParams)
			if (call != nil) {
				let callId = call!.callLog?.callId
				// The LinphoneCallAppData object should be set on call creation with callback
				// - (void)onCall:StateChanged:withMessage:. If not, we are in big trouble and expect it to crash
				// We are NOT responsible for creating the AppData.
				let data = CallManager.instance().getAppData(callId: callId ?? "")
				if (data == nil) {
					Log.directLog(BCTBX_LOG_ERROR, text: "New call instanciated but app data was not set. Expect it to crash.")
					/* will be used later to notify user if video was not activated because of the linphone core*/
				} else {
					data!.videoRequested = lcallParams.videoEnabled
					CallManager.instance().setAppData(callId: callId!, appData: data!)
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
			heldCall(uuid: currentUuid!, onHold: false)
		} else {
			try? lc?.addAllToConference()
		}
	}

	func heldCall(uuid: UUID, onHold: Bool) {
		let heldAction = CXSetHeldCallAction(call: uuid, onHold: onHold)
		let otherTransacation = CXTransaction(action: heldAction)
		requestTransaction(otherTransacation, action: "heldCall")
	}

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

	//pragma mark - LPConfig Functions
	@objc func lpConfigSetString(value:String, key:String, section:String) {
		if (!key.isEmpty) {
			config?.setString(section: section, key: key, value: value)
		}
	}

	@objc func lpConfigSetString(value:String, key:String) {
		lpConfigSetString(value: value, key: key, section: applicationKey)
	}

	@objc func lpConfigStringForKey(key:String, section:String, defaultValue:String) -> String {
		if (key.isEmpty) {
			return defaultValue
		}
		return config?.getString(section: section, key: key, defaultString: "") ?? defaultValue
	}

	@objc func lpConfigStringForKey(key:String, section:String) -> String {
		return lpConfigStringForKey(key: key, section: section, defaultValue: "")
	}

	@objc func lpConfigStringForKey(key:String, defaultValue:String) -> String {
		return lpConfigStringForKey(key: key, section: applicationKey, defaultValue: defaultValue)
	}

	@objc func lpConfigStringForKey(key:String) -> String {
		return lpConfigStringForKey(key: key, defaultValue: "")
	}

	@objc func lpConfigSetInt(value:Int, key:String, section:String) {
		if(!key.isEmpty) {
			config?.setInt(section: section, key: key, value: value)
		}
	}

	@objc func lpConfigSetInt(value:Int, key:String) {
		lpConfigSetInt(value: value, key: key, section: applicationKey)
	}

	@objc func lpConfigIntForKey(key:String, section:String, defaultValue:Int) -> Int {
		if (key.isEmpty) {
			return defaultValue
		}
		return config?.getInt(section: section, key: key, defaultValue: defaultValue) ?? defaultValue
	}

	@objc func lpConfigIntForKey(key:String, section:String) -> Int {
		return lpConfigIntForKey(key: key, section: section, defaultValue: -1)
	}

	@objc func lpConfigIntForKey(key:String, defaultValue:Int) -> Int {
		return lpConfigIntForKey(key: key, section: applicationKey, defaultValue: defaultValue)
	}

	@objc func lpConfigIntForKey(key:String) -> Int {
		return lpConfigIntForKey(key: key, defaultValue: -1)
	}

	@objc func lpConfigSetBool(value:Bool, key:String, section:String) {
		lpConfigSetInt(value: value ? 1:0, key: key, section: section)
	}

	@objc func lpConfigSetBool(value:Bool, key:String) {
		lpConfigSetBool(value: value, key: key, section: applicationKey)
	}

	@objc func lpConfigBoolForKey(key:String, section:String, defaultValue:Bool) -> Bool {
		if (key.isEmpty) {
			return defaultValue
		}
		let val = lpConfigIntForKey(key: key, section: section, defaultValue: -1)
		return (val != -1) ? (val == 1) : defaultValue
	}

	@objc func lpConfigBoolForKey(key:String, section:String) -> Bool {
		return lpConfigBoolForKey(key: key, section: section, defaultValue: false)
	}

	@objc func lpConfigBoolForKey(key:String, defaultValue:Bool) -> Bool {
		return lpConfigBoolForKey(key: key, section: applicationKey, defaultValue: defaultValue)
	}

	@objc func lpConfigBoolForKey(key:String) -> Bool {
		return lpConfigBoolForKey(key: key, defaultValue: false)
	}
}

class CoreManager: CoreDelegate {
	static var speaker_already_enabled : Bool = false

	override func onCallStateChanged(lc: Core, call: Call, cstate: Call.State, message: String) {
		let addr = call.remoteAddress;
		let address = FastAddressBook.displayName(for: addr?.getCobject) ?? "Unknow"
		let callLog = call.callLog
		let callId = callLog?.callId
		let video = call.params?.videoEnabled ?? false
		// we keep the speaker auto-enabled state in this static so that we don't
		// force-enable it on ICE re-invite if the user disabled it.
		CoreManager.speaker_already_enabled = false

		if (callId != nil && CallManager.instance().callAppDatas["\(callId!)"] == nil) {
			CallManager.instance().callAppDatas.updateValue(CallAppData(), forKey: callId!)
		}

		switch cstate {
			case .IncomingReceived:
				if (CallManager.callKitEnabled()) {
					let uuid = CallManager.instance().providerDelegate.uuids["\(callId!)"]
					if (uuid != nil) {
						// Tha app is now registered, updated the call already existed.
						CallManager.instance().providerDelegate.updateCall(uuid: uuid!, handle: address, hasVideo: video)
						let callInfo = CallManager.instance().providerDelegate.callInfos[uuid!]
						let connected = callInfo?.connected ?? false
						if (connected) {
							// The call is already answered.
							CallManager.instance().acceptCall(call: call, hasVideo: video)
						}
					} else {
						// Nothing happped before, display a new Incoming call.
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
						if (callInfo?.isOutgoing ?? false) {
							Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: outgoing call connected with uuid \(uuid!) and callId \(callId!)")
							CallManager.instance().providerDelegate.reportOutgoingCallConnected(uuid: uuid!)
						}
					}
				}

				if (CallManager.instance().speakerBeforePause) {
					CallManager.instance().speakerBeforePause = false
					CallManager.instance().setSpeakerEnabled(enable: true)
					CoreManager.speaker_already_enabled = true
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
					}
				}
				break
			case .End,
				 .Error:
				UIDevice.current.isProximityMonitoringEnabled = false
				CoreManager.speaker_already_enabled = false
				if (CallManager.instance().lc!.callsNb == 0) {
					CallManager.instance().setSpeakerEnabled(enable: false)
					// disable this because I don't find anygood reason for it: _bluetoothAvailable = FALSE;
					// furthermore it introduces a bug when calling multiple times since route may not be
					// reconfigured between cause leading to bluetooth being disabled while it should not
					CallManager.instance().bluetoothEnabled = false
				}

				if callLog == nil || callLog?.status == .Missed || callLog?.status == .Aborted || callLog?.status == .EarlyAborted  {
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
					// end CallKit
					let uuid = CallManager.instance().providerDelegate.uuids["\(callId!)"]
					if (uuid != nil) {
						let transaction = CXTransaction(action:
							CXEndCallAction(call: uuid!))
						CallManager.instance().requestTransaction(transaction, action: "endCall")
					}
				}
				break
			case .Released:
				CallManager.instance().callAppDatas.removeValue(forKey: callId ?? "")
				break
			default:
				break
		}

		if (cstate == .IncomingReceived || cstate == .OutgoingInit || cstate == .Connected || cstate == .StreamsRunning) {
			if (video && CoreManager.speaker_already_enabled && CallManager.instance().bluetoothEnabled) {
				CallManager.instance().setSpeakerEnabled(enable: true)
				CoreManager.speaker_already_enabled = true
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


