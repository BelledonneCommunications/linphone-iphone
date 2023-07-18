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
import SnapKit

class BackActionsNavigationView:  UIViewController {
	let contentMessageView = ContentMessageView()
	
	let topBar = UIView()
	let top_bar_height = 66.0
	let side_buttons_margin = 5
	
	let titleLabel = StyledLabel(VoipTheme.chat_conversation_title)
	let titleParticipants = UIView()
	let titleGroupLabel = StyledLabel(VoipTheme.chat_conversation_title)
  	let participantsGroupLabel = StyledLabel(VoipTheme.chat_conversation_participants)
	
	var backAction : (() -> Void)? = nil
	var action1 : (() -> Void)? = nil
	var action2 : (() -> Void)? = nil
	
	let backButton = CallControlButton(buttonTheme:VoipTheme.nav_button("back_default"))
	let cancelButton = CallControlButton(buttonTheme:VoipTheme.nav_button("cancel_edit_default"))
	let action1Button = CallControlButton(buttonTheme:VoipTheme.nav_button("call_audio_start_default"))
	let action1BisButton = CallControlButton(buttonTheme:VoipTheme.nav_button("voip_conference_new"))
	let action1SelectAllButton = CallControlButton(buttonTheme:VoipTheme.nav_button("select_all_default"))
	let action1DeselectAllButton = CallControlButton(buttonTheme:VoipTheme.nav_button("deselect_all"))
	let action2Button = CallControlButton(buttonTheme:VoipTheme.nav_button("more_menu_default"))
	let action2Delete = CallControlButton(buttonTheme:VoipTheme.nav_button("delete_default"))
	
	var isGroupChat : Bool = false
    
    func viewDidLoad(backAction : @escaping () -> Void,
                     action1 : @escaping () -> Void,
                     action2 : @escaping () -> Void,
					 action3 : @escaping () -> Void,
					 action4 : @escaping () -> Void,
                     title: String,
					 participants: String?) {
        self.backAction = backAction
        self.action1 = action1
        self.action2 = action2
		
        self.view.addSubview(topBar)
        //topBar.alignParentTop().height(top_bar_height).matchParentSideBorders().done()
		topBar.alignParentTop().height(top_bar_height).done()
		topBar.leftAnchor.constraint(equalTo: view.safeAreaLayoutGuide.leftAnchor).isActive = true
		topBar.rightAnchor.constraint(equalTo: view.safeAreaLayoutGuide.rightAnchor).isActive = true
        
        topBar.addSubview(backButton)
        backButton.alignParentLeft(withMargin: side_buttons_margin).matchParentHeight().done()
        backButton.onClickAction = backAction
		
		topBar.addSubview(cancelButton)
		cancelButton.alignParentLeft(withMargin: side_buttons_margin).matchParentHeight().done()
		cancelButton.onClickAction = editModeOff
		cancelButton.isHidden = true
        
        topBar.addSubview(action2Button)
        action2Button.alignParentRight(withMargin: side_buttons_margin).matchParentHeight().done()
        action2Button.onClickAction = action2
		action2Button.onLongClick(action: action4)
		
		topBar.addSubview(action2Delete)
		action2Delete.alignParentRight(withMargin: side_buttons_margin).matchParentHeight().done()
		action2Delete.onClickAction = deleteSelected
		action2Delete.isHidden = true
		
		topBar.addSubview(action1Button)
		topBar.addSubview(action1BisButton)
        action1Button.toLeftOf(action2Button, withRightMargin: 20).matchParentHeight().done()
		action1BisButton.toLeftOf(action2Button, withRightMargin: 12).matchParentHeight().done()
		action1Button.size(w: 34, h: 34).done()

        action1Button.onClickAction = action1
		action1BisButton.onClickAction = action1
		action1BisButton.isHidden = true
		
		topBar.addSubview(action1SelectAllButton)
		topBar.addSubview(action1DeselectAllButton)
		action1SelectAllButton.toLeftOf(action2Button, withRightMargin: 12).matchParentHeight().done()
		action1DeselectAllButton.toLeftOf(action2Button, withRightMargin: 12).matchParentHeight().done()
		action1SelectAllButton.onClickAction = selectDeselectAll
		action1DeselectAllButton.onClickAction = selectDeselectAll
		action1SelectAllButton.isHidden = true
		action1DeselectAllButton.isHidden = true

        topBar.addSubview(titleLabel)
        titleLabel.toRightOf(backButton, withLeftMargin: 10).matchParentHeight().done()
		titleLabel.toLeftOf(action1Button, withRightMargin: 20).done()
		titleLabel.text = title
         
		topBar.addSubview(titleParticipants)
		titleParticipants.toRightOf(backButton, withLeftMargin: 10).matchParentHeight().done()
		titleParticipants.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
		titleParticipants.toLeftOf(action1Button, withRightMargin: 20).done()
		
		titleParticipants.addSubview(titleGroupLabel)
		titleGroupLabel.alignParentTop(withMargin: 10).matchParentSideBorders().done()
		titleGroupLabel.text = title
		
		titleParticipants.addSubview(participantsGroupLabel)
		participantsGroupLabel.alignParentBottom(withMargin: 10).matchParentSideBorders().done()
		participantsGroupLabel.text = participants
        
        super.viewDidLoad()
		
		view.addSubview(contentMessageView.view)
		
		view.bringSubviewToFront(topBar)
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.topBar.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
			self.titleParticipants.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
		}
    }
	
	func resetRecordingProgressBar(){
		
	}
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        topBar.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
    }
	
	func editModeOn(){
		backButton.isHidden = true
		cancelButton.isHidden = false
		action1Button.isHidden = true
		action1BisButton.isHidden = true
		action1SelectAllButton.isHidden = false
		action1DeselectAllButton.isHidden = true
		action2Button.isHidden = true
		action2Delete.isHidden = false
		action2Delete.isEnabled = false
	}
	
	func editModeOff(){
		backButton.isHidden = false
		cancelButton.isHidden = true
		action1DeselectAllButton.isHidden = true
		action1SelectAllButton.isHidden = true
		action2Button.isHidden = false
		action2Delete.isHidden = true
		changeCallIcon(groupChat: isGroupChat)
	}
	
	func changeCallIcon(groupChat: Bool){
		isGroupChat = groupChat
		let defaultAccount = Core.getSwiftObject(cObject: LinphoneManager.getLc()).defaultAccount
		if(groupChat && (defaultAccount != nil) && (defaultAccount!.params!.audioVideoConferenceFactoryAddress != nil)){
			action1Button.isHidden = true
			action1BisButton.isHidden = false
		}else if(groupChat){
			action1Button.isHidden = true
			action1BisButton.isHidden = true
		}else{
			action1Button.isHidden = false
			action1BisButton.isHidden = true
		}
	}
	
	func selectDeselectAll(){
		if(action1SelectAllButton.isHidden){
			action1SelectAllButton.isHidden = false
			action1DeselectAllButton.isHidden = true
			action2Delete.isEnabled = false
		}else{
			action1SelectAllButton.isHidden = true
			action1DeselectAllButton.isHidden = false
			action2Delete.isEnabled = true
		}
	}
	
	func changeTitle(titleString: String){
		titleLabel.text = titleString
		titleGroupLabel.text = titleString
	}
	
	func deleteSelected(){
	}
}

extension UIView {
	func setHeight(_ h:CGFloat, animateTime:TimeInterval?=nil) {
		if let c = self.constraints.first(where: { $0.firstAttribute == .height && $0.relation == .equal }) {
			c.constant = CGFloat(h)
			if self.superview != nil {
				if let animateTime = animateTime {
					UIView.animate(withDuration: animateTime, animations:{
						self.superview?.layoutIfNeeded()
					})
				} else {
					self.superview?.layoutIfNeeded()
				}
			}
		}
	}
}
