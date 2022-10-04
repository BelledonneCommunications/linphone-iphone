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
import linphonesw
import SnapKit

class Avatar : UIImageView {
	
	static let diameter_for_call_views = 191
	static let diameter_for_call_views_land = 130
	static let groupAvatar = UIImage(named:"voip_multiple_contacts_avatar")
	static let singleAvatar = UIImage(named:"avatar")?.tinted(with: .white)
	
	required init?(coder: NSCoder) {
		initialsLabel =  StyledLabel(VoipTheme.call_generated_avatar_large)
		super.init(coder: coder)
	}
	
	let initialsLabel: StyledLabel
	
	init (color:LightDarkColor,textStyle:TextStyle) {
		initialsLabel =  StyledLabel(textStyle)
		super.init(frame: .zero)
		clipsToBounds = true
		self.backgroundColor = color.get()
		addSubview(initialsLabel)
		_ = initialsLabel.matchParentSideBorders().matchParentHeight()
        accessibilityLabel = "Avatar"
        isAccessibilityElement = true
	}
	
	
	func fillFromAddress(address:Address, isGroup:Bool = false) {
		if (isGroup) {
			self.image = Avatar.groupAvatar
			initialsLabel.isHidden = true
		} else if let image = address.contact()?.avatar() {
			self.image = image
			initialsLabel.isHidden = true
		} else {
			if (Core.get().defaultAccount?.isPhoneNumber(username: address.username) == true) {
				self.image = Avatar.singleAvatar
				initialsLabel.isHidden = true
			} else {
				self.image = nil
				initialsLabel.text = address.initials()
				initialsLabel.isHidden = false
			}
		}
	}
	
	func showAsAvatarIcon() {
		self.image = Avatar.singleAvatar
		initialsLabel.isHidden = true
	}
		
	override func layoutSubviews() {
		super.layoutSubviews()
		layer.cornerRadius = self.frame.width / 2.0
	}
	
}

