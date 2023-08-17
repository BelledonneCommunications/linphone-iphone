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

class ContentMessageView:  UIViewController {
	var message_height = 66.0
	let top_bar_height = 66.0
	let contentView = UIView()
	let isComposingView = UIView()
	let isComposingTextView = StyledLabel(VoipTheme.chat_conversation_is_composing_text)
	let replyLabelTextView = StyledLabel(VoipTheme.chat_conversation_reply_label)
	let replyContentTextView = StyledLabel(VoipTheme.chat_conversation_reply_content)
	let replyContentTextSpacing = UIView()
	let replyContentForMeetingTextView = StyledLabel(VoipTheme.chat_conversation_reply_content)
	let replyDeleteButton = CallControlButton(width: 22, height: 22, buttonTheme:VoipTheme.nav_color_button("reply_cancel"))
	let replyMeetingSchedule = UIImageView()
	let recordingView = UIView()
	let recordingDeleteButton = CallControlButton(width: 40, height: 40, buttonTheme:VoipTheme.nav_button("delete_default"))
	let recordingPlayButton = CallControlButton(width: 40, height: 40, buttonTheme:VoipTheme.nav_button("vr_play"))
	let recordingStopButton = CallControlButton(width: 40, height: 40, buttonTheme:VoipTheme.nav_button("vr_stop"))
	var recordingWaveView = UIProgressView()
	let recordingDurationTextView = StyledLabel(VoipTheme.chat_conversation_recording_duration)
	let recordingWaveImage = UIImageView(image: UIImage(named: "vr_wave.png"))
	let recordingWaveImageMask = UIView()
	
	let recordingPlayerImage = UIView()
	
	let messageView = MessageView()
	let mediaSelector  = UIView()
	let mediaSelectorReply  = UIView()
	var replyBubble = UIView()
	var backgroundReplyColor = UIView()
	
	var isSecure : Bool = false
	let floatingButton = CallControlButton(buttonTheme:VoipTheme.nav_button(""))
	var constraintFloatingButton : NSLayoutConstraint? = nil
	var constraintLandscapeFloatingButton : NSLayoutConstraint? = nil
	
	var stackView = UIStackView()
	var stackViewReply = UIStackView()
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		view.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()

		stackView.axis = .vertical;
		stackView.distribution = .fill;
		stackView.alignment = .center;
		stackView.spacing = 1;
		
		stackView.translatesAutoresizingMaskIntoConstraints = false
		view.addSubview(stackView)
		
		let keyWindow = UIApplication.shared.windows.filter {$0.isKeyWindow}.first
		if keyWindow != nil {
			stackView.alignParentTop().alignParentBottom(withMargin: keyWindow!.safeAreaInsets.bottom/2).done()
		}else{
			stackView.alignParentTop().alignParentBottom().done()
		}

		stackView.leftAnchor.constraint(equalTo: view.safeAreaLayoutGuide.leftAnchor).isActive = true
		stackView.rightAnchor.constraint(equalTo: view.safeAreaLayoutGuide.rightAnchor).isActive = true
		
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
		
		resetRecordingProgressBar()
		
		stackView.addArrangedSubview(mediaSelector)
		mediaSelector.height(top_bar_height*2).matchParentSideBorders().done()
		mediaSelector.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
		mediaSelector.isHidden = true

		stackView.addArrangedSubview(messageView)
		if keyWindow != nil {
			message_height = 66 - ((keyWindow!.safeAreaInsets.bottom/2)/2)
		}
				
		messageView.alignParentBottom().height(message_height).matchParentSideBorders().done()
		
		stackView.translatesAutoresizingMaskIntoConstraints = false;
		view.addSubview(stackView)
		
		view.addSubview(floatingButton)
		constraintFloatingButton = floatingButton.rightAnchor.constraint(equalTo: self.view.rightAnchor, constant: 3)
		constraintLandscapeFloatingButton = floatingButton.rightAnchor.constraint(equalTo: self.view.rightAnchor, constant: -56)
		if UIDevice.current.orientation.isLandscape {
			constraintLandscapeFloatingButton!.isActive = true
		} else {
			constraintFloatingButton!.isActive = true
		}
		floatingButton.topAnchor.constraint(equalTo: self.view.layoutMarginsGuide.topAnchor, constant: top_bar_height).isActive = true
		floatingButton.setImage(UIImage(named:"security_alert_indicator.png"), for: .normal)
		floatingButton.imageEdgeInsets = UIEdgeInsets(top: 42, left: 42, bottom: 42, right: 42)
		
		stackView.centerXAnchor.constraint(equalTo:self.view.centerXAnchor).isActive = true
		stackView.centerYAnchor.constraint(equalTo:self.view.centerYAnchor).isActive = true
		
		self.dismissKeyboard()
		
		NotificationCenter.default.addObserver(self, selector: #selector(self.rotated), name: UIDevice.orientationDidChangeNotification, object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(self.changeSizeOfTextView), name: Notification.Name("LinphoneTextViewSize"), object: nil)
		NotificationCenter.default.addObserver(self, selector: #selector(self.resetSizeOfTextView), name: Notification.Name("LinphoneResetTextViewSize"), object: nil)
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.replyBubble.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
			self.recordingView.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
			self.mediaSelector.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
			self.isComposingView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
			self.recordingWaveView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
			self.recordingWaveImageMask.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
			self.view.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
		}
	}
	
	deinit {
		 NotificationCenter.default.removeObserver(self)
	}
	
	func resetRecordingProgressBar(){
		recordingView.addSubview(recordingDeleteButton)
		recordingDeleteButton.alignParentLeft(withMargin: 10).matchParentHeight().done()
		
		recordingView.addSubview(recordingPlayButton)
		recordingPlayButton.alignParentRight(withMargin: 10).matchParentHeight().done()
		recordingPlayButton.isHidden = true
		
		recordingView.addSubview(recordingStopButton)
		recordingStopButton.alignParentRight(withMargin: 10).matchParentHeight().done()
		
		let newRecordingWaveView = UIProgressView()
		recordingWaveView = newRecordingWaveView
		
		recordingView.addSubview(recordingWaveView)
		recordingWaveView.toRightOf(recordingDeleteButton, withLeftMargin: 10).toLeftOf(recordingStopButton, withRightMargin: 10).alignParentTop(withMargin: 10).alignParentBottom(withMargin: 10).done()
		recordingWaveView.progressViewStyle = .bar
		recordingWaveView.layer.cornerRadius = 5
		recordingWaveView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		recordingWaveView.progressImage = UIImage.withColor(UIColor("L"))
		recordingWaveView.clipsToBounds = true
		
		recordingWaveView.addSubview(recordingDurationTextView)
		recordingDurationTextView.alignParentRight(withMargin: 10).matchParentHeight().done()
		
		recordingWaveView.addSubview(recordingWaveImage)
		recordingWaveImage.alignParentTop(withMargin: 10).alignParentBottom(withMargin: 10).alignParentLeft(withMargin: 10).alignParentRight(withMargin: 65).done()
		
		recordingWaveView.addSubview(recordingWaveImageMask)
		recordingWaveImageMask.alignParentTop(withMargin: 5).alignParentBottom(withMargin: 5).alignParentLeft(withMargin: 10).alignParentRight(withMargin: 65).done()
		recordingWaveImageMask.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
	}
	
	@objc func rotated() {
		if UIDevice.current.orientation.isLandscape {
			constraintLandscapeFloatingButton!.isActive = true
			constraintFloatingButton!.isActive = false
		} else if UIDevice.current.orientation.isPortrait {
			constraintLandscapeFloatingButton!.isActive = false
			constraintFloatingButton!.isActive = true
		}
	}
	
	@objc func changeSizeOfTextView(){
		let numLines = (messageView.messageText.contentSize.height / messageView.messageText.font!.lineHeight)
		if numLines >= 2 && numLines <= 6 {
			messageView.setHeight((message_height * numLines)/2)
		} else if numLines < 2 {
			messageView.setHeight(message_height)
		}
	}
	
	@objc func resetSizeOfTextView(){
		messageView.setHeight(message_height)
	}
	
	func dismissKeyboard() {
		let tap: UITapGestureRecognizer = UITapGestureRecognizer( target: self, action: #selector(self.dismissKeyboardTouchOutside))
		tap.cancelsTouchesInView = false
		view.addGestureRecognizer(tap)
	}
	
	@objc private func dismissKeyboardTouchOutside() {
		view.endEditing(true)
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
}
