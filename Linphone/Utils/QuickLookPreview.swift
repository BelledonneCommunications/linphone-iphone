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
import QuickLook

struct QuickLookFullScreenView: View {
	@Environment(\.presentationMode) var presentationMode
	
	@ObservedObject var conversationViewModel: ConversationViewModel
	
	@Binding var currentIndex: Int

	var body: some View {
		NavigationView {
			QuickLookPreview(fileURLs: conversationViewModel.attachments.map { $0.full }, startIndex: currentIndex)
			   .navigationBarTitleDisplayMode(.inline)
			   .toolbar {
				   ToolbarItem(placement: .principal) {
					   Text(conversationViewModel.attachments.first?.name ?? "File error")
						   .lineLimit(1)
						   .truncationMode(.middle)
						   .font(.headline)
				   }
			   }
			   .navigationBarItems(trailing: Button("Close") {
				   presentationMode.wrappedValue.dismiss()
			   })
		}
	}
}

struct QuickLookPreview: UIViewControllerRepresentable {
	let fileURLs: [URL]
	let startIndex: Int

	func makeUIViewController(context: Context) -> QLPreviewController {
		let previewController = QLPreviewController()
		previewController.dataSource = context.coordinator
		previewController.currentPreviewItemIndex = startIndex // Définir l’index de départ
		return previewController
	}

	func updateUIViewController(_ uiViewController: QLPreviewController, context: Context) {
		// No update needed
	}

	func makeCoordinator() -> Coordinator {
		Coordinator(self)
	}

	class Coordinator: NSObject, QLPreviewControllerDataSource {
		var parent: QuickLookPreview

		init(_ parent: QuickLookPreview) {
			self.parent = parent
		}

		func numberOfPreviewItems(in controller: QLPreviewController) -> Int {
			return parent.fileURLs.count
		}

		func previewController(_ controller: QLPreviewController, previewItemAt index: Int) -> QLPreviewItem {
			return parent.fileURLs[index] as QLPreviewItem
		}
	}
}
