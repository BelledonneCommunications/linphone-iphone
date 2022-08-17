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


import UIKit
import Foundation
import SnapKit
import linphonesw

class PausedCallOrConferenceView: UIView {
	
	// Layout constants
	let icon_size = 200
	let icon_padding = 80.0
	let title_margin_top = 20
	
	var icon : UIImageView? = nil
	let title = StyledLabel(VoipTheme.call_or_conference_title)
	let subtitle = StyledLabel(VoipTheme.call_or_conference_subtitle)
	
	var onClickAction : (()->Void)? = nil
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
	init (iconName:String, titleText:String, subTitleText:String? = nil, onClickAction :  (()->Void)? = nil) {
		super.init(frame: .zero)
		
		backgroundColor = VoipTheme.voip_translucent_popup_background
		
		let centeredView = UIView()
		icon = UIImageView(image: UIImage(named:iconName)?.withPadding(padding: icon_padding))
		icon!.backgroundColor = VoipTheme.primary_color
		icon!.layer.cornerRadius = CGFloat(icon_size/2)
		icon!.clipsToBounds = true
		icon!.contentMode = .scaleAspectFit
		centeredView.addSubview(icon!)
		icon!.square(icon_size).centerX().done()
		
		title.numberOfLines = 0
		centeredView.addSubview(title)
		title.alignUnder(view:icon!, withMargin:title_margin_top).matchParentSideBorders().done()
		title.text = titleText
		
		subtitle.numberOfLines = 0
		centeredView.addSubview(subtitle)
		subtitle.alignUnder(view: title).matchParentSideBorders().done()
		subtitle.text = subTitleText
		
		self.addSubview(centeredView)
		centeredView.center().matchParentSideBorders().wrapContentY().done()
		
		self.onClickAction = onClickAction
		icon!.onClick {
			self.onClickAction?()
		}
		
	}
	
}
