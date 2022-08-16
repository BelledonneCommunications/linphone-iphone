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

import Foundation
import UIKit

class ConferenceLayoutPickerView: UIView {

	// Layout constants
	let corner_radius = 6.7
	let margin = 10.0
	let stackView = UIStackView()
	let insets = 5.0


	init (orientation:UIDeviceOrientation) {
		super.init(frame: .zero)
		stackView.distribution = .fillProportionally
		stackView.alignment = .center
		stackView.spacing = ControlsView.controls_button_spacing
		backgroundColor = VoipTheme.voip_gray
		layer.cornerRadius = corner_radius
		clipsToBounds = true
	
		let grid = CallControlButton(imageInset : UIEdgeInsets(top: 5, left: 5, bottom: 5, right: 5),buttonTheme: VoipTheme.conf_waiting_room_layout_picker, onClickAction: {
			ConferenceWaitingRoomViewModel.sharedModel.joinLayout.value = .Grid
			ConferenceWaitingRoomViewModel.sharedModel.showLayoutPicker.value = false

		})
		grid.applyTintedIcons(tintedIcons: [UIButton.State.normal.rawValue : TintableIcon(name: "voip_conference_mosaic" ,tintColor: LightDarkColor(.white,.white))])
		stackView.addArrangedSubview(grid)
		
		let activeSpeaker = CallControlButton(imageInset : UIEdgeInsets(top: 5, left: 5, bottom: 5, right: 5),buttonTheme: VoipTheme.conf_waiting_room_layout_picker, onClickAction: {
			ConferenceWaitingRoomViewModel.sharedModel.joinLayout.value = .ActiveSpeaker
			ConferenceWaitingRoomViewModel.sharedModel.showLayoutPicker.value = false
		})
		activeSpeaker.applyTintedIcons(tintedIcons: [UIButton.State.normal.rawValue : TintableIcon(name: "voip_conference_active_speaker" ,tintColor: LightDarkColor(.white,.white))])
		stackView.addArrangedSubview(activeSpeaker)
		
		let audioOnly = CallControlButton(imageInset : UIEdgeInsets(top: 5, left: 5, bottom: 5, right: 5),buttonTheme: VoipTheme.conf_waiting_room_layout_picker, onClickAction: {
			ConferenceWaitingRoomViewModel.sharedModel.joinLayout.value = .AudioOnly
			ConferenceWaitingRoomViewModel.sharedModel.showLayoutPicker.value = false
		})
		audioOnly.applyTintedIcons(tintedIcons: [UIButton.State.normal.rawValue : TintableIcon(name: "voip_conference_audio_only" ,tintColor: LightDarkColor(.white,.white))])
		stackView.addArrangedSubview(audioOnly)
		
		ConferenceWaitingRoomViewModel.sharedModel.joinLayout.readCurrentAndObserve { layout in
			grid.isSelected = layout == .Grid
			activeSpeaker.isSelected = layout == .ActiveSpeaker
			audioOnly.isSelected = layout == .AudioOnly
		}
		
		stackView.axis = .vertical
		addSubview(stackView)
		wrapContent(inset: UIEdgeInsets(top: insets, left: insets, bottom: insets, right: insets)).done()
	}
	
	required init(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
}


