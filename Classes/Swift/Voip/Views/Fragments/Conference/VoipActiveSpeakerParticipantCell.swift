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

class VoipActiveSpeakerParticipantCell: UICollectionViewCell {
	
	// Layout Constants
	let corner_radius = 20.0
	static let avatar_size =  50.0
	let switch_camera_button_margins = 8.0
	let switch_camera_button_size = 30
	static let mute_size = 25
	let mute_margin = 5


	let videoView = UIView()
	let avatar = Avatar(diameter:VoipActiveSpeakerParticipantCell.avatar_size,color:VoipTheme.voipBackgroundColor, textStyle: VoipTheme.call_generated_avatar_medium)
	let pause = UIImageView(image: UIImage(named: "voip_pause")?.tinted(with: .white))
	let switchCamera = UIImageView(image: UIImage(named:"voip_change_camera")?.tinted(with:.white))
	let displayName = StyledLabel(VoipTheme.conference_participant_name_font_as)
	let pauseLabel = StyledLabel(VoipTheme.conference_participant_name_font_as,VoipTexts.conference_participant_paused)
	let muted = MicMuted(VoipActiveSpeakerParticipantCell.mute_size)

	var participantData: ConferenceParticipantDeviceData? = nil {
		didSet {
			if let data = participantData {
				data.isInConference.clearObservers()
				data.isInConference.readCurrentAndObserve { (isIn) in
					self.updateBackground()
					self.pause.isHidden = isIn == true
					self.pauseLabel.isHidden = self.pause.isHidden
					self.videoView.isHidden = data.videoEnabled.value != true
					self.switchCamera.isHidden = data.videoEnabled.value != true || !data.isSwitchCameraAvailable()
				}
				data.videoEnabled.clearObservers()
				data.videoEnabled.readCurrentAndObserve { (videoEnabled) in
					self.updateBackground()
					if (videoEnabled == true) {
						self.videoView.isHidden = false
						data.setVideoView(view: self.videoView)
						self.avatar.isHidden = true
					} else {
						self.videoView.isHidden = true
						self.avatar.isHidden = false
					}
					self.switchCamera.isHidden = videoEnabled != true || !data.isSwitchCameraAvailable()
				}
				data.participantDevice.address.map {
					avatar.fillFromAddress(address: $0)
					if let displayName = $0.addressBookEnhancedDisplayName() {
						self.displayName.text = displayName
					}
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
	
	func updateBackground() {
		if let data = participantData  {
			if (data.isInConference.value != true) {
				self.contentView.backgroundColor = VoipTheme.voip_conference_participant_paused_background
			} else if (data.videoEnabled.value == true) {
				self.contentView.backgroundColor = .black
			} else {
				self.contentView.backgroundColor = VoipTheme.voipParticipantBackgroundColor.get()

			}
		}
	}
	
	
	override init(frame:CGRect) {
		super.init(frame:.zero)
		layer.cornerRadius = corner_radius
		clipsToBounds = true
		layer.borderColor = VoipTheme.primary_color.cgColor
		
		contentView.addSubview(videoView)
		videoView.matchParentDimmensions().done()
		
		contentView.addSubview(avatar)
		avatar.size(w: VoipActiveSpeakerParticipantCell.avatar_size, h: VoipActiveSpeakerParticipantCell.avatar_size).center().done()
		
		contentView.addSubview(pause)
		pause.layer.cornerRadius = VoipActiveSpeakerParticipantCell.avatar_size/2
		pause.clipsToBounds = true
		pause.backgroundColor = VoipTheme.voip_gray
		pause.size(w: VoipActiveSpeakerParticipantCell.avatar_size, h: VoipActiveSpeakerParticipantCell.avatar_size).center().done()
		
		contentView.addSubview(switchCamera)
		switchCamera.alignParentTop(withMargin: switch_camera_button_margins).alignParentRight(withMargin: switch_camera_button_margins).square(switch_camera_button_size).done()
		switchCamera.contentMode = .scaleAspectFit
		
		switchCamera.onClick {
			Core.get().toggleCamera()
		}
		
		contentView.addSubview(displayName)
		displayName.matchParentSideBorders(insetedByDx:ActiveCallView.bottom_displayname_margin_left).alignParentBottom(withMargin:ActiveCallView.bottom_displayname_margin_bottom).done()
	
		// Paused label commented out as in Android 10.06.2022
		// contentView.addSubview(pauseLabel)
		//pauseLabel.toRightOf(displayName).alignParentBottom(withMargin:ActiveCallView.bottom_displayname_margin_bottom).done()

		contentView.addSubview(muted)
		muted.alignParentLeft(withMargin: mute_margin).alignParentTop(withMargin:mute_margin).done()
		
		contentView.matchParentDimmensions().done()
		makeHeightMatchWidth().done()
		
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
}
