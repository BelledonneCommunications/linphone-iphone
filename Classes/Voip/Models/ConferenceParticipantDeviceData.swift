/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linhome
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

import linphonesw
import Foundation

class ConferenceParticipantDeviceData  {
	
	var participantDevice:ParticipantDevice
	let isMe:Bool
	
	let videoEnabled = MutableLiveData<Bool>()
	let activeSpeaker = MutableLiveData<Bool>()
	let isInConference = MutableLiveData<Bool>()
	let core = Core.get()

	private var participantDeviceDelegate :  ParticipantDeviceDelegate?
	
	init (participantDevice:ParticipantDevice, isMe:Bool)  {
		self.isMe = isMe
		self.participantDevice = participantDevice
		participantDeviceDelegate = ParticipantDeviceDelegateStub(
			onIsSpeakingChanged: { (participantDevice, isSpeaking) in
				Log.i("[Conference Participant Device] Participant \(participantDevice ) isspeaking = \(isSpeaking)")
				self.activeSpeaker.value = isSpeaking
			}, onConferenceJoined: { (participantDevice) in
				Log.i("[Conference Participant Device] Participant \(participantDevice)  has joined the conference")
				self.isInConference.value = true
			}, onConferenceLeft: { (participantDevice) in
				Log.i("[Conference Participant Device] Participant \(participantDevice)  has left the conference")
				self.isInConference.value = false
			}, onAudioDirectionChanged: { (participantDevice, direction) in
				Log.i("[Conference Participant Device] Participant \(participantDevice) audio stream direction changed: \(direction)")
			}, onVideoDirectionChanged: { (participantDevice, direction) in
				Log.i("[Conference Participant Device] Participant \(participantDevice) video stream direction changed: \(direction)")
				self.videoEnabled.value = direction == MediaDirection.SendOnly || direction == MediaDirection.SendRecv
			}, onTextDirectionChanged: { (participantDevice, direction) in
				Log.i("[Conference Participant Device] Participant \(participantDevice)  text stream direction changed: \(direction)")
			})
		
		participantDevice.addDelegate(delegate: participantDeviceDelegate!)
		activeSpeaker.value = false
		
		// TODO: What happens if we have disabled video locally?
		videoEnabled.value = participantDevice.videoDirection == MediaDirection.SendOnly || participantDevice.videoDirection == MediaDirection.SendRecv
		isInConference.value = participantDevice.isInConference
		
	}
	
	func destroy() {
		participantDevice.removeDelegate(delegate: participantDeviceDelegate!)
	}
	
	func switchCamera() {
		Core.get().toggleCamera()
	}
	
	func isSwitchCameraAvailable() -> Bool {
		return isMe && Core.get().showSwitchCameraButton()
	}
	
	func setVideoView(view:UIView) {
		if (!isMe && participantDevice.videoDirection != MediaDirection.SendRecv) {
			Log.e("[Conference Participant Device] Participant \(participantDevice) device video direction is \(participantDevice.videoDirection), don't set video window!")
			return
		}
		Log.i("[Conference Participant Device] Setting textureView \(view) for participant \(participantDevice)")
		if (isMe) { // TODO: remove
			core.nativePreviewWindow = view
		} else {
			participantDevice.nativeVideoWindowId = UnsafeMutableRawPointer(Unmanaged.passRetained(view).toOpaque())
		}
	}
}
