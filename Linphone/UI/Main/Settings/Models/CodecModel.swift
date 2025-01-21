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

class CodecModel: ObservableObject, Identifiable {
	let id = UUID()
	let mimeType: String
	let clockRate: Int
	let channels: Int
	let recvFmtp: String?
	let isAudioCodec: Bool
	private let onEnabledChanged: (Bool) -> Void

	@Published var isEnabled: Bool
	@Published var subtitle: String

	init(
		mimeType: String,
		clockRate: Int,
		channels: Int,
		recvFmtp: String?,
		isAudioCodec: Bool,
		enabled: Bool,
		onEnabledChanged: @escaping (Bool) -> Void
	) {
		self.mimeType = mimeType
		self.clockRate = clockRate
		self.channels = channels
		self.recvFmtp = recvFmtp
		self.isAudioCodec = isAudioCodec
		self.isEnabled = enabled
		self.onEnabledChanged = onEnabledChanged

		if isAudioCodec {
			self.subtitle = "\(clockRate) Hz" + (isAudioCodec ? " (\(channels == 1 ? "Mono" : "Stereo"))" : "")
		} else {
			self.subtitle = recvFmtp ?? ""
		}
	}

	func toggleEnabled() {
		let newValue = !isEnabled
		onEnabledChanged(newValue)
		isEnabled = newValue
	}
}
