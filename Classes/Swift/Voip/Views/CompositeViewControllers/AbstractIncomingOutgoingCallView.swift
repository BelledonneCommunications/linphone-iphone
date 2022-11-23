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

@objc class AbstractIncomingOutgoingCallView: UIViewController { // Shared between IncomingCallView and OutgoingCallVIew (all upper part except control buttons)

	// Layout constants
	static let spinner_size = 50
	static let spinner_margin_top = 8.0
	static let call_type_margin_top = 10.0
	static let duration_margin_top = 10.0
	static let display_name_height = 20.0
	static let display_name_margin_top = 18.0
	static let sip_address_height = 16.0
	static let sip_address_margin_top = 6.0
	static let answer_decline_inset = UIEdgeInsets(top: 2, left: 7, bottom: 2, right: 7)
	
	let spinner = RotatingSpinner()
	let duration = CallTimer(nil, VoipTheme.call_header_subtitle)
	let avatar = Avatar(color:VoipTheme.voipParticipantBackgroundColor, textStyle: VoipTheme.call_generated_avatar_large)
	let displayName = StyledLabel(VoipTheme.call_header_title)
	let sipAddress = StyledLabel(VoipTheme.call_header_subtitle)

	var callData: CallData? = nil {
		didSet {
			duration.call = callData?.call
			callData?.call.remoteAddress.map {
				avatar.fillFromAddress(address: $0)
				displayName.text = $0.addressBookEnhancedDisplayName()
				sipAddress.text = $0.asStringUriOnly()
			}
		}
	}
			
	func viewDidLoad(forCallType:String) {
		super.viewDidLoad()
				
		view.backgroundColor = VoipTheme.voipBackgroundColor.get()
		
		view.addSubview(spinner)
		spinner.square(AbstractIncomingOutgoingCallView.spinner_size).matchParentSideBorders().alignParentTop(withMargin:AbstractIncomingOutgoingCallView.spinner_margin_top + UIDevice.notchHeight()).done()
		
		let callType = StyledLabel(VoipTheme.call_header_title,forCallType) 
		view.addSubview(callType)
		callType.matchParentSideBorders().alignUnder(view:spinner,withMargin:AbstractIncomingOutgoingCallView.call_type_margin_top).done()
				
		self.view.addSubview(duration)
		duration.matchParentSideBorders().alignUnder(view:callType,withMargin:AbstractIncomingOutgoingCallView.duration_margin_top).done()
		
		// Center : Avatar + Display name + SIP Address
		let centerSection = UIView()
		centerSection.addSubview(avatar)
		centerSection.addSubview(displayName)
		displayName.height(AbstractIncomingOutgoingCallView.display_name_height).matchParentSideBorders().alignUnder(view:avatar,withMargin:AbstractIncomingOutgoingCallView.display_name_margin_top).done()
		centerSection.addSubview(sipAddress)
		sipAddress.height(AbstractIncomingOutgoingCallView.sip_address_height).matchParentSideBorders().alignUnder(view:displayName,withMargin:AbstractIncomingOutgoingCallView.sip_address_margin_top).done()
		self.view.addSubview(centerSection)
		centerSection.matchParentSideBorders().center().done()
		
		layoutRotatableElements()
	}
	
	func layoutRotatableElements() {
		avatar.removeConstraints().done()
		if ([.landscapeLeft, .landscapeRight].contains( UIDevice.current.orientation)) {
			avatar.square(Avatar.diameter_for_call_views_land).center().done()
		} else {
			avatar.square(Avatar.diameter_for_call_views).center().done()
		}
	}
	
	override func didRotate(from fromInterfaceOrientation: UIInterfaceOrientation) {
		super.didRotate(from: fromInterfaceOrientation)
		self.layoutRotatableElements()
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		spinner.startRotation()
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		spinner.stopRotation()
		super.viewWillDisappear(animated)
	}
	
	@objc func setCall(call:OpaquePointer) {
		callData = CallData(call: Call.getSwiftObject(cObject: call))
	}
	
}
