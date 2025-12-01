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

import Foundation
import Combine

class RecordingMediaPlayerViewModel: ObservableObject {
	private let TAG = "[RecordingMediaPlayerViewModel]"
	
	private var coreContext = CoreContext.shared
	
	@Published var recording: RecordingModel
	
	@Published var isPlaying: Bool = false
	
	var vrpManager: VoiceRecordPlayerManager?
	
	init(recording: RecordingModel) {
		self.recording = recording
		if let url = URL(string: "file://" + recording.filePath) {
			startVoiceRecordPlayer(voiceRecordPath: url)
		}
	}
	
	func startVoiceRecordPlayer(voiceRecordPath: URL) {
		coreContext.doOnCoreQueue { core in
			if self.vrpManager == nil || self.vrpManager!.voiceRecordPath != voiceRecordPath {
				self.vrpManager = VoiceRecordPlayerManager(core: core, voiceRecordPath: voiceRecordPath)
			}
			
			if self.vrpManager != nil {
				self.vrpManager!.startVoiceRecordPlayer()
				DispatchQueue.main.async {
					self.isPlaying = true
				}
			}
		}
	}
	
	func getPositionVoiceRecordPlayer() -> Double {
		if self.vrpManager != nil {
			return self.vrpManager!.positionVoiceRecordPlayer()
		} else {
			return 0
		}
	}
	
	func isPlayingVoiceRecordPlayer(voiceRecordPath: URL) -> Bool {
		if self.vrpManager != nil && self.vrpManager!.voiceRecordPath == voiceRecordPath {
			return true
		} else {
			return false
		}
	}
	
	func startVoiceRecordPlayer() {
		coreContext.doOnCoreQueue { _ in
			if self.vrpManager != nil {
				self.vrpManager!.startVoiceRecordPlayer()
				DispatchQueue.main.async {
					self.isPlaying = true
				}
			}
		}
	}
	
	func pauseVoiceRecordPlayer() {
		coreContext.doOnCoreQueue { _ in
			if self.vrpManager != nil {
				self.vrpManager!.pauseVoiceRecordPlayer()
				DispatchQueue.main.async {
					self.isPlaying = false
				}
			}
		}
	}
	
	func stopVoiceRecordPlayer() {
		coreContext.doOnCoreQueue { _ in
			if self.vrpManager != nil {
				self.vrpManager!.stopVoiceRecordPlayer()
				DispatchQueue.main.async {
					self.isPlaying = false
				}
			}
		}
	}
}
