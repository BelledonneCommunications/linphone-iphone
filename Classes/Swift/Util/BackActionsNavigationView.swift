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

@objc class BackActionsNavigationView:  UIViewController {
    
    
    let top_bar_height = 66.0
    let side_buttons_margin = 5
        
    let titleLabel = StyledLabel(VoipTheme.chat_conversation_title)
	let titleParticipants = UIView()
	let titleGroupLabel = StyledLabel(VoipTheme.chat_conversation_title)
	let participantsGroupLabel = StyledLabel(VoipTheme.chat_conversation_participants)
    
    let topBar = UIView()
    let contentView = UIView()
	let isComposingView = UIView()
	let isComposingTextView = StyledLabel(VoipTheme.chat_conversation_is_composing_text)
	let replyLabelTextView = StyledLabel(VoipTheme.chat_conversation_reply_label)
	let replyContentTextView = StyledLabel(VoipTheme.chat_conversation_reply_content)
	let replyContentTextSpacing = UIView()
	let replyContentForMeetingTextView = StyledLabel(VoipTheme.chat_conversation_reply_content)
	let replyDeleteButton = CallControlButton(width: 22, height: 22, buttonTheme:VoipTheme.nav_button("reply_cancel"))
	let replyMeetingSchedule = UIImageView()
	let recordingView = UIView()
	let recordingDeleteButton = CallControlButton(width: 40, height: 40, buttonTheme:VoipTheme.nav_button("delete_default"))
	let recordingPlayButton = CallControlButton(width: 40, height: 40, buttonTheme:VoipTheme.nav_button("vr_play"))
	let recordingStopButton = CallControlButton(width: 40, height: 40, buttonTheme:VoipTheme.nav_button("vr_stop"))
	let recordingWaveView = UIProgressView()
	let recordingDurationTextView = StyledLabel(VoipTheme.chat_conversation_recording_duration)
	let recordingWaveImage = UIImageView(image: UIImage(named: "vr_wave.png"))
	let recordingWaveImageMask = UIView()
	
	let recordingPlayerImage = UIView()
	
	let messageView = MessageView()
	let mediaSelector  = UIView()
	let mediaSelectorReply  = UIView()
	var replyBubble = UIView()
	var backgroundReplyColor = UIView()
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
	
	var isSecure : Bool = false
	var isGroupChat : Bool = false
	let floatingButton = CallControlButton(buttonTheme:VoipTheme.nav_button(""))
	
	var stackView = UIStackView()
	var stackViewReply = UIStackView()
    
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
        topBar.alignParentTop().height(top_bar_height).matchParentSideBorders().done()
        
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
		
		titleParticipants.isHidden = true
        
        super.viewDidLoad()
		
		stackView.axis = .vertical;
		stackView.distribution = .fill;
		stackView.alignment = .center;
		stackView.spacing = 1;
		
		stackView.translatesAutoresizingMaskIntoConstraints = false
		view.addSubview(stackView)
		
		stackView.alignParentTop().alignParentBottom().matchParentSideBorders().done()
		
		stackView.addArrangedSubview(contentView)
		contentView.alignParentTop(withMargin: top_bar_height).matchParentSideBorders().done()

		stackView.addArrangedSubview(isComposingView)
		isComposingView.height(top_bar_height/2).matchParentSideBorders().done()
		isComposingView.isHidden = true
		
		isComposingView.addSubview(isComposingTextView)
		isComposingTextView.alignParentLeft(withMargin: 10).alignParentRight(withMargin: 10).alignParentTop(withMargin: 10).matchParentHeight().done()
		isComposingView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		
		stackView.addArrangedSubview(replyBubble)
		replyBubble.matchParentSideBorders().maxHeight(top_bar_height*3).done()
		replyBubble.translatesAutoresizingMaskIntoConstraints = false
		replyBubble.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
		replyBubble.isHidden = true
		
		replyBubble.addSubview(backgroundReplyColor)
		backgroundReplyColor.matchParentSideBorders().matchParentHeight().done()
		
		stackViewReply.axis = .vertical;
		stackViewReply.distribution = .fill;
		stackViewReply.alignment = .leading;
		
		replyBubble.addSubview(stackViewReply)
		stackViewReply.alignParentLeft(withMargin: 10).alignParentRight(withMargin: 50).alignParentBottom(withMargin: 10).matchParentHeight().wrapContentY().done()
		stackViewReply.translatesAutoresizingMaskIntoConstraints = false

		stackViewReply.addArrangedSubview(replyLabelTextView)
		replyLabelTextView.height(30).done()
		
		stackViewReply.addArrangedSubview(replyMeetingSchedule)
		replyMeetingSchedule.size(w: 100, h: 40).wrapContentY().done()
		replyMeetingSchedule.contentMode = .scaleAspectFit
		replyMeetingSchedule.isHidden = true
		
		stackViewReply.addArrangedSubview(replyContentForMeetingTextView)
		replyContentForMeetingTextView.width(100).wrapContentY().done()
		replyContentForMeetingTextView.textAlignment = .center
		replyContentForMeetingTextView.numberOfLines = 5
		replyContentForMeetingTextView.isHidden = true
		
		stackViewReply.addArrangedSubview(mediaSelectorReply)
		mediaSelectorReply.height(top_bar_height).wrapContentY().alignParentRight(withMargin: 50).done()
		mediaSelectorReply.isHidden = true
		
		stackViewReply.addArrangedSubview(replyContentTextSpacing)
		replyContentTextSpacing.height(8 ).wrapContentY().done()
		replyContentTextSpacing.isHidden = true
		
		stackViewReply.addArrangedSubview(replyContentTextView)
		replyContentTextView.wrapContentY().done()
		replyContentTextView.numberOfLines = 5
				
		replyBubble.addSubview(replyDeleteButton)
		replyDeleteButton.alignParentRight(withMargin: 15).centerY().done()
		
		stackView.addArrangedSubview(recordingView)
		recordingView.height(top_bar_height).wrapContentY().matchParentSideBorders().done()
		recordingView.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
		recordingView.isHidden = true
		
		recordingView.addSubview(recordingDeleteButton)
		recordingDeleteButton.alignParentLeft(withMargin: 10).matchParentHeight().done()
		
		recordingView.addSubview(recordingPlayButton)
		recordingPlayButton.alignParentRight(withMargin: 10).matchParentHeight().done()
		recordingPlayButton.isHidden = true
		
		recordingView.addSubview(recordingStopButton)
		recordingStopButton.alignParentRight(withMargin: 10).matchParentHeight().done()
		
		recordingView.addSubview(recordingWaveView)
		recordingWaveView.toRightOf(recordingDeleteButton, withLeftMargin: 10).toLeftOf(recordingStopButton, withRightMargin: 10).alignParentTop(withMargin: 10).alignParentBottom(withMargin: 10).done()
		recordingWaveView.progressViewStyle = .bar
		recordingWaveView.layer.cornerRadius = 5
		recordingWaveView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		recordingWaveView.progressTintColor = UIColor("L")
		recordingWaveView.clipsToBounds = true
		recordingWaveView.layer.sublayers![1].cornerRadius = 5
		recordingWaveView.subviews[1].clipsToBounds = true
		
		recordingWaveView.addSubview(recordingDurationTextView)
		recordingDurationTextView.alignParentRight(withMargin: 10).matchParentHeight().done()
		
		recordingWaveView.addSubview(recordingWaveImage)
		recordingWaveImage.alignParentTop(withMargin: 10).alignParentBottom(withMargin: 10).alignParentLeft(withMargin: 10).alignParentRight(withMargin: 65).done()
		
		recordingWaveView.addSubview(recordingWaveImageMask)
		recordingWaveImageMask.alignParentTop(withMargin: 5).alignParentBottom(withMargin: 5).alignParentLeft(withMargin: 10).alignParentRight(withMargin: 65).done()
		recordingWaveImageMask.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		
		stackView.addArrangedSubview(mediaSelector)
		mediaSelector.height(top_bar_height*2).matchParentSideBorders().done()
		mediaSelector.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
		mediaSelector.isHidden = true

		stackView.addArrangedSubview(messageView)
		messageView.alignParentBottom().height(top_bar_height).matchParentSideBorders().done()
		
		stackView.translatesAutoresizingMaskIntoConstraints = false;
		view.addSubview(stackView)
		
	 	view.addSubview(floatingButton)
		floatingButton.rightAnchor.constraint(equalTo: self.view.rightAnchor, constant: -1).isActive = true
		floatingButton.topAnchor.constraint(equalTo: self.view.layoutMarginsGuide.topAnchor, constant: top_bar_height + 4).isActive = true
		floatingButton.setImage(UIImage(named:"security_alert_indicator.png"), for: .normal)
		floatingButton.imageEdgeInsets = UIEdgeInsets(top: 45, left: 45, bottom: 45, right: 45)
		floatingButton.onClickAction = action3
		
		stackView.centerXAnchor.constraint(equalTo:self.view.centerXAnchor).isActive = true
		stackView.centerYAnchor.constraint(equalTo:self.view.centerYAnchor).isActive = true
		
		view.bringSubviewToFront(topBar)
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.topBar.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
			self.titleParticipants.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
			self.replyBubble.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
			self.recordingView.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
			self.mediaSelector.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
			self.isComposingView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
			self.recordingWaveView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
			self.recordingWaveImageMask.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		}
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        topBar.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
    }
	
	func changeTitle(titleString: String){
		titleLabel.text = titleString
		titleGroupLabel.text = titleString
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
	
	func changeCallIcon(groupChat: Bool){
		isGroupChat = groupChat
		if(groupChat){
			action1Button.isHidden = true
			action1BisButton.isHidden = false
		}else{
			action1Button.isHidden = false
			action1BisButton.isHidden = true
		}
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
	
	func deleteSelected(){

	}
}
