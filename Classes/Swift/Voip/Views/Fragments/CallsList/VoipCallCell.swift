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

class VoipCallCell: UITableViewCell {
	
	// Layout Constants
	let cell_height = 80.0
	let call_status_icon_size = 65.0
	static let avatar_size =  45.0
	let avatar_left_margin = 40.0
	let texts_left_margin = 20.0
	let side_menu_icon_size =  70.0
	
	var onMenuClickAction : (()->Void) = {}
	let callStatusIcon = UIImageView()
	let avatar = Avatar(color:LightDarkColor(VoipTheme.voip_contact_avatar_calls_list,VoipTheme.voip_contact_avatar_calls_list), textStyle: VoipTheme.call_generated_avatar_small)
	let displayName = StyledLabel(VoipTheme.call_list_active_name_font)
	let sipAddress = StyledLabel(VoipTheme.call_list_active_sip_uri_font)
	var menuButton : CallControlButton? = nil
	var owningCallsListView : CallsListView? = nil

	var callData: CallData? = nil {
		didSet {
			if let data = callData {
				contentView.backgroundColor = data.isPaused.value == true ? VoipTheme.voip_calls_list_inactive_background : VoipTheme.voip_dark_gray
				callStatusIcon.image =
					data.isIncoming.value == true ?  UIImage(named:"voip_call_header_incoming") :
					data.isOutgoing.value == true ? UIImage(named:"voip_call_header_outgoing") :
					data.isPaused.value == true ? UIImage(named:"voip_call_header_paused") :
					UIImage(named:"voip_call_header_active")
				if (data.isInRemoteConference.value == true) {
					displayName.text = data.remoteConferenceSubject.value
					//sipAddress.text = data.call.conference?.participantList.map{ String($0.address?.addressBookEnhancedDisplayName())}.joined(separator: ",")
					avatar.fillFromAddress(address: data.call.remoteAddress!,isGroup:true)
				} else {
					displayName.text = data.call.remoteAddress?.addressBookEnhancedDisplayName()
					avatar.fillFromAddress(address: data.call.remoteAddress!)
					sipAddress.text = data.call.remoteAddress?.asStringUriOnly()
				}
				displayName.applyStyle(data.isPaused.value == true ? VoipTheme.call_list_name_font : VoipTheme.call_list_active_name_font)
				sipAddress.applyStyle(data.isPaused.value == true ? VoipTheme.call_list_sip_uri_font : VoipTheme.call_list_active_sip_uri_font)
				menuButton?.applyTintedIcons(tintedIcons: data.isPaused.value == true ? VoipTheme.voip_call_list_menu.tintableStateIcons : VoipTheme.voip_call_list_active_menu.tintableStateIcons)
			}
		}
	}
	
	
	override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
		super.init(style: style, reuseIdentifier: reuseIdentifier)
		contentView.height(cell_height).matchParentSideBorders().done()
		
		contentView.addSubview(callStatusIcon)
		callStatusIcon.size(w: call_status_icon_size, h: call_status_icon_size).done()
		
		contentView.addSubview(avatar)
		avatar.size(w: VoipCallCell.avatar_size, h: VoipCallCell.avatar_size).centerY().alignParentLeft(withMargin: avatar_left_margin).done()
				
		let nameAddress = UIView()
		nameAddress.addSubview(displayName)
		nameAddress.addSubview(sipAddress)
		displayName.alignParentTop().done()
		sipAddress.alignUnder(view: displayName).done()
		contentView.addSubview(nameAddress)
		nameAddress.toRightOf(avatar,withLeftMargin:texts_left_margin).wrapContentY().centerY().done()
		
		menuButton = CallControlButton(width:Int(side_menu_icon_size), height:Int(side_menu_icon_size),  buttonTheme: VoipTheme.voip_call_list_active_menu, onClickAction: {
			self.owningCallsListView?.toggleMenu(forCell: self)
		})
		addSubview(menuButton!)
		menuButton!.alignParentRight().centerY().done()
		
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
}
