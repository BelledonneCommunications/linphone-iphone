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

class BouncingCounter : UIBouncingView {
	
	// Layout constants
	let size = 20.0
	let center_offset = 20

	let owningButton : UIButton
	let label : StyledLabel
	
	var dataSource : MutableLiveData<Int>? {
		didSet {
			if let dataSource = dataSource {
				self.size(w:self.size,h:self.size).matchCenterXOf(view: self.owningButton, withDx: self.center_offset).matchCenterYOf(view: self.owningButton, withDy: -self.center_offset).done()
				dataSource.readCurrentAndObserve { (value) in
					if (value! > 0) {
						self.label.text = value! < 100 ? String(value!) : "99+"
						self.isHidden = true // to force legacy startAnimating to unhide and animate
						self.startAnimating(true)
					} else {
						self.isHidden = false // to force legacy startAnimating to hide and animate
						self.stopAnimating(true)
					}
				}
			} else {
				self.isHidden = false
				self.stopAnimating(true)
			}
		}
	}
	
	
	init (inButton:UIButton) {
		owningButton = inButton
		label = StyledLabel(VoipTheme.unread_count_font)
		super.init(frame:.zero)
		addSubview(label)
		label.matchParentDimmensions().done()
		backgroundColor = VoipTheme.primary_color
		layer.masksToBounds = true
		layer.cornerRadius = size/2
		self.isHidden = true
	}
	
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}


	
}
