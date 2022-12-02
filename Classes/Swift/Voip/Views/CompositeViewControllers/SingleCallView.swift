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


@objc class SingleCallView: AbstractCallView, UICompositeViewDelegate {
	
	var callPausedByRemoteView : PausedCallOrConferenceView? = nil
	var callPausedByLocalView : PausedCallOrConferenceView? = nil
	var currentCallView : ActiveCallView? = nil
	
	static let compositeDescription = UICompositeViewDescription(SingleCallView.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: nil, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	override func viewDidLoad() {
		super.viewDidLoad()
	
		// Current (Single) Call (VoipCallView)
		currentCallView = ActiveCallView()
		fullScreenMutableContainerView.addSubview(currentCallView!)
		CallsViewModel.shared.currentCallData.readCurrentAndObserve { (currentCallData) in
			guard currentCallData??.isOngoingSingleCall() == true else {
				return
			}
			self.currentCallView!.callData = currentCallData != nil ? currentCallData! : nil
			currentCallData??.isRemotelyPaused.readCurrentAndObserve { remotelyPaused in
				self.callPausedByRemoteView?.isHidden = remotelyPaused != true
			}
			currentCallData??.isPaused.readCurrentAndObserve { locallyPaused in
				self.callPausedByLocalView?.isHidden = locallyPaused != true
			}
			if (currentCallData == nil) {
				self.callPausedByRemoteView?.isHidden = true
				self.callPausedByLocalView?.isHidden = true
				
			}
			self.extraButtonsView.isHidden = true
		}
		
		currentCallView!.matchParentDimmensions().done()
		
		// Paused by remote (Call)
		callPausedByRemoteView = PausedCallOrConferenceView(iconName: "voip_conference_paused_big",titleText: VoipTexts.call_remotely_paused_title,subTitleText: nil)
		view.addSubview(callPausedByRemoteView!)
		callPausedByRemoteView?.matchParentSideBorders().matchParentHeight().alignAbove(view:controlsView,withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		callPausedByRemoteView?.isHidden = true
		
		// Paused by local (Call)
		callPausedByLocalView = PausedCallOrConferenceView(iconName: "voip_conference_play_big",titleText: VoipTexts.call_locally_paused_title,subTitleText: VoipTexts.call_locally_paused_subtitle, onClickAction: {
			CallsViewModel.shared.currentCallData.value??.togglePause()
		})
		view.addSubview(callPausedByLocalView!)
		callPausedByLocalView?.matchParentSideBorders().matchParentHeight().alignAbove(view:controlsView,withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		callPausedByLocalView?.isHidden = true
				
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
		view.onClick {
			ControlsViewModel.shared.audioRoutesSelected.value = false
		}
	}
	
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(true)
		CallsViewModel.shared.currentCallData.notifyValue()
	}
	
	override func didRotate(from fromInterfaceOrientation: UIInterfaceOrientation) {
		super.didRotate(from: fromInterfaceOrientation)
		self.currentCallView?.layoutRotatableElements()
	}
	
}
