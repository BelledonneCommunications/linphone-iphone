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

class SettingsViewModel: ObservableObject {
	
	static let TAG = "[SettingsViewModel]"
	
	private var coreDelegate: CoreDelegate?
	
	// Security settings
	@Published var enableVfs: Bool = false
	//@Published var preventScreenshots: Bool = false
	
	// Calls settings
	@Published var adaptiveRateControl: Bool = false
	@Published var enableVideo: Bool = false
	@Published var autoRecord: Bool = false
	
	// Conversations settings
	@Published var autoDownload: Bool = false
	
	// Meetings settings
	@Published var defaultLayout: String = ""
	
	// Network settings
	@Published var useWifiOnly: Bool = false
	@Published var allowIpv6: Bool = false
	
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
			
			DispatchQueue.main.async {
				self.enableVfs = enableVfsTmp
				
				self.adaptiveRateControl = adaptiveRateControlTmp
				self.enableVideo = enableVideoTmp
				self.autoRecord = autoRecordTmp
				
				self.autoDownload = autoDownloadTmp
				
				self.defaultLayout = defaultLayoutTmp
				
				self.useWifiOnly = useWifiOnlyTmp
				self.allowIpv6 = allowIpv6Tmp
				
				self.coreDelegate = CoreDelegateStub(
					onAudioDevicesListUpdated: { (_: Core) in
						Log.info(
							"\(SettingsViewModel.TAG) Audio devices list has changed, update available input/output audio devices list"
						)
						// setupAudioDevices()
					}
				)
				core.addDelegate(delegate: self.coreDelegate!)
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
		}
	}
}
