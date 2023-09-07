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

class ReactionCell: UITableViewCell {
	
	// Layout Constants
	static let cell_height = 50.0
	let avatar_left_margin = 15.0
	let texts_left_margin = 15.0
	let avatar_size = 35.0

	let avatar = Avatar(color:VoipTheme.primaryTextColor, textStyle: VoipTheme.call_generated_avatar_small)
	let displayName = StyledLabel(VoipTheme.conference_participant_name_font)
	let displayEmoji = StyledLabel(VoipTheme.conference_participant_name_font)

	
	var owningParticpantsListView : ParticipantsListView? = nil
	
	var reactionData: ChatMessageReaction? = nil {
		didSet {
			if let data = reactionData {
				avatar.fillFromAddress(address: data.fromAddress!)
				displayName.text = data.fromAddress!.addressBookEnhancedDisplayName()
				displayEmoji.text = data.body
			}
		}
	}
	
	override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
		super.init(style: style, reuseIdentifier: reuseIdentifier)
		contentView.height(ReactionCell.cell_height).matchParentSideBorders().done()
	
		addSubview(avatar)
		avatar.size(w: avatar_size, h: avatar_size).centerY().alignParentLeft(withMargin: avatar_left_margin).done()
		
		// Name Address
		
		let nameAddress = UIStackView()
		nameAddress.addArrangedSubview(displayName)
		nameAddress.axis = .vertical
		addSubview(nameAddress)
		nameAddress.toRightOf(avatar,withLeftMargin:texts_left_margin).centerY().done()
		
		
		addSubview(displayEmoji)
		displayEmoji.alignParentRight(withMargin: avatar_left_margin*2).centerY().done()
		
		contentView.backgroundColor = .clear
		backgroundColor = .clear
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
}
