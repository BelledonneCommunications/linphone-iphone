/*
 * Copyright (c) 2010-2021 Belledonne Communications SARL.
 *
 * This file is part of linphone-android
 * (see https://www.linphone.org).
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
 * aDouble with this program. If not, see <http://www.gnu.org/licenses/>.
 */


import Foundation
import linphonesw

class ConferenceWaitingRoomViewModel: ControlsViewModel {
	
	
	static let sharedModel = ConferenceWaitingRoomViewModel()

	
	let joinLayout = MutableLiveData<ConferenceDisplayMode>()
	let joinInProgress = MutableLiveData<Bool>(false)
	let showLayoutPicker = MutableLiveData<Bool>()
	
	
	override init() {
		super.init()
		self.reset()
	}
	
	func reset() {
		joinLayout.value = Core.get().defaultConferenceLayout == .Grid ? .Grid : .ActiveSpeaker
		joinInProgress.value = false
		isMicrophoneMuted.value = !micAuthorized()
		isMuteMicrophoneEnabled.value = true
		isSpeakerSelected.value = true
		isVideoEnabled.value = false
		isVideoAvailable.value = core.videoCaptureEnabled
		showLayoutPicker.value = false
	}
	
	override func toggleMuteMicrophone() {
		if (!micAuthorized()) {
			AVAudioSession.sharedInstance().requestRecordPermission { granted in
				if granted {
					self.isMicrophoneMuted.value = self.isMicrophoneMuted.value != true
				}
			}
		}
		self.isMicrophoneMuted.value = self.isMicrophoneMuted.value != true
	}
	
	override func toggleSpeaker() {
		isSpeakerSelected.value = isSpeakerSelected.value != true
	}
	
	override func toggleVideo() {
		isVideoEnabled.value = isVideoEnabled.value != true
	}
	
	override func updateUI() {
		
	}
	
}
