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

@objc class IncomingCallView: AbstractIncomingOutgoingCallView, UICompositeViewDelegate {
	
	// Layout constants
	let buttons_distance_from_center_x = 38
	
    static let compositeDescription = UICompositeViewDescription(IncomingCallView.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: nil, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }

	var earlyMediaView : UIView? = nil
			
	override func viewDidLoad() {
		
		super.viewDidLoad(forCallType: VoipTexts.call_incoming_title)
		
		// Accept
		let accept = CallControlButton(width: CallControlButton.hungup_width, imageInset:AbstractIncomingOutgoingCallView.answer_decline_inset, buttonTheme: VoipTheme.call_accept, onClickAction: {
			self.callData.map { CallManager.instance().acceptCall(call: $0.call.getCobject, hasVideo: false)}
		})
		view.addSubview(accept)
		accept.centerX(withDx: buttons_distance_from_center_x).alignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		
		// Decline
		let decline = CallControlButton(width: CallControlButton.hungup_width, imageInset:AbstractIncomingOutgoingCallView.answer_decline_inset,  buttonTheme: VoipTheme.call_terminate, onClickAction: {
			self.callData.map { CallManager.instance().terminateCall(call: $0.call.getCobject)}
		})
		view.addSubview(decline)
		decline.centerX(withDx: -buttons_distance_from_center_x).alignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
	}
	
	@objc override func setCall(call:OpaquePointer) {
		super.setCall(call: call)
		callData?.iFrameReceived.observe(onChange: { (video) in
			if (video == true) {
				Core.get().nativeVideoWindow = self.earlyMediaView
				self.earlyMediaView?.isHidden = false
			}
		})
		callData?.callState.readCurrentAndObserve(onChange: { (state) in
			if (ConfigManager.instance().lpConfigBoolForKey(key: "pref_accept_early_media") && state == .IncomingReceived) {
				try?self.callData?.call.acceptEarlyMedia()
				self.callData?.call.requestNotifyNextVideoFrameDecoded()
			}
		})
		callData?.isIncoming.readCurrentAndObserve { (incoming) in
			if (incoming != true) {
				PhoneMainView.instance().popView(self.compositeViewDescription())
			}
		}

		if (ConfigManager.instance().lpConfigBoolForKey(key: "auto_answer")) {
			CallManager.instance().acceptCall(call: call, hasVideo: false) // TODO check with old version for Video accept separate button - Not implemented in Android
		}
	}
	

}
