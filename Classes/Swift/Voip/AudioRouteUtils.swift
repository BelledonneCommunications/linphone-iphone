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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

import Foundation
import AVFoundation
import linphonesw

@objc class AudioRouteUtils : NSObject {
	
	static var core : Core { get { Core.get() } }

	static private func applyAudioRouteChange( call: Call?, types: [AudioDeviceType], output: Bool = true) {
		let typesNames = types.map { String(describing: $0) }.joined(separator: "/")
		
		let currentCall = core.callsNb > 0 ? (call != nil) ? call : core.currentCall != nil ? core.currentCall : core.calls[0] : nil
		if (currentCall == nil) {
			Log.w("[Audio Route Helper] No call found, setting audio route on Core")
		}
		let conference = core.conference
		let capability = output ? AudioDeviceCapabilities.CapabilityPlay : AudioDeviceCapabilities.CapabilityRecord

		var found = false
		
		core.audioDevices.forEach { (audioDevice) in
			Log.i("[Audio Route Helper] registered coe audio devices are : [\(audioDevice.deviceName)] [\(audioDevice.type)] [\(audioDevice.capabilities)] ")
		}

		core.audioDevices.forEach { (audioDevice) in
			if (!found && types.contains(audioDevice.type) && audioDevice.hasCapability(capability: capability)) {
				if (conference != nil && conference?.isIn == true) {
					Log.i("[Audio Route Helper] Found [\(audioDevice.type)] \(output ?  "playback" : "recorder") audio device [\(audioDevice.deviceName)], routing conference audio to it")
					if (output) {
						conference?.outputAudioDevice = audioDevice
					} else {
						conference?.inputAudioDevice = audioDevice
					}
				} else if (currentCall != nil) {
					Log.i("[Audio Route Helper] Found [\(audioDevice.type)] \(output ?  "playback" : "recorder") audio device [\(audioDevice.deviceName)], routing call audio to it")
					if (output) {
						currentCall?.outputAudioDevice = audioDevice
					}
					else {
						currentCall?.inputAudioDevice = audioDevice
					}
				} else {
					Log.i("[Audio Route Helper] Found [\(audioDevice.type)] \(output ?  "playback" : "recorder") audio device [\(audioDevice.deviceName)], changing core default audio device")
					if (output) {
						core.outputAudioDevice = audioDevice
					} else {
						core.inputAudioDevice = audioDevice
					}
				}
				found = true
			}
		}
		if (!found) {
			Log.e("[Audio Route Helper] Couldn't find \(typesNames) audio device")
		}
	}
	
	static private func changeCaptureDeviceToMatchAudioRoute(call: Call?, types: [AudioDeviceType]) {
		switch (types.first) {
		case .Bluetooth :if (isBluetoothAudioRecorderAvailable()) {
			Log.i("[Audio Route Helper] Bluetooth device is able to record audio, also change input audio device")
			applyAudioRouteChange(call: call, types: [AudioDeviceType.Bluetooth], output: false)
		}
		case .Headset, .Headphones : if (isHeadsetAudioRecorderAvailable()) {
			Log.i("[Audio Route Helper] Headphones/headset device is able to record audio, also change input audio device")
			applyAudioRouteChange(call:call,types: [AudioDeviceType.Headphones, AudioDeviceType.Headset], output:false)
		}
		default: applyAudioRouteChange(call:call,types: [AudioDeviceType.Microphone], output:false)
		}
	}
	
	static private func routeAudioTo( call: Call?, types: [AudioDeviceType]) {
		let currentCall = call != nil ? call : core.currentCall != nil ? core.currentCall : (core.callsNb > 0 ? core.calls[0] : nil)
		if (call != nil || currentCall != nil) {
			let callToUse = call != nil ? call : currentCall
			applyAudioRouteChange(call: callToUse, types: types)
			changeCaptureDeviceToMatchAudioRoute(call: callToUse, types: types)
		} else {
			applyAudioRouteChange(call: call, types: types)
			changeCaptureDeviceToMatchAudioRoute(call: call, types: types)
		}
	}
	
	static func routeAudioToEarpiece(call: Call? = nil) {
		routeAudioTo(call: call, types: [AudioDeviceType.Microphone]) // on iOS Earpiece = Microphone
	}
	
	static func routeAudioToSpeaker(call: Call? = nil) {
		routeAudioTo(call: call, types: [AudioDeviceType.Speaker])
	}
	
	@objc static func routeAudioToSpeaker() {
		routeAudioTo(call: nil, types: [AudioDeviceType.Speaker])
	}
	
	static func routeAudioToBluetooth(call: Call? = nil) {
		routeAudioTo(call: call, types: [AudioDeviceType.Bluetooth])
	}
	
	static func routeAudioToHeadset(call: Call? = nil) {
		routeAudioTo(call: call, types: [AudioDeviceType.Headphones, AudioDeviceType.Headset])
	}
	
	static func isSpeakerAudioRouteCurrentlyUsed(call: Call? = nil) -> Bool {
		
		let currentCall = core.callsNb > 0 ? (call != nil) ? call : core.currentCall != nil ? core.currentCall : core.calls[0] : nil
		if (currentCall == nil) {
			Log.w("[Audio Route Helper] No call found, setting audio route on Core")
		}
		
		let conference = core.conference
		let audioDevice = conference != nil && conference?.isIn == true ? conference!.outputAudioDevice : currentCall != nil ? currentCall!.outputAudioDevice : core.outputAudioDevice
		Log.i("[Audio Route Helper] Playback audio currently in use is [\(audioDevice?.deviceName ?? "n/a")] with type (\(audioDevice?.type ?? .Unknown)")
		return audioDevice?.type == AudioDeviceType.Speaker
	}
	
	static func isBluetoothAudioRouteCurrentlyUsed(call: Call? = nil) -> Bool {
		if (core.callsNb == 0) {
			Log.w("[Audio Route Helper] No call found, so bluetooth audio route isn't used")
			return false
		}
		let currentCall = call != nil  ? call : core.currentCall != nil ? core.currentCall : core.calls[0]
		let conference = core.conference
		
		let audioDevice =  conference != nil && conference?.isIn == true ? conference!.outputAudioDevice : currentCall?.outputAudioDevice
		Log.i("[Audio Route Helper] Playback audio device currently in use is [\(audioDevice?.deviceName ?? "n/a")] with type (\(audioDevice?.type  ?? .Unknown)")
		return audioDevice?.type == AudioDeviceType.Bluetooth
	}
	
	static func isBluetoothAudioRouteAvailable() -> Bool {
		if let device = core.audioDevices.first(where: { $0.type == AudioDeviceType.Bluetooth &&  $0.hasCapability(capability: .CapabilityPlay) }) {
			Log.i("[Audio Route Helper] Found bluetooth audio device [\(device.deviceName)]")
			return true
		}
		return false
	}
	
	static private func isBluetoothAudioRecorderAvailable() -> Bool {
		if let device = core.audioDevices.first(where: { $0.type == AudioDeviceType.Bluetooth &&  $0.hasCapability(capability: .CapabilityRecord) }) {
			Log.i("[Audio Route Helper] Found bluetooth audio recorder [\(device.deviceName)]")
			return true
		}
		return false
	}
	
	static func isHeadsetAudioRouteAvailable() -> Bool {
		if let device = core.audioDevices.first(where: { ($0.type == AudioDeviceType.Headset||$0.type == AudioDeviceType.Headphones) &&  $0.hasCapability(capability: .CapabilityPlay) }) {
			Log.i("[Audio Route Helper] Found headset/headphones audio device  [\(device.deviceName)]")
			return true
		}
		return false
	}
	
	static private func isHeadsetAudioRecorderAvailable() -> Bool {
		if let device = core.audioDevices.first(where: { ($0.type == AudioDeviceType.Headset||$0.type == AudioDeviceType.Headphones) &&  $0.hasCapability(capability: .CapabilityRecord) }) {
			Log.i("[Audio Route Helper] Found headset/headphones audio recorder  [\(device.deviceName)]")
			return true
		}
		return false
	}
	

	
	static func isReceiverEnabled() -> Bool {
		if let outputDevice = core.outputAudioDevice {
			return outputDevice.type == AudioDeviceType.Microphone
		}
		return false
	}
	
}
