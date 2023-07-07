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
import SnapKit
import UIKit

extension UIButton {
	func addSidePadding(p:CGFloat = 10) { // Requires a width to be set prior to this ! SnapKit does not support updateOrCreate.
		if let w = titleLabel?.textWidth {
			updateWidth(w+2*p).done()
		}
	}
	
	func applyTintedIcons(tintedIcons:  [UInt: TintableIcon]) {
		tintedIcons.keys.forEach { (stateRawValue) in
			let tintedIcon = tintedIcons[stateRawValue]!
			UIImage(named:tintedIcon.name).map {
				setImage($0.tinted(with: tintedIcon.tintColor?.get()),for:  UIButton.State(rawValue: stateRawValue))
			}
		}
	}
	
}
