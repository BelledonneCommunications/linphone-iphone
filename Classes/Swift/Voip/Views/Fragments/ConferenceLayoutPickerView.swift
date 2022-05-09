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

class ConferenceLayoutPickerView: UIStackView {
	
	// Layout constants
	let corner_radius = 6.7
	let margin = 10.0

	init () {
		super.init(frame: .zero)
		axis = .vertical
		distribution = .equalCentering
		alignment = .center
		spacing = ControlsView.controls_button_spacing
		backgroundColor = VoipTheme.voip_gray
		layer.cornerRadius = corner_radius
		clipsToBounds = true
	
		let padding = UIView()
		padding.height(margin/2).done()
		addArrangedSubview(padding)

		
		let grid = CallControlButton(imageInset : UIEdgeInsets(top: 5, left: 5, bottom: 5, right: 5),buttonTheme: VoipTheme.conf_waiting_room_layout_picker, onClickAction: {
			ConferenceWaitingRoomViewModel.sharedModel.joinLayout.value = .Grid
			ConferenceWaitingRoomViewModel.sharedModel.showLayoutPicker.value = false

		})
		grid.applyTintedIcons(tintedIcons: [UIButton.State.normal.rawValue : TintableIcon(name: "voip_conference_mosaic" ,tintColor: LightDarkColor(.white,.white))])
		addArrangedSubview(grid)
		
		let activeSpeaker = CallControlButton(imageInset : UIEdgeInsets(top: 5, left: 5, bottom: 5, right: 5),buttonTheme: VoipTheme.conf_waiting_room_layout_picker, onClickAction: {
			ConferenceWaitingRoomViewModel.sharedModel.joinLayout.value = .ActiveSpeaker
			ConferenceWaitingRoomViewModel.sharedModel.showLayoutPicker.value = false
		})
		activeSpeaker.applyTintedIcons(tintedIcons: [UIButton.State.normal.rawValue : TintableIcon(name: "voip_conference_active_speaker" ,tintColor: LightDarkColor(.white,.white))])
		addArrangedSubview(activeSpeaker)
		
		let audioOnly = CallControlButton(imageInset : UIEdgeInsets(top: 5, left: 5, bottom: 5, right: 5),buttonTheme: VoipTheme.conf_waiting_room_layout_picker, onClickAction: {
			ConferenceWaitingRoomViewModel.sharedModel.joinLayout.value = .Grid
			ConferenceWaitingRoomViewModel.sharedModel.showLayoutPicker.value = false
		})
		audioOnly.applyTintedIcons(tintedIcons: [UIButton.State.normal.rawValue : TintableIcon(name: "voip_conference_audio_only" ,tintColor: LightDarkColor(.white,.white))])
		addArrangedSubview(audioOnly)
		
		ConferenceWaitingRoomViewModel.sharedModel.joinLayout.readCurrentAndObserve { layout in
			grid.isSelected = layout == .Grid
			activeSpeaker.isSelected = layout == .ActiveSpeaker
			audioOnly.isSelected = layout == .Grid
		}
		
		let padding2 = UIView()
		padding2.height(margin/2).done()
		addArrangedSubview(padding2)

		
		size(w:CGFloat(CallControlButton.default_size)+margin, h : 3*CGFloat(CallControlButton.default_size)+3*CGFloat(ControlsView.controls_button_spacing)+2*margin).done()

	}
	
	required init(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
}


