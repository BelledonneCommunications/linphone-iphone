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
    
    
    let top_bar_height = 66.0
    let side_buttons_margin = 5
        
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
	let action1BisButton = CallControlButton(buttonTheme:VoipTheme.nav_button("voip_conference_new"))
    let action2Button = CallControlButton(buttonTheme:VoipTheme.nav_button("more_menu_default"))
	
	
	var isSecure : Bool = false
	let floatingButton = CallControlButton(buttonTheme:VoipTheme.nav_button(""))
    
    func viewDidLoad(backAction : @escaping () -> Void,
                     action1 : @escaping () -> Void,
                     action2 : @escaping () -> Void,
					 action3 : @escaping () -> Void,
					 action4 : @escaping () -> Void,
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
		action2Button.onLongClick(action: action4)
		
        topBar.addSubview(action1Button)
		topBar.addSubview(action1BisButton)
        action1Button.toLeftOf(action2Button, withRightMargin: 20).matchParentHeight().done()
		action1BisButton.toLeftOf(action2Button, withRightMargin: 20).matchParentHeight().done()

        action1Button.onClickAction = action1
		action1BisButton.onClickAction = action1
		
		action1BisButton.isHidden = true

        topBar.addSubview(titleLabel)
        titleLabel.toRightOf(backButton, withLeftMargin: 10).matchParentHeight().done()
		titleLabel.toLeftOf(action1Button, withRightMargin: 20).done()
        titleLabel.text = title
        
        super.viewDidLoad()

        view.addSubview(scrollView)
        scrollView.alignUnder(view: topBar).alignParentBottom().matchParentSideBorders().done()
        scrollView.addSubview(contentView)
        contentView.matchBordersOf(view: view).alignParentBottom().alignParentTop().done()
		
		view.addSubview(messageView)
		messageView.alignParentBottom().height(top_bar_height).matchParentSideBorders().done()
		
		view.addSubview(floatingButton)
		floatingButton.rightAnchor.constraint(equalTo: self.view.rightAnchor, constant: -5).isActive = true
		floatingButton.topAnchor.constraint(equalTo: self.view.layoutMarginsGuide.topAnchor, constant: top_bar_height + 5).isActive = true
		floatingButton.setImage(UIImage(named:"security_alert_indicator.png"), for: .normal)
		floatingButton.imageEdgeInsets = UIEdgeInsets(top: 45, left: 45, bottom: 45, right: 45)
		floatingButton.onClickAction = action3
            
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        topBar.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
        
    }
	
	func changeTitle(titleString: String){
		titleLabel.text = titleString
	}
	
	func changeSecureLevel(secureLevel: Bool, imageBadge: UIImage?){
		isSecure = secureLevel
		if(isSecure){
			floatingButton.isHidden = false
			floatingButton.setImage(imageBadge, for: .normal)
		}else{
			floatingButton.isHidden = true
		}
	}
	
	func changeCallIcon(groupeChat: Bool){
		if(groupeChat){
			action1Button.isHidden = true
			action1BisButton.isHidden = false
		}else{
			action1Button.isHidden = false
			action1BisButton.isHidden = true
		}
	}
}
