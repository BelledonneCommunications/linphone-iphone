/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
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
// swiftlint:disable line_length

import Foundation
import AVFoundation
import linphonesw

class AudioRouteUtils {
	
	static private func applyAudioRouteChange( core: Core, call: Call?, types: [AudioDevice.Kind], output: Bool = true) {
		let typesNames = types.map { String(describing: $0) }.joined(separator: "/")
		
		let currentCall = core.callsNb > 0 ? (call != nil) ? call : core.currentCall != nil ? core.currentCall : core.calls[0] : nil
		if currentCall == nil {
			print("[Audio Route Helper] No call found, setting audio route on Core")
		}
		let conference = call?.conference
		let capability = output ? AudioDevice.Capabilities.CapabilityPlay : AudioDevice.Capabilities.CapabilityRecord
		
		var found = false
		
		core.audioDevices.forEach { (audioDevice) in
			print("[Audio Route Helper] registered core audio devices are : [\(audioDevice.deviceName)] [\(audioDevice.type)] [\(audioDevice.capabilities)] ")
		}
		
		core.audioDevices.forEach { (audioDevice) in
			if !found && types.contains(audioDevice.type) && audioDevice.hasCapability(capability: capability) {
				if conference != nil && conference?.isIn == true {
					print("[Audio Route Helper] Found [\(audioDevice.type)] \(output ?  "playback" : "recorder") audio device [\(audioDevice.deviceName)], routing conference audio to it")
					if output {
						conference?.outputAudioDevice = audioDevice
					} else {
						conference?.inputAudioDevice = audioDevice
					}
				} else if currentCall != nil {
					print("[Audio Route Helper] Found [\(audioDevice.type)] \(output ?  "playback" : "recorder") audio device [\(audioDevice.deviceName)], routing call audio to it")
					if output {
						currentCall?.outputAudioDevice = audioDevice
					} else {
						currentCall?.inputAudioDevice = audioDevice
					}
				} else {
					print("[Audio Route Helper] Found [\(audioDevice.type)] \(output ?  "playback" : "recorder") audio device [\(audioDevice.deviceName)], changing core default audio device")
					if output {
						core.outputAudioDevice = audioDevice
					} else {
						core.inputAudioDevice = audioDevice
					}
				}
				found = true
			}
		}
		if !found {
			print("[Audio Route Helper] Couldn't find \(typesNames) audio device")
		}
	}
	
	static private func changeCaptureDeviceToMatchAudioRoute(core: Core, call: Call?, types: [AudioDevice.Kind]) {
		switch types.first {
		case .Bluetooth: if isBluetoothAudioRecorderAvailable(core: core) {
			print("[Audio Route Helper] Bluetooth device is able to record audio, also change input audio device")
			applyAudioRouteChange(core: core, call: call, types: [AudioDevice.Kind.Bluetooth], output: false)
		}
		case .Headset, .Headphones: if isHeadsetAudioRecorderAvailable(core: core) {
			print("[Audio Route Helper] Headphones/headset device is able to record audio, also change input audio device")
			applyAudioRouteChange(core: core, call: call, types: [AudioDevice.Kind.Headphones, AudioDevice.Kind.Headset], output: false)
		}
		default: applyAudioRouteChange(core: core, call: call, types: [AudioDevice.Kind.Microphone], output: false)
		}
	}
	
	static private func routeAudioTo(core: Core, call: Call?, types: [AudioDevice.Kind]) {
		let currentCall = call != nil ? call : core.currentCall != nil ? core.currentCall : (core.callsNb > 0 ? core.calls[0] : nil)
		if call != nil || currentCall != nil {
			let callToUse = call != nil ? call : currentCall
			applyAudioRouteChange(core: core, call: callToUse, types: types)
			changeCaptureDeviceToMatchAudioRoute(core: core, call: callToUse, types: types)
		} else {
			applyAudioRouteChange(core: core, call: call, types: types)
			changeCaptureDeviceToMatchAudioRoute(core: core, call: call, types: types)
		}
	}
	
	static func routeAudioToEarpiece(core: Core, call: Call? = nil) {
		routeAudioTo(core: core, call: call, types: [AudioDevice.Kind.Microphone]) // on iOS Earpiece = Microphone
	}
	
	static func routeAudioToSpeaker(core: Core, call: Call? = nil) {
		routeAudioTo(core: core, call: call, types: [AudioDevice.Kind.Speaker])
	}
	
	static func routeAudioToSpeaker(core: Core) {
		routeAudioTo(core: core, call: nil, types: [AudioDevice.Kind.Speaker])
	}
	
	static func routeAudioToBluetooth(core: Core, call: Call? = nil) {
		routeAudioTo(core: core, call: call, types: [AudioDevice.Kind.Bluetooth])
	}
	
	static func routeAudioToHeadset(core: Core, call: Call? = nil) {
		routeAudioTo(core: core, call: call, types: [AudioDevice.Kind.Headphones, AudioDevice.Kind.Headset])
	}
	
	static func isSpeakerAudioRouteCurrentlyUsed(core: Core, call: Call? = nil) -> Bool {
		
		let currentCall = core.callsNb > 0 ? (call != nil) ? call : core.currentCall != nil ? core.currentCall : core.calls[0] : nil
		if currentCall == nil {
			print("[Audio Route Helper] No call found, setting audio route on Core")
		}
		
		let audioDevice = currentCall != nil ? currentCall!.outputAudioDevice : core.outputAudioDevice
		print("[Audio Route Helper] Playback audio currently in use is [\(audioDevice?.deviceName ?? "n/a")] with type (\(audioDevice?.type ?? .Unknown)")
		return audioDevice?.type == AudioDevice.Kind.Speaker
	}
	
	static func isBluetoothAudioRouteCurrentlyUsed(core: Core, call: Call? = nil) -> Bool {
		if core.callsNb == 0 {
			print("[Audio Route Helper] No call found, so bluetooth audio route isn't used")
			return false
		}
		let currentCall = call != nil  ? call : core.currentCall != nil ? core.currentCall : core.calls[0]
		let audioDevice = currentCall != nil ? currentCall!.outputAudioDevice : core.outputAudioDevice
		print("[Audio Route Helper] Playback audio device currently in use is [\(audioDevice?.deviceName ?? "n/a")] with type (\(audioDevice?.type  ?? .Unknown)")
		return audioDevice?.type == AudioDevice.Kind.Bluetooth
	}
	
	static func isBluetoothAudioRouteAvailable(core: Core) -> Bool {
		if let device = core.audioDevices.first(where: { $0.type == AudioDevice.Kind.Bluetooth &&  $0.hasCapability(capability: .CapabilityPlay) }) {
			print("[Audio Route Helper] Found bluetooth audio device [\(device.deviceName)]")
			return true
		}
		return false
	}
	
	static private func isBluetoothAudioRecorderAvailable(core: Core) -> Bool {
		if let device = core.audioDevices.first(where: { $0.type == AudioDevice.Kind.Bluetooth &&  $0.hasCapability(capability: .CapabilityRecord) }) {
			print("[Audio Route Helper] Found bluetooth audio recorder [\(device.deviceName)]")
			return true
		}
		return false
	}
	
	static func isHeadsetAudioRouteAvailable(core: Core) -> Bool {
		if let device = core.audioDevices.first(where: { ($0.type == AudioDevice.Kind.Headset||$0.type == AudioDevice.Kind.Headphones) &&  $0.hasCapability(capability: .CapabilityPlay) }) {
			print("[Audio Route Helper] Found headset/headphones audio device  [\(device.deviceName)]")
			return true
		}
		return false
	}
	
	static private func isHeadsetAudioRecorderAvailable(core: Core) -> Bool {
		if let device = core.audioDevices.first(where: { ($0.type == AudioDevice.Kind.Headset||$0.type == AudioDevice.Kind.Headphones) &&  $0.hasCapability(capability: .CapabilityRecord) }) {
			print("[Audio Route Helper] Found headset/headphones audio recorder  [\(device.deviceName)]")
			return true
		}
		return false
	}
	
	static func isReceiverEnabled(core: Core) -> Bool {
		if let outputDevice = core.outputAudioDevice {
			return outputDevice.type == AudioDevice.Kind.Microphone
		}
		return false
	}
	
	static func isBluetoothAvailable(core: Core) -> Bool {
		for device in core.audioDevices {
			if device.type == AudioDevice.Kind.Bluetooth || device.type == AudioDevice.Kind.BluetoothA2DP {
				return true
			}
		}
		return false
	}
	
}
// swiftlint:enable line_length
