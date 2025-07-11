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

import SwiftUI
import ElegantEmojiPicker

struct EmojiPickerView: UIViewControllerRepresentable {
	@Binding var selected: String?
	@Binding var isSheetVisible: Bool
	
	var configuration = ElegantConfiguration(showRandom: false, showReset: false)

	func makeUIViewController(context: Context) -> UIViewController {
		let picker = ElegantEmojiPicker(delegate: context.coordinator,
										configuration: configuration,
										localization: .init())
		
		let container = NotifyingViewController()
		container.onWillDisappear = {
			isSheetVisible = false
		}
		
		container.addChild(picker)
		container.view.addSubview(picker.view)
		picker.view.frame = container.view.bounds
		picker.view.autoresizingMask = [.flexibleWidth, .flexibleHeight]
		picker.didMove(toParent: container)
		
		return container
	}

	func updateUIViewController(_ uiViewController: UIViewController, context: Context) {
	}

	func makeCoordinator() -> Coordinator {
		Coordinator(self)
	}
	
	class NotifyingViewController: UIViewController {
		var onWillDisappear: (() -> Void)?
		
		override func viewWillDisappear(_ animated: Bool) {
			super.viewWillDisappear(animated)
			onWillDisappear?()
		}
	}

	final class Coordinator: NSObject, ElegantEmojiPickerDelegate {
		let parent: EmojiPickerView
		init(_ parent: EmojiPickerView) { self.parent = parent }

		func emojiPicker(_ picker: ElegantEmojiPicker, didSelectEmoji emoji: Emoji?) {
			parent.selected = emoji?.emoji
			picker.dismiss(animated: true)
		}

		func emojiPicker(_ picker: ElegantEmojiPicker,
						 loadEmojiSections withConfiguration: ElegantConfiguration,
						 _ withLocalization: ElegantLocalization) -> [EmojiSection] {
			return ElegantEmojiPicker.getDefaultEmojiSections()
		}
	}
}
