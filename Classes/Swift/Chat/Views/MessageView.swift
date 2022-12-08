//
//  MessageView.swift
//  linphone
//
//  Created by Benoît Martins on 07/12/2022.
//

import Foundation


class MessageView:  UIView {
	
	let top_bar_height = 66.0
	let side_buttons_margin = 10
	
	var backAction : (() -> Void)? = nil
	var action1 : (() -> Void)? = nil
	var action2 : (() -> Void)? = nil
	
	let pictureButton = CallControlButton(buttonTheme:VoipTheme.nav_button(""))
	let voiceRecordButton = CallControlButton(buttonTheme:VoipTheme.nav_button("vr_off"))
	let sendButton = CallControlButton(buttonTheme:VoipTheme.nav_button(""))
	let messageTextView = UIView()
	let messageText = UITextView()
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		setupView()
  	}
	
  	required init?(coder aDecoder: NSCoder) {
		super.init(coder: aDecoder)
		setupView()
  	}
	
	private func setupView() {
		backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
		
		addSubview(pictureButton)
		pictureButton.alignParentLeft(withMargin: side_buttons_margin).matchParentHeight().done()
		pictureButton.setImage(UIImage(named:"chat_attachment_default.png"), for: UIControl.State.normal)
		pictureButton.setImage(UIImage(named:"chat_attachment_over.png"), for: UIControl.State.highlighted)
		pictureButton.onClickAction = backAction
		
		addSubview(voiceRecordButton)
		voiceRecordButton.toRightOf(pictureButton, withLeftMargin: 10).matchParentHeight().done()
		voiceRecordButton.size(w: 30, h: 30).done()
		voiceRecordButton.onClickAction = action2

		addSubview(sendButton)
		sendButton.alignParentRight(withMargin: side_buttons_margin).matchParentHeight().done()
		sendButton.setImage(UIImage(named:"chat_send_default.png"), for: UIControl.State.normal)
		sendButton.setImage(UIImage(named:"chat_send_over.png"), for: UIControl.State.highlighted)
		sendButton.onClickAction = action1
		
		addSubview(messageTextView)
		messageTextView.toRightOf(voiceRecordButton, withLeftMargin: 5).toLeftOf(sendButton).matchParentHeight().done()
		
		messageTextView.addSubview(messageText)
		messageText.matchParentDimmensions(insetedByDx: 10).done()
		messageText.font = UIFont.systemFont(ofSize: 18)
		messageText.backgroundColor = UIColor.white
	}
}
