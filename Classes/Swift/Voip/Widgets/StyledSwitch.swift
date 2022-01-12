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

class StyledSwitch: UISwitch {
	
	var liveValue:MutableLiveData<Bool>?
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
	init (liveValue:MutableLiveData<Bool>) {
		super.init(frame: .zero)
		self.liveValue = liveValue
		tintColor = VoipTheme.light_grey_color
		onTintColor = VoipTheme.green_color
		addTarget(self, action: #selector(valueChanged), for: .valueChanged)
		liveValue.readCurrentAndObserve { (value) in
			self.isOn = value == true
		}
		transform = CGAffineTransform(scaleX: 0.75, y: 0.75)
   }

	@objc func valueChanged(mySwitch: UISwitch) {
		liveValue!.value = mySwitch.isOn
	}
}
