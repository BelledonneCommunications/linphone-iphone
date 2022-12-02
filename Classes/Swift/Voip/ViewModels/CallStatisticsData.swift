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

import linphonesw

class CallStatisticsData {
	
	var call : Call
	var audioStats:[StatItemData] = []
	var videoStats:[StatItemData] = []
	let isVideoEnabled = MutableLiveData<Bool>()
	let statsUpdated = MutableLiveData(true)
	
	private var callDelegate :  CallDelegateStub?
	
	init (call:Call)  {
		self.call = call
		callDelegate = CallDelegateStub(
			onStatsUpdated : { (call: Call, stats: CallStats) ->  Void in
				self.isVideoEnabled.value = self.videoStatsAvailable(call)
				self.updateCallStats(stats: stats)
				self.statsUpdated.value = true
			}
		)
		call.addDelegate(delegate: callDelegate!)
		initCallStats()
		isVideoEnabled.value = videoStatsAvailable(call)
		call.audioStats.map { updateCallStats(stats: $0) }
		call.videoStats.map { updateCallStats(stats: $0) }
	}
	
	private func videoStatsAvailable(_ call:Call) -> Bool {
		return call.conference != nil ? call.params?.videoDirection == .SendRecv :  call.currentParams?.videoEnabled == true
	}
	
	private func initCallStats() {
		
		audioStats.append(StatItemData(type: StatType.CAPTURE))
		audioStats.append(StatItemData(type: StatType.PLAYBACK))
		audioStats.append(StatItemData(type: StatType.PAYLOAD))
		audioStats.append(StatItemData(type: StatType.ENCODER))
		audioStats.append(StatItemData(type: StatType.DECODER))
		audioStats.append(StatItemData(type: StatType.DOWNLOAD_BW))
		audioStats.append(StatItemData(type: StatType.UPLOAD_BW))
		audioStats.append(StatItemData(type: StatType.ICE))
		audioStats.append(StatItemData(type: StatType.IP_FAM))
		audioStats.append(StatItemData(type: StatType.SENDER_LOSS))
		audioStats.append(StatItemData(type: StatType.RECEIVER_LOSS))
		audioStats.append(StatItemData(type: StatType.JITTER))

		videoStats.append(StatItemData(type: StatType.CAPTURE))
		videoStats.append(StatItemData(type: StatType.PLAYBACK))
		videoStats.append(StatItemData(type: StatType.PAYLOAD))
		videoStats.append(StatItemData(type: StatType.ENCODER))
		videoStats.append(StatItemData(type: StatType.DECODER))
		videoStats.append(StatItemData(type: StatType.DOWNLOAD_BW))
		videoStats.append(StatItemData(type: StatType.UPLOAD_BW))
		videoStats.append(StatItemData(type: StatType.ESTIMATED_AVAILABLE_DOWNLOAD_BW))
		videoStats.append(StatItemData(type: StatType.ICE))
		videoStats.append(StatItemData(type: StatType.IP_FAM))
		videoStats.append(StatItemData(type: StatType.SENDER_LOSS))
		videoStats.append(StatItemData(type: StatType.RECEIVER_LOSS))
		videoStats.append(StatItemData(type: StatType.SENT_RESOLUTION))
		videoStats.append(StatItemData(type: StatType.RECEIVED_RESOLUTION))
		videoStats.append(StatItemData(type: StatType.SENT_FPS))
		videoStats.append(StatItemData(type: StatType.RECEIVED_FPS))
	}
	
	private func updateCallStats(stats: CallStats) {
		if (stats.type == StreamType.Audio) {
			audioStats.forEach{ $0.update(call: call, stats: stats)}
		} else if (stats.type == StreamType.Video) {
			videoStats.forEach{ $0.update(call: call, stats: stats)}
		}
	}
}


enum StatType  {
	case CAPTURE
	case PLAYBACK
	case PAYLOAD
	case ENCODER
	case DECODER
	case DOWNLOAD_BW
	case UPLOAD_BW
	case ICE
	case IP_FAM
	case SENDER_LOSS
	case RECEIVER_LOSS
	case JITTER
	case SENT_RESOLUTION
	case RECEIVED_RESOLUTION
	case SENT_FPS
	case RECEIVED_FPS
	case ESTIMATED_AVAILABLE_DOWNLOAD_BW
}

struct StatItemData  {
	var type:StatType
	
	let value = MutableLiveData<String>()
	
	func update(call: Call, stats: CallStats) {
		guard let payloadType = stats.type == StreamType.Audio ?  call.currentParams?.usedAudioPayloadType : call.currentParams?.usedVideoPayloadType, let core = call.core else {
			value.value = "n/a"
			return
		}
		switch(type) {
		case StatType.CAPTURE: value.value = stats.type == StreamType.Audio ?  core.captureDevice : core.videoDevice
		case StatType.PLAYBACK: value.value = stats.type == StreamType.Audio ? core.playbackDevice : core.videoDisplayFilter
		case StatType.PAYLOAD: value.value = "\(payloadType.mimeType)/\(payloadType.clockRate / 1000) kHz"
		case StatType.ENCODER: value.value = payloadType.description
		case StatType.DECODER: value.value = payloadType.description
		case StatType.DOWNLOAD_BW: value.value = "\(stats.downloadBandwidth) kbits/s"
		case StatType.UPLOAD_BW: value.value = "\(stats.uploadBandwidth) kbits/s"
		case StatType.ICE: value.value = stats.iceState.toString()
		case StatType.IP_FAM: value.value = stats.ipFamilyOfRemote == AddressFamily.Inet6 ?  "IPv6" : "IPv4"
		case StatType.SENDER_LOSS: value.value = String(format: "%.2f%",stats.senderLossRate)
		case StatType.RECEIVER_LOSS: value.value = String(format: "%.2f%",stats.receiverLossRate)
		case StatType.JITTER: value.value = String(format: "%.2f ms",stats.jitterBufferSizeMs)
		case StatType.SENT_RESOLUTION: value.value = call.currentParams?.sentVideoDefinition?.name
		case StatType.RECEIVED_RESOLUTION: value.value = call.currentParams?.receivedVideoDefinition?.name
		case StatType.SENT_FPS: value.value = "\(call.currentParams?.sentFramerate ?? 0.0)"
		case StatType.RECEIVED_FPS: value.value = "\(call.currentParams?.receivedFramerate ?? 0.0)"
		case StatType.ESTIMATED_AVAILABLE_DOWNLOAD_BW: value.value = "\(stats.estimatedDownloadBandwidth) kbit/s"
		}
	}
	
	
	func getTypeTitle() -> String {
		switch (type) {
		case .CAPTURE: return  VoipTexts.call_stats_capture_filter
		case .PLAYBACK: return VoipTexts.call_stats_player_filter
		case .PAYLOAD: return VoipTexts.call_stats_codec
		case .ENCODER: return VoipTexts.call_stats_encoder_name
		case .DECODER: return VoipTexts.call_stats_decoder_name
		case .DOWNLOAD_BW: return VoipTexts.call_stats_download
		case .UPLOAD_BW: return VoipTexts.call_stats_upload
		case .ICE: return VoipTexts.call_stats_ice
		case .IP_FAM: return VoipTexts.call_stats_ip
		case .SENDER_LOSS: return VoipTexts.call_stats_sender_loss_rate
		case .RECEIVER_LOSS: return VoipTexts.call_stats_receiver_loss_rate
		case .JITTER: return VoipTexts.call_stats_jitter_buffer
		case .SENT_RESOLUTION: return VoipTexts.call_stats_video_resolution_sent
		case .RECEIVED_RESOLUTION: return VoipTexts.call_stats_video_resolution_received
		case .SENT_FPS: return VoipTexts.call_stats_video_fps_sent
		case .RECEIVED_FPS: return VoipTexts.call_stats_video_fps_received
		case .ESTIMATED_AVAILABLE_DOWNLOAD_BW: return VoipTexts.call_stats_estimated_download
		}
	}
	
	
	
	
	
}
