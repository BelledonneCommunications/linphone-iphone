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
import CallKit
import UIKit
import linphonesw
import AVFoundation
import os

@objc class CallInfo: NSObject {
	var callId: String = ""
	var toAddr: Address?
	var isOutgoing = false
	var sasEnabled = false
	var connected = false
	var reason: Reason = Reason.None
	var displayName: String?

	static func newIncomingCallInfo(callId: String) -> CallInfo {
		let callInfo = CallInfo()
		callInfo.callId = callId
		return callInfo
	}
	
	static func newOutgoingCallInfo(addr: Address, isSas: Bool, displayName: String) -> CallInfo {
		let callInfo = CallInfo()
		callInfo.isOutgoing = true
		callInfo.sasEnabled = isSas
		callInfo.toAddr = addr
		callInfo.displayName = displayName
		return callInfo
	}
}

/*
* A delegate to support callkit.
*/
class ProviderDelegate: NSObject {
	private let provider: CXProvider
	var uuids: [String : UUID] = [:]
	var callInfos: [UUID : CallInfo] = [:]

	override init() {
		provider = CXProvider(configuration: ProviderDelegate.providerConfiguration)
		super.init()
		provider.setDelegate(self, queue: nil)
	}

	static var providerConfiguration: CXProviderConfiguration = {
		let providerConfiguration = CXProviderConfiguration(localizedName: Bundle.main.infoDictionary!["CFBundleName"] as! String)
		providerConfiguration.ringtoneSound = "notes_of_the_optimistic.caf"
		providerConfiguration.supportsVideo = true
		providerConfiguration.iconTemplateImageData = UIImage(named: "callkit_logo")?.pngData()
		providerConfiguration.supportedHandleTypes = [.generic, .phoneNumber, .emailAddress]

		providerConfiguration.maximumCallsPerCallGroup = 10
		providerConfiguration.maximumCallGroups = 2

		//not show app's calls in tel's history
		//providerConfiguration.includesCallsInRecents = YES;
		
		return providerConfiguration
	}()

	func reportIncomingCall(call:Call?, uuid: UUID, handle: String, hasVideo: Bool, displayName:String) {
		let update = CXCallUpdate()
		update.remoteHandle = CXHandle(type:.generic, value: handle)
		update.hasVideo = hasVideo
		update.localizedCallerName = displayName

		let callInfo = callInfos[uuid]
		let callId = callInfo?.callId
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: report new incoming call with call-id: [\(String(describing: callId))] and UUID: [\(uuid.description)]")
		//CallManager.instance().setHeldOtherCalls(exceptCallid: callId ?? "") 
		provider.reportNewIncomingCall(with: uuid, update: update) { error in
			if error == nil {
				if CallManager.instance().endCallkit {
					let call = CallManager.instance().lc?.getCallByCallid(callId: callId!)
					if (call?.state == .PushIncomingReceived) {
						try? call?.terminate()
					}
				}
			} else {
				Log.directLog(BCTBX_LOG_ERROR, text: "CallKit: cannot complete incoming call with call-id: [\(String(describing: callId))] and UUID: [\(uuid.description)] from [\(handle)] caused by [\(error!.localizedDescription)]")
				let code = (error as NSError?)?.code
				switch code {
				case CXErrorCodeIncomingCallError.filteredByDoNotDisturb.rawValue:
					callInfo?.reason = Reason.DoNotDisturb
				case CXErrorCodeIncomingCallError.filteredByBlockList.rawValue:
					callInfo?.reason = Reason.DoNotDisturb
				default:
					callInfo?.reason = Reason.Unknown
				}
				self.callInfos.updateValue(callInfo!, forKey: uuid)
				try? call?.decline(reason: callInfo!.reason)
			}
		}
	}

	func updateCall(uuid: UUID, handle: String, hasVideo: Bool = false, displayName:String) {
		let update = CXCallUpdate()
		update.remoteHandle = CXHandle(type:.generic, value:handle)
		update.localizedCallerName = displayName
		update.hasVideo = hasVideo
		provider.reportCall(with:uuid, updated:update);
	}

	func reportOutgoingCallStartedConnecting(uuid:UUID) {
		provider.reportOutgoingCall(with: uuid, startedConnectingAt: nil)
	}

	func reportOutgoingCallConnected(uuid:UUID) {
		provider.reportOutgoingCall(with: uuid, connectedAt: nil)
	}
	
	func endCall(uuid: UUID) {
		provider.reportCall(with: uuid, endedAt: .init(), reason: .failed)
	}

	func endCallNotExist(uuid: UUID, timeout: DispatchTime) {
		DispatchQueue.main.asyncAfter(deadline: timeout) {
			let callId = CallManager.instance().providerDelegate.callInfos[uuid]?.callId
			if (callId == nil) {
				// callkit already ended
				return
			}
			let call = CallManager.instance().callByCallId(callId: callId)
			if (call == nil) {
				Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: terminate call with call-id: \(String(describing: callId)) and UUID: \(uuid) which does not exist.")
				CallManager.instance().providerDelegate.endCall(uuid: uuid)
			}
		}
	}
}

// MARK: - CXProviderDelegate
extension ProviderDelegate: CXProviderDelegate {
	func provider(_ provider: CXProvider, perform action: CXEndCallAction) {
		
		let uuid = action.callUUID
		let callId = callInfos[uuid]?.callId

		// remove call infos first, otherwise CXEndCallAction will be called more than onece
		if (callId != nil) {
			uuids.removeValue(forKey: callId!)
		}
		callInfos.removeValue(forKey: uuid)

		let call = CallManager.instance().callByCallId(callId: callId)
		if let call = call {
			CallManager.instance().terminateCall(call: call.getCobject);
			Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: Call ended with call-id: \(String(describing: callId)) an UUID: \(uuid.description).")
		}
		action.fulfill()
	}

	func provider(_ provider: CXProvider, perform action: CXAnswerCallAction) {
		let uuid = action.callUUID
		let callInfo = callInfos[uuid]
		let callId = callInfo?.callId
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: answer call with call-id: \(String(describing: callId)) and UUID: \(uuid.description).")

		let call = CallManager.instance().callByCallId(callId: callId)
		
		if (UIApplication.shared.applicationState != .active) {
			CallManager.instance().backgroundContextCall = call
			CallManager.instance().backgroundContextCameraIsEnabled = call!.params?.videoEnabled ?? false
			call?.cameraEnabled = false // Disable camera while app is not on foreground
		}
		CallManager.instance().lc?.configureAudioSession()
		CallManager.instance().acceptCall(call: call!, hasVideo: call!.params?.videoEnabled ?? false)
		action.fulfill()
	}

	func provider(_ provider: CXProvider, perform action: CXSetHeldCallAction) {
		let uuid = action.callUUID
		let callId = callInfos[uuid]?.callId
		let call = CallManager.instance().callByCallId(callId: callId)
		action.fulfill()
		if (call == nil) {
			return
		}

		do {
			if (call?.conference != nil && action.isOnHold) {
				try CallManager.instance().lc?.leaveConference()
				Log.directLog(BCTBX_LOG_DEBUG, text: "CallKit: call-id: [\(String(describing: callId))] leaving conference")
				NotificationCenter.default.post(name: Notification.Name("LinphoneCallUpdate"), object: self)
				return
			}

			let state = action.isOnHold ? "Paused" : "Resumed"
			Log.directLog(BCTBX_LOG_DEBUG, text: "CallKit: Call  with call-id: [\(String(describing: callId))] and UUID: [\(uuid)] paused status changed to: [\(state)]")
			if (action.isOnHold) {
				if (call!.params?.localConferenceMode ?? false) {
					return
				}
				CallManager.instance().speakerBeforePause = CallManager.instance().isSpeakerEnabled()
				try call!.pause()
			} else {
				if (call?.conference != nil && CallManager.instance().lc?.callsNb ?? 0 > 1) {
					try CallManager.instance().lc?.enterConference()
					NotificationCenter.default.post(name: Notification.Name("LinphoneCallUpdate"), object: self)
				} else {
					try call!.resume()
				}
			}
		} catch {
			Log.directLog(BCTBX_LOG_ERROR, text: "CallKit: Call set held (paused or resumed) \(uuid) failed because \(error)")
		}
	}

	func provider(_ provider: CXProvider, perform action: CXStartCallAction) {
		
		
		do {
						
			let uuid = action.callUUID
			let callInfo = callInfos[uuid]
			let update = CXCallUpdate()
			update.remoteHandle = action.handle
			update.localizedCallerName = callInfo?.displayName
			self.provider.reportCall(with: action.callUUID, updated: update)
			
			let addr = callInfo?.toAddr
			if (addr == nil) {
				Log.directLog(BCTBX_LOG_ERROR, text: "CallKit: can not call a null address!")
				action.fail()
			}

			CallManager.instance().lc?.configureAudioSession()
			try CallManager.instance().doCall(addr: addr!, isSas: callInfo?.sasEnabled ?? false)
		} catch {
			Log.directLog(BCTBX_LOG_ERROR, text: "CallKit: Call started failed because \(error)")
			action.fail()
		}
		action.fulfill()
	}

	func provider(_ provider: CXProvider, perform action: CXSetGroupCallAction) {
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: Call grouped callUUid : \(action.callUUID) with callUUID: \(String(describing: action.callUUIDToGroupWith)).")
		do {
			try CallManager.instance().lc?.addAllToConference()
		} catch {
			Log.directLog(BCTBX_LOG_ERROR, text: "CallKit: Call grouped failed because \(error)")
		}
		action.fulfill()
	}

	func provider(_ provider: CXProvider, perform action: CXSetMutedCallAction) {
		let uuid = action.callUUID
		let callId = callInfos[uuid]?.callId
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: Call muted with call-id: \(String(describing: callId)) an UUID: \(uuid.description).")
		CallManager.instance().lc!.micEnabled = !CallManager.instance().lc!.micEnabled
		action.fulfill()
	}

	func provider(_ provider: CXProvider, perform action: CXPlayDTMFCallAction) {
		let uuid = action.callUUID
		let callId = callInfos[uuid]?.callId
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: Call send dtmf with call-id: \(String(describing: callId)) an UUID: \(uuid.description).")
		let call = CallManager.instance().callByCallId(callId: callId)
		if (call != nil) {
			let digit = (action.digits.cString(using: String.Encoding.utf8)?[0])!
			do {
				try call!.sendDtmf(dtmf: digit)
			} catch {
				Log.directLog(BCTBX_LOG_ERROR, text: "CallKit: Call send dtmf \(uuid) failed because \(error)")
			}
		}
		action.fulfill()
	}

	func provider(_ provider: CXProvider, timedOutPerforming action: CXAction) {
		let uuid = action.uuid
		let callId = callInfos[uuid]?.callId
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: Call time out with call-id: \(String(describing: callId)) an UUID: \(uuid.description).")
		action.fulfill()
	}

	func providerDidReset(_ provider: CXProvider) {
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: did reset.")
	}

	func provider(_ provider: CXProvider, didActivate audioSession: AVAudioSession) {
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: audio session activated.")
		CallManager.instance().lc?.activateAudioSession(actived: true)
	}

	func provider(_ provider: CXProvider, didDeactivate audioSession: AVAudioSession) {
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: audio session deactivated.")
		CallManager.instance().lc?.activateAudioSession(actived: false)
	}
}

