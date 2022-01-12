/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
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

extension UIImage {
	func tinted(with color: UIColor?) -> UIImage? {
		if (color == nil) {
			return self
		}
		defer { UIGraphicsEndImageContext() }
		UIGraphicsBeginImageContextWithOptions(self.size, false, self.scale)
		color!.set()
		self.withRenderingMode(.alwaysTemplate).draw(in: CGRect(origin: .zero, size: self.size))
		return UIGraphicsGetImageFromCurrentImageContext()
	}
	
	func withInsets(insets: UIEdgeInsets) -> UIImage? {
		UIGraphicsBeginImageContextWithOptions(
			CGSize(width: self.size.width + insets.left + insets.right,
				   height: self.size.height + insets.top + insets.bottom), false, self.scale)
		let _ = UIGraphicsGetCurrentContext()
		let origin = CGPoint(x: insets.left, y: insets.top)
		self.draw(at: origin)
		let imageWithInsets = UIGraphicsGetImageFromCurrentImageContext()
		UIGraphicsEndImageContext()
		return imageWithInsets
	}
	
	func withPadding(padding: CGFloat) -> UIImage? {
		let insets = UIEdgeInsets(top: padding, left: padding, bottom: padding, right: padding)
		return withInsets(insets: insets)
	}
}
