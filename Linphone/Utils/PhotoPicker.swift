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
import PhotosUI

struct PhotoPicker: UIViewControllerRepresentable {
	typealias UIViewControllerType = PHPickerViewController
	
	let filter: PHPickerFilter
	var limit: Int = 0
	let onComplete: ([PHPickerResult]) -> Void
	
	func makeUIViewController(context: Context) -> PHPickerViewController {
		
		var configuration = PHPickerConfiguration()
		configuration.filter = filter
		configuration.selectionLimit = limit
		
		let controller = PHPickerViewController(configuration: configuration)
		
		controller.delegate = context.coordinator
		return controller
	}
	
	static func convertToUIImageArray(fromResults results: [PHPickerResult], onComplete: @escaping ([UIImage]?, Error?) -> Void) {
		var images = [UIImage]()
		
		let dispatchGroup = DispatchGroup()
		for result in results {
			dispatchGroup.enter()
			let itemProvider = result.itemProvider
			if itemProvider.canLoadObject(ofClass: UIImage.self) {
				itemProvider.loadObject(ofClass: UIImage.self) { (imageOrNil, errorOrNil) in
					if let error = errorOrNil {
						onComplete(nil, error)
					}
					if let image = imageOrNil as? UIImage {
						images.append(image)
					}
					dispatchGroup.leave()
				}
			}
		}
		dispatchGroup.notify(queue: .main) {
			onComplete(images, nil)
		}
	}
	
	func updateUIViewController(_ uiViewController: PHPickerViewController, context: Context) {}
	
	func makeCoordinator() -> Coordinator {
		return Coordinator(self)
	}
	
	class Coordinator: PHPickerViewControllerDelegate {
		
		private let parent: PhotoPicker
		
		init(_ parent: PhotoPicker) {
			self.parent = parent
		}
		
		func picker(_ picker: PHPickerViewController, didFinishPicking results: [PHPickerResult]) {
			picker.dismiss(animated: true)
			parent.onComplete(results)
		}
	}
}
