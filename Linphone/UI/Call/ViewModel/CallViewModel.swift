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

class CallViewModel: ObservableObject {
	
	var coreContext = CoreContext.shared
	var telecomManager = TelecomManager.shared
	
	@Published var displayName: String = "Example Linphone"
	@Published var direction: Call.Dir = .Outgoing
	@Published var remoteAddressString: String = "example.linphone@sip.linphone.org"
	@Published var remoteAddress: Address?
	@Published var avatarModel: ContactAvatarModel?
	@Published var audioSessionImage: String = ""
	@State var micMutted: Bool = false
	@State var timeElapsed: Int = 0
	
	let timer = Timer.publish(every: 1, on: .main, in: .common).autoconnect()
	
	init() {
		setupNotifications()
		coreContext.doOnCoreQueue { core in
			if core.currentCall != nil && core.currentCall!.remoteAddress != nil {
				DispatchQueue.main.async {
					self.direction = .Incoming
					self.remoteAddressString = String(core.currentCall!.remoteAddress!.asStringUriOnly().dropFirst(4))
					self.remoteAddress = core.currentCall!.remoteAddress!
					
					let friend = ContactsManager.shared.getFriendWithAddress(address: core.currentCall!.remoteAddress!)
					if friend != nil && friend!.address != nil && friend!.address!.displayName != nil {
						self.displayName = friend!.address!.displayName!
					} else {
						if core.currentCall!.remoteAddress!.displayName != nil {
							self.displayName = core.currentCall!.remoteAddress!.displayName!
						} else if core.currentCall!.remoteAddress!.username != nil {
							self.displayName = core.currentCall!.remoteAddress!.username!
						}
					}
				}
			}
		}
	}
	
	func terminateCall() {
		withAnimation {
			telecomManager.callInProgress = false
			telecomManager.callStarted = false
		}
		
		coreContext.doOnCoreQueue { core in
			if core.currentCall != nil {
				self.telecomManager.terminateCall(call: core.currentCall!)
			}
		}
		
		timer.upstream.connect().cancel()
	}
	
	func acceptCall() {
		withAnimation {
			telecomManager.callInProgress = true
			telecomManager.callStarted = true
		}
		
		coreContext.doOnCoreQueue { core in
			if core.currentCall != nil {
				self.telecomManager.acceptCall(core: core, call: core.currentCall!, hasVideo: false)
			}
		}
		
		timer.upstream.connect().cancel()
	}
	
	func muteCall() {
		coreContext.doOnCoreQueue { core in
			if core.currentCall != nil {
				self.micMutted = !self.micMutted
				core.currentCall!.microphoneMuted = self.micMutted
			}
		}
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
	
	func setupNotifications() {
		/*
		notifCenter.addObserver(self,
					   selector: #selector(handleRouteChange),
					   name: AVAudioSession.routeChangeNotification,
					   object: nil)
		*/
		
		//NotificationCenter.default.addObserver(self, selector: Selector(("handleRouteChange")), name: UITextView.textDidChangeNotification, object: nil)
	}


	func handleRouteChange(notification: Notification) {
		guard let userInfo = notification.userInfo,
			let reasonValue = userInfo[AVAudioSessionRouteChangeReasonKey] as? UInt,
			let reason = AVAudioSession.RouteChangeReason(rawValue: reasonValue) else {
				return
		}
		
		// Switch over the route change reason.
		switch reason {


		case .newDeviceAvailable, .oldDeviceUnavailable: // New device found.
			print("handleRouteChangehandleRouteChange handleRouteChange")
			
			AVAudioSession.sharedInstance().currentRoute.outputs.filter({ $0.portType.rawValue == "Speaker" }).isEmpty
			? (
				AVAudioSession.sharedInstance().currentRoute.outputs.filter({ $0.portType.rawValue.contains("Bluetooth") }).isEmpty
				? (
					AVAudioSession.sharedInstance().currentRoute.outputs.filter({ $0.portType.rawValue == "Receiver" }).isEmpty
					? "headset"
					: "speaker-slash"
				)
				: "bluetooth"
			)
			: "speaker-high"
		
			/*
		case .oldDeviceUnavailable: // Old device removed.
			if let previousRoute =
				userInfo[AVAudioSessionRouteChangePreviousRouteKey] as? AVAudioSessionRouteDescription {
				
			}
		*/
		default: ()
		}
	}


	func hasHeadphones(in routeDescription: AVAudioSessionRouteDescription) -> Bool {
		// Filter the outputs to only those with a port type of headphones.
		return !routeDescription.outputs.filter({$0.portType == .headphones}).isEmpty
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
}
