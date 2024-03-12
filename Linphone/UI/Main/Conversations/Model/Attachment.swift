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
	case video
	case gif

	public var title: String {
		switch self {
		case .image:
			return "Image"
		default:
			return "Video"
		}
	}

	public init(mediaType: MediaType) {
		switch mediaType {
		case .image:
			self = .image
		default:
			self = .video
		}
	}
}

public struct Attachment: Codable, Identifiable, Hashable {
	public let id: String
	public let thumbnail: URL
	public let full: URL
	public let type: AttachmentType

	public init(id: String, thumbnail: URL, full: URL, type: AttachmentType) {
		self.id = id
		self.thumbnail = thumbnail
		self.full = full
		self.type = type
	}

	public init(id: String, url: URL, type: AttachmentType) {
		self.init(id: id, thumbnail: url, full: url, type: type)
	}
}
