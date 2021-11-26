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

class ControlsView: UIStackView {
	
	// Layout constants
	static let controls_button_spacing = 5.0
	
	init (showVideo:Bool) {
		super.init(frame: .zero)
		axis = .horizontal
		distribution = .equalSpacing
		alignment = .center
		spacing = ControlsView.controls_button_spacing
	
		// Mute
		let mute = CallControlButton(buttonTheme: VoipTheme.call_mute, onClickAction: {
			ControlsViewModel.shared.toggleMuteMicrophone()
		})
		addArrangedSubview(mute)
		ControlsViewModel.shared.isMicrophoneMuted.readCurrentAndObserve { (muted) in
			mute.isSelected = muted == true
		}
		ControlsViewModel.shared.isMuteMicrophoneEnabled.readCurrentAndObserve { (enabled) in
			mute.isEnabled = enabled == true
		}
		
		// Speaker
		let speaker = CallControlButton(buttonTheme: VoipTheme.call_speaker, onClickAction: {
			ControlsViewModel.shared.toggleSpeaker()
		})
		addArrangedSubview(speaker)
		ControlsViewModel.shared.isSpeakerSelected.readCurrentAndObserve { (selected) in
			speaker.isSelected = selected == true
		}
		
		// Audio routes
		let routes = CallControlButton(buttonTheme: VoipTheme.call_audio_route, onClickAction: {
			ControlsViewModel.shared.toggleRoutesMenu()
		})
		addArrangedSubview(routes)
		ControlsViewModel.shared.audioRoutesSelected.readCurrentAndObserve { (selected) in
			routes.isSelected = selected == true
		}
		
		ControlsViewModel.shared.audioRoutesEnabled.readCurrentAndObserve { (routesEnabled) in
			speaker.isHidden = routesEnabled == true
			routes.isHidden = !speaker.isHidden
		}
		
		// Video
		if (showVideo) {
			let video = CallControlButton(buttonTheme: VoipTheme.call_video, onClickAction: {
				if AVCaptureDevice.authorizationStatus(for: .video) ==  .authorized {
					ControlsViewModel.shared.toggleVideo()
				} else {
					AVCaptureDevice.requestAccess(for: .video, completionHandler: { (granted: Bool) in
						if granted {
							ControlsViewModel.shared.toggleVideo()
						} else {
							VoipDialog(message:VoipTexts.camera_required_for_video).show()
						}
					})
				}
			})
			addArrangedSubview(video)
			video.showActivityIndicatorDataSource = ControlsViewModel.shared.isVideoUpdateInProgress
			ControlsViewModel.shared.isVideoEnabled.readCurrentAndObserve { (selected) in
				video.isSelected = selected == true
			}
			ControlsViewModel.shared.isVideoAvailable.readCurrentAndObserve { (available) in
				video.isEnabled = available == true && ControlsViewModel.shared.isVideoUpdateInProgress.value != true
			}
			ControlsViewModel.shared.isVideoUpdateInProgress.readCurrentAndObserve { (updateInProgress) in
				video.isEnabled = updateInProgress != true && ControlsViewModel.shared.isVideoAvailable.value == true
			}

		}
		
		height(CallControlButton.default_size).done()
		
	}
	
	required init(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
}


