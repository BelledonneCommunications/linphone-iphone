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

class FormButton : ButtonWithStateBackgrounds {

	let button_radius = 3.0
	let button_height = 40.0
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
	var title: String? {
		didSet {
			setTitle(title, for: .normal)
			addSidePadding()
		}
	}
	
	init (backgroundStateColors: [UInt: LightDarkColor], bold:Bool = true) {
		super.init(backgroundStateColors: backgroundStateColors)
		layer.cornerRadius = button_radius
		clipsToBounds = true
		height(button_height).done()
		addSidePadding()
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.applyTitleStyle(bold ? VoipTheme.form_button_bold : VoipTheme.form_button_light)
		}
	}
	
	convenience init (title:String, backgroundStateColors: [UInt: LightDarkColor], bold:Bool = true, fixedSize:Bool = true) {
		self.init(backgroundStateColors: backgroundStateColors,bold:bold)
		self.title = title
		setTitle(title, for: .normal)
		if (!fixedSize) {
			addSidePadding()
		}
	}

	
}
