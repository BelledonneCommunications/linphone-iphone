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


@objc class AbstractCallView: UIViewController {
		
	let extraButtonsView = VoipExtraButtonsView()
	var numpadView : NumpadView? = nil
	var currentCallStatsVew : CallStatsView? = nil
	var shadingMask = UIView()
	var videoAcceptDialog : VoipDialog? = nil
	var dismissableView :  DismissableView? = nil
	
	var audioRoutesView : AudioRoutesView? = nil
	let fullScreenMutableContainerView = UIView()
	let controlsView = ControlsView(showVideo: true, controlsViewModel: ControlsViewModel.shared)
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.view.backgroundColor = VoipTheme.voipBackgroundColor.get()
		}
		
		// Hangup
		let hangup = CallControlButton(width: 65, imageInset:AbstractIncomingOutgoingCallView.answer_decline_inset, buttonTheme: VoipTheme.call_terminate, onClickAction: {
			ControlsViewModel.shared.hangUp()
		})
		view.addSubview(hangup)
		hangup.alignParentLeft(withMargin:SharedLayoutConstants.margin_call_view_side_controls_buttons).alignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		
		
		// Controls
		view.addSubview(controlsView)
		controlsView.alignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).centerX().done()
		
		// Container view
		fullScreenMutableContainerView.backgroundColor = .clear
		self.view.addSubview(fullScreenMutableContainerView)
		fullScreenMutableContainerView.alignParentLeft(withMargin: SharedLayoutConstants.content_inset).alignParentRight(withMargin: SharedLayoutConstants.content_inset).matchParentHeight().alignAbove(view:controlsView,withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		
		
		// Calls List
		ControlsViewModel.shared.goToCallsListEvent.observe { _ in
			if (self.view.superview != nil) {
				self.dismissableView = CallsListView()
				self.view.addSubview(self.dismissableView!)
				self.dismissableView?.matchParentDimmensions().done()
			}
		}
		
		// Goto chat
		ControlsViewModel.shared.goToChatEvent.observe { (_) in
			self.goToChat()
		}
		
		
		// Shading mask, everything before will be shaded upon displaying of the mask
		shadingMask.backgroundColor = VoipTheme.voip_translucent_popup_background
		shadingMask.isHidden = true
		self.view.addSubview(shadingMask)
		shadingMask.matchParentDimmensions().done()
		
		// Extra Buttons
		let showextraButtons = CallControlButton(imageInset:AbstractIncomingOutgoingCallView.answer_decline_inset, buttonTheme: VoipTheme.call_more, onClickAction: {
			self.showModalSubview(view: self.extraButtonsView)
			ControlsViewModel.shared.audioRoutesSelected.value = false
		})
		view.addSubview(showextraButtons)
		showextraButtons.alignParentRight(withMargin:SharedLayoutConstants.margin_call_view_side_controls_buttons).alignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).done()
		
		let boucingCounter = BouncingCounter(inButton:showextraButtons)
		view.addSubview(boucingCounter)
		boucingCounter.dataSource = CallsViewModel.shared.chatAndCallsCount
		
		view.addSubview(extraButtonsView)
		extraButtonsView.matchParentSideBorders(insetedByDx: SharedLayoutConstants.content_inset).alignParentBottom(withMargin:SharedLayoutConstants.bottom_margin_notch_clearance).done()
		ControlsViewModel.shared.hideExtraButtons.readCurrentAndObserve { (_) in
			self.hideModalSubview(view: self.extraButtonsView)
		}
		shadingMask.onClick {
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
				self.numpadView = NumpadView(superView: self.view,callData:  CallsViewModel.shared.currentCallData.value!!,marginTop: 0.0, above:self.controlsView, onDismissAction: {
					ControlsViewModel.shared.numpadVisible.value = false
				})
			} else {
				self.numpadView?.removeFromSuperview()
				self.shadingMask.isHidden = true
			}
			
		}
		
		// Call stats
		ControlsViewModel.shared.callStatsVisible.readCurrentAndObserve { (visible) in
			if (visible == true && CallsViewModel.shared.currentCallData.value != nil ) {
				self.currentCallStatsVew?.removeFromSuperview()
				self.shadingMask.isHidden = false
				self.currentCallStatsVew = CallStatsView(superView: self.view,callData:  CallsViewModel.shared.currentCallData.value!!,marginTop:0.0, above:self.controlsView, onDismissAction: {
					ControlsViewModel.shared.callStatsVisible.value = false
				})
			} else {
				self.currentCallStatsVew?.removeFromSuperview()
				self.shadingMask.isHidden = true
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
		ControlsViewModel.shared.audioRoutesSelected.value = false
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		dismissableView?.removeFromSuperview()
		dismissableView = nil
				
		ControlsViewModel.shared.numpadVisible.value = false
		ControlsViewModel.shared.callStatsVisible.value = false
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
		
	func goToChat() {
		/*guard
		 let chatRoom = CallsViewModel.shared.currentCallData.value??.chatRoom
		 else {
		 Log.w("[Call] Failed to find existing chat room associated to call")
		 return
		 }*/
		PhoneMainView.instance().changeCurrentView(ChatsListView.compositeViewDescription())
		
	}
	
	func layoutRotatableElements() {
		let leftMargin = UIDevice.current.orientation == .landscapeLeft && UIDevice.hasNotch() ? UIApplication.shared.keyWindow!.safeAreaInsets.left : SharedLayoutConstants.content_inset
		let rightMargin = UIDevice.current.orientation == .landscapeRight && UIDevice.hasNotch() ? UIApplication.shared.keyWindow!.safeAreaInsets.right : SharedLayoutConstants.content_inset
		fullScreenMutableContainerView.updateAlignParentLeft(withMargin: leftMargin).updateAlignParentRight(withMargin: rightMargin).done()
		controlsView.updateAlignParentBottom(withMargin:SharedLayoutConstants.buttons_bottom_margin).centerX().done()
	}
	
	override func didRotate(from fromInterfaceOrientation: UIInterfaceOrientation) {
		super.didRotate(from: fromInterfaceOrientation)
		self.layoutRotatableElements()
	}
	
	
}
