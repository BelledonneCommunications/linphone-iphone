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

class VoipParticipantCell: UITableViewCell {
	
	// Layout Constants
	
	let dismiss_icon_inset = UIEdgeInsets(top: 10, left: 10, bottom: 10, right: 10)
	let dismiss_right_margin = 10
	let check_box_size = 20.0
	static let cell_height = 80.0
	let avatar_left_margin = 15.0
	let texts_left_margin = 20.0
	let lime_badge_width = 18.0
	let lime_badge_offset = -10.0


	let avatar = Avatar(diameter:VoipCallCell.avatar_size,color:VoipTheme.primaryTextColor, textStyle: VoipTheme.call_generated_avatar_small)
	let limeBadge = UIImageView(image: UIImage(named: "security_toggle_icon_green"))
	let displayName = StyledLabel(VoipTheme.conference_participant_name_font)
	let sipAddress = StyledLabel(VoipTheme.conference_participant_sip_uri_font)
	let isAdminView = UIView()
	var removePart : CallControlButton?

	
	var owningParticpantsListView : ParticipantsListView? = nil

	var participantData: ConferenceParticipantData? = nil {
		didSet {
			if let data = participantData {
				limeBadge.isHidden = true
				avatar.fillFromAddress(address: data.participant.address!)
				displayName.text = data.participant.address?.addressBookEnhancedDisplayName()
				sipAddress.text = data.participant.address?.asStringUriOnly()
				data.isAdmin.readCurrentAndObserve { (isAdmin) in self.isAdminView.isHidden = isAdmin != true
					
				}
				data.isMeAdmin.readCurrentAndObserve { (isMeAdmin) in
					self.removePart!.isHidden = isMeAdmin != true
					self.isAdminView.alpha = isMeAdmin == true ? 1.0 : 0.6
					self.isAdminView.isUserInteractionEnabled = isMeAdmin == true
				}
				self.isAdminView.onClick {
					data.conference.setParticipantAdminStatus(participant: data.participant, isAdmin: data.isAdmin.value != true)
					self.owningParticpantsListView?.participantsListTableView.reloadData()
				}
				self.removePart?.onClick {
					try?data.conference.removeParticipant(participant: data.participant)
					self.owningParticpantsListView?.participantsListTableView.reloadData()
				}
			}
		}
	}
	
	var scheduleConfParticipantAddress: Address?  = nil {
		didSet {
			if let address = scheduleConfParticipantAddress {
				avatar.fillFromAddress(address: address)
				displayName.text = address.addressBookEnhancedDisplayName()
				sipAddress.text = address.asStringUriOnly()
				self.isAdminView.isHidden = true
				self.removePart?.isHidden = true
			}
		}
	}
	
	
	override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
		super.init(style: style, reuseIdentifier: reuseIdentifier)
		contentView.height(VoipParticipantCell.cell_height).matchParentSideBorders().done()
	
		contentView.addSubview(avatar)
		avatar.size(w: VoipCallCell.avatar_size, h: VoipCallCell.avatar_size).centerY().alignParentLeft(withMargin: avatar_left_margin).done()
		
		limeBadge.contentMode = .scaleAspectFit
		contentView.addSubview(limeBadge)
		limeBadge.toRightOf(avatar,withLeftMargin: lime_badge_offset).width(lime_badge_width).done()
	
		let nameAddress = UIView()
		nameAddress.addSubview(displayName)
		nameAddress.addSubview(sipAddress)
		displayName.alignParentTop().done()
		sipAddress.alignUnder(view: displayName).done()
		contentView.addSubview(nameAddress)
		nameAddress.toRightOf(avatar,withLeftMargin:texts_left_margin).wrapContentY().centerY().done()
		
		removePart = CallControlButton(imageInset:dismiss_icon_inset,buttonTheme: VoipTheme.voip_cancel, onClickAction: {
			self.removeFromSuperview()
		})
		contentView.addSubview(removePart!)
		removePart!.alignParentRight(withMargin: dismiss_right_margin).centerY().done()
		
		let isAdminLabel = StyledLabel(VoipTheme.conference_participant_admin_label,VoipTexts.chat_room_group_info_admin)
		isAdminView.addSubview(isAdminLabel)
		isAdminLabel.alignParentRight().centerY().done()
		
		let isAdminCheck = UIImageView(image: UIImage(named:("check_unselected")))
		isAdminView.addSubview(isAdminCheck)
		isAdminCheck.size(w: check_box_size, h: check_box_size).toLeftOf(isAdminLabel).done()
	
		contentView.addSubview(isAdminView)
		isAdminView.height(check_box_size).toLeftOf(removePart!).centerY().done()

		
		
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
}
