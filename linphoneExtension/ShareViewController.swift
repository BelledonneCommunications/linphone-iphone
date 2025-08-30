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
import UniformTypeIdentifiers

class ShareViewController: SLComposeServiceViewController {
	
	var remainingSlots = 12
    var fileURLs: [URL] = []

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
        // Get all items shared to the extension
        guard let extensionItems = self.extensionContext?.inputItems as? [NSExtensionItem] else { return }

        let dispatchGroup = DispatchGroup()

        for item in extensionItems {
            if let attachments = item.attachments {
                for provider in attachments {
                    guard remainingSlots > 0 else { break }
                    remainingSlots -= 1
                    dispatchGroup.enter()

                    if provider.hasItemConformingToTypeIdentifier(UTType.image.identifier) {
                        // Image / screenshot
                        provider.loadItem(forTypeIdentifier: UTType.image.identifier, options: nil) { item, error in
                            defer { dispatchGroup.leave() }
                            self.handleImageItem(item)
                        }
                    } else if provider.hasItemConformingToTypeIdentifier("public.item") {
                        // Other files (PDF, video, document, etc.)
                        provider.loadFileRepresentation(forTypeIdentifier: "public.item") { url, error in
                            defer { dispatchGroup.leave() }
                            if let url = url, let saved = self.copyFileToSharedContainer(from: url) {
                                self.fileURLs.append(saved)
                                print("File copied to App Group: \(saved.path)")
                            }
                        }
                    } else {
                        // Unsupported type, just skip
                        dispatchGroup.leave()
                        print("Unsupported file type encountered.")
                    }
                }
            }
        }

        // Once all providers are processed, open parent app if we have files
        dispatchGroup.notify(queue: .main) {
            if !self.fileURLs.isEmpty {
                self.openParentApp(with: self.fileURLs)
            } else {
                self.extensionContext?.completeRequest(returningItems: [], completionHandler: nil)
                print("No valid files to send.")
            }
        }
    }
    
    private func handleImageItem(_ item: NSSecureCoding?) {
        guard let item = item else { return }

        if let image = item as? UIImage {
            // Create a temporary file for the image
            let tempURL = FileManager.default.temporaryDirectory
                .appendingPathComponent(UUID().uuidString + ".png")

            if let data = image.pngData() {
                do {
                    try data.write(to: tempURL)
                    if let saved = self.copyFileToSharedContainer(from: tempURL) {
                        fileURLs.append(saved)
                        print("Image copied to App Group: \(saved.path)")
                    }
                } catch {
                    print("Error writing UIImage -> file: \(error)")
                }
            }
        } else if let url = item as? URL {
            // If iOS returned a URL
            if let saved = self.copyFileToSharedContainer(from: url) {
                fileURLs.append(saved)
                print("Image file copied from URL: \(saved.path)")
            }
        } else {
            print("Unsupported image type: \(type(of: item))")
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
        guard let sharedContainerURL = FileManager.default.containerURL(
            forSecurityApplicationGroupIdentifier: "group.org.linphone.phone.linphoneExtension"
        ) else { return nil }
		
        let destinationURL = sharedContainerURL.appendingPathComponent(url.lastPathComponent)
		
        do {
            if FileManager.default.fileExists(atPath: destinationURL.path) {
                try FileManager.default.removeItem(at: destinationURL)
            }
            try FileManager.default.copyItem(at: url, to: destinationURL)
			
            let attrs = try FileManager.default.attributesOfItem(atPath: destinationURL.path)
            if let size = attrs[.size] as? NSNumber, size.intValue > 0 {
                return destinationURL
            } else {
                try? FileManager.default.removeItem(at: destinationURL)
                return nil
            }
        } catch {
            return nil
        }
    }
}

