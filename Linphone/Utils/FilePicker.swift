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
import UniformTypeIdentifiers

struct FilePicker: UIViewControllerRepresentable {
	var onDocumentsPicked: ([URL]) -> Void

	func makeCoordinator() -> Coordinator {
		Coordinator(onDocumentsPicked: onDocumentsPicked)
	}

	func makeUIViewController(context: Context) -> UIDocumentPickerViewController {
		let picker = UIDocumentPickerViewController(forOpeningContentTypes: [UTType.item], asCopy: true)
		picker.allowsMultipleSelection = true
		picker.delegate = context.coordinator
		return picker
	}

	func updateUIViewController(_ uiViewController: UIDocumentPickerViewController, context: Context) {}

	class Coordinator: NSObject, UIDocumentPickerDelegate {
		let onDocumentsPicked: ([URL]) -> Void

		init(onDocumentsPicked: @escaping ([URL]) -> Void) {
			self.onDocumentsPicked = onDocumentsPicked
		}

		func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
			onDocumentsPicked(urls)
		}

		func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
			onDocumentsPicked([])
		}
	}
	
	static func convertToAttachmentArray(fromResults results: [URL], onComplete: @escaping ([Attachment]?, Error?) -> Void) {
		var medias = [Attachment]()
		
		let dispatchGroup = DispatchGroup()
		for urlFile in results {
			dispatchGroup.enter()
			do {
				let mimeType = urlFile.mimeType()
				if !mimeType.isEmpty {
					let type = mimeType.components(separatedBy: "/").first ?? ""
					let subtype = mimeType.components(separatedBy: "/").last ?? ""
					let dataResult = try Data(contentsOf: urlFile)
					
					var typeTmp: AttachmentType = .other
					switch type {
					case "image":
						typeTmp = urlFile.lastPathComponent.lowercased().hasSuffix("gif") ? .gif : .image
					case "audio":
						typeTmp = .audio
					case "application":
						typeTmp = subtype.lowercased() == "pdf" ? .pdf : .other
					case "text":
						typeTmp = .text
					case "video":
						typeTmp = .video
					default:
						typeTmp = .other
					}
					
					if typeTmp == .video {
						let urlImage = PhotoPicker.saveMedia(name: urlFile.lastPathComponent, data: dataResult, type: .video)
						let urlThumbnail = PhotoPicker.getURLThumbnail(name: urlFile.lastPathComponent)
						
						if urlImage != nil {
							let attachment = Attachment(id: UUID().uuidString, name: urlImage!.lastPathComponent, thumbnail: urlThumbnail, full: urlImage!, type: .video)
							medias.append(attachment)
						}
					} else {
						let urlImage = PhotoPicker.saveMedia(name: urlFile.lastPathComponent, data: dataResult, type: typeTmp)
						
						if urlImage != nil {
							let attachment = Attachment(id: UUID().uuidString, name: urlImage!.lastPathComponent, url: urlImage!, type: typeTmp)
							medias.append(attachment)
						}
					}
				}
			} catch {
				
			}
			dispatchGroup.leave()
		}
		
		dispatchGroup.notify(queue: .main) {
			onComplete(medias, nil)
		}
	}
}
