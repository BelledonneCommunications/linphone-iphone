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

public enum AttachmentType: String, Codable {
	case image
	case gif
	case video
	case audio
	case voiceRecording
	case pdf
	case text
	case fileTransfer
	case other

	public var title: String {
		switch self {
		case .image:
			return "Image"
		case .gif:
			return "GIF"
		case .video:
			return "Video"
		case .audio:
			return "Audio"
		case .voiceRecording:
			return "Voice Recording"
		case .pdf:
			return "PDF"
		case .text:
			return "Text"
		case .fileTransfer:
			return "File Transfer"
		default:
			return "Other"
		}
	}

	public init(mediaType: MediaType) {
		switch mediaType {
		case .image:
			self = .image
		case .gif:
			self = .gif
		case .video:
			self = .video
		case .audio:
			self = .audio
		case .voiceRecording:
			self = .voiceRecording
		case .pdf:
			self = .pdf
		case .text:
			self = .text
		case .fileTransfer:
			self = .fileTransfer
		default:
			self = .other
		}
	}
}

public struct Attachment: Codable, Identifiable, Hashable {
	public let id: String
	public let name: String
	public let thumbnail: URL
	public let full: URL
	public let type: AttachmentType
	public let duration: Int
	public let size: Int
	public var transferProgressIndication: Int

	public init(id: String, name: String, thumbnail: URL, full: URL, type: AttachmentType, duration: Int = 0, size: Int = 0, transferProgressIndication: Int = 0) {
		self.id = id
		self.name = name
		self.thumbnail = thumbnail
		self.full = full
		self.type = type
		self.duration = duration
		self.size = size
		self.transferProgressIndication = transferProgressIndication
	}

	public init(id: String, name: String, url: URL, type: AttachmentType, duration: Int = 0, size: Int = 0, transferProgressIndication: Int = 0) {
		self.init(id: id, name: name, thumbnail: url, full: url, type: type, duration: duration, size: size, transferProgressIndication: transferProgressIndication)
	}
}

extension Attachment: Equatable {
	public static func == (lhs: Attachment, rhs: Attachment) -> Bool {
		lhs.id == rhs.id && lhs.transferProgressIndication == rhs.transferProgressIndication
	}
}
