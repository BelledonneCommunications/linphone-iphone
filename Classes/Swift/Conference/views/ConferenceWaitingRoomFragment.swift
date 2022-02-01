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
	let buttonsView = UIStackView()
	let cancel = FormButton(title: VoipTexts.cancel.uppercased(), backgroundStateColors: VoipTheme.primary_colors_background_gray, bold:false)
	let start = FormButton(title: VoipTexts.conference_waiting_room_start_call.uppercased(), backgroundStateColors: VoipTheme.primary_colors_background)
	
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
		let controlsView = ControlsView(showVideo: true)
		view.addSubview(controlsView)
		controlsView.alignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).centerX().done()
		
		
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
			PhoneMainView.instance().popView(self.compositeViewDescription())
		}
		
		start.onClick {
			self.conferenceUrl.map{ CallManager.instance().startCall(addr: $0, isSas: false) }
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
		
		// Audio Routes
		audioRoutesView = AudioRoutesView()
		view.addSubview(audioRoutesView!)
		audioRoutesView!.alignBottomWith(otherView: controlsView).done()
		ControlsViewModel.shared.audioRoutesSelected.readCurrentAndObserve { (audioRoutesSelected) in
			self.audioRoutesView!.isHidden = audioRoutesSelected != true
		}
		audioRoutesView!.alignAbove(view:controlsView,withMargin:SharedLayoutConstants.buttons_bottom_margin).centerX().done()
			
						
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(true)
		ControlsViewModel.shared.audioRoutesSelected.value = false
		Core.get().nativePreviewWindow = localVideo
		Core.get().videoPreviewEnabled = true
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		ControlsViewModel.shared.fullScreenMode.value = false
		Core.get().nativePreviewWindow = nil
		Core.get().videoPreviewEnabled = false
		super.viewWillDisappear(animated)
	}
	
	@objc func setDetails(subject:String, url:String) {
		self.conferenceSubject.value = subject
		self.conferenceUrl = url
	}
	
}
