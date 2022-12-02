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


import Foundation
import linphonesw
import AVFoundation


class ControlsViewModel {
	var core : Core { get { Core.get() } }

	let isSpeakerSelected = MutableLiveData<Bool>()
	let isMicrophoneMuted = MutableLiveData<Bool>()
	let isMuteMicrophoneEnabled = MutableLiveData<Bool>()
	let isBluetoothHeadsetSelected = MutableLiveData<Bool>()
	let isBluetoothHeadsetAvailable = MutableLiveData<Bool>()
	let nonEarpieceOutputAudioDevice = MutableLiveData<Bool>()
	let audioRoutesSelected = MutableLiveData<Bool>()
	let audioRoutesEnabled = MutableLiveData<Bool>()
	
	let isVideoUpdateInProgress = MutableLiveData<Bool>()
	let isVideoEnabled = MutableLiveData<Bool>()
	let isVideoAvailable = MutableLiveData<Bool>()
	
	let fullScreenMode = MutableLiveData(false)
	let numpadVisible = MutableLiveData(false)
	let callStatsVisible = MutableLiveData(false)
	let goToConferenceLayoutSettings = MutableLiveData(false)
	let goToConferenceParticipantsListEvent = MutableLiveData(false)
	let goToChatEvent = MutableLiveData(false)
	let goToCallsListEvent = MutableLiveData(false)
	let hideExtraButtons = MutableLiveData(true)
	
	let proximitySensorEnabled = MutableLiveData<Bool>()

	
	static let shared = ControlsViewModel()
	private var coreDelegate :  CoreDelegateStub?
	private var previousCallState = Call.State.Idle

	
	init ()  {
		coreDelegate = CoreDelegateStub(
			onCallStateChanged : { (core: Core, call: Call, state: Call.State, message:String) -> Void in
				Log.i("[Call Controls] Call state changed: \(call) : \(state)")
				if (state == Call.State.StreamsRunning) {
					self.isVideoUpdateInProgress.value = false
				}
				self.updateUI()
				self.setAudioRoutes(call,state)
				self.previousCallState = state
			},
			onAudioDeviceChanged : { (core: Core, audioDevice: AudioDevice) -> Void in
				Log.i("[Call Controls] Audio device changed: \(audioDevice.deviceName)")
				self.nonEarpieceOutputAudioDevice.value = audioDevice.type != AudioDeviceType.Microphone // on iOS Earpiece = Microphone
				self.updateSpeakerState()
				self.updateBluetoothHeadsetState()
			},
			onAudioDevicesListUpdated : { (core: Core) -> Void in
				self.isBluetoothHeadsetAvailable.value = !core.audioDevices.filter { [.Bluetooth,.BluetoothA2DP].contains($0.type)}.isEmpty
			}
		)
		Core.get().addDelegate(delegate: coreDelegate!)
		proximitySensorEnabled.value = shouldProximitySensorBeEnabled()
		isVideoEnabled.readCurrentAndObserve { _ in
			self.proximitySensorEnabled.value = self.shouldProximitySensorBeEnabled()
		}
		nonEarpieceOutputAudioDevice.readCurrentAndObserve { _ in
			self.proximitySensorEnabled.value = self.shouldProximitySensorBeEnabled()
		}
		proximitySensorEnabled.readCurrentAndObserve { (enabled) in
			UIDevice.current.isProximityMonitoringEnabled = enabled == true
		}
		updateUI()
		ConferenceViewModel.shared.conferenceDisplayMode.readCurrentAndObserve { _ in
			self.updateVideoAvailable()
		}
		self.isBluetoothHeadsetAvailable.value = !core.audioDevices.filter { [.Bluetooth,.BluetoothA2DP].contains($0.type)}.isEmpty
	}
	
	private func setAudioRoutes(_ call:Call,_ state:Call.State) {
		if (state == .OutgoingProgress) {
			if (core.callsNb == 1 && ConfigManager.instance().lpConfigBoolForKey(key: "route_audio_to_bluetooth_if_available",defaultValue:true)) {
				AudioRouteUtils.routeAudioToBluetooth(call: call)
			}
		}
		if (state == .StreamsRunning) {
			if (core.callsNb == 1) {
				// Only try to route bluetooth / headphone / headset when the call is in StreamsRunning for the first time
				if (previousCallState == Call.State.Connected) {
					Log.i("[Context] First call going into StreamsRunning state for the first time, trying to route audio to headset or bluetooth if available")
					if (AudioRouteUtils.isHeadsetAudioRouteAvailable()) {
						AudioRouteUtils.routeAudioToHeadset(call: call)
					} else if (ConfigManager.instance().lpConfigBoolForKey(key: "route_audio_to_bluetooth_if_available",defaultValue:true) &&
							   AudioRouteUtils.isBluetoothAudioRouteAvailable()) {
						AudioRouteUtils.routeAudioToBluetooth(call: call)
					}
				}
			}
			
			if (ConfigManager.instance().lpConfigBoolForKey(key: "route_audio_to_speaker_when_video_enabled",defaultValue:true) && call.currentParams?.videoEnabled == true && call.conference == nil) {
				// Do not turn speaker on when video is enabled if headset or bluetooth is used
				if (!AudioRouteUtils.isHeadsetAudioRouteAvailable() &&
					!AudioRouteUtils.isBluetoothAudioRouteCurrentlyUsed(call: call)
				) {
					Log.i("[Context] Video enabled and no wired headset not bluetooth in use, routing audio to speaker")
					AudioRouteUtils.routeAudioToSpeaker(call: call)
				}
			}
		}
	}

	
	private func shouldProximitySensorBeEnabled() -> Bool {
		return core.callsNb > 0 && isVideoEnabled.value != true && nonEarpieceOutputAudioDevice.value != true
	}
	

	func hangUp() {
		if (core.currentCall != nil) {
			try?core.currentCall?.terminate()
		} else if (core.conference?.isIn == true) {
			try?core.terminateConference()
		} else {
			try?core.terminateAllCalls()
		}
	}
	
	func toggleVideo() {
		if let currentCall = core.currentCall {
			if (currentCall.conference != nil) {
				if (ConferenceViewModel.shared.conferenceDisplayMode.value == .AudioOnly) {
					ConferenceViewModel.shared.changeLayout(layout: .ActiveSpeaker, sendVideo:true)
				} else if let params = try?core.createCallParams(call: currentCall) {
					isVideoUpdateInProgress.value = true
					params.videoDirection = params.videoDirection == MediaDirection.RecvOnly ? MediaDirection.SendRecv : MediaDirection.RecvOnly
					try?currentCall.update(params: params)
				}
			} else {
				let state = currentCall.state
				if (state == Call.State.End || state == Call.State.Released || state == Call.State.Error) {
					return
				}
				isVideoUpdateInProgress.value = true
				if let params = try?core.createCallParams(call: currentCall) {
					params.videoEnabled = !(currentCall.currentParams?.videoEnabled == true)
					try?currentCall.update(params: params)
					if (params.videoEnabled) {
						currentCall.requestNotifyNextVideoFrameDecoded()
					}
				}
			}
		}
	}
	
	
	func updateUI() {
		updateVideoAvailable()
		updateVideoEnabled()
		updateMicState()
		updateSpeakerState()
		updateAudioRoutesState()
		proximitySensorEnabled.value = shouldProximitySensorBeEnabled()
	}
	
	private func updateAudioRoutesState() {
		let bluetoothDeviceAvailable = AudioRouteUtils.isBluetoothAudioRouteAvailable()
		audioRoutesEnabled.value = bluetoothDeviceAvailable
		
		if (!bluetoothDeviceAvailable) {
			audioRoutesSelected.value = false
			audioRoutesEnabled.value = false
		}
	}
	
	private func updateSpeakerState() {
		isSpeakerSelected.value = AudioRouteUtils.isSpeakerAudioRouteCurrentlyUsed()
	}
	
	private func updateBluetoothHeadsetState() {
		isBluetoothHeadsetSelected.value = AudioRouteUtils.isBluetoothAudioRouteCurrentlyUsed()
	}
	
	private func updateVideoAvailable() {
		let currentCall = core.currentCall
		isVideoAvailable.value =
			(core.videoCaptureEnabled || core.videoPreviewEnabled) &&
			currentCall?.state != .Paused &&
			currentCall?.state != .PausedByRemote &&
		((currentCall != nil && currentCall?.mediaInProgress() != true) || (core.conference?.isIn == true))
	}
	
	private func updateVideoEnabled() {
		let enabled = isVideoCallOrConferenceActive()
		isVideoEnabled.value = enabled
	}
	
	func updateMicState() {
		isMicrophoneMuted.value = !micAuthorized() || !core.micEnabled
		isMuteMicrophoneEnabled.value = CallsViewModel.shared.currentCallData.value??.call != nil
	}
	
	func micAuthorized() -> Bool {
		return AVCaptureDevice.authorizationStatus(for: .audio) == .authorized
	}
	
	func isVideoCallOrConferenceActive() -> Bool {
		if let currentCall = core.currentCall, let params = currentCall.params {
			return currentCall.state != .PausedByRemote &&  params.videoEnabled && (currentCall.conference  == nil || params.videoDirection == MediaDirection.SendRecv)
		} else {
			return false
		}
	}
	
	func toggleFullScreen() {
		ControlsViewModel.shared.audioRoutesSelected.value = false
		fullScreenMode.value = fullScreenMode.value != true
	}
	
	func toggleMuteMicrophone() {
		if (!micAuthorized()) {
			AVAudioSession.sharedInstance().requestRecordPermission { granted in
				if granted {
					self.core.micEnabled = !self.core.micEnabled
					self.updateMicState()
				}
			}
		}
		core.micEnabled = !core.micEnabled
		updateMicState()
	}
	
	func forceEarpieceAudioRoute() {
		if (AudioRouteUtils.isHeadsetAudioRouteAvailable()) {
			Log.i("[Call Controls] Headset found, route audio to it instead of earpiece")
			AudioRouteUtils.routeAudioToHeadset()
		} else {
			AudioRouteUtils.routeAudioToEarpiece()
		}
	}
	
	func forceSpeakerAudioRoute() {
		AudioRouteUtils.routeAudioToSpeaker()
	}
	
	func forceBluetoothAudioRoute() {
		AudioRouteUtils.routeAudioToBluetooth()
	}
	
	func toggleSpeaker() {
		if (AudioRouteUtils.isSpeakerAudioRouteCurrentlyUsed()) {
			forceEarpieceAudioRoute()
		} else {
			forceSpeakerAudioRoute()
		}
	}
	
	func toggleRoutesMenu() {
		audioRoutesSelected.value = audioRoutesSelected.value != true
	}
	
}

@objc class ControlsViewModelBridge: NSObject {
	@objc static func showParticipants() {
		ControlsViewModel.shared.goToConferenceParticipantsListEvent.value = true
	}
	@objc static func toggleStatsVisibility() -> Void {
			if (ControlsViewModel.shared.callStatsVisible.value == true) {
					ControlsViewModel.shared.callStatsVisible.value = false
			} else {
					ControlsViewModel.shared.callStatsVisible.value = true
			}
	}
}
