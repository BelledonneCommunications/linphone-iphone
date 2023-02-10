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


class MessageView:  UIView, UITextViewDelegate {
	
	let side_buttons_margin = 10
	
	var action1 : (() -> Void)? = nil
	var action2 : (() -> Void)? = nil
	var action3 : (() -> Void)? = nil
	
	let pictureButton = CallControlButton(buttonTheme:VoipTheme.nav_button(""))
	let voiceRecordButton = CallControlButton(buttonTheme:VoipTheme.nav_button("vr_off"))
	let sendButton = CallControlButton(buttonTheme:VoipTheme.nav_button(""))
	let messageTextView = UIView()
	let messageText = UITextView()
	let ephemeralIndicator = UIImageView(image: UIImage(named: "ephemeral_messages_color_A.png"))
	var fileContext = false
	var isComposing = false
	var isLoading = false
	
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
		pictureButton.setImage(UIImage(named:"chat_attachment_default.png"), for: .normal)
		pictureButton.setImage(UIImage(named:"chat_attachment_over.png"), for: .highlighted)
		
		addSubview(voiceRecordButton)
		voiceRecordButton.toRightOf(pictureButton, withLeftMargin: 10).matchParentHeight().done()
		voiceRecordButton.size(w: 30, h: 30).done()
		voiceRecordButton.setImage(UIImage(named:"vr_off.png"), for: .normal)
		voiceRecordButton.setImage(UIImage(named:"vr_on.png"), for: .selected)
		voiceRecordButton.onClickAction = action3

		addSubview(ephemeralIndicator)
		ephemeralIndicator.alignParentRight(withMargin: 4).alignParentTop(withMargin: 4).size(w: 9, h: 10).done()
		ephemeralIndicator.isHidden = true
		
		
		addSubview(sendButton)
		sendButton.alignParentRight(withMargin: side_buttons_margin).matchParentHeight().done()
		sendButton.setImage(UIImage(named:"chat_send_default.png"), for: .normal)
		sendButton.setImage(UIImage(named:"chat_send_over.png"), for: .highlighted)
		sendButton.isEnabled = false
		sendButton.onClickAction = action2
		
		addSubview(messageTextView)
		messageTextView.toRightOf(voiceRecordButton, withLeftMargin: 5).toLeftOf(sendButton).matchParentHeight().done()
		
		messageTextView.addSubview(messageText)
		messageText.matchParentDimmensions(insetedByDx: 10).done()
		messageText.font = UIFont.systemFont(ofSize: 18)
		messageText.delegate = self
	}
	
	func textViewDidChangeSelection(_ textView: UITextView) {
		let chatRoom = ChatRoom.getSwiftObject(cObject: PhoneMainView.instance().currentRoom)
		if ((messageText.text.isEmpty && !fileContext) || isLoading)  {
			sendButton.isEnabled = false
		} else {
			if !isComposing {
				chatRoom.compose()
				let timer = Timer.scheduledTimer(withTimeInterval: 10.0, repeats: false) { timer in
					self.isComposing = false
				}
			}
			isComposing = true

			sendButton.isEnabled = true
		}
	}
}
