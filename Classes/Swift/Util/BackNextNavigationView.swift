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
import linphonesw

@objc class BackNextNavigationView:  UIViewController {
	
	
	// layout constants
	let top_bar_height = 66.0
	let navigation_buttons_padding = 18.0
	let content_margin_top = 20
	let side_buttons_margin = 5
	
	// User by subviews
	let form_margin = 10.0
	let form_input_height = 40.0
	let schdule_for_later_height = 80.0
	let description_height = 150.0
		
	let titleLabel = StyledLabel(VoipTheme.calls_list_header_font)
	
	let topBar = UIView()
	let scrollView = UIScrollView()
	let contentView = UIView()
	var backAction : (() -> Void)? = nil
	var nextAction : (() -> Void)? = nil
	
	let backButton = CallControlButton(buttonTheme:VoipTheme.nav_button("back_default"))
	let nextButton = CallControlButton(buttonTheme:VoipTheme.nav_button("next_default"))
	
	func viewDidLoad(backAction : @escaping () -> Void,
					 nextAction : @escaping () -> Void,
					 nextActionEnableCondition: MutableLiveData<Bool>,
					 title:String) {
		self.backAction = backAction
		self.nextAction = nextAction
		
		self.view.addSubview(topBar)
		topBar.alignParentTop().height(top_bar_height).matchParentSideBorders().done()
		
		topBar.addSubview(backButton)
		backButton.alignParentLeft(withMargin: side_buttons_margin).matchParentHeight().done()
		backButton.onClickAction = backAction
		
		topBar.addSubview(nextButton)
		nextButton.alignParentRight(withMargin: side_buttons_margin).matchParentHeight().done()
		nextButton.onClickAction = nextAction
		nextActionEnableCondition.readCurrentAndObserve { (enableNext) in
			self.nextButton.isEnabled = enableNext == true
		}

		topBar.addSubview(titleLabel)
		titleLabel.matchParentHeight().centerX().done()
		titleLabel.text = title
		
		super.viewDidLoad()

		view.addSubview(scrollView)
		scrollView.alignUnder(view: topBar, withMargin: content_margin_top).alignParentBottom().matchParentSideBorders().done()
		scrollView.addSubview(contentView)
		contentView.matchBordersOf(view: view).alignParentBottom().alignParentTop().done() // don't forget a bottom constraint b/w last element of contentview and contentview
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.topBar.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
		}
	}
	
}
