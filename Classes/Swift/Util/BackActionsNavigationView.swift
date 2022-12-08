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

@objc class BackActionsNavigationView:  UIViewController {
    
    
    // layout constants
    let top_bar_height = 66.0
    let navigation_buttons_padding = 18.0
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
	let messageView = MessageView()
    var backAction : (() -> Void)? = nil
    var action1 : (() -> Void)? = nil
    var action2 : (() -> Void)? = nil
    
    let backButton = CallControlButton(buttonTheme:VoipTheme.nav_button("back_default"))
    let action1Button = CallControlButton(buttonTheme:VoipTheme.nav_button("call_audio_start_default"))
    let action2Button = CallControlButton(buttonTheme:VoipTheme.nav_button("more_menu_default"))
    
    func viewDidLoad(backAction : @escaping () -> Void,
                     action1 : @escaping () -> Void,
                     action2 : @escaping () -> Void,
                     title:String) {
        self.backAction = backAction
        self.action1 = action1
        self.action2 = action2
        
        self.view.addSubview(topBar)
        topBar.alignParentTop().height(top_bar_height).matchParentSideBorders().done()
        
        topBar.addSubview(backButton)
        backButton.alignParentLeft(withMargin: side_buttons_margin).matchParentHeight().done()
        backButton.onClickAction = backAction
        
        topBar.addSubview(action2Button)
        action2Button.alignParentRight(withMargin: side_buttons_margin).matchParentHeight().done()
        action2Button.onClickAction = action2
        
        topBar.addSubview(action1Button)
        action1Button.toLeftOf(action2Button, withRightMargin: 20).matchParentHeight().done()
		action1Button.size(w: 35, h: 35)
        action1Button.onClickAction = action1

        topBar.addSubview(titleLabel)
        titleLabel.toRightOf(backButton, withLeftMargin: 10).matchParentHeight().done()
		titleLabel.toLeftOf(action1Button, withRightMargin: 20).done()
        titleLabel.text = title
        
        super.viewDidLoad()

        view.addSubview(scrollView)
        scrollView.alignUnder(view: topBar).alignParentBottom().matchParentSideBorders().done()
        scrollView.addSubview(contentView)
        contentView.matchBordersOf(view: view).alignParentBottom().alignParentTop().done() // don't forget a bottom constraint b/w last element of contentview and contentview
		
		view.addSubview(messageView)
		messageView.alignParentBottom().height(top_bar_height).matchParentSideBorders().done()
            
    }
    
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        topBar.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
        
    }
    
}
