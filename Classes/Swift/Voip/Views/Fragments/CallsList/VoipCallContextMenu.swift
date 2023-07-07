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
import SnapKit
import linphonesw

class VoipCallContextMenu: UIStackView {
	
	//Layout constants
	static let item_height = 50.0
	let width = 250.0
	let margin_bw_items = 1.0
	static let texts_margin_left = 10.0

	
	let resume : ButtonWithStateBackgrounds
	let pause : ButtonWithStateBackgrounds
	let transfer : ButtonWithStateBackgrounds
	let answer : ButtonWithStateBackgrounds
	let terminate : ButtonWithStateBackgrounds

	var callData: CallData? = nil {
		didSet {
			callData?.callState.readCurrentAndObserve(onChange: { (state) in
				self.resume.isHidden = false
				self.pause.isHidden = false
				self.transfer.isHidden = false
				self.answer.isHidden = false
				self.terminate.isHidden = false
				var count = 5.0

				if let callData = self.callData {
					if (callData.isPaused.value == true ||
						callData.isIncoming.value == true ||
						callData.isOutgoing.value == true ||
						callData.isInRemoteConference.value == true
					) {
						self.pause.isHidden = true
						count -= 1
					}
					
					if (callData.isIncoming.value == true ||
						callData.isOutgoing.value == true ||
						callData.isInRemoteConference.value == true
					) {
						self.resume.isHidden = true
						self.transfer.isHidden = true
						count -=  2
					} else if (callData.isPaused.value == false) {
						self.resume.isHidden = true
						count -= 1
					}
					
					if (callData.isIncoming.value == false) {
						count -= 1
						self.answer.isHidden = true
					}
					self.size(w:self.width,h:count*VoipCallContextMenu.item_height).done()
				}
				
			})
		}
	}

	
	init () {
		
		resume = VoipCallContextMenu.getButton(title: VoipTexts.call_context_action_resume)
		pause = VoipCallContextMenu.getButton(title: VoipTexts.call_context_action_pause)
		transfer = VoipCallContextMenu.getButton(title: VoipTexts.call_context_action_transfer)
		answer = VoipCallContextMenu.getButton(title: VoipTexts.call_context_action_answer)
		terminate = VoipCallContextMenu.getButton(title: VoipTexts.call_context_action_hangup)

		super.init(frame: .zero)
		backgroundColor = .white
		axis = .vertical
		spacing = margin_bw_items

		
		addArrangedSubview(resume)
		addArrangedSubview(pause)
		addArrangedSubview(transfer)
		addArrangedSubview(answer)
		addArrangedSubview(terminate)
	
		resume.onClick {
			self.isHidden = true
			guard let call = self.callData?.call else { return }
			if (CallManager.callKitEnabled()) {
				CallManager.instance().setHeld(call:call.getCobject!,hold:false);
			} else {
				try?call.resume()
			}
		}
		pause.onClick {
			self.isHidden = true
			guard let call = self.callData?.call else { return }
			if (CallManager.callKitEnabled()) {
				CallManager.instance().setHeld(call:call,hold:true);
			} else {
				try?call.pause()
			}
		}
		transfer.onClick {
			let view: DialerView = self.VIEW(DialerView.compositeViewDescription());
			view.setAddress("")
			CallManager.instance().nextCallIsTransfer = true
			PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())			
		}
		answer.onClick {
			self.isHidden = true
			guard let call = self.callData?.call else { return }
			if (CallManager.callKitEnabled()) {
				CallManager.instance().acceptCall(call: call, hasVideo: false)
			} else {
				try?call.accept()
			}
		}
		terminate.onClick {
			self.isHidden = true
			guard let call = self.callData?.call else { return }
			CallManager.instance().terminateCall(call: call.getCobject)
		}
	}
	
	required init(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	static func getButton(title:String) -> ButtonWithStateBackgrounds {
		let button = ButtonWithStateBackgrounds(backgroundStateColors: VoipTheme.button_call_context_menu_background)
		button.setTitle(title, for: .normal)
		button.applyTitleStyle(VoipTheme.call_context_menu_item_font)
		button.titleEdgeInsets = UIEdgeInsets(top: 0, left: texts_margin_left, bottom: 0, right: 0)
		button.height(VoipCallContextMenu.item_height).done()
		return button
	}
	

	
}
