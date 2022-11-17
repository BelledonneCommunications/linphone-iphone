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

class DismissableView:  UIView {
	
	// Layout constants
	let header_height = 60.0
	let title_left_margin = 20
	let dismiss_right_margin = 10
	let dismiss_icon_inset = UIEdgeInsets(top: 10, left: 10, bottom: 10, right: 10)
	let headerView = UIView()
	let contentView = UIView()
	var dismiss : CallControlButton? = nil

	init(title:String) {
		super.init(frame:.zero)
		
		headerView.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
		self.addSubview(headerView)
		headerView.matchParentSideBorders().alignParentTop().height(header_height).done()

		dismiss = CallControlButton(imageInset:dismiss_icon_inset,buttonTheme: VoipTheme.voip_cancel, onClickAction: {
			self.removeFromSuperview()
		})
		headerView.addSubview(dismiss!)
		dismiss?.alignParentRight(withMargin: dismiss_right_margin).centerY().done()
		
		let title = StyledLabel(VoipTheme.calls_list_header_font,title)
		headerView.addSubview(title)
		title.alignParentTop().alignParentLeft(withMargin: title_left_margin).centerY().done()
		
		self.addSubview(contentView)
		contentView.alignUnder(view: headerView).matchParentSideBorders().alignParentBottom().done()
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.headerView.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
		}

	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
}
