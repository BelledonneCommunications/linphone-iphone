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
	let isSpeaking = MutableLiveData<Bool>()
	let micMuted = MutableLiveData<Bool>()
	
	let isInConference = MutableLiveData<Bool>()
	
	let isJoining = MutableLiveData<Bool>()
	
	
	var core : Core { get { Core.get() } }
	
	private var participantDeviceDelegate :  ParticipantDeviceDelegate?
	
	init (participantDevice:ParticipantDevice, isMe:Bool)  {
		self.isMe = isMe
		self.participantDevice = participantDevice
		participantDeviceDelegate = ParticipantDeviceDelegateStub(
			onIsSpeakingChanged: { (participantDevice, isSpeaking) in
				Log.i("[Conference Participant Device] Participant \(participantDevice.address?.asStringUriOnly()) isspeaking = \(isSpeaking)")
				self.isSpeaking.value = isSpeaking
			},
			onIsMuted: { (participantDevice, isMuted) in
				Log.i("[Conference Participant Device] Participant \(participantDevice.address?.asStringUriOnly()) muted = \(isMuted)")
				self.micMuted.value = isMuted
			},
			onStateChanged: { (participantDevice, state) in
				Log.i("[Conference Participant Device] Participant \(participantDevice.address?.asStringUriOnly()) state has changed: \(state)")
				if ([.Joining,.Alerting].contains(state)) {
					self.isJoining.value = true
				} else if (state == .OnHold) {
					self.isInConference.value = false
				} else if (state == .Present) {
					self.isJoining.value = false
					self.isInConference.value = true
				}
			},
			onStreamCapabilityChanged: { (participantDevice, direction, streamType) in
				Log.i("[Conference Participant Device] Participant \(participantDevice.address?.asStringUriOnly()) video stream direction changed: \(direction)")
				self.videoEnabled.value = direction == MediaDirection.SendOnly || direction == MediaDirection.SendRecv
				if (streamType == StreamType.Video) {
					Log.i("[Conference Participant Device] Participant [\(participantDevice.address?.asStringUriOnly())] video capability changed to \(direction)")
				}
			},
			onStreamAvailabilityChanged: { (participantDevice, available, streamType) in
				if (streamType == StreamType.Video) {
					Log.i("[Conference Participant Device] Participant [\(participantDevice.address?.asStringUriOnly())] video availability changed to \(available)")
					self.videoEnabled.value = available
				}
			}
			
		)
		
		participantDevice.addDelegate(delegate: participantDeviceDelegate!)
		isSpeaking.value = false
		micMuted.value = participantDevice.isMuted
		
		videoEnabled.value = participantDevice.getStreamAvailability(streamType: .Video)
		
		isInConference.value = participantDevice.isInConference
		let videoCapability = participantDevice.getStreamCapability(streamType: .Video)
		
		isJoining.value = [.Joining,.Alerting].contains(participantDevice.state)
		
		Log.i("[Conference Participant Device] Participant [\(participantDevice.address?.asStringUriOnly())], is in conf? \(isInConference.value), is video enabled? \(videoEnabled.value) \(videoCapability)")
	}
	
	func destroy() {
		clearObservers()
		participantDevice.removeDelegate(delegate: participantDeviceDelegate!)
	}
	
	func clearObservers() {
		isInConference.clearObservers()
		videoEnabled.clearObservers()
		isSpeaking.clearObservers()
		isJoining.clearObservers()
		micMuted.clearObservers()
	}
	
	func switchCamera() {
		Core.get().toggleCamera()
	}
	
	func isSwitchCameraAvailable() -> Bool {
		return isMe && Core.get().showSwitchCameraButton()
	}
	
	func setVideoView(view:UIView) {
		Log.i("[Conference Participant Device] Setting textureView \(view) for participant \(participantDevice.address?.asStringUriOnly())")
		view.contentMode = .scaleAspectFill
		if (isMe) {
			core.usePreviewWindow(yesno: false)
			core.nativePreviewWindow = view
		} else {
			participantDevice.nativeVideoWindowId = UnsafeMutableRawPointer(Unmanaged.passRetained(view).toOpaque())
		}
	}
}
