/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
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

import linphonesw
import SwiftUI

class SettingsViewModel: ObservableObject {
	
	static let TAG = "[SettingsViewModel]"
	
	private var coreDelegate: CoreDelegate?
	
	// Security settings
	@Published var enableVfs: Bool = false
	// @Published var preventScreenshots: Bool = false
	
	// Calls settings
	@Published var adaptiveRateControl: Bool = false
	@Published var enableVideo: Bool = false
	@Published var autoRecord: Bool = false
	
	// Conversations settings
	@Published var autoDownload: Bool = false
	
	// Contacts settings
	@Published var ldapServers: [String] = []
	@Published var cardDavFriendsLists: [String] = []
	
	// Meetings settings
	@Published var defaultLayout: String = ""
	
	// Network settings
	@Published var useWifiOnly: Bool = false
	@Published var allowIpv6: Bool = false
	
	// Advanced settings
	@Published var enableFec: Bool = false
	@Published var mediaEncryption: String = ""
	@Published var mediaEncryptionMandatory: Bool = false
	@Published var acceptEarlyMedia: Bool = false
	@Published var allowOutgoingEarlyMedia: Bool = false
	@Published var deviceId: String = ""
	@Published var uploadServerUrl: String = ""
	@Published var remoteProvisioningUrl: String = ""
	
	@Published var inputAudioDeviceLabels: [String] = []
	@Published var inputAudioDeviceValues: [AudioDevice] = []
	@Published var inputAudioDeviceIndex: Int = 0
	
	@Published var outputAudioDeviceLabels: [String] = []
	@Published var outputAudioDeviceValues: [AudioDevice] = []
	@Published var outputAudioDeviceIndex: Int = 0
	
	@Published var audioCodecs: [CodecModel] = []
	@Published var videoCodecs: [CodecModel] = []
	
	init() {
		CoreContext.shared.doOnCoreQueue { core in
			
			let enableVfsTmp = CorePreferences.vfsEnabled
			
			let adaptiveRateControlTmp = core.adaptiveRateControlEnabled
			let enableVideoTmp = core.videoEnabled
			let autoRecordTmp = CorePreferences.automaticallyStartCallRecording
			
			let autoDownloadTmp = core.maxSizeForAutoDownloadIncomingFiles == 0
			
			let defaultLayoutTmp = core.defaultConferenceLayout.rawValue == 0 ? String(localized: "settings_meetings_layout_mosaic_label") : String(localized: "settings_meetings_layout_active_speaker_label")
			
			let useWifiOnlyTmp = core.wifiOnlyEnabled
			let allowIpv6Tmp = core.ipv6Enabled
			
			// Advanced settings
			let enableFecTmp = core.fecEnabled
			var mediaEncryptionTmp = ""
			switch core.mediaEncryption {
			case .None:
				mediaEncryptionTmp = "None"
			case .SRTP:
				mediaEncryptionTmp = "SRTP"
			case .ZRTP:
				mediaEncryptionTmp = "ZRTP"
			case .DTLS:
				mediaEncryptionTmp = "DTLS"
			}
			let mediaEncryptionMandatoryTmp = core.isMediaEncryptionMandatory
			let acceptEarlyMediaTmp = CorePreferences.acceptEarlyMedia
			let allowOutgoingEarlyMediaTmp = CorePreferences.allowOutgoingEarlyMedia
			let deviceIdTmp = CorePreferences.deviceName
			let fileSharingServerUrlTmp = core.fileTransferServer
   			let remoteProvisioningUrlTmp = core.provisioningUri
			
			DispatchQueue.main.async {
				self.enableVfs = enableVfsTmp
				
				self.adaptiveRateControl = adaptiveRateControlTmp
				self.enableVideo = enableVideoTmp
				self.autoRecord = autoRecordTmp
				
				self.autoDownload = autoDownloadTmp
				
				self.defaultLayout = defaultLayoutTmp
				
				self.useWifiOnly = useWifiOnlyTmp
				self.allowIpv6 = allowIpv6Tmp
				
				// Advanced settings
				self.enableFec = enableFecTmp
				self.mediaEncryption = mediaEncryptionTmp
				self.mediaEncryptionMandatory = mediaEncryptionMandatoryTmp
				self.acceptEarlyMedia = acceptEarlyMediaTmp
				self.allowOutgoingEarlyMedia = allowOutgoingEarlyMediaTmp
				
				self.deviceId = deviceIdTmp
				self.uploadServerUrl = fileSharingServerUrlTmp ?? ""
				self.remoteProvisioningUrl = remoteProvisioningUrlTmp ?? ""
				
				/*
				self.setupAudioDevices()
				
				self.coreDelegate = CoreDelegateStub(
					onAudioDevicesListUpdated: { (_: Core) in
						Log.info(
							"\(SettingsViewModel.TAG) Audio devices list has changed, update available input/output audio devices list"
						)
						self.setupAudioDevices()
					}
				)
				core.addDelegate(delegate: self.coreDelegate!)
				*/
				
				self.reloadLdapServers()
				self.reloadConfiguredCardDavServers()
				self.setupCodecs()
			}
		}
	}
	
	deinit {
		if let delegate = coreDelegate {
			CoreContext.shared.doOnCoreQueue { core in
				core.removeDelegate(delegate: delegate)
			}
		}
	}
	
	func reloadLdapServers() {
		CoreContext.shared.doOnCoreQueue { core in
			var list: [String] = []

			core.ldapList.forEach({ ldap in
				let label = ldap.params?.server ?? ""
				if !label.isEmpty {
					list.append(label)
				}
			})

			DispatchQueue.main.async {
				self.ldapServers = list
			}
		}
	}
	
	func reloadConfiguredCardDavServers() {
		CoreContext.shared.doOnCoreQueue { core in
			var list: [String] = []
			
			core.friendsLists.forEach({ friendList in
				if friendList.type == .CardDAV {
					let label = friendList.displayName ?? friendList.uri ?? ""
					if !label.isEmpty {
						list.append(label)
					}
				}
			})

			DispatchQueue.main.async {
				self.cardDavFriendsLists = list
				SharedMainViewModel.shared.updateCardDavFriendsListsCount(cardDavFriendsListsCount: self.cardDavFriendsLists.count)
			}
		}
	}

	
	func downloadAndApplyRemoteProvisioning() {
		Log.info("\(SettingsViewModel.TAG) Updating remote provisioning URI now and then download/apply it")
		
		CoreContext.shared.doOnCoreQueue { core in
			if core.provisioningUri != self.remoteProvisioningUrl && !(core.provisioningUri == nil && self.remoteProvisioningUrl.isEmpty) {
				try? core.setProvisioninguri(newValue: self.remoteProvisioningUrl)
				
				Log.info("\(SettingsViewModel.TAG) Restarting the Core to apply configuration changes")
				core.stop()
				Log.info("\(SettingsViewModel.TAG) Core has been stopped, restarting it")
				try? core.start()
				Log.info("\(SettingsViewModel.TAG) Core has been restarted")
			}
		}
	}
	
	func setInputAudioDevice(index: Int) {
		CoreContext.shared.doOnCoreQueue { core in
			let audioDevice = self.inputAudioDeviceValues[index]
			core.defaultInputAudioDevice = audioDevice
		}
	}
	
	func setOutputAudioDevice(index: Int) {
		CoreContext.shared.doOnCoreQueue { core in
			let audioDevice = self.outputAudioDeviceValues[index]
			core.defaultOutputAudioDevice = audioDevice
		}
	}
	
	func setupAudioDevices() {
		CoreContext.shared.doOnCoreQueue { core in
			DispatchQueue.main.async {
				self.inputAudioDeviceLabels = []
				self.inputAudioDeviceValues = []
				self.inputAudioDeviceIndex = 0
				self.outputAudioDeviceLabels = []
				self.outputAudioDeviceValues = []
				self.outputAudioDeviceIndex = 0
			}
			
			// let audioSession = AVAudioSession.sharedInstance()
			// try? audioSession.setActive(true)
			
			// Input Audio Devices
			var inputIndex = 0
			let defaultInputAudioDevice = core.defaultInputAudioDevice
			Log.info("\(SettingsViewModel.TAG) Current default input audio device is \(defaultInputAudioDevice?.id ?? "Error defaultInputAudioDevice id")")
			
			var inputAudioDeviceLabelsTmp = [String]()
			var inputAudioDeviceValuesTmp = [AudioDevice]()
			var inputAudioDeviceIndexTmp = inputIndex
			
			core.extendedAudioDevices.forEach { audioDevice in
				if audioDevice.hasCapability(capability: AudioDevice.Capabilities.CapabilityRecord) {
					inputAudioDeviceLabelsTmp.append(audioDevice.id)
					inputAudioDeviceValuesTmp.append(audioDevice)
					if audioDevice.id == defaultInputAudioDevice?.id {
						inputAudioDeviceIndexTmp = inputIndex
					}
					Log.info("\(SettingsViewModel.TAG) Input audio device is \(audioDevice.id) \(audioDevice.capabilities)")
					inputIndex += 1
				}
				Log.info("\(SettingsViewModel.TAG) ---- 1 Input audio device is \(audioDevice.id) \(audioDevice.capabilities)")
			}
			
			DispatchQueue.main.async {
				self.inputAudioDeviceLabels = inputAudioDeviceLabelsTmp
				self.inputAudioDeviceValues = inputAudioDeviceValuesTmp
				self.inputAudioDeviceIndex = inputAudioDeviceIndexTmp
			}
			
			// Output Audio Devices
			var outputIndex = 0
			let defaultOutputAudioDevice = core.defaultOutputAudioDevice
			Log.info("\(SettingsViewModel.TAG) Current default output audio device is \(defaultOutputAudioDevice?.id ?? "Error defaultOutputAudioDevice id")")
			
			var outputAudioDeviceLabelsTmp = [String]()
			var outputAudioDeviceValuesTmp = [AudioDevice]()
			var outputAudioDeviceIndexTmp = outputIndex
			
			core.extendedAudioDevices.forEach { audioDevice in
				if audioDevice.hasCapability(capability: AudioDevice.Capabilities.CapabilityPlay) {
					outputAudioDeviceLabelsTmp.append(audioDevice.id)
					outputAudioDeviceValuesTmp.append(audioDevice)
					if audioDevice.id == defaultOutputAudioDevice?.id {
						outputAudioDeviceIndexTmp = outputIndex
					}
					Log.info("\(SettingsViewModel.TAG) Output audio device is \(audioDevice.id) \(audioDevice.capabilities)")
					outputIndex += 1
				}
			}
			
			DispatchQueue.main.async {
				self.outputAudioDeviceLabels = outputAudioDeviceLabelsTmp
				self.outputAudioDeviceValues = outputAudioDeviceValuesTmp
				self.outputAudioDeviceIndex = outputAudioDeviceIndexTmp
			}
		}
	}
	
	func setupCodecs() {
		CoreContext.shared.doOnCoreQueue { core in
			var audioCodecsList: [CodecModel] = []
			for payload in core.audioPayloadTypes {
				let model = CodecModel(
					mimeType: payload.mimeType,
					clockRate: payload.clockRate,
					channels: payload.channels,
					recvFmtp: nil,
					isAudioCodec: true,
					enabled: payload.enabled(),
					onEnabledChanged: { enabled in
						payload.enable(enabled: enabled)
					}
				)
				audioCodecsList.append(model)
			}
			
			var videoCodecsList: [CodecModel] = []
			for payload in core.videoPayloadTypes {
				let model = CodecModel(
					mimeType: payload.mimeType,
					clockRate: -1,
					channels: -1,
					recvFmtp: payload.recvFmtp,
					isAudioCodec: false,
					enabled: payload.enabled(),
					onEnabledChanged: { enabled in
						payload.enable(enabled: enabled)
					}
				)
				videoCodecsList.append(model)
			}
			
			DispatchQueue.main.async {
				self.audioCodecs = audioCodecsList
				self.videoCodecs = videoCodecsList
			}
		}
	}
	
	func saveChangesWhenLeaving() {
		CoreContext.shared.doOnCoreQueue { core in
			if CorePreferences.vfsEnabled != self.enableVfs {
				CorePreferences.vfsEnabled = self.enableVfs
			}
			
			if core.adaptiveRateControlEnabled != self.adaptiveRateControl {
				core.adaptiveRateControlEnabled = self.adaptiveRateControl
			}
			
			if core.videoEnabled != self.enableVideo {
				core.videoCaptureEnabled = self.enableVideo
				core.videoDisplayEnabled = self.enableVideo
			}
			
			if CorePreferences.automaticallyStartCallRecording != self.autoRecord {
				CorePreferences.automaticallyStartCallRecording = self.autoRecord
			}
			
			if (core.maxSizeForAutoDownloadIncomingFiles == 0) != self.autoDownload {
				core.maxSizeForAutoDownloadIncomingFiles = self.autoDownload ? 0 : -1
			}
			
			if (core.defaultConferenceLayout.rawValue == 0) != (self.defaultLayout == String(localized: "settings_meetings_layout_mosaic_label")) {
				core.defaultConferenceLayout = self.defaultLayout == String(localized: "settings_meetings_layout_mosaic_label") ? .Grid : .ActiveSpeaker
			}
			
			if core.wifiOnlyEnabled != self.useWifiOnly {
				core.wifiOnlyEnabled = self.useWifiOnly
			}
			
			if core.ipv6Enabled != self.allowIpv6 {
				core.ipv6Enabled = self.allowIpv6
			}
			
			if core.fecEnabled != self.enableFec {
				core.fecEnabled = self.enableFec
			}
			
			if (core.mediaEncryption == .None && self.mediaEncryption != "None")
				|| (core.mediaEncryption == .SRTP && self.mediaEncryption != "SRTP")
				|| (core.mediaEncryption == .ZRTP && self.mediaEncryption != "ZRTP")
				|| (core.mediaEncryption == .DTLS && self.mediaEncryption != "DTLS") {
				
				switch self.mediaEncryption {
				case "None":
					try? core.setMediaencryption(newValue: .None)
				case "SRTP":
					try? core.setMediaencryption(newValue: .SRTP)
				case "ZRTP":
					try? core.setMediaencryption(newValue: .ZRTP)
				case "DTLS":
					try? core.setMediaencryption(newValue: .DTLS)
				default:
					break
				}
			}
			
			if core.isMediaEncryptionMandatory != self.mediaEncryptionMandatory {
				core.mediaEncryptionMandatory = self.mediaEncryptionMandatory
			}
			
			if CorePreferences.acceptEarlyMedia != self.acceptEarlyMedia {
				CorePreferences.acceptEarlyMedia = self.acceptEarlyMedia
			}
			
			if CorePreferences.allowOutgoingEarlyMedia != self.allowOutgoingEarlyMedia {
				CorePreferences.allowOutgoingEarlyMedia = self.allowOutgoingEarlyMedia
			}
			
			if CorePreferences.deviceName != self.deviceId {
				CorePreferences.deviceName = self.deviceId
			}
			
			if core.fileTransferServer != self.uploadServerUrl && !(core.fileTransferServer == nil && self.uploadServerUrl.isEmpty) {
				core.fileTransferServer = self.uploadServerUrl
			}
		}
	}
}
