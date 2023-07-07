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


@objc class ConferenceCallView: AbstractCallView, UICompositeViewDelegate {
	
	var conferencePausedView : PausedCallOrConferenceView? = nil
	var conferenceGridView: VoipConferenceGridView? = nil
	var conferenceActiveSpeakerView: VoipConferenceActiveSpeakerView? = nil
	var conferenceAudioOnlyView: VoipConferenceAudioOnlyView? = nil
	let conferenceJoinSpinner = RotatingSpinner(color:VoipTheme.dark_grey_color)
	@objc var participantsListView :  ParticipantsListView? = nil
	
	static let compositeDescription = UICompositeViewDescription(ConferenceCallView.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: nil, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	override func viewDidLoad() {
		super.viewDidLoad()
	
		// Conference paused
		conferencePausedView = PausedCallOrConferenceView(iconName: "voip_conference_play_big",titleText: VoipTexts.conference_paused_title,subTitleText: VoipTexts.conference_paused_subtitle, onClickAction: {
			ConferenceViewModel.shared.togglePlayPause()
		})
		view.addSubview(conferencePausedView!)
		conferencePausedView?.matchParentSideBorders().matchParentHeight().alignAbove(view:controlsView,withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		conferencePausedView?.isHidden = true
		
		// Conference grid
		conferenceGridView = VoipConferenceGridView()
		fullScreenMutableContainerView.addSubview(conferenceGridView!)
		conferenceGridView?.matchParentDimmensions().done()
		conferenceGridView?.isHidden = true
		ConferenceViewModel.shared.conferenceExists.readCurrentAndObserve { (exists) in
			if (exists == true) {
				self.extraButtonsView.isHidden = true
				self.conferencePausedView?.isHidden = ConferenceViewModel.shared.isConferenceLocallyPaused.value != true
				self.displaySelectedConferenceLayout()
				UIApplication.shared.isIdleTimerDisabled = true
			} else {
				self.conferenceGridView?.isHidden = true
				self.conferenceActiveSpeakerView?.isHidden = true
				self.conferenceActiveSpeakerView?.isHidden = true
				self.conferencePausedView?.isHidden = true
				UIApplication.shared.isIdleTimerDisabled = false
			}
		}
		
		ConferenceViewModel.shared.conferenceCreationPending.readCurrentAndObserve { isCreationPending in
			if (isCreationPending == true) {
				self.fullScreenMutableContainerView.addSubview(self.conferenceJoinSpinner)
				self.conferenceJoinSpinner.square(AbstractIncomingOutgoingCallView.spinner_size).center().done()
				self.conferenceJoinSpinner.startRotation()
			} else {
				self.conferenceJoinSpinner.removeFromSuperview()
				self.conferenceJoinSpinner.stopRotation()
				self.switchToFullScreenIfPossible(conference: ConferenceViewModel.shared.conference.value)
			}
		}
		
		// Conference active speaker
		conferenceActiveSpeakerView = VoipConferenceActiveSpeakerView()
		fullScreenMutableContainerView.addSubview(conferenceActiveSpeakerView!)
		conferenceActiveSpeakerView?.matchParentDimmensions().done()
		conferenceActiveSpeakerView?.isHidden = true
		
		// Conference audio only
		conferenceAudioOnlyView = VoipConferenceAudioOnlyView()
		fullScreenMutableContainerView.addSubview(conferenceAudioOnlyView!)
		conferenceAudioOnlyView?.matchParentDimmensions().done()
		conferenceAudioOnlyView?.isHidden = true
		
		ConferenceViewModel.shared.conferenceDisplayMode.readCurrentAndObserve { (conferenceMode) in
			if (ConferenceViewModel.shared.conferenceExists.value == true) {
				self.displaySelectedConferenceLayout()
				if (conferenceMode != .AudioOnly) {
					self.switchToFullScreenIfPossible(conference: ConferenceViewModel.shared.conference.value)
				}
			}
		}
		ConferenceViewModel.shared.isConferenceLocallyPaused.readCurrentAndObserve { (paused) in
			self.conferencePausedView?.isHidden = paused != true
		}

		// Conference Participants List
		ControlsViewModel.shared.goToConferenceParticipantsListEvent.observe { (_) in
			self.participantsListView = ParticipantsListView()
			self.view.addSubview(self.participantsListView!)
			self.participantsListView?.matchParentDimmensions().done()
		}
		
		// Conference mode selection
		ControlsViewModel.shared.goToConferenceLayoutSettings.observe { (_) in
			self.dismissableView = VoipConferenceDisplayModeSelectionView()
			self.view.addSubview(self.dismissableView!)
			self.dismissableView?.matchParentDimmensions().done()
		}
		
		// First/Last to join conference :
		ConferenceViewModel.shared.allParticipantsLeftEvent.observe { (allLeft) in
			if (allLeft == true) {
				VoipDialog.toast(message: VoipTexts.conference_last_user)
			}
		}
		ConferenceViewModel.shared.firstToJoinEvent.observe { (first) in
			if (first == true) {
				VoipDialog.toast(message: VoipTexts.conference_first_to_join)
			}
		}
		
		view.onClick {
			ControlsViewModel.shared.audioRoutesSelected.value = false
		}
		
	}
	
	func displaySelectedConferenceLayout() {
		let conferenceMode = ConferenceViewModel.shared.conferenceDisplayMode.value
		self.conferenceGridView!.isHidden = conferenceMode != .Grid
		self.conferenceActiveSpeakerView!.isHidden = conferenceMode != .ActiveSpeaker
		self.conferenceAudioOnlyView!.isHidden = conferenceMode != .AudioOnly
		if (conferenceMode == .Grid) {
			self.conferenceGridView?.conferenceViewModel = ConferenceViewModel.shared
		}
		if (conferenceMode == .AudioOnly) {
			self.conferenceAudioOnlyView?.conferenceViewModel = ConferenceViewModel.shared
		}
		if (conferenceMode == .ActiveSpeaker) {
			self.conferenceActiveSpeakerView?.conferenceViewModel = ConferenceViewModel.shared
		}
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(true)
		ConferenceViewModel.shared.conferenceExists.notifyValue()
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		super.viewWillDisappear(animated)
		participantsListView?.removeFromSuperview()
		participantsListView = nil
	}
		
	override func didRotate(from fromInterfaceOrientation: UIInterfaceOrientation) {
		super.didRotate(from: fromInterfaceOrientation)
		self.conferenceActiveSpeakerView?.layoutRotatableElements()
	}
	
	private func switchToFullScreenIfPossible(conference: Conference?) {
		if (ConfigManager.instance().lpConfigBoolForKey(key: "enter_video_conference_enable_full_screen_mode", defaultValue: true)) {
			if (conference?.currentParams?.isVideoEnabled == true) {
				if (conference?.me?.devices.count == 0) {
					Log.i("[Conference Call] Conference has video enabled but our device hasn't joined yet")
				} else if (conference?.me?.devices.filter { $0.isInConference && $0.getStreamAvailability(streamType: StreamType.Video) }.first != nil) {
					Log.i("[Conference Call] Conference has video enabled & our device has video enabled, enabling full screen mode")
					ControlsViewModel.shared.fullScreenMode.value = true
				} else {
					Log.i("[Conference Call] Conference has video enabled but our device video is disabled")
				}
			}
		}
	}
	
	
}
