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

@objc class OutgoingCallView: AbstractIncomingOutgoingCallView, UICompositeViewDelegate {
		
	// Layout constants
	let numpad_icon_padding = 10.0

	var numpadView : NumpadView? = nil
	var showNumPad : CallControlButton? = nil
	var shadingMask = UIView()

	
    static let compositeDescription = UICompositeViewDescription(OutgoingCallView.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: nil, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return OutgoingCallView.compositeDescription }
			
	override func viewDidLoad() {
		super.viewDidLoad(forCallType: VoipTexts.call_outgoing_title)
		
		// Cancel
		let cancelCall = CallControlButton(width: CallControlButton.hungup_width, imageInset:AbstractIncomingOutgoingCallView.answer_decline_inset, buttonTheme: VoipTheme.call_terminate, onClickAction: {
			self.callData.map { CallManager.instance().terminateCall(call: $0.call.getCobject)}
		})
		view.addSubview(cancelCall)
		cancelCall.alignParentLeft(withMargin:SharedLayoutConstants.margin_call_view_side_controls_buttons).alignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		
		// Controls
		let controlsView = ControlsView(showVideo: false, controlsViewModel: ControlsViewModel.shared)
		view.addSubview(controlsView)
		controlsView.alignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).centerX().done()
		
		// Shading mask, everything after will be shaded upon displayed
	   shadingMask.backgroundColor = VoipTheme.voip_translucent_popup_background
	   shadingMask.isHidden = true
	   self.view.addSubview(shadingMask)
	   shadingMask.matchParentDimmensions().done()
		
		// Numpad
		showNumPad = CallControlButton(imageInset:UIEdgeInsets(top: numpad_icon_padding, left: numpad_icon_padding, bottom: numpad_icon_padding, right: numpad_icon_padding), buttonTheme: VoipTheme.call_numpad, onClickAction: {
			self.numpadView?.removeFromSuperview()
			self.shadingMask.isHidden = false
			self.numpadView = NumpadView(superView: self.view,callData: self.callData!, marginTop: 0.0,above:controlsView, onDismissAction: {
				self.numpadView?.removeFromSuperview()
				self.shadingMask.isHidden = true
			})
		})
		view.addSubview(showNumPad!)
		showNumPad?.alignParentRight(withMargin:SharedLayoutConstants.margin_call_view_side_controls_buttons).alignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		showNumPad!.isHidden = true
		
		// Audio Routes
		let audioRoutesView = AudioRoutesView()
		view.addSubview(audioRoutesView)
		audioRoutesView.alignBottomWith(otherView: controlsView).done()
		ControlsViewModel.shared.audioRoutesSelected.readCurrentAndObserve { (audioRoutesSelected) in
			audioRoutesView.isHidden = audioRoutesSelected != true
		}
		audioRoutesView.alignAbove(view:controlsView,withMargin:SharedLayoutConstants.buttons_bottom_margin).matchRightOf(view: controlsView, withMargin:+ControlsView.controls_button_spacing).done()
	}
	
	
	override func viewWillAppear(_ animated: Bool) {
		ControlsViewModel.shared.audioRoutesSelected.value = false
		super.viewWillAppear(animated)
		if (Core.get().callsNb == 0) {
			PhoneMainView.instance().popView(self.compositeViewDescription())
		}
	}
	
	@objc override func setCall(call:OpaquePointer) {
		super.setCall(call: call)
		self.callData?.outgoingEarlyMedia.readCurrentAndObserve(onChange: { (outgoingEM) in
			self.showNumPad!.isHidden = outgoingEM != true
		})
		callData?.isOutgoing.readCurrentAndObserve { (outgoing) in
			if (outgoing != true) {
				PhoneMainView.instance().popView(self.compositeViewDescription())
			}
		}
	}
	

}
