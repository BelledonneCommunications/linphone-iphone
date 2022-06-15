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
import UIKit
import SwiftUI

class ButtonWithStateBackgrounds : UIButton {

	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
	init (backgroundStateColors: [UInt: LightDarkColor], iconName:String? = nil) {
		super.init(frame: .zero)
		backgroundStateColors.keys.forEach { (stateRawValue) in
			setBackgroundColor(color: backgroundStateColors[stateRawValue]!.get(), forState: UIButton.State(rawValue: stateRawValue))
		}
		iconName.map { setImage(UIImage(named: $0), for: .normal) }
	}
	
	func setBackgroundColor(color: UIColor, forState: UIControl.State) {
		UIGraphicsBeginImageContext(CGSize(width: 1, height: 1))
		UIGraphicsGetCurrentContext()!.setFillColor(color.cgColor)
		UIGraphicsGetCurrentContext()!.fill(CGRect(x: 0, y: 0, width: 1, height: 1))
		let colorImage = UIGraphicsGetImageFromCurrentImageContext()
		UIGraphicsEndImageContext()
		self.setBackgroundImage(colorImage, for: forState)
	}
	


}
