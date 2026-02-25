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

enum AudioMode {
	case notification
	case voiceMessage
	case recording
	case call
}

func configureAudio(_ mode: AudioMode) throws {
	let session = AVAudioSession.sharedInstance()
	
	switch mode {
		
	case .notification:
		try session.setCategory(
			.ambient,
			mode: .default,
			options: [.mixWithOthers]
		)
		
	case .voiceMessage:
		try session.setCategory(
			.playback,
			mode: .spokenAudio,
			options: [.allowBluetoothHFP, .mixWithOthers]
		)
		
	case .recording:
		try session.setCategory(
			.playAndRecord,
			mode: .default,
			options: [.allowBluetoothHFP, .defaultToSpeaker, .mixWithOthers]
		)
		
	case .call:
		try session.setCategory(
			.playAndRecord,
			mode: .voiceChat,
			options: [.allowBluetoothHFP]
		)
	}
	
	try session.setActive(true)
}
