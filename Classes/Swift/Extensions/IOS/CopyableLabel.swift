/*
 * Copyright (c) 2020-2030 Belledonne Communications SARL.
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


class CopyableLabel: UILabel {
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		setupTextSelection()
	}
	required init?(coder aDecoder: NSCoder) {
		super.init(coder: aDecoder)
		setupTextSelection()
	}
	
	private func setupTextSelection() {
		let longPress = UILongPressGestureRecognizer(target: self, action: #selector(didLongPress))
		addGestureRecognizer(longPress)
		isUserInteractionEnabled = true
	}
	
	@objc private func didLongPress(_ gesture: UILongPressGestureRecognizer) {
		guard let text = text, !text.isEmpty else { return }
		becomeFirstResponder()
		
		let menu = UIMenuController.shared
		menu.menuItems = [UIMenuItem(title: "Copy", action: #selector(copyToPasteboard))]
		
		if !menu.isMenuVisible {
			menu.setTargetRect(textRect(forBounds: bounds, limitedToNumberOfLines: numberOfLines).insetBy(dx: 5, dy: 5), in: self)
			menu.setMenuVisible(true, animated: true)
		}
	}
	
	@objc private func copyToPasteboard(_ sender: Any?) {
		UIPasteboard.general.string = text
	}
}
