/*
 * Copyright (c) 2010-2025 Belledonne Communications SARL.
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

import AVFoundation

final class SoundPlayer {
	static let shared = SoundPlayer()

	private var player: AVAudioPlayer?

	func playIncomingMessage() {
		guard let url = Bundle.main.url(
			forResource: "incoming_chat",
			withExtension: "wav"
		) else {
			print("Sound not found")
			return
		}
		
		do {
			try configureAudio(.notification)
		} catch {
			print("Audio session error: \(error)")
		}

		do {
			player = try AVAudioPlayer(contentsOf: url)
			player?.prepareToPlay()
			player?.play()
		} catch {
			print("Audio error:", error)
		}
	}
}
