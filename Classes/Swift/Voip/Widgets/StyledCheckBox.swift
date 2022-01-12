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
import DropDown


class StyledCheckBox: UIButton {
	
	// layout constants
	let button_size = 25.0

	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
	init (liveValue:MutableLiveData<Bool>) {
		super.init(frame: .zero)
		setBackgroundImage(UIImage(named:"voip_checkbox_unchecked")/*?.tinted(with: VoipTheme.light_grey_color)*/,for: .normal) // tinting not working with those icons
		setBackgroundImage(UIImage(named:"voip_checkbox_checked")/*?.tinted(with: VoipTheme.primary_color)*/,for: .selected)
		onClick {
			liveValue.value = !liveValue.value!
			self.isSelected = liveValue.value!
		}
		
		size(w: button_size,h: button_size).done()
		
		liveValue.readCurrentAndObserve { (value) in
			self.isSelected = value!
		}
		
		
   }


}
