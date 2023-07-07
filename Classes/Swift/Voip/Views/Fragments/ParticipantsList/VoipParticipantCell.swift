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
	
	static let dismiss_icon_inset = UIEdgeInsets(top: 10, left: 10, bottom: 10, right: 10)
	let dismiss_right_margin = 10
	let check_box_size = 15
	static let cell_height = 80.0
	let avatar_left_margin = 15.0
	let texts_left_margin = 20.0
	let lime_badge_width = 18.0
	let lime_badge_offset = -10.0

	let avatar = Avatar(color:VoipTheme.primaryTextColor, textStyle: VoipTheme.call_generated_avatar_small)
	let limeBadge = UIImageView(image: UIImage(named: "security_toggle_icon_green"))
	let displayName = StyledLabel(VoipTheme.conference_participant_name_font)
	let sipAddress = StyledLabel(VoipTheme.conference_participant_sip_uri_font)
	let isAdminView = UIStackView()
	let isAdminLabel = StyledLabel(VoipTheme.conference_participant_admin_label,VoipTexts.chat_room_group_info_admin)
	let isAdminCheck = UIImageView(image: UIImage(named:("check_unselected")))
	let removePart = CallControlButton(imageInset:dismiss_icon_inset,buttonTheme: VoipTheme.voip_cancel, onClickAction: {})

	
	var owningParticpantsListView : ParticipantsListView? = nil
	
	var participantData: ConferenceParticipantData? = nil {
		didSet {
			if let data = participantData {
				limeBadge.isHidden = true
				avatar.fillFromAddress(address: data.participant.address!)
				displayName.text = data.participant.address?.addressBookEnhancedDisplayName()
				sipAddress.text = data.participant.address?.asStringUriOnly()
				data.isAdmin.readCurrentAndObserve { _ in
					self.setAdminStatus(data: data)
				}
				data.isMeAdmin.readCurrentAndObserve { _ in
					self.setAdminStatus(data: data)
				}
				self.isAdminView.onClick {
					data.conference.setParticipantAdminStatus(participant: data.participant, isAdmin: data.isAdmin.value != true)
					self.owningParticpantsListView?.participantsListTableView.reloadData()
				}
				self.removePart.onClick {
					try?data.conference.removeParticipant(participant: data.participant)
					self.owningParticpantsListView?.participantsListTableView.reloadData()
				}
			}
		}
	}
	
	func setAdminStatus(data:ConferenceParticipantData) {
		let isAdmin = data.isAdmin.value!
		let isMeAdmin = data.isMeAdmin.value!
		self.removePart.isHidden = !isMeAdmin
		self.isAdminView.isUserInteractionEnabled = isMeAdmin
		self.isAdminLabel.textColor = !isAdmin ? VoipTheme.primarySubtextLightColor.get() : VoipTheme.primaryTextColor.get()
		self.isAdminView.isHidden = !isAdmin && !isMeAdmin // Non admin don't see status of others non admin (they just see admins)
	}
	
	var scheduleConfParticipantAddress: Address?  = nil {
		didSet {
			if let address = scheduleConfParticipantAddress {
				avatar.fillFromAddress(address: address)
				displayName.text = address.addressBookEnhancedDisplayName()
				sipAddress.text = address.asStringUriOnly()
				self.isAdminView.isHidden = true
				self.removePart.isHidden = true
			}
		}
	}
	
	
	override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
		super.init(style: style, reuseIdentifier: reuseIdentifier)
		contentView.height(VoipParticipantCell.cell_height).matchParentSideBorders().done()
	
		addSubview(avatar)
		avatar.size(w: VoipCallCell.avatar_size, h: VoipCallCell.avatar_size).centerY().alignParentLeft(withMargin: avatar_left_margin).done()
		
		limeBadge.contentMode = .scaleAspectFit
		addSubview(limeBadge)
		limeBadge.toRightOf(avatar,withLeftMargin: lime_badge_offset).width(lime_badge_width).done()
		
		// Name Address
		
		let nameAddress = UIStackView()
		nameAddress.addArrangedSubview(displayName)
		nameAddress.addArrangedSubview(sipAddress)
		nameAddress.axis = .vertical
		addSubview(nameAddress)
		nameAddress.toRightOf(avatar,withLeftMargin:texts_left_margin).centerY().done()
		
		// Admin section
		isAdminView.spacing = 5
		
		isAdminView.addArrangedSubview(isAdminCheck)
		isAdminCheck.square(check_box_size).done()
		isAdminCheck.contentMode = .scaleAspectFit
		isAdminCheck.setContentHuggingPriority(.defaultHigh, for: .horizontal)
		
		isAdminView.addArrangedSubview(isAdminLabel)
		isAdminLabel.setContentHuggingPriority(.defaultHigh, for: .horizontal)

		isAdminView.addArrangedSubview(removePart)
		removePart.setContentHuggingPriority(.defaultHigh, for: .horizontal)

		addSubview(isAdminView)
		isAdminView.leadingAnchor.constraint(greaterThanOrEqualTo: leadingAnchor).isActive = true
		isAdminView.trailingAnchor.constraint(equalTo: trailingAnchor).isActive = true
		isAdminView.matchParentHeight().toRightOf(nameAddress).alignParentRight(withMargin: dismiss_right_margin).done()
		contentView.backgroundColor = .clear
		backgroundColor = .clear
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
}
