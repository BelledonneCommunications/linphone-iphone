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

class Avatar : UIView {
	
	static let diameter_for_call_views = 191
	static let diameter_for_call_views_land = 130
	static let groupAvatar = UIImage(named:"voip_multiple_contacts_avatar")
	static let singleAvatar = UIImage(named:"avatar")
	
	required init?(coder: NSCoder) {
		initialsLabel =  StyledLabel(VoipTheme.call_generated_avatar_large)
		super.init(coder: coder)
	}
	
	let initialsLabel: StyledLabel
	let iconImageView = UIImageView()

	init (color:LightDarkColor,textStyle:TextStyle) {
		initialsLabel =  StyledLabel(textStyle)
		super.init(frame: .zero)
		clipsToBounds = true
		self.backgroundColor = color.get()
		addSubview(initialsLabel)
		addSubview(iconImageView)
		iconImageView.backgroundColor = .white
		initialsLabel.matchParentSideBorders().matchParentHeight().done()
		iconImageView.matchParentDimmensions().done()
	}
	
	
	func fillFromAddress(address:Address, isGroup:Bool = false) {
		if (isGroup) {
			iconImageView.image = Avatar.groupAvatar
			iconImageView.isHidden = false
			initialsLabel.isHidden = true
		} else if let image = address.contact()?.avatar() {
			iconImageView.image = image
			initialsLabel.isHidden = true
			iconImageView.isHidden = false
		} else {
			if (Core.get().defaultAccount?.isPhoneNumber(username: address.username) == true) {
				iconImageView.image = Avatar.singleAvatar
				initialsLabel.isHidden = true
				iconImageView.isHidden = false
			} else {
				initialsLabel.text = address.initials()
				initialsLabel.isHidden = false
				iconImageView.isHidden = true
			}
		}
	}
	
	func showAsAvatarIcon() {
		iconImageView.image = Avatar.singleAvatar
		initialsLabel.isHidden = true
		iconImageView.isHidden = false
	}
		
	override func layoutSubviews() {
		super.layoutSubviews()
		layer.cornerRadius = self.frame.width / 2.0
	}
	
}

