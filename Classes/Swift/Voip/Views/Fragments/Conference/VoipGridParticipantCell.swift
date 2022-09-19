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

class VoipGridParticipantCell: UICollectionViewCell {
	
	// Layout Constants
	let corner_radius = 20.0
	static let avatar_size =  80.0
	let switch_camera_button_margins = 8.0
	let switch_camera_button_size = 30
	let pause_label_left_margin = 5
	static let mute_size = 25
	let mute_margin = 5


	let videoView = UIView()
	let avatar = Avatar(color:VoipTheme.voipBackgroundColor, textStyle: VoipTheme.call_generated_avatar_medium)
	let pause = UIImageView(image: UIImage(named: "voip_pause")?.tinted(with: .white))
	let switchCamera = UIImageView(image: UIImage(named:"voip_change_camera")?.tinted(with:.white))
	let displayName = StyledLabel(VoipTheme.conference_participant_name_font_grid)
	let pauseLabel = StyledLabel(VoipTheme.conference_participant_name_font_grid,VoipTexts.conference_participant_paused)
	let muted = MicMuted(VoipActiveSpeakerParticipantCell.mute_size)
	let joining = RotatingSpinner()

	var participantData: ConferenceParticipantDeviceData? = nil {
		didSet {
			if let data = participantData {
				self.updateElements()
				data.isJoining.clearObservers()
				data.isJoining.observe { _ in
					self.updateElements()
				}
				data.isInConference.clearObservers()
				data.isInConference.observe { _ in
					self.updateElements()
				}
				data.videoEnabled.clearObservers()
				data.videoEnabled.observe { _ in
					self.updateElements()
				}
				data.participantDevice.address.map {
					avatar.fillFromAddress(address: $0)
					if let displayName = $0.addressBookEnhancedDisplayName() {
						self.displayName.text = displayName
					}
				}
				data.isSpeaking.clearObservers()
				data.isSpeaking.observe { _ in
					self.updateElements(skipVideo: true)
				}
				data.micMuted.clearObservers()
				data.micMuted.observe { _ in
					self.updateElements(skipVideo: true)
				}
			}
		}
	}

	func updateElements(skipVideo:Bool = false) {
		if let data = participantData  {
			
			// Background
			if (data.isInConference.value != true && data.isJoining.value != true) {
				self.contentView.backgroundColor = VoipTheme.voip_conference_participant_paused_background
			} else if (data.videoEnabled.value == true) {
				self.contentView.backgroundColor = .black
			} else {
				self.contentView.backgroundColor = data.isMe ? VoipTheme.voipParticipantMeBackgroundColor.get() :  VoipTheme.voipParticipantBackgroundColor.get()
			}
			
			// Avatar
			self.avatar.isHidden = (data.isInConference.value != true && data.isJoining.value != true) || data.videoEnabled.value == true

			// Video
			if (!skipVideo) {
				self.videoView.isHidden = data.isInConference.value != true || data.videoEnabled.value != true
				if (!self.videoView.isHidden) {
					data.setVideoView(view: self.videoView)
				}
				self.switchCamera.isHidden = self.videoView.isHidden || !data.isSwitchCameraAvailable()
			}
			
			// Pause
			self.pause.isHidden = data.isInConference.value == true || data.isJoining.value == true
			self.pauseLabel.isHidden = self.pause.isHidden
			
			// Border for active speaker
			self.layer.borderWidth = data.isSpeaking.value == true ? 2 : 0
			
			// Joining indicator
			if (data.isJoining.value == true) {
				self.joining.isHidden = false
				self.joining.startRotation()
			} else {
				self.joining.isHidden = true
				self.joining.stopRotation()
			}
			
			// Muted
			self.muted.isHidden = data.micMuted.value != true
			
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
		avatar.size(w: VoipGridParticipantCell.avatar_size, h: VoipGridParticipantCell.avatar_size).center().done()
		
		contentView.addSubview(pause)
		pause.layer.cornerRadius = VoipGridParticipantCell.avatar_size/2
		pause.clipsToBounds = true
		pause.backgroundColor = VoipTheme.voip_gray
		pause.size(w: VoipGridParticipantCell.avatar_size, h: VoipGridParticipantCell.avatar_size).center().done()
		
		contentView.addSubview(switchCamera)
		switchCamera.alignParentTop(withMargin: switch_camera_button_margins).alignParentRight(withMargin: switch_camera_button_margins).square(switch_camera_button_size).done()
		switchCamera.contentMode = .scaleAspectFit
		
		switchCamera.onClick {
			Core.get().toggleCamera()
		}
		
		contentView.addSubview(displayName)
		displayName.alignParentLeft(withMargin:ActiveCallView.bottom_displayname_margin_left).alignParentBottom(withMargin:ActiveCallView.bottom_displayname_margin_bottom).alignParentRight().done()

		contentView.addSubview(pauseLabel)
		pauseLabel.toRightOf(displayName,withLeftMargin: pause_label_left_margin).alignParentBottom(withMargin:ActiveCallView.bottom_displayname_margin_bottom).done()

		contentView.addSubview(muted)
		muted.alignParentLeft(withMargin: mute_margin).alignParentTop(withMargin:mute_margin).done()
		
		contentView.addSubview(joining)
		joining.square(VoipActiveSpeakerParticipantCell.mute_size).alignParentTop(withMargin: mute_margin).alignParentLeft(withMargin: mute_margin).done()
		
		
		contentView.matchParentDimmensions().done()
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
}
