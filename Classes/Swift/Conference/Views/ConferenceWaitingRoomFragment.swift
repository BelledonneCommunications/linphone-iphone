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
import linphonesw


@objc class ConferenceWaitingRoomFragment: UIViewController, UICompositeViewDelegate { // Replaces CallView
	
	// Layout constants
	let common_margin = 17.0
	let switch_camera_button_size = 50
	let switch_camera_button_margins = 7.0
	let content_inset = 12.0
	let button_spacing = 15.0
	let center_view_corner_radius = 20.0
	let button_width = 150

	
	var audioRoutesView : AudioRoutesView? = nil
	let subject = StyledLabel(VoipTheme.conference_preview_subject_font)
	let localVideo = UIView()
	let switchCamera = UIImageView(image: UIImage(named:"voip_change_camera")?.tinted(with:.white))
	let noVideoLabel = StyledLabel(VoipTheme.conference_waiting_room_no_video_font, VoipTexts.conference_waiting_room_video_disabled)

	let buttonsView = UIStackView()
	let cancel = FormButton(title: VoipTexts.cancel.uppercased(), backgroundStateColors: VoipTheme.primary_colors_background_gray, bold:false)
	let start = FormButton(title: VoipTexts.conference_waiting_room_start_call.uppercased(), backgroundStateColors: VoipTheme.primary_colors_background)
	let conferenceJoinSpinner = RotatingSpinner()

	var conferenceUrl : String? = nil
	let conferenceSubject = MutableLiveData<String>()

	static let compositeDescription = UICompositeViewDescription(ConferenceWaitingRoomFragment.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: nil, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		view.backgroundColor = VoipTheme.voipBackgroundColor.get()
		
		view.addSubview(subject)
		subject.centerX().alignParentTop(withMargin: common_margin).done()
		conferenceSubject.observe { subject in
			self.subject.text = subject
		}
				
		// Controls
		let controlsView = ControlsView(showVideo: true, controlsViewModel: ConferenceWaitingRoomViewModel.sharedModel)
		view.addSubview(controlsView)
		controlsView.alignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).centerX().done()
		
		// Layoout picker
		let layoutPicker = CallControlButton(imageInset : UIEdgeInsets(top: 8, left: 8, bottom: 8, right: 8),buttonTheme: VoipTheme.conf_waiting_room_layout_picker, onClickAction: {
			ConferenceWaitingRoomViewModel.sharedModel.showLayoutPicker.value = ConferenceWaitingRoomViewModel.sharedModel.showLayoutPicker.value != true
		})
		view.addSubview(layoutPicker)
		layoutPicker.alignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).alignParentRight(withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		
		ConferenceWaitingRoomViewModel.sharedModel.joinLayout.readCurrentAndObserve { layout in
			var icon = ""
			switch (layout!) {
			case .Grid: icon = "voip_conference_mosaic"; break
			case .ActiveSpeaker: icon = "voip_conference_active_speaker"; break
			case .Legacy: icon = "voip_conference_audio_only"; break
			}
			layoutPicker.applyTintedIcons(tintedIcons: [UIButton.State.normal.rawValue : TintableIcon(name: icon ,tintColor: LightDarkColor(.white,.white))])
		}
		
		let layoutPickerView = ConferenceLayoutPickerView()
		view.addSubview(layoutPickerView)
		layoutPickerView.alignAbove(view:layoutPicker,withMargin:button_spacing).alignVerticalCenterWith(layoutPicker).done()
		
		ConferenceWaitingRoomViewModel.sharedModel.showLayoutPicker.readCurrentAndObserve { show in
			layoutPicker.isSelected = show == true
			layoutPickerView.isHidden = show != true
			if (show == true) {
				self.view.bringSubviewToFront(layoutPickerView)
			}
		}
		
		// Form buttons
		buttonsView.axis = .horizontal
		buttonsView.spacing = button_spacing
		view.addSubview(buttonsView)
		buttonsView.alignAbove(view: controlsView,withMargin: SharedLayoutConstants.buttons_bottom_margin).centerX().done()
		
		start.width(button_width).done()
		cancel.width(button_width).done()

		buttonsView.addArrangedSubview(cancel)
		buttonsView.addArrangedSubview(start)

		cancel.onClick {
			Core.get().calls.forEach { call in
				if ([Call.State.OutgoingInit, Call.State.OutgoingRinging, Call.State.OutgoingProgress].contains(call.state)) {
					CallManager.instance().terminateCall(call: call.getCobject)
				}
			}
			ConferenceWaitingRoomViewModel.sharedModel.joinInProgress.value = false
			PhoneMainView.instance().popView(self.compositeViewDescription())
		}
	
		start.onClick {
			ConferenceWaitingRoomViewModel.sharedModel.joinInProgress.value = true
			self.conferenceUrl.map{ CallManager.instance().startCall(addr: $0, isSas: false, isVideo: ConferenceWaitingRoomViewModel.sharedModel.isVideoEnabled.value!, isConference: true) }
		}
		
		ConferenceWaitingRoomViewModel.sharedModel.joinInProgress.readCurrentAndObserve { joining in
			self.start.isEnabled = joining != true
			//self.localVideo.isHidden = joining == true (UX question as video window goes black by the core, better black or hidden ?)
			self.noVideoLabel.isHidden = joining == true
			layoutPicker.isHidden = joining == true
			if (joining == true) {
				self.view.addSubview(self.conferenceJoinSpinner)
				self.conferenceJoinSpinner.square(IncomingOutgoingCommonView.spinner_size).center().done()
				self.conferenceJoinSpinner.startRotation()
				controlsView.isHidden = true
			} else {
				self.conferenceJoinSpinner.stopRotation()
				self.conferenceJoinSpinner.removeFromSuperview()
				controlsView.isHidden = false
			}
		}
		
		
		// localVideo view
		localVideo.layer.cornerRadius = center_view_corner_radius
		localVideo.clipsToBounds = true
		localVideo.contentMode = .scaleAspectFit
		localVideo.backgroundColor = .black
		self.view.addSubview(localVideo)
		localVideo.matchParentSideBorders(insetedByDx: content_inset).alignAbove(view:buttonsView,withMargin:SharedLayoutConstants.buttons_bottom_margin).alignUnder(view: subject,withMargin: common_margin).done()
		localVideo.addSubview(switchCamera)
		switchCamera.alignParentTop(withMargin: switch_camera_button_margins).alignParentRight(withMargin: switch_camera_button_margins).square(switch_camera_button_size).done()
		switchCamera.contentMode = .scaleAspectFit
		switchCamera.onClick {
			Core.get().videoPreviewEnabled = false
			Core.get().toggleCamera()
			Core.get().nativePreviewWindow = self.localVideo
			Core.get().videoPreviewEnabled = true
		}
		
		self.view.addSubview(noVideoLabel)
		noVideoLabel.center().done()
		
		ConferenceWaitingRoomViewModel.sharedModel.isVideoEnabled.readCurrentAndObserve { videoEnabled in
			Core.get().videoPreviewEnabled = videoEnabled == true
			self.localVideo.isHidden = videoEnabled != true
			self.switchCamera.isHidden = videoEnabled != true
			self.noVideoLabel.isHidden = videoEnabled == true
		}
				
		
		// Audio Routes
		audioRoutesView = AudioRoutesView()
		view.addSubview(audioRoutesView!)
		audioRoutesView!.alignBottomWith(otherView: controlsView).done()
		ConferenceWaitingRoomViewModel.sharedModel.audioRoutesSelected.readCurrentAndObserve { (audioRoutesSelected) in
			self.audioRoutesView!.isHidden = audioRoutesSelected != true
		}
		audioRoutesView!.alignAbove(view:controlsView,withMargin:SharedLayoutConstants.buttons_bottom_margin).centerX().done()
			
						
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(true)
		ConferenceWaitingRoomViewModel.sharedModel.audioRoutesSelected.value = false
		ConferenceWaitingRoomViewModel.sharedModel.reset()
		Core.get().nativePreviewWindow = localVideo
		Core.get().videoPreviewEnabled = ConferenceWaitingRoomViewModel.sharedModel.isVideoEnabled.value == true
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		ControlsViewModel.shared.fullScreenMode.value = false
		Core.get().nativePreviewWindow = nil
		Core.get().videoPreviewEnabled = false
		ConferenceWaitingRoomViewModel.sharedModel.joinInProgress.value = false
		super.viewWillDisappear(animated)
	}
	
	@objc func setDetails(subject:String, url:String) {
		self.conferenceSubject.value = subject
		self.conferenceUrl = url
	}
	
}
