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
import SwiftUI
import AVFAudio

class MeetingWaitingRoomViewModel: ObservableObject {
	var coreContext = CoreContext.shared
	var telecomManager = TelecomManager.shared
	
	@Published var userName: String = ""
	@Published var avatarModel: ContactAvatarModel?
	@Published var micMutted: Bool = false
	@Published var isRemoteDeviceTrusted: Bool = false
	@Published var selectedCall: Call?
	@Published var isConference: Bool = false
	@Published var videoDisplayed: Bool = false
	@Published var avatarDisplayed: Bool = true
	@Published var imageAudioRoute: String = ""
	@Published var meetingDate: String = ""
	
	init() {
		do {
			try AVAudioSession.sharedInstance().setCategory(.playAndRecord, mode: .voiceChat, options: .allowBluetooth)
		} catch _ {
			
		}
		if !telecomManager.callStarted {
			self.resetMeetingRoomView()
		}
	}
	
	func resetMeetingRoomView() {
		if self.telecomManager.meetingWaitingRoomSelected != nil {
			do {
				try AVAudioSession.sharedInstance().setCategory(.playAndRecord, mode: .voiceChat, options: .allowBluetooth)
			} catch _ {
				
			}
			coreContext.doOnCoreQueue { core in
				
				let conf = core.findConferenceInformationFromUri(uri: self.telecomManager.meetingWaitingRoomSelected!)
				
				do {
					try core.setVideodevice(newValue: "AV Capture: com.apple.avfoundation.avcapturedevice.built-in_video:1")
				} catch _ {
					
				}
				
				if conf != nil && conf!.uri != nil {
					let confNameTmp = conf?.subject ?? "Conference"
					var userNameTmp = ""
					
					let friend = core.defaultAccount != nil && core.defaultAccount!.contactAddress != nil
					? ContactsManager.shared.getFriendWithAddress(address: core.defaultAccount?.contactAddress)
					: nil
					
					let addressTmp = friend?.address?.asStringUriOnly() ?? ""
					
					if friend != nil && friend!.address != nil && friend!.address!.displayName != nil {
						userNameTmp = friend!.address!.displayName!
					} else {
						if core.defaultAccount != nil && core.defaultAccount!.contactAddress != nil {
							if core.defaultAccount!.contactAddress!.displayName != nil {
								userNameTmp = core.defaultAccount!.contactAddress!.displayName!
							} else if core.defaultAccount!.contactAddress!.username != nil {
								userNameTmp = core.defaultAccount!.contactAddress!.username!
							} else {
								userNameTmp = String(core.defaultAccount!.contactAddress!.asStringUriOnly().dropFirst(4))
							}
						}
					}
					
					let avatarModelTmp = friend != nil
					? ContactsManager.shared.avatarListModel.first(where: {
						$0.friend!.name == friend!.name
						&& $0.friend!.address!.asStringUriOnly() == core.defaultAccount!.contactAddress!.asStringUriOnly()
					}) ?? ContactAvatarModel(friend: nil, name: userNameTmp, address: addressTmp, withPresence: false)
					: ContactAvatarModel(friend: nil, name: userNameTmp, address: addressTmp, withPresence: false)
					
					if core.videoEnabled && !core.videoPreviewEnabled {
						DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
							core.videoPreviewEnabled = true
							self.videoDisplayed = true
						}
					}
					
					core.micEnabled = true
					
					let micMuttedTmp = !core.micEnabled
					
					let timeInterval = TimeInterval(conf!.dateTime)
					let date = Date(timeIntervalSince1970: timeInterval)
					let dateFormatter = DateFormatter()
					dateFormatter.dateStyle = .full
					dateFormatter.timeStyle = .none
					let dateTmp = dateFormatter.string(from: date)
					
					let timeFormatter = DateFormatter()
					timeFormatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
					let timeTmp = timeFormatter.string(from: date)
					
					let timeBisInterval = TimeInterval(conf!.dateTime + (Int(conf!.duration) * 60))
					let timeBis = Date(timeIntervalSince1970: timeBisInterval)
					let timeBisTmp = timeFormatter.string(from: timeBis)
					
					let meetingDateTmp = "\(dateTmp) | \(timeTmp) - \(timeBisTmp)"
					
					DispatchQueue.main.async {
						if self.telecomManager.meetingWaitingRoomName.isEmpty || self.telecomManager.meetingWaitingRoomName != confNameTmp {
							self.telecomManager.meetingWaitingRoomName = confNameTmp
						}
						
						self.userName = userNameTmp
						self.avatarModel = avatarModelTmp
						self.micMutted = micMuttedTmp
						self.meetingDate = meetingDateTmp
					}
				}
			}
		}
	}
	
	func enableVideoPreview() {
		self.coreContext.doOnCoreQueue { core in
			if core.videoEnabled {
				DispatchQueue.main.async {
					self.videoDisplayed = true
				}
				core.videoPreviewEnabled = true
			}
		}
	}
	
	func disableVideoPreview() {
		coreContext.doOnCoreQueue { core in
			if core.videoEnabled {
				DispatchQueue.main.async {
					self.videoDisplayed = false
				}
				core.videoPreviewEnabled = false
			}
		}
	}
	
	func switchCamera() {
		coreContext.doOnCoreQueue { core in
			let currentDevice = core.videoDevice
			Log.info("[CallViewModel] Current camera device is \(currentDevice ?? "nil")")
			
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
	
	func enableMicrophone() {
		self.micMutted = false
	}
	
	func toggleMuteMicrophone() {
		self.micMutted = !self.micMutted
	}
	
	func enableAVAudioSession() {
		do {
			try AVAudioSession.sharedInstance().setActive(true)
		} catch _ {
			
		}
	}
	
	func disableAVAudioSession() {
		do {
			try AVAudioSession.sharedInstance().setActive(false)
		} catch _ {
			
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
	
	func joinMeeting() {
		if self.telecomManager.meetingWaitingRoomSelected != nil {
			if self.micMutted {
				coreContext.doOnCoreQueue { core in
					core.micEnabled = false
				}
			}
			
			let audioSession = imageAudioRoute
			
			telecomManager.doCallWithCore(
				addr: self.telecomManager.meetingWaitingRoomSelected!, isVideo: self.videoDisplayed, isConference: true
			)
			
			DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
				switch audioSession {
				case "bluetooth":
					do {
						try AVAudioSession.sharedInstance().overrideOutputAudioPort(.none)
						try AVAudioSession.sharedInstance().setPreferredInput(AVAudioSession.sharedInstance().availableInputs?
							.filter({ $0.portType.rawValue.contains("Bluetooth") }).first)
					} catch _ {
						
					}
				case "speaker-high":
					do {
						try AVAudioSession.sharedInstance().overrideOutputAudioPort(.speaker)
					} catch _ {
						
					}
				default:
					do {
						try AVAudioSession.sharedInstance().overrideOutputAudioPort(.none)
						if self.isHeadPhoneAvailable() {
							try AVAudioSession.sharedInstance().setPreferredInput(AVAudioSession.sharedInstance()
								.availableInputs?.filter({ $0.portType.rawValue.contains("Receiver") }).first)
						} else {
							try AVAudioSession.sharedInstance().setPreferredInput(AVAudioSession.sharedInstance().availableInputs?.first)
						}
					} catch _ {
						
					}
				}
			}
		}
	}
	
	func cancelMeeting() {
		coreContext.doOnCoreQueue { core in
			if core.currentCall != nil {
				self.telecomManager.terminateCall(call: core.currentCall!)
			}
		}
	}
}
