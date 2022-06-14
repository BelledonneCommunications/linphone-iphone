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

class VoipAudioOnlyParticipantCell: UICollectionViewCell {
	
	// Layout Constants
	static let cell_height = 80.0
	static let avatar_size =  40.0
	static let mute_size =  30
	let corner_radius = 6.7
	let common_margin = 10.0

	
	let avatar = Avatar(diameter:VoipCallCell.avatar_size,color:VoipTheme.voipBackgroundColor, textStyle: VoipTheme.call_generated_avatar_small)
	let paused = UIImageView(image: UIImage(named: "voip_pause")?.tinted(with: .white))
	let muted = MicMuted(VoipAudioOnlyParticipantCell.mute_size)

	let displayName = StyledLabel(VoipTheme.conference_participant_name_font_as)
	
	var participantData: ConferenceParticipantDeviceData? = nil {
		didSet {
			if let data = participantData {
				self.displayName.text = ""
				data.isInConference.clearObservers()
				data.isInConference.readCurrentAndObserve { (isIn) in
					self.avatar.isHidden = isIn != true
					self.paused.isHidden = isIn == true
					data.participantDevice.address.map {
						self.avatar.fillFromAddress(address: $0)
						if let displayName = $0.addressBookEnhancedDisplayName() {
							self.displayName.text = displayName + (isIn == true ? "" : " \(VoipTexts.conference_participant_paused)")
						}
					}
				}
				if (data.participantDevice.address == nil) {
					avatar.isHidden = true
				}
				data.activeSpeaker.clearObservers()
				data.activeSpeaker.readCurrentAndObserve { (active) in
					if (active == true) {
						self.layer.borderWidth = 2
					} else {
						self.layer.borderWidth = 0
					}
				}
				data.micMuted.clearObservers()
				data.micMuted.readCurrentAndObserve { (muted) in
					self.muted.isHidden = muted != true
				}
			}
		}
	}
	
	
	override init(frame:CGRect) {
		super.init(frame:.zero)
		contentView.height(VoipAudioOnlyParticipantCell.cell_height).matchParentSideBorders().done()
		
		layer.cornerRadius = corner_radius
		clipsToBounds = true
		contentView.backgroundColor = VoipTheme.voipParticipantBackgroundColor.get()
		layer.borderColor = VoipTheme.primary_color.cgColor
	
		contentView.addSubview(avatar)
		avatar.size(w: VoipCallCell.avatar_size, h: VoipCallCell.avatar_size).centerY().alignParentLeft(withMargin: common_margin).done()
		
		contentView.addSubview(paused)
		paused.layer.cornerRadius = VoipAudioOnlyParticipantCell.avatar_size/2
		paused.clipsToBounds = true
		paused.backgroundColor = VoipTheme.voip_gray
		paused.size(w: VoipAudioOnlyParticipantCell.avatar_size, h: VoipAudioOnlyParticipantCell.avatar_size).alignParentLeft(withMargin: common_margin).centerY().done()
				
		contentView.addSubview(displayName)
		displayName.centerY().toRightOf(avatar,withLeftMargin: common_margin).done()
		displayName.numberOfLines = 3
		
		contentView.addSubview(muted)
		muted.alignParentRight(withMargin: common_margin).toRightOf(displayName,withLeftMargin: common_margin).centerY().done()
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
}
