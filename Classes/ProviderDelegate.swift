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
import CallKit
import UIKit
import linphonesw
import AVFoundation
import os

class ProviderDelegate: NSObject {
	private let provider: CXProvider
	private let callController: CXCallController

	var uuids: [String : UUID] = [:]
	var calls: [UUID : String] = [:]
	var connecteds: [UUID : Bool] = [:]
	var addrs: [UUID : Address] = [:]
	var outgoingUuids: [String : UUID] = [:]
	var isSas: [UUID : Bool] = [:]
	

	override init() {
		provider = CXProvider(configuration: ProviderDelegate.providerConfiguration)
		callController = CXCallController()

		super.init()

		provider.setDelegate(self, queue: nil)
	}

	static var providerConfiguration: CXProviderConfiguration = {
		let providerConfiguration = CXProviderConfiguration(localizedName: Bundle.main.infoDictionary!["CFBundleName"] as! String)
		providerConfiguration.ringtoneSound = "notes_of_the_optimistic.caf"
		providerConfiguration.supportsVideo = true
		providerConfiguration.iconTemplateImageData = UIImage(named: "callkit_logo")?.pngData()
		providerConfiguration.supportedHandleTypes = [.generic]

		providerConfiguration.maximumCallsPerCallGroup = 3
		providerConfiguration.maximumCallGroups = 2

		//not show app's calls in tel's history
		//providerConfiguration.includesCallsInRecents = YES;
		
		return providerConfiguration
	}()

	func reportIncomingCall(call:Call?, uuid: UUID, handle: String, hasVideo: Bool) {
		let update = CXCallUpdate()
		update.remoteHandle = CXHandle(type:.generic, value: handle)
		update.hasVideo = hasVideo

		let callId = CallManager.instance().providerDelegate.calls[uuid]
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: report new incoming call with call-id: [\(String(describing: callId))] and UUID: [\(uuid.description)]")
		provider.reportNewIncomingCall(with: uuid, update: update) { error in
			if error == nil {
			} else {
				Log.directLog(BCTBX_LOG_ERROR, text: "CallKit: cannot complete incoming call with call-id: [\(String(describing: callId))] and UUID: [\(uuid.description)] from [\(handle)] caused by [\(error!.localizedDescription)]")
				let code = (error as NSError?)?.code
				if code == CXErrorCodeIncomingCallError.filteredByBlockList.rawValue || code == CXErrorCodeIncomingCallError.filteredByDoNotDisturb.rawValue {
					try? call?.decline(reason: Reason.Busy)
					} else {
					try? call?.decline(reason: Reason.Unknown)
				}
			}
		}
	}

	func updateCall(uuid: UUID, handle: String, hasVideo: Bool = false) {
		let update = CXCallUpdate()
		update.remoteHandle = CXHandle(type:.generic, value:handle)
		update.hasVideo = hasVideo

		provider.reportCall(with:uuid, updated:update);
	}

	func reportOutgoingCallStartedConnecting(uuid:UUID) {
		provider.reportOutgoingCall(with: uuid, startedConnectingAt: nil)
	}

	func reportOutgoingCallConnected(uuid:UUID) {
		provider.reportOutgoingCall(with: uuid, connectedAt: nil)
	}
}

// MARK: - CXProviderDelegate
extension ProviderDelegate: CXProviderDelegate {
	func provider(_ provider: CXProvider, perform action: CXEndCallAction) {
		let uuid = action.callUUID
		let callId = calls[uuid]
		let addr = addrs[uuid]
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: Call ended with call-id: \(String(describing: callId)) an UUID: \(uuid.description).")

		// remove call infos first, otherwise CXEndCallAction will be called more than onece
		if (addr != nil) {
			addrs.removeValue(forKey: uuid)
			outgoingUuids.removeValue(forKey: addr!.asStringUriOnly())
			isSas.removeValue(forKey: uuid)
		}
		let call = CallManager.instance().callByCallId(callId: callId)
		if (callId != nil) {
			uuids.removeValue(forKey: callId!)
		}
		calls.removeValue(forKey: uuid)
		connecteds.removeValue(forKey: uuid)

		if (call != nil) {
			do {
				try call!.terminate()
			} catch {
				Log.directLog(BCTBX_LOG_ERROR, text: "CallKit: Call ended \(uuid) failed because \(error)")
			}
		}
		action.fulfill()
	}

	func provider(_ provider: CXProvider, perform action: CXAnswerCallAction) {
		let uuid = action.callUUID
		let callId = calls[uuid]
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: answer call with call-id: \(String(describing: callId)) and UUID: \(uuid.description).")

		let call = CallManager.instance().callByCallId(callId: callId)
		if (call == nil) {
			// The application is not yet registered, mark the call as connected. The audio session must be configured here.
			CallManager.configAudioSession(audioSession: AVAudioSession.sharedInstance())
			CallManager.instance().providerDelegate.connecteds.updateValue(true, forKey: uuid)
		} else {
			CallManager.instance().acceptCall(call: call!, hasVideo: call!.params?.videoEnabled ?? false)
		}
		action.fulfill()
	}

	func provider(_ provider: CXProvider, perform action: CXSetHeldCallAction) {
		let uuid = action.callUUID
		let callId = calls[uuid]
		let call = CallManager.instance().callByCallId(callId: callId)

		if (call != nil && UIApplication.shared.applicationState != .active) {
			do {
				if action.isOnHold {
					Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: Call paused with call-id: \(String(describing: callId)) an UUID: \(uuid.description).")
					try call!.pause()
				} else {
					Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: Call resumed with call-id: \(String(describing: callId)) an UUID: \(uuid.description).")
					try call!.resume()
				}
			} catch {
				Log.directLog(BCTBX_LOG_ERROR, text: "CallKit: Call set held (paused or resumed) \(uuid) failed because \(error)")
			}
		}
		action.fulfill()
	}

	func provider(_ provider: CXProvider, perform action: CXStartCallAction) {
		do {
			let uuid = action.callUUID
			let addr = addrs[uuid]
			if (addr == nil) {
				Log.directLog(BCTBX_LOG_ERROR, text: "CallKit: can not call a null address!")
				action.fail()
			}

			try CallManager.instance().doCall(addr: addr!, isSas: CallManager.instance().providerDelegate.isSas[uuid] ?? false)
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
		let callId = calls[uuid]
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: Call muted with call-id: \(String(describing: callId)) an UUID: \(uuid.description).")

		CallManager.instance().lc!.micEnabled = !CallManager.instance().lc!.micEnabled
		action.fulfill()
	}

	func provider(_ provider: CXProvider, perform action: CXPlayDTMFCallAction) {
		let uuid = action.callUUID
		let callId = calls[uuid]
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
		let callId = calls[uuid]
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: Call time out with call-id: \(String(describing: callId)) an UUID: \(uuid.description).")
		action.fulfill()
	}

	func providerDidReset(_ provider: CXProvider) {
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: did reset.")
	}

	func provider(_ provider: CXProvider, didActivate audioSession: AVAudioSession) {
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: audio session activated.")
		CallManager.instance().lc?.audioSessionActivated(actived: true)
	}

	func provider(_ provider: CXProvider, didDeactivate audioSession: AVAudioSession) {
		Log.directLog(BCTBX_LOG_MESSAGE, text: "CallKit: audio session deactivated.")
		CallManager.instance().lc?.audioSessionActivated(actived: false)
	}
}

