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

import SwiftUI

public struct Message: Identifiable, Hashable {

	public enum Status: Equatable, Hashable {
		case sending
		case sent
		case read
		case error(DraftMessage)

		public func hash(into hasher: inout Hasher) {
			switch self {
			case .sending:
				return hasher.combine("sending")
			case .sent:
				return hasher.combine("sent")
			case .read:
				return hasher.combine("read")
			case .error:
				return hasher.combine("error")
			}
		}

		public static func == (lhs: Message.Status, rhs: Message.Status) -> Bool {
			switch (lhs, rhs) {
			case (.sending, .sending):
				return true
			case (.sent, .sent):
				return true
			case (.read, .read):
				return true
			case ( .error(_), .error(_)):
				return true
			default:
				return false
			}
		}
	}

	public var id: String
	public var status: Status?
	public var createdAt: Date
	public var isOutgoing: Bool

	public var text: String
	public var attachments: [Attachment]
	public var recording: Recording?
	public var replyMessage: ReplyMessage?

	public init(
		id: String,
		status: Status? = nil,
		createdAt: Date = Date(),
		isOutgoing: Bool,
		text: String = "",
		attachments: [Attachment] = [],
		recording: Recording? = nil,
		replyMessage: ReplyMessage? = nil
	) {
		self.id = id
		self.status = status
		self.createdAt = createdAt
		self.isOutgoing = isOutgoing
		self.text = text
		self.attachments = attachments
		self.recording = recording
		self.replyMessage = replyMessage
	}

	public static func makeMessage(
		id: String,
		status: Status? = nil,
		draft: DraftMessage) async -> Message {
			let attachments = await draft.medias.asyncCompactMap { media -> Attachment? in
				guard let thumbnailURL = await media.getThumbnailURL() else {
					return nil
				}

				switch media.type {
				case .image:
					return Attachment(id: UUID().uuidString, url: thumbnailURL, type: .image)
				case .video:
					guard let fullURL = await media.getURL() else {
						return nil
					}
					return Attachment(id: UUID().uuidString, thumbnail: thumbnailURL, full: fullURL, type: .video)
				}
			}

			return Message(
				id: id,
				status: status,
				createdAt: draft.createdAt,
				isOutgoing: draft.isOutgoing,
				text: draft.text,
				attachments: attachments,
				recording: draft.recording,
				replyMessage: draft.replyMessage
			)
		}
}

extension Message {
	var time: String {
		DateFormatter.timeFormatter.string(from: createdAt)
	}
}

extension Message: Equatable {
	public static func == (lhs: Message, rhs: Message) -> Bool {
		lhs.id == rhs.id && lhs.status == rhs.status
	}
}

public struct Recording: Codable, Hashable {
	public var duration: Double
	public var waveformSamples: [CGFloat]
	public var url: URL?

	public init(duration: Double = 0.0, waveformSamples: [CGFloat] = [], url: URL? = nil) {
		self.duration = duration
		self.waveformSamples = waveformSamples
		self.url = url
	}
}

public struct ReplyMessage: Codable, Identifiable, Hashable {
	public static func == (lhs: ReplyMessage, rhs: ReplyMessage) -> Bool {
		lhs.id == rhs.id
	}

	public var id: String

	public var text: String
	public var isOutgoing: Bool
	public var attachments: [Attachment]
	public var recording: Recording?

	public init(id: String,
				text: String = "",
				isOutgoing: Bool,
				attachments: [Attachment] = [],
				recording: Recording? = nil) {

		self.id = id
		self.text = text
		self.isOutgoing = isOutgoing
		self.attachments = attachments
		self.recording = recording
	}

	func toMessage() -> Message {
		Message(id: id, isOutgoing: isOutgoing, text: text, attachments: attachments, recording: recording)
	}
}

public extension Message {

	func toReplyMessage() -> ReplyMessage {
		ReplyMessage(id: id, text: text, isOutgoing: isOutgoing, attachments: attachments, recording: recording)
	}
}

public struct DraftMessage {
	public var id: String?
	public let isOutgoing: Bool
	public let text: String
	public let medias: [Media]
	public let recording: Recording?
	public let replyMessage: ReplyMessage?
	public let createdAt: Date

	public init(id: String? = nil,
				isOutgoing: Bool,
				text: String,
				medias: [Media],
				recording: Recording?,
				replyMessage: ReplyMessage?,
				createdAt: Date) {
		self.id = id
		self.isOutgoing = isOutgoing
		self.text = text
		self.medias = medias
		self.recording = recording
		self.replyMessage = replyMessage
		self.createdAt = createdAt
	}
}

public enum MediaType {
	case image
	case video
}

public struct Media: Identifiable, Equatable {
	public var id = UUID()
	internal let source: MediaModelProtocol

	public static func == (lhs: Media, rhs: Media) -> Bool {
		lhs.id == rhs.id
	}
}

public extension Media {

	var type: MediaType {
		source.mediaType ?? .image
	}

	var duration: CGFloat? {
		source.duration
	}

	func getURL() async -> URL? {
		await source.getURL()
	}

	func getThumbnailURL() async -> URL? {
		await source.getThumbnailURL()
	}

	func getData() async -> Data? {
		try? await source.getData()
	}

	func getThumbnailData() async -> Data? {
		await source.getThumbnailData()
	}
}

protocol MediaModelProtocol {
	var mediaType: MediaType? { get }
	var duration: CGFloat? { get }

	func getURL() async -> URL?
	func getThumbnailURL() async -> URL?

	func getData() async throws -> Data?
	func getThumbnailData() async -> Data?
}

extension Sequence {
	func asyncCompactMap<T>(
		_ transform: (Element) async throws -> T?
	) async rethrows -> [T] {
		var values = [T]()

		for element in self {
			if let el = try await transform(element) {
				values.append(el)
			}
		}

		return values
	}
}

extension DateFormatter {
	static let timeFormatter = {
		let formatter = DateFormatter()

		formatter.dateStyle = .none
		formatter.timeStyle = .short

		return formatter
	}()

	static func timeString(_ seconds: Int) -> String {
		let hour = Int(seconds) / 3600
		let minute = Int(seconds) / 60 % 60
		let second = Int(seconds) % 60

		if hour > 0 {
			return String(format: "%02i:%02i:%02i", hour, minute, second)
		}
		return String(format: "%02i:%02i", minute, second)
	}
}
