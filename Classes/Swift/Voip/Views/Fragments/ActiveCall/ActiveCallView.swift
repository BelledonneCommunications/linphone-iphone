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

class ActiveCallView: UIView { // = currentCall
	
	// Layout constants :
	static let top_displayname_margin_top = 20.0
	let sip_address_margin_top = 4.0
	static let remote_recording_margin_top = 10.0
	static let remote_recording_height = 30
	static let bottom_displayname_margin_bottom = 10.0
	static let bottom_displayname_margin_left = 12.0
	static let center_view_margin_top = 15.0
	static let center_view_corner_radius = 20.0
	let record_pause_button_size = 40
	let record_pause_button_inset = UIEdgeInsets(top: 7, left: 7, bottom: 7, right: 7)
	let record_pause_button_margin = 10.0
	static let local_video_width = 150.0
	static let local_video_margins = 15.0

		
	let upperSection = UIStackView()
	let displayNameTop = StyledLabel(VoipTheme.call_display_name_duration)
	let duration = CallTimer(nil, VoipTheme.call_display_name_duration)
	let sipAddress = StyledLabel(VoipTheme.call_sip_address)
	let remotelyRecordedIndicator =  RemotelyRecordingView(height: ActiveCallView.remote_recording_height,text: VoipTexts.call_remote_recording)
	
	let centerSection = UIView()
	let avatar = Avatar(color:VoipTheme.voipBackgroundColor, textStyle: VoipTheme.call_generated_avatar_large)
	let displayNameBottom = StyledLabel(VoipTheme.call_remote_name)
	var recordCallButtons : [CallControlButton] = []
	var pauseCallButtons :  [CallControlButton] = []
	let remoteVideo = UIView()
	let localVideo = LocalVideoView(width: local_video_width)

	var callData: CallData? = nil {
		didSet {
			guard let callData = callData else {
				return
			}
			duration.call = callData.call
			callData.call.remoteAddress.map {
				avatar.fillFromAddress(address: $0)
				if let displayName = $0.addressBookEnhancedDisplayName() {
					displayNameTop.text = displayName+" - "
					displayNameBottom.text = displayName
				}
				sipAddress.text = $0.asStringUriOnly()
			}
			self.remotelyRecordedIndicator.isRemotelyRecorded = callData.isRemotelyRecorded
			callData.isRecording.readCurrentAndObserve { (selected) in
				self.recordCallButtons.forEach {
					$0.isSelected = selected == true
				}
			}
			callData.isPaused.readCurrentAndObserve { (paused) in
				self.pauseCallButtons.forEach {
					$0.isSelected = paused == true
				}
				if (paused == true) {
					self.localVideo.isHidden = true
				}
			}
			callData.isRemotelyRecorded.readCurrentAndObserve { (remotelyRecorded) in
				self.centerSection.removeConstraints().matchParentSideBorders().alignUnder(view:remotelyRecorded == true ? self.remotelyRecordedIndicator : self.upperSection ,withMargin: ActiveCallView.center_view_margin_top).alignParentBottom().done()
				self.setNeedsLayout()
			}
						
			Core.get().nativeVideoWindow = remoteVideo
			Core.get().nativePreviewWindowId = UnsafeMutableRawPointer(Unmanaged.passRetained(localVideo).toOpaque())
			
			ControlsViewModel.shared.isVideoEnabled.readCurrentAndObserve{ (video) in
				self.remoteVideo.isHidden = video != true
				self.localVideo.isHidden = video != true
				self.recordCallButtons.first?.isHidden = video != true
				self.pauseCallButtons.first?.isHidden = video != true
				self.recordCallButtons.last?.isHidden = video == true
				self.pauseCallButtons.last?.isHidden = video == true
				UIApplication.shared.isIdleTimerDisabled = video == true
			}

		}
	}
			
	init() {
		super.init(frame: .zero)
		let stack = UIStackView()
		stack.distribution = .equalSpacing
		stack.alignment = .bottom
		stack.spacing = record_pause_button_margin
		stack.axis = .vertical
				
		let displayNameDurationSipAddress = UIView()
		
		displayNameDurationSipAddress.addSubview(displayNameTop)
		displayNameTop.alignParentLeft().done()
		
		displayNameDurationSipAddress.addSubview(duration)
		duration.toRightOf(displayNameTop).alignParentRight().done()
		
		displayNameDurationSipAddress.addSubview(sipAddress)
		sipAddress.matchParentSideBorders().alignUnder(view: displayNameTop,withMargin:sip_address_margin_top).done()
	
		upperSection.distribution = .equalSpacing
		upperSection.alignment = .center
		upperSection.spacing = record_pause_button_margin
		upperSection.axis = .horizontal
		
		upperSection.addArrangedSubview(displayNameDurationSipAddress)
		displayNameDurationSipAddress.wrapContentY().done()
		
		let recordPauseView = UIStackView()
		recordPauseView.spacing = record_pause_button_margin
		
		// Record (with video)
		var recordCall = CallControlButton(width: record_pause_button_size, height: record_pause_button_size, imageInset:record_pause_button_inset, buttonTheme: VoipTheme.call_record, onClickAction: {
			self.callData.map { $0.toggleRecord() }
		})
		recordCallButtons.append(recordCall)
		recordPauseView.addArrangedSubview(recordCall)
		
		// Pause (with video)
		var pauseCall = CallControlButton(width: record_pause_button_size, height: record_pause_button_size, imageInset:record_pause_button_inset, buttonTheme: VoipTheme.call_pause, onClickAction: {
			self.callData.map { $0.togglePause() }
		})
		pauseCallButtons.append(pauseCall)
		recordPauseView.addArrangedSubview(pauseCall)
		upperSection.addArrangedSubview(recordPauseView)

		
		stack.addArrangedSubview(upperSection)
		upperSection.matchParentSideBorders().alignParentTop(withMargin:ActiveCallView.top_displayname_margin_top).done()
		
		
		stack.addArrangedSubview(remotelyRecordedIndicator)
		remotelyRecordedIndicator.matchParentSideBorders().height(CGFloat(ActiveCallView.remote_recording_height)).done()

		// Center Section : Avatar + video + record/pause buttons + videos
		centerSection.layer.cornerRadius = ActiveCallView.center_view_corner_radius
		centerSection.clipsToBounds = true
		centerSection.backgroundColor = VoipTheme.voipParticipantBackgroundColor.get()
		
		// Record (w/o video)
		recordCall = CallControlButton(width: record_pause_button_size, height: record_pause_button_size, imageInset:record_pause_button_inset, buttonTheme: VoipTheme.call_record, onClickAction: {
			self.callData.map { $0.toggleRecord() }
		})
		recordCallButtons.append(recordCall)
		centerSection.addSubview(recordCall)
		recordCall.alignParentLeft(withMargin:record_pause_button_margin).alignParentTop(withMargin:record_pause_button_margin).done()
		
		// Pause (w/o video)
		pauseCall = CallControlButton(width: record_pause_button_size, height: record_pause_button_size, imageInset:record_pause_button_inset, buttonTheme: VoipTheme.call_pause, onClickAction: {
			self.callData.map { $0.togglePause() }
		})
		pauseCallButtons.append(pauseCall)
		centerSection.addSubview(pauseCall)
		pauseCall.alignParentRight(withMargin:record_pause_button_margin).alignParentTop(withMargin:record_pause_button_margin).done()
		
		// Avatar
		centerSection.addSubview(avatar)
		
		// Remote Video Display
		centerSection.addSubview(remoteVideo)
		remoteVideo.isHidden = true
		remoteVideo.matchParentDimmensions().done()
				
		// Local Video Display
		centerSection.addSubview(localVideo)
		localVideo.backgroundColor = .black
		localVideo.alignParentBottom(withMargin: ActiveCallView.local_video_margins).alignParentRight(withMargin: ActiveCallView.local_video_margins).done()
		localVideo.isHidden = true
		localVideo.dragZone = centerSection
		
		// Full screen video togggle
		remoteVideo.onClick {
			ControlsViewModel.shared.toggleFullScreen()
		}
		ControlsViewModel.shared.fullScreenMode.observe { (fullScreen) in
			if (self.superview?.superview?.superview == nil) {
				return
			}
			self.remoteVideo.removeConstraints().done()
			self.localVideo.removeConstraints().done()
			if (fullScreen == true) {
				self.remoteVideo.removeFromSuperview()
				self.localVideo.removeFromSuperview()
				PhoneMainView.instance().mainViewController.view?.addSubview(self.remoteVideo)
				PhoneMainView.instance().mainViewController.view?.addSubview(self.localVideo)
			} else {
				self.remoteVideo.removeFromSuperview()
				self.localVideo.removeFromSuperview()
				self.centerSection.addSubview(self.remoteVideo)
				self.centerSection.addSubview(self.localVideo)
			}
			self.remoteVideo.matchParentDimmensions().done()
			self.localVideo.alignParentBottom(withMargin: ActiveCallView.local_video_margins).alignParentRight(withMargin: ActiveCallView.local_video_margins).done()
			self.localVideo.setSizeConstraint()
		}


		// Bottom display name
		centerSection.addSubview(displayNameBottom)
		displayNameBottom.alignParentLeft(withMargin:ActiveCallView.bottom_displayname_margin_left).alignParentRight().alignParentBottom(withMargin:ActiveCallView.bottom_displayname_margin_bottom).done()
		
		stack.addArrangedSubview(centerSection)
				
		addSubview(stack)
		stack.matchParentDimmensions().done()
	
		layoutRotatableElements()
	}
	
	func layoutRotatableElements() {
		avatar.removeConstraints().done()
		if ([.landscapeLeft, .landscapeRight].contains( UIDevice.current.orientation)) {
			avatar.square(Avatar.diameter_for_call_views_land).center().done()
		} else {
			avatar.square(Avatar.diameter_for_call_views).center().done()
		}
		localVideo.updateSizeConstraint()
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
}
