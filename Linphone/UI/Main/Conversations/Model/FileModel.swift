/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
 *
 * This file is part of Linphone
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
import UIKit
import AVFoundation
import PDFKit

final class FileModel: ObservableObject, Equatable {
	
	// MARK: - Inputs
	let path: String
	let fileName: String
	let fileSize: Int64
	let fileCreationTimestamp: Int64
	let isEncrypted: Bool
	let originalPath: String
	let isFromEphemeralMessage: Bool
	let isWaitingToBeDownloaded: Bool
	let flexboxLayoutWrapBefore: Bool
	private let onClicked: ((FileModel) -> Void)?
	
	@Published var formattedFileSize: String = ""
	@Published var transferProgress: Int = -1
	@Published var transferProgressLabel: String = ""
	@Published var mediaPreview: String? = nil
	@Published var mediaPreviewAvailable: Bool = false
	@Published var audioVideoDuration: String? = nil
	
	// MARK: - Computed
	let mimeTypeString: String
	let isMedia: Bool
	let isImage: Bool
	let isVideoPreview: Bool
	let isPdf: Bool
	let isAudio: Bool
	
	let month: String
	let dateTime: String
	
	static func == (lhs: FileModel, rhs: FileModel) -> Bool {
		return lhs.path == rhs.path
	}
	
	// MARK: - Init
	init(
		path: String,
		fileName: String,
		fileSize: Int64,
		fileCreationTimestamp: Int64,
		isEncrypted: Bool,
		originalPath: String,
		isFromEphemeralMessage: Bool,
		isWaitingToBeDownloaded: Bool = false,
		flexboxLayoutWrapBefore: Bool = false,
		onClicked: ((FileModel) -> Void)? = nil
	) {
		self.path = path
		self.fileName = fileName
		self.fileSize = fileSize
		self.fileCreationTimestamp = fileCreationTimestamp
		self.isEncrypted = isEncrypted
		self.originalPath = originalPath
		self.isFromEphemeralMessage = isFromEphemeralMessage
		self.isWaitingToBeDownloaded = isWaitingToBeDownloaded
		self.flexboxLayoutWrapBefore = flexboxLayoutWrapBefore
		self.onClicked = onClicked
		
		let ext = (path as NSString).pathExtension.lowercased()
		self.isPdf = ext == "pdf"
		
		let mime = FileModel.mimeType(from: ext)
		self.mimeTypeString = mime
		
		self.isImage = mime.hasPrefix("image/")
		self.isVideoPreview = mime.hasPrefix("video/")
		self.isAudio = mime.hasPrefix("audio/")
		self.isMedia = isImage || isVideoPreview
		
		self.month = FileModel.month(from: fileCreationTimestamp)
		self.dateTime = FileModel.formatDate(timestamp: fileCreationTimestamp)
		
		computeFileSize(fileSize)
		updateTransferProgress(-1)
		
		if !isWaitingToBeDownloaded {
			if isPdf { loadPdfPreview() }
			if isImage {
				mediaPreview = path
				mediaPreviewAvailable = true
			} else if isVideoPreview {
				loadVideoPreview()
			}
			if isVideoPreview || isAudio {
				getDuration()
			}
		}
	}
	
	// MARK: - Actions
	func onClick() {
		onClicked?(self)
	}
	
	func destroy() {
		guard isEncrypted else { return }
		DispatchQueue.global(qos: .background).async {
			try? FileManager.default.removeItem(atPath: self.path)
		}
	}
	
	func deleteFile() async {
		try? FileManager.default.removeItem(atPath: path)
	}
	
	func computeFileSize(_ size: Int64) {
		formattedFileSize = FileModel.bytesToReadable(size)
	}
	
	func updateTransferProgress(_ percent: Int) {
		transferProgress = percent
		transferProgressLabel = (percent < 0 || percent > 100) ? "" : "\(percent)%"
	}
	
	// MARK: - Preview
	private func loadPdfPreview() {
		DispatchQueue.global(qos: .utility).async {
			guard let pdf = PDFDocument(url: URL(fileURLWithPath: self.path)),
				  let page = pdf.page(at: 0) else { return }
			
			let pageRect = page.bounds(for: .mediaBox)
			let renderer = UIGraphicsImageRenderer(size: pageRect.size)
			let image = renderer.image { ctx in
				UIColor.white.set()
				ctx.fill(pageRect)
				page.draw(with: .mediaBox, to: ctx.cgContext)
			}
			
			if let data = image.jpegData(compressionQuality: 0.8) {
				let url = FileModel.cacheFileURL(name: "\(self.fileName).jpg")
				try? data.write(to: url)
				DispatchQueue.main.async {
					self.mediaPreview = url.path
					self.mediaPreviewAvailable = true
				}
			}
		}
	}
	
	private func loadVideoPreview() {
		DispatchQueue.global(qos: .utility).async {
			let asset = AVAsset(url: URL(fileURLWithPath: self.path))
			let generator = AVAssetImageGenerator(asset: asset)
			generator.appliesPreferredTrackTransform = true
			
			let time = CMTime(seconds: 1, preferredTimescale: 600)
			if let cgImage = try? generator.copyCGImage(at: time, actualTime: nil) {
				let image = UIImage(cgImage: cgImage)
				if let data = image.jpegData(compressionQuality: 0.8) {
					let url = FileModel.cacheFileURL(name: "\(self.fileName).jpg")
					try? data.write(to: url)
					DispatchQueue.main.async {
						self.mediaPreview = url.path
						self.mediaPreviewAvailable = true
					}
				}
			}
		}
	}
	
	private func getDuration() {
		let asset = AVAsset(url: URL(fileURLWithPath: path))
		let seconds = Int(CMTimeGetSeconds(asset.duration))
		audioVideoDuration = FileModel.formatDuration(seconds)
	}
	
	// MARK: - Utils
	private static func cacheFileURL(name: String) -> URL {
		let dir = FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask)[0]
		return dir.appendingPathComponent(name)
	}
	
	private static func bytesToReadable(_ bytes: Int64) -> String {
		let formatter = ByteCountFormatter()
		formatter.countStyle = .file
		return formatter.string(fromByteCount: bytes)
	}
	
	private static func mimeType(from ext: String) -> String {
		switch ext {
		case "jpg", "jpeg", "png", "heic": return "image/jpeg"
		case "mp4", "mov": return "video/mp4"
		case "mp3", "wav", "m4a": return "audio/mpeg"
		case "pdf": return "application/pdf"
		default: return "application/octet-stream"
		}
	}
	
	private static func formatDuration(_ seconds: Int) -> String {
		let min = seconds / 60
		let sec = seconds % 60
		return String(format: "%02d:%02d", min, sec)
	}
	
	private static func formatDate(timestamp: Int64) -> String {
		let date = Date(timeIntervalSince1970: TimeInterval(timestamp / 1000))
		let formatter = DateFormatter()
		formatter.dateStyle = .medium
		formatter.timeStyle = .short
		return formatter.string(from: date)
	}
	
	private static func month(from timestamp: Int64) -> String {
		let date = Date(timeIntervalSince1970: TimeInterval(timestamp / 1000))
		let formatter = DateFormatter()
		formatter.dateFormat = "MMMM"
		return formatter.string(from: date)
	}
}
