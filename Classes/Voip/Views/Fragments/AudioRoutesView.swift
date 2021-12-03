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

class AudioRoutesView: UIStackView {
	
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
	
		// bluetooth
		let blueTooth = CallControlButton(buttonTheme: VoipTheme.route_bluetooth, onClickAction: {
			ControlsViewModel.shared.forceBluetoothAudioRoute()
			ControlsViewModel.shared.audioRoutesSelected.value = false
		})
		addArrangedSubview(blueTooth)
		
		ControlsViewModel.shared.isBluetoothHeadsetSelected.readCurrentAndObserve { (selected) in
			blueTooth.isSelected = selected == true
		}
		
		// Earpiece
		let earpiece = CallControlButton(buttonTheme: VoipTheme.route_earpiece, onClickAction: {
			ControlsViewModel.shared.forceEarpieceAudioRoute()
			ControlsViewModel.shared.audioRoutesSelected.value = false
		})
		addArrangedSubview(earpiece)
		ControlsViewModel.shared.isSpeakerSelected.readCurrentAndObserve { (isSpeakerSelected) in
			earpiece.isSelected = isSpeakerSelected != true && ControlsViewModel.shared.isBluetoothHeadsetSelected.value != true
		}
		ControlsViewModel.shared.isBluetoothHeadsetSelected.readCurrentAndObserve { (isBluetoothHeadsetSelected) in
			earpiece.isSelected = isBluetoothHeadsetSelected != true && ControlsViewModel.shared.isSpeakerSelected.value != true
		}
		
		// Speaker
		let speaker = CallControlButton(buttonTheme: VoipTheme.route_speaker, onClickAction: {
			ControlsViewModel.shared.forceSpeakerAudioRoute()
			ControlsViewModel.shared.audioRoutesSelected.value = false
		})
		addArrangedSubview(speaker)
		ControlsViewModel.shared.isSpeakerSelected.readCurrentAndObserve { (selected) in
			speaker.isSelected = selected == true
		}
		
		size(w:CGFloat(CallControlButton.default_size)+margin, h : 3*CGFloat(CallControlButton.default_size)+2*CGFloat(ControlsView.controls_button_spacing)+margin).done()

	}
	
	required init(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
}


