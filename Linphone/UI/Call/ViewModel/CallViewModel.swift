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

import SwiftUI
import linphonesw
import AVFAudio
import Combine

class CallViewModel: ObservableObject {
	
	var coreContext = CoreContext.shared
	var telecomManager = TelecomManager.shared
	
	@Published var displayName: String = "Example Linphone"
	@Published var direction: Call.Dir = .Outgoing
	@Published var remoteAddressString: String = "example.linphone@sip.linphone.org"
	@Published var remoteAddress: Address?
	@Published var avatarModel: ContactAvatarModel?
	@Published var micMutted: Bool = false
	@Published var isRecording: Bool = false
	@Published var isRemoteRecording: Bool = false
	@Published var isPaused: Bool = false
	@Published var timeElapsed: Int = 0
	@Published var zrtpPopupDisplayed: Bool = false
	@Published var upperCaseAuthTokenToRead = ""
	@Published var upperCaseAuthTokenToListen = ""
	@Published var isMediaEncrypted: Bool = false
	@Published var isZrtpPq: Bool = false
	@Published var isRemoteDeviceTrusted: Bool = false
	@Published var selectedCall: Call? = nil
	
	var calls: [Call] = []
	
	let timer = Timer.publish(every: 1, on: .main, in: .common).autoconnect()
	
	var currentCall: Call?
	
	private var callSuscriptions = Set<AnyCancellable?>()
	
	init() {
		do {
			try AVAudioSession.sharedInstance().setCategory(.playAndRecord, mode: .voiceChat, options: .allowBluetooth)
		} catch _ {
			
		}
		resetCallView()
	}
	
	func enableAVAudioSession() {
		do {
			try AVAudioSession.sharedInstance().setActive(true)
		} catch _ {
			
		}
	}
	
	func disableAVAudioSession(){
		do {
			try AVAudioSession.sharedInstance().setActive(false)
		} catch _ {
			
		}
	}
	
	func resetCallView() {
		coreContext.doOnCoreQueue { core in
			if core.currentCall != nil && core.currentCall!.remoteAddress != nil {
				self.currentCall = core.currentCall
				DispatchQueue.main.async {
					self.direction = self.currentCall!.dir
					self.remoteAddressString = String(self.currentCall!.remoteAddress!.asStringUriOnly().dropFirst(4))
					self.remoteAddress = self.currentCall!.remoteAddress!
					
					let friend = ContactsManager.shared.getFriendWithAddress(address: self.currentCall!.remoteAddress!)
					if friend != nil && friend!.address != nil && friend!.address!.displayName != nil {
						self.displayName = friend!.address!.displayName!
					} else {
						if self.currentCall!.remoteAddress!.displayName != nil {
							self.displayName = self.currentCall!.remoteAddress!.displayName!
						} else if self.currentCall!.remoteAddress!.username != nil {
							self.displayName = self.currentCall!.remoteAddress!.username!
						}
					}
					
					//self.avatarModel = ???
					self.micMutted = self.currentCall!.microphoneMuted
					self.isRecording = self.currentCall!.params!.isRecording
					self.isPaused = self.isCallPaused()
					self.timeElapsed = self.currentCall?.duration ?? 0
					
					let authToken = self.currentCall!.authenticationToken
					let isDeviceTrusted = self.currentCall!.authenticationTokenVerified && authToken != nil
					self.isRemoteDeviceTrusted = self.telecomManager.callInProgress ? isDeviceTrusted : false
					
					self.getCallsList()
				}
				
				self.callSuscriptions.insert(self.currentCall!.publisher?.onEncryptionChanged?.postOnMainQueue {(cbVal: (call: Call, on: Bool, authenticationToken: String?)) in
					_ = self.updateEncryption()
				})
			}
		}
	}
	
	func getCallsList() {
		coreContext.doOnCoreQueue { core in
			self.calls = core.calls
		}
	}
	
	func terminateCall() {
		coreContext.doOnCoreQueue { core in
			if self.currentCall != nil {
				self.telecomManager.terminateCall(call: self.currentCall!)
			}
			
			if core.callsNb == 0 {
				self.timer.upstream.connect().cancel()
			}
		}
	}
	
	func acceptCall() {
		withAnimation {
			telecomManager.outgoingCallStarted = false
			telecomManager.callInProgress = true
			telecomManager.callStarted = true
		}
		
		coreContext.doOnCoreQueue { core in
			if self.currentCall != nil {
				self.telecomManager.acceptCall(core: core, call: self.currentCall!, hasVideo: false)
			}
		}
		
		timer.upstream.connect().cancel()
	}
	
	func toggleMuteMicrophone() {
		coreContext.doOnCoreQueue { _ in
			if self.currentCall != nil {
				self.currentCall!.microphoneMuted = !self.currentCall!.microphoneMuted
				self.micMutted = self.currentCall!.microphoneMuted
				Log.info(
					"[CallViewModel] Microphone mute switch \(self.micMutted)"
				)
			}
		}
	}
	
	func toggleVideo() {
		coreContext.doOnCoreQueue { core in
			if self.currentCall != nil {
				do {
					let params = try core.createCallParams(call: self.currentCall)
					
					params.videoEnabled = !params.videoEnabled
					Log.info(
						"[CallViewModel] Updating call with video enabled set to \(params.videoEnabled)"
					)
					try self.currentCall!.update(params: params)
				} catch {
					
				}
			}
		}
	}
	
	func switchCamera() {
		coreContext.doOnCoreQueue { core in
			let currentDevice = core.videoDevice
			Log.info("[CallViewModel] Current camera device is \(currentDevice)")
			
			core.videoDevicesList.forEach { camera in
				if camera != currentDevice && camera != "StaticImage: Static picture" {
					Log.info("[CallViewModel] New camera device will be \(camera)")
					do {
						try core.setVideodevice(newValue: camera)
					} catch _ {
						
					}
				}
			}
		}
	}
	
	func toggleRecording() {
		coreContext.doOnCoreQueue { _ in
			if self.currentCall != nil && self.currentCall!.params != nil {
				if self.currentCall!.params!.isRecording {
					Log.info("[CallViewModel] Stopping call recording")
					self.currentCall!.stopRecording()
				} else {
					Log.info("[CallViewModel] Starting call recording \(self.currentCall!.params!.isRecording)")
					self.currentCall!.startRecording()
				}
				
				self.isRecording = self.currentCall!.params!.isRecording
			}
		}
	}
	
	func togglePause() {
		coreContext.doOnCoreQueue { _ in
			if self.currentCall != nil && self.currentCall!.remoteAddress != nil {
				do {
					if self.isCallPaused() {
						Log.info("[CallViewModel] Resuming call \(self.currentCall!.remoteAddress!.asStringUriOnly())")
						try self.currentCall!.resume()
						self.isPaused = false
					} else {
						Log.info("[CallViewModel] Pausing call \(self.currentCall!.remoteAddress!.asStringUriOnly())")
						try self.currentCall!.pause()
						self.isPaused = true
					}
				} catch _ {
					
				}
			}
		}
	}
	
	func isCallPaused() -> Bool {
		var result = false
		if self.currentCall != nil {
			switch self.currentCall!.state {
			case Call.State.Paused, Call.State.Pausing:
				result = true
			default:
				result = false
			}
		}
		return result
	}
	
	func counterToMinutes() -> String {
		let currentTime = timeElapsed
		let seconds = currentTime % 60
		let minutes = String(format: "%02d", Int(currentTime / 60))
		let hours = String(format: "%02d", Int(currentTime / 3600))
		
		if Int(currentTime / 3600) > 0 {
			return "\(hours):\(minutes):\(seconds < 10 ? "0" : "")\(seconds)"
		} else {
			return "\(minutes):\(seconds < 10 ? "0" : "")\(seconds)"
		}
	}
	
	func isHeadPhoneAvailable() -> Bool {
		guard let availableInputs = AVAudioSession.sharedInstance().availableInputs else {return false}
		for inputDevice in availableInputs {
			if inputDevice.portType == .headsetMic  || inputDevice.portType == .headphones {
				return true
			}
		}
		return false
	}
	
	func getAudioRoute() -> Int {
		if AVAudioSession.sharedInstance().currentRoute.outputs.filter({ $0.portType.rawValue == "Speaker" }).isEmpty {
			if AVAudioSession.sharedInstance().currentRoute.outputs.filter({ $0.portType.rawValue.contains("Bluetooth") }).isEmpty {
				return 1
			} else {
				return 3
			}
		} else {
			return 2
		}
	}
	
	func orientationUpdate(orientation: UIDeviceOrientation) {
		coreContext.doOnCoreQueue { core in
			let oldLinphoneOrientation = core.deviceRotation
			var newRotation = 0
			switch orientation {
			case .portrait:
				newRotation = 0
			case .portraitUpsideDown:
				newRotation = 180
			case .landscapeRight:
				newRotation = 90
			case .landscapeLeft:
				newRotation = 270
			default:
				newRotation = oldLinphoneOrientation
			}
			
			if oldLinphoneOrientation != newRotation {
				core.deviceRotation = newRotation
			}
		}
	}
	
	func lettersClicked(letters: String) {
		let verified = letters == self.upperCaseAuthTokenToListen
		Log.info(
			"[ZRTPPopup] User clicked on \(verified ? "right" : "wrong") letters"
		)
		
		if verified {
			coreContext.doOnCoreQueue { core in
				if core.currentCall != nil {
					core.currentCall!.authenticationTokenVerified = verified
				}
			}
		}
	}
	
	private func updateEncryption() -> Bool {
		if currentCall != nil && currentCall!.currentParams != nil {
			switch currentCall!.currentParams!.mediaEncryption {
			case MediaEncryption.ZRTP:
				let authToken = currentCall!.authenticationToken
				let isDeviceTrusted = currentCall!.authenticationTokenVerified && authToken != nil
				
				Log.info(
					"[CallViewModel] Current call media encryption is ZRTP, auth token is \(isDeviceTrusted ? "trusted" : "not trusted yet")"
				)
				
				isRemoteDeviceTrusted = isDeviceTrusted
				
				if isDeviceTrusted {
					ToastViewModel.shared.toastMessage = "Info_call_securised"
					ToastViewModel.shared.displayToast = true
				}
				
				/*
				 let securityLevel = isDeviceTrusted ? SecurityLevel.Safe : SecurityLevel.Encrypted
				 let avatarModel = contact
				 if (avatarModel != nil) {
				 avatarModel.trust.postValue(securityLevel)
				 contact.postValue(avatarModel!!)
				 } else {
				 Log.error("$TAG No avatar model found!")
				 }
				 */
				
				isMediaEncrypted = true
				// When Post Quantum is available, ZRTP is Post Quantum
				isZrtpPq = Core.getPostQuantumAvailable
				
				if !isDeviceTrusted && authToken != nil && !authToken!.isEmpty {
					Log.info("[CallViewModel] Showing ZRTP SAS confirmation dialog")
					showZrtpSasDialog(authToken: authToken!)
				}
				
				return isDeviceTrusted
			case MediaEncryption.SRTP, MediaEncryption.DTLS:
				isMediaEncrypted = true
				isZrtpPq = false
				return false
			default:
				isMediaEncrypted = false
				isZrtpPq = false
				return false
			}
		}
		return false
	}
	
	func showZrtpSasDialogIfPossible() {
		if currentCall != nil && currentCall!.currentParams != nil && currentCall!.currentParams!.mediaEncryption == MediaEncryption.ZRTP {
			let authToken = currentCall!.authenticationToken
			let isDeviceTrusted = currentCall!.authenticationTokenVerified && authToken != nil
			Log.info(
				"[CallViewModel] Current call media encryption is ZRTP, auth token is \(isDeviceTrusted ? "trusted" : "not trusted yet")"
			)
			if (authToken != nil && !authToken!.isEmpty) {
				showZrtpSasDialog(authToken: authToken!)
			}
		}
	}
	
	private func showZrtpSasDialog(authToken: String) {
		if self.currentCall != nil {
			let upperCaseAuthToken = authToken.localizedUppercase
			
			let mySubstringPrefix = upperCaseAuthToken.prefix(2)
			
			let mySubstringSuffix = upperCaseAuthToken.suffix(2)
			
			switch self.currentCall!.dir {
			case Call.Dir.Incoming:
				self.upperCaseAuthTokenToRead = String(mySubstringPrefix)
				self.upperCaseAuthTokenToListen = String(mySubstringSuffix)
			default:
				self.upperCaseAuthTokenToRead = String(mySubstringSuffix)
				self.upperCaseAuthTokenToListen = String(mySubstringPrefix)
			}
			
			self.zrtpPopupDisplayed = true
		}
	}
}
