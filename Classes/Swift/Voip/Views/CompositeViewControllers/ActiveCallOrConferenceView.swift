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


@objc class ActiveCallOrConferenceView: UIViewController, UICompositeViewDelegate { // Replaces CallView
	
	// Layout constants
	let content_inset = 12.0
	
	var callPausedByRemoteView : PausedCallOrConferenceView? = nil
	var conferencePausedView : PausedCallOrConferenceView? = nil

	var currentCallView : ActiveCallView? = nil
	var conferenceGridView: VoipConferenceGridView? = nil
	var conferenceActiveSpeakerView: VoipConferenceActiveSpeakerView? = nil
	let conferenceJoinSpinner = RotatingSpinner()


	let extraButtonsView = VoipExtraButtonsView()
	var numpadView : NumpadView? = nil
	var currentCallStatsVew : CallStatsView? = nil
	var shadingMask = UIView()
	var videoAcceptDialog : VoipDialog? = nil
	var dismissableView :  DismissableView? = nil
	@objc var participantsListView :  ParticipantsListView? = nil
	
	var audioRoutesView : AudioRoutesView? = nil

	
	static let compositeDescription = UICompositeViewDescription(ActiveCallOrConferenceView.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: nil, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		view.backgroundColor = VoipTheme.voipBackgroundColor.get()
		
		// Hangup
		let hangup = CallControlButton(width: 65, imageInset:IncomingOutgoingCommonView.answer_decline_inset, buttonTheme: VoipTheme.call_terminate, onClickAction: {
			ControlsViewModel.shared.hangUp()
		})
		view.addSubview(hangup)
		hangup.alignParentLeft(withMargin:SharedLayoutConstants.margin_call_view_side_controls_buttons).alignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		
		
		// Controls
		let controlsView = ControlsView(showVideo: true, controlsViewModel: ControlsViewModel.shared)
		view.addSubview(controlsView)
		controlsView.alignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).centerX().done()
		
	
		// Container view
		let fullScreenMutableContainerView = UIView()
		fullScreenMutableContainerView.backgroundColor = .clear
		self.view.addSubview(fullScreenMutableContainerView)
		fullScreenMutableContainerView.matchParentSideBorders(insetedByDx: content_inset).matchParentHeight().alignAbove(view:controlsView,withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
	
		// Current (Single) Call (VoipCallView)
		currentCallView = ActiveCallView()
		currentCallView!.isHidden = true
		fullScreenMutableContainerView.addSubview(currentCallView!)
		CallsViewModel.shared.currentCallData.readCurrentAndObserve { (currentCallData) in
			self.updateNavigation()
			self.currentCallView!.isHidden = currentCallData == nil || ConferenceViewModel.shared.conferenceExists.value == true
			self.currentCallView!.callData = currentCallData != nil ? currentCallData! : nil
			currentCallData??.isRemotelyPaused.readCurrentAndObserve { remotelyPaused in
				self.callPausedByRemoteView?.isHidden = remotelyPaused != true
			}
			if (currentCallData == nil) {
				self.callPausedByRemoteView?.isHidden = true
			} else {
				currentCallData??.isIncoming.readCurrentAndObserve { _ in self.updateNavigation() }
				currentCallData??.isOutgoing.readCurrentAndObserve { _ in self.updateNavigation() }
			}
			self.extraButtonsView.isHidden = true
			self.conferencePausedView?.isHidden = true
		}
	
		currentCallView!.matchParentDimmensions().done()
		
		// Paused by remote (Call)
		callPausedByRemoteView = PausedCallOrConferenceView(iconName: "voip_conference_paused_big",titleText: VoipTexts.call_remotely_paused_title,subTitleText: nil)
		view.addSubview(callPausedByRemoteView!)
		callPausedByRemoteView?.matchParentSideBorders().matchParentHeight().alignAbove(view:controlsView,withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		callPausedByRemoteView?.isHidden = true
		
		// Conference paused
		conferencePausedView = PausedCallOrConferenceView(iconName: "voip_conference_paused_big",titleText: VoipTexts.conference_paused_title,subTitleText: VoipTexts.conference_paused_subtitle)
		view.addSubview(conferencePausedView!)
		conferencePausedView?.matchParentSideBorders().matchParentHeight().alignAbove(view:controlsView,withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		conferencePausedView?.isHidden = true
		
		// Conference grid
		conferenceGridView = VoipConferenceGridView()
		fullScreenMutableContainerView.addSubview(conferenceGridView!)
		conferenceGridView?.matchParentDimmensions().done()
		conferenceGridView?.isHidden = true
		ConferenceViewModel.shared.conferenceExists.readCurrentAndObserve { (isInConference) in
			self.updateNavigation()
			if (isInConference == true) { 
				self.currentCallView!.isHidden = true
				self.extraButtonsView.isHidden = true
				self.conferencePausedView?.isHidden = true
				let conferenceMode = ConferenceViewModel.shared.conferenceDisplayMode.value
				self.conferenceGridView!.isHidden = conferenceMode != .Grid
				self.conferenceActiveSpeakerView?.isHidden = conferenceMode != .ActiveSpeaker
				self.conferenceGridView?.conferenceViewModel = ConferenceViewModel.shared
			} else {
				self.conferenceGridView?.isHidden = true
			}
		}
		
		ConferenceViewModel.shared.conferenceCreationPending.readCurrentAndObserve { isCreationPending in
			if (ConferenceViewModel.shared.conferenceExists.value == true && isCreationPending == true) {
				fullScreenMutableContainerView.addSubview(self.conferenceJoinSpinner)
				self.conferenceJoinSpinner.square(IncomingOutgoingCommonView.spinner_size).center().done()
				self.conferenceJoinSpinner.startRotation()
			} else {
				self.conferenceJoinSpinner.removeFromSuperview()
				self.conferenceJoinSpinner.stopRotation()
			}
		}
		
		// Conference active speaker
		conferenceActiveSpeakerView = VoipConferenceActiveSpeakerView()
		fullScreenMutableContainerView.addSubview(conferenceActiveSpeakerView!)
		conferenceActiveSpeakerView?.matchParentDimmensions().done()
		conferenceActiveSpeakerView?.isHidden = true
		
		// Conference mode switching

		ConferenceViewModel.shared.conferenceDisplayMode.readCurrentAndObserve { (conferenceMode) in
			if (ConferenceViewModel.shared.conferenceExists.value == true) {
				self.conferenceGridView!.isHidden = conferenceMode != .Grid
				self.conferenceActiveSpeakerView!.isHidden = conferenceMode != .ActiveSpeaker
				self.conferenceActiveSpeakerView?.conferenceViewModel = ConferenceViewModel.shared
			} else {
				self.conferenceActiveSpeakerView?.isHidden = true
			}
		}
		
		ConferenceViewModel.shared.conferenceExists.readCurrentAndObserve { (isInConference) in
			self.updateNavigation()
		}
		
		// Calls List
		ControlsViewModel.shared.goToCallsListEvent.observe { (_) in
			self.dismissableView = CallsListView()
			self.view.addSubview(self.dismissableView!)
			self.dismissableView?.matchParentDimmensions().done()
		}
		
		// Conference Participants List
		ControlsViewModel.shared.goToConferenceParticipantsListEvent.observe { (_) in
			self.participantsListView = ParticipantsListView()
			self.view.addSubview(self.participantsListView!)
			self.participantsListView?.matchParentDimmensions().done()
		}
		
		// Goto chat
		ControlsViewModel.shared.goToChatEvent.observe { (_) in
			self.goToChat()
		}
		
		// Conference mode selection
		ControlsViewModel.shared.goToConferenceLayoutSettings.observe { (_) in
			self.dismissableView = VoipConferenceDisplayModeSelectionView()
			self.view.addSubview(self.dismissableView!)
			self.dismissableView?.matchParentDimmensions().done()
			let activeDisplayMode =  ConferenceViewModel.shared.conferenceDisplayMode.value!
			let indexPath = IndexPath(row: activeDisplayMode == .Grid ? 0 : 1, section: 0)
			(self.dismissableView as! VoipConferenceDisplayModeSelectionView).optionsListView.selectRow(at:indexPath, animated: true, scrollPosition: .bottom)

		}
		
		// Shading mask, everything before will be shaded upon displaying of the mask
		shadingMask.backgroundColor = VoipTheme.voip_translucent_popup_background
		shadingMask.isHidden = true
		self.view.addSubview(shadingMask)
		shadingMask.matchParentDimmensions().done()
		
		// Extra Buttons
		let showextraButtons = CallControlButton(imageInset:IncomingOutgoingCommonView.answer_decline_inset, buttonTheme: VoipTheme.call_more, onClickAction: {
			self.showModalSubview(view: self.extraButtonsView)
			ControlsViewModel.shared.audioRoutesSelected.value = false
		})
		view.addSubview(showextraButtons)
		showextraButtons.alignParentRight(withMargin:SharedLayoutConstants.margin_call_view_side_controls_buttons).alignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).done()

		let boucingCounter = BouncingCounter(inButton:showextraButtons)
		view.addSubview(boucingCounter)
		boucingCounter.dataSource = CallsViewModel.shared.chatAndCallsCount
		
		view.addSubview(extraButtonsView)
		extraButtonsView.matchParentSideBorders(insetedByDx: content_inset).alignParentBottom(withMargin:SharedLayoutConstants.bottom_margin_notch_clearance).done()
		ControlsViewModel.shared.hideExtraButtons.readCurrentAndObserve { (_) in
			self.hideModalSubview(view: self.extraButtonsView)
		}
		self.view.onClick {
			if (!self.extraButtonsView.isHidden) {
				self.hideModalSubview(view: self.extraButtonsView)
			}
			ControlsViewModel.shared.audioRoutesSelected.value = false
		}
		
		// Numpad
		ControlsViewModel.shared.numpadVisible.readCurrentAndObserve { (visible) in
			if (visible == true && CallsViewModel.shared.currentCallData.value != nil ) {
				self.numpadView?.removeFromSuperview()
				self.shadingMask.isHidden = false
				self.numpadView = NumpadView(superView: self.view,callData:  CallsViewModel.shared.currentCallData.value!!,marginTop:self.currentCallView?.centerSection.frame.origin.y ?? 0.0, onDismissAction: {
					self.numpadView?.removeFromSuperview()
					self.shadingMask.isHidden = true
				})
			}
		}
		
		// Call stats
		ControlsViewModel.shared.callStatsVisible.readCurrentAndObserve { (visible) in
			if (visible == true && CallsViewModel.shared.currentCallData.value != nil ) {
				self.currentCallStatsVew?.removeFromSuperview()
				self.shadingMask.isHidden = false
				self.currentCallStatsVew = CallStatsView(superView: self.view,callData:  CallsViewModel.shared.currentCallData.value!!,marginTop:self.currentCallView?.centerSection.frame.origin.y ?? 0.0,  onDismissAction: {
					self.currentCallStatsVew?.removeFromSuperview()
					self.shadingMask.isHidden = true
				})
			}
		}
		
		// Video activation dialog request
		CallsViewModel.shared.callUpdateEvent.observe { (call) in
			let core = Core.get()
			if (call?.state == .StreamsRunning) {
				self.videoAcceptDialog?.removeFromSuperview()
				self.videoAcceptDialog = nil
			} else if (call?.state == .UpdatedByRemote) {
				if (core.videoCaptureEnabled || core.videoDisplayEnabled) {
					if (call?.currentParams?.videoEnabled != call?.remoteParams?.videoEnabled) {
						let accept = ButtonAttributes(text:VoipTexts.dialog_accept, action: {call?.answerVideoUpdateRequest(accept: true)}, isDestructive:false)
						let cancel = ButtonAttributes(text:VoipTexts.dialog_decline, action: {call?.answerVideoUpdateRequest(accept: false)}, isDestructive:true)
						self.videoAcceptDialog = VoipDialog(message:VoipTexts.call_video_update_requested_dialog, givenButtons:  [cancel,accept])
						self.videoAcceptDialog?.show()
					}
				} else {
					Log.w("[Call] Video display & capture are disabled, don't show video dialog")
				}
			}
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
		extraButtonsView.refresh()
		ControlsViewModel.shared.callStatsVisible.notifyValue()
		CallsViewModel.shared.currentCallData.notifyValue()
		ControlsViewModel.shared.audioRoutesSelected.value = false
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		dismissableView?.removeFromSuperview()
		dismissableView = nil
		
		participantsListView?.removeFromSuperview()
		participantsListView = nil
		
		ControlsViewModel.shared.fullScreenMode.value = false
		super.viewWillDisappear(animated)
	}
	
	func showModalSubview(view:UIView) {
		view.isHidden = false
		shadingMask.isHidden = false
	}
	func hideModalSubview(view:UIView) {
		view.isHidden = true
		shadingMask.isHidden = true
	}
	
	func updateNavigation() {
		if (Core.get().callsNb == 0) {
			PhoneMainView.instance().popView(self.compositeViewDescription())
			PhoneMainView.instance().mainViewController.removeCallFromCache()
		} else {
			if let data = CallsViewModel.shared.currentCallData.value {
				if (data?.isOutgoing.value == true || data?.isIncoming.value == true) {
					PhoneMainView.instance().popView(self.compositeViewDescription())
				} else {
					if (data!.isCallingAConference()) {
						PhoneMainView.instance().pop(toView: self.compositeViewDescription())
					} else {
						PhoneMainView.instance().changeCurrentView(self.compositeViewDescription())
					}
				}
			} else {
					PhoneMainView.instance().changeCurrentView(self.compositeViewDescription())
			}
		}
	}
	
	func goToChat() {
		guard
			let chatRoom = CallsViewModel.shared.currentCallData.value??.chatRoom
		else {
			Log.w("[Call] Failed to find existing chat room associated to call")
				return
		}
		PhoneMainView.instance().go(to: chatRoom.getCobject)
		
	}
	
	
}
