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
	
	let pictureButton = CallControlButton(buttonTheme:VoipTheme.nav_button("new_chat_attachment_default"))
	let voiceRecordButton = CallControlButton(buttonTheme:VoipTheme.nav_button("new_vr_off"))
	let sendButton = CallControlButton(buttonTheme:VoipTheme.nav_button("new_chat_send_default"))
	let emojisButton = CallControlButton(buttonTheme:VoipTheme.nav_button("emoji"))
	let messageTextView = UIView()
	let messageWithEmojiView = UIStackView()
	let messageText = UITextView()
	let ephemeralIndicator = UIImageView(image: UIImage(named: "ephemeral_messages_color_A.png"))
	var fileContext = false
	var isComposing = false
	var isLoading = false
	var lastNumLines = 0.0
	
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
		
		addSubview(voiceRecordButton)
		voiceRecordButton.toRightOf(pictureButton, withLeftMargin: -8).matchParentHeight().done()
		voiceRecordButton.onClickAction = action3

		addSubview(ephemeralIndicator)
		ephemeralIndicator.alignParentRight(withMargin: 4).alignParentTop(withMargin: 4).size(w: 9, h: 10).done()
		ephemeralIndicator.isHidden = true
		
		addSubview(sendButton)
		sendButton.alignParentRight(withMargin: side_buttons_margin).matchParentHeight().done()
		sendButton.isEnabled = false
		sendButton.onClickAction = action2
		
		addSubview(messageTextView)
		messageTextView.toRightOf(voiceRecordButton, withLeftMargin: -8).toLeftOf(sendButton, withRightMargin: -8).matchParentHeight().done()

		messageTextView.addSubview(messageWithEmojiView)
		messageWithEmojiView.matchParentDimmensions(insetedByDx: 10).done()
		messageWithEmojiView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		
		messageWithEmojiView.addArrangedSubview(messageText)
		messageText.matchParentHeight().alignParentLeft().alignParentRight(withMargin: 40).done()
		messageText.font = UIFont.systemFont(ofSize: 18)
		messageText.delegate = self
		messageText.textColor = UIColor.lightGray
		messageText.text = "Message"
		messageText.inputAccessoryView = UIView()
		messageWithEmojiView.addArrangedSubview(emojisButton)
		emojisButton.alignParentRight().matchParentHeight().done()
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
			self.messageWithEmojiView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		}
	}
	
	func textViewDidChangeSelection(_ textView: UITextView) {
		if messageText.textColor != UIColor.lightGray {
			let chatRoom = ChatRoom.getSwiftObject(cObject: PhoneMainView.instance().currentRoom)
			if ((messageText.text.isEmpty && !fileContext) || isLoading)  {
				sendButton.isEnabled = false
				emojisButton.isHidden = false
				NotificationCenter.default.post(name: Notification.Name("LinphoneResetTextViewSize"), object: self)
				lastNumLines = 0
			} else {
				if (messageText.text.trimmingCharacters(in: .whitespacesAndNewlines).unicodeScalars.first?.properties.isEmojiPresentation == true){
					var onlyEmojis = true
					messageText.text.trimmingCharacters(in: .whitespacesAndNewlines).unicodeScalars.forEach { emoji in
						if !emoji.properties.isEmojiPresentation && !emoji.properties.isWhitespace{
							onlyEmojis = false
						}
					}
					if onlyEmojis {
						emojisButton.isHidden = false
					} else {
						emojisButton.isHidden = true
					}
				} else {
					emojisButton.isHidden = true
				}
				if !isComposing {
					chatRoom.compose()
					let timer = Timer.scheduledTimer(withTimeInterval: 10.0, repeats: false) { timer in
						self.isComposing = false
					}
				}
				isComposing = true

				sendButton.isEnabled = true
				
				let numLines = (messageText.contentSize.height / messageText.font!.lineHeight)
				if(Int(numLines) != Int(lastNumLines)){
					NotificationCenter.default.post(name: Notification.Name("LinphoneTextViewSize"), object: self)
				}
				lastNumLines = numLines
			}
		}
	}
	
	func textViewDidBeginEditing(_ textView: UITextView) {
		if messageText.textColor == UIColor.lightGray {
			messageText.text = nil
			messageText.textColor = UIColor.black
		}
	}
	
	func textViewDidEndEditing(_ textView: UITextView) {
		if messageText.text.isEmpty {
			messageText.textColor = UIColor.lightGray
			messageText.text = "Message"
		}
	}
}

extension UIView {
	func setWidth(_ w:CGFloat, animateTime:TimeInterval?=nil) {
		if let c = self.constraints.first(where: { $0.firstAttribute == .width && $0.relation == .equal }) {
			c.constant = CGFloat(w)

			if let animateTime = animateTime {
				UIView.animate(withDuration: animateTime, animations:{
					self.layoutIfNeeded()
				})
			}
			else {
				self.layoutIfNeeded()
			}
		}
	}
}
