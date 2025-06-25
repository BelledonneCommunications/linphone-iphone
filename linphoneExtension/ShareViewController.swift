/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
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

import UIKit
import Social

class ShareViewController: SLComposeServiceViewController {
	
	var remainingSlots = 12

	override func isContentValid() -> Bool {
		return true
	}

	override func viewDidAppear(_ animated: Bool) {
		handleSharedFiles()
	}

	override func configurationItems() -> [Any]! {
		return []
	}

	private func handleSharedFiles() {
		guard let extensionItems = self.extensionContext?.inputItems as? [NSExtensionItem] else { return }
		
		var fileURLs: [URL] = []
		let dispatchGroup = DispatchGroup()

		for item in extensionItems {
			if let attachments = item.attachments {
				for provider in attachments {
					guard remainingSlots > 0 else { break }
					if provider.hasItemConformingToTypeIdentifier("public.item") {
						remainingSlots -= 1
						dispatchGroup.enter()
						provider.loadFileRepresentation(forTypeIdentifier: "public.item") { urlFile, error in
							if let url = urlFile {
								if let urlSaved = self.copyFileToSharedContainer(from: url) {
									fileURLs.append(urlSaved)
								}
							}
							dispatchGroup.leave()
						}
					}
				}
			}
		}

		dispatchGroup.notify(queue: .main) {
			if !fileURLs.isEmpty {
				self.openParentApp(with: fileURLs)
			} else {
				self.extensionContext?.completeRequest(returningItems: [], completionHandler: nil)
			}
		}
	}

	func openParentApp(with fileURLs: [URL]) {
		let urlStrings = fileURLs.map { $0.path }
		let joinedURLs = urlStrings.joined(separator: ",")

		let urlScheme = "linphone-message://\(joinedURLs)"
		if let url = URL(string: urlScheme) {
			var responder: UIResponder? = self
			while responder != nil {
				if let application = responder as? UIApplication {
					application.open(url)
					break
				}
				responder = responder?.next
			}
		}
		
		self.extensionContext?.completeRequest(returningItems: [], completionHandler: nil)
	}
	
	func copyFileToSharedContainer(from url: URL) -> URL? {
		let fileManager = FileManager.default
		guard let sharedContainerURL = fileManager.containerURL(forSecurityApplicationGroupIdentifier: "group.org.linphone.phone.linphoneExtension") else {
			return nil
		}

		let destinationURL = sharedContainerURL.appendingPathComponent(url.lastPathComponent)

		do {
			if fileManager.fileExists(atPath: destinationURL.path) {
				try fileManager.removeItem(at: destinationURL)
			}

			try fileManager.copyItem(at: url, to: destinationURL)
			return destinationURL
		} catch {
			return nil
		}
	}
}

