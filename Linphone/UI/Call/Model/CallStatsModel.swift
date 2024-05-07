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

class CallStatsModel: ObservableObject {
	var coreContext = CoreContext.shared
	
	@Published var audioCodec = ""
	@Published var audioBandwidth = ""
	
	@Published var isVideoEnabled = false
	@Published var videoCodec = ""
	@Published var videoBandwidth = ""
	@Published var videoResolution = ""
	@Published var videoFps = ""
	
	func update(call: Call, stats: CallStats) {
		coreContext.doOnCoreQueue { core in
			if call.params != nil {
				self.isVideoEnabled = call.params!.videoEnabled && call.currentParams!.videoDirection != .Inactive
				switch stats.type {
				case .Audio:
					if call.currentParams != nil {
						let payloadType = call.currentParams!.usedAudioPayloadType
						let clockRate = (payloadType?.clockRate != nil ? payloadType!.clockRate : 0) / 1000
						let codecLabel = "Codec: " + "\(payloadType != nil ? payloadType!.mimeType : "")/\(clockRate) kHz"
						
						guard !(stats.uploadBandwidth.rounded().isNaN || stats.uploadBandwidth.rounded().isInfinite || stats.downloadBandwidth.rounded().isNaN || stats.downloadBandwidth.rounded().isInfinite) else {
							return
						}
						let uploadBandwidth = Int(stats.uploadBandwidth.rounded())
						let downloadBandwidth = Int(stats.downloadBandwidth.rounded())
						let bandwidthLabel = "Bandwidth: " + "↑ \(uploadBandwidth) kbits/s ↓ \(downloadBandwidth) kbits/s"
						
						DispatchQueue.main.async {
							self.audioCodec = codecLabel
							self.audioBandwidth = bandwidthLabel
						}
					}
				case .Video:
					if call.currentParams != nil {
						let payloadType = call.currentParams!.usedVideoPayloadType
						let clockRate = (payloadType?.clockRate != nil ? payloadType!.clockRate : 0) / 1000
						let codecLabel = "Codec: " + "\(payloadType != nil ? payloadType!.mimeType : "null")/\(clockRate) kHz"
						
						guard !(stats.uploadBandwidth.rounded().isNaN || stats.uploadBandwidth.rounded().isInfinite || stats.downloadBandwidth.rounded().isNaN || stats.downloadBandwidth.rounded().isInfinite) else {
							return
						}
						let uploadBandwidth = Int(stats.uploadBandwidth.rounded())
						let downloadBandwidth = Int(stats.downloadBandwidth.rounded())
						let bandwidthLabel = "Bandwidth: " + "↑ \(uploadBandwidth) kbits/s ↓ \(downloadBandwidth) kbits/s"
						
						let sentResolution = call.currentParams!.sentVideoDefinition!.name
						let receivedResolution = call.currentParams!.receivedVideoDefinition!.name
						let resolutionLabel = "Resolution: " + "↑ \(sentResolution!) ↓ \(receivedResolution!)"
						
						let sentFps = Int(call.currentParams!.sentFramerate.rounded())
						let receivedFps = Int(call.currentParams!.receivedFramerate.rounded())
						let fpsLabel = "FPS: " + "↑ \(sentFps) ↓ \(receivedFps)"
						
						DispatchQueue.main.async {
							self.videoCodec = codecLabel
							self.videoBandwidth = bandwidthLabel
							self.videoResolution = resolutionLabel
							self.videoFps = fpsLabel
						}
					}
				default: break
				}
			}
		}
	}
}
