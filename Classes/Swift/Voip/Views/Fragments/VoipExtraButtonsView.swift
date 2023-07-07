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

class VoipExtraButtonsView: UIStackView {
	
	//Layout constants
	let height = 200.0
	let corner_radius = 20.0

	required init(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	init () {
		super.init(frame: .zero)
		
		axis = .vertical
		distribution = .fillEqually
		alignment = .center
		layer.cornerRadius = corner_radius
		clipsToBounds = true
        accessibilityIdentifier = "active_call_extra_buttons_view"
        accessibilityViewIsModal = true
				
		let background = UIView()
		background.backgroundColor = VoipTheme.voipExtraButtonsBackgroundColor.get()
		addSubview(background)
		background.layer.cornerRadius = corner_radius
		background.clipsToBounds = true
		background.matchParentDimmensions().done()
		
		height(height).done()
		
		let row1 = UIStackView()
		row1.axis = .horizontal
		row1.distribution = .fillEqually
		row1.alignment = .center

		
		// First row
		let numpad = VoipExtraButton(text: VoipTexts.call_action_numpad, buttonTheme: VoipTheme.call_action("voip_call_numpad"),onClickAction: {
            ControlsViewModel.shared.numpadVisible.value = true
		})
		row1.addArrangedSubview(numpad)
        numpad.accessibilityIdentifier = "active_call_extra_buttons_numpad"
		
		let stats = VoipExtraButton(text: VoipTexts.call_action_statistics, buttonTheme: VoipTheme.call_action("voip_call_stats"),onClickAction: {
			ControlsViewModel.shared.callStatsVisible.value = true
		})
		row1.addArrangedSubview(stats)
        stats.accessibilityIdentifier = "active_call_extra_buttons_stats"
		
		let chats = VoipExtraButton(text: VoipTexts.call_action_chat, buttonTheme: VoipTheme.call_action("voip_call_chat"),withbBoucinCounterDataSource:CallsViewModel.shared.currentCallUnreadChatMessageCount,  onClickAction: {
			ControlsViewModel.shared.goToChatEvent.notifyAllObservers(with: true)
		})
		row1.addArrangedSubview(chats)
        chats.accessibilityIdentifier = "active_call_extra_buttons_chats"
			
		addArrangedSubview(row1)
		row1.matchParentSideBorders().done()

		// Second row

		let row2 = UIStackView()
		row2.axis = .horizontal
		row2.distribution = .fillEqually
		row2.alignment = .center
		
		var transfer = VoipExtraButton(text: CallsViewModel.shared.inactiveCallsCount.value! < 1 ? VoipTexts.call_action_transfer_call : VoipTexts.call_context_action_attended_transfer, buttonTheme: VoipTheme.call_action("voip_call_forward"),onClickAction: {
			if CallsViewModel.shared.inactiveCallsCount.value! < 1 {
				let view: DialerView = self.VIEW(DialerView.compositeViewDescription());
				view.setAddress("")
				CallManager.instance().nextCallIsTransfer = true
				PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
			}else{
				self.attendedTransfer()
			}
		})
		row2.addArrangedSubview(transfer)
        transfer.accessibilityIdentifier = "active_call_extra_buttons_transfer"
		
		let participants = VoipExtraButton(text: VoipTexts.call_action_participants_list, buttonTheme: VoipTheme.call_action("voip_call_participants"),onClickAction: {
			ControlsViewModel.shared.goToConferenceParticipantsListEvent.notifyAllObservers(with: true)
		})
		row2.addArrangedSubview(participants)
        participants.accessibilityIdentifier = "active_call_extra_buttons_participants"
		
		let addcall = VoipExtraButton(text: VoipTexts.call_action_add_call, buttonTheme: VoipTheme.call_action("voip_call_add"),onClickAction: {
			let view: DialerView = self.VIEW(DialerView.compositeViewDescription());
			view.setAddress("")
			CallManager.instance().nextCallIsTransfer = false
			PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
		})
		row2.addArrangedSubview(addcall)
        addcall.accessibilityIdentifier = "active_call_extra_buttons_add_call"
		
		let layoutselect = VoipExtraButton(text: VoipTexts.call_action_change_conf_layout, buttonTheme: VoipTheme.call_action("voip_conference_mosaic"),onClickAction: {
			ControlsViewModel.shared.goToConferenceLayoutSettings.notifyAllObservers(with: true)
		})
		row2.addArrangedSubview(layoutselect)
		
		if (Core.get().config?.getBool(section: "app", key: "disable_video_feature", defaultValue: false) == true) {
			layoutselect.isEnabled = false
			layoutselect.setTitleColor(.gray, for: .disabled)
			if #available(iOS 13.0, *) {
				layoutselect.setImage(UIImage(named: "voip_conference_mosaic")!.withTintColor(.gray), for: .disabled)
			}
		}
		
		let calls = VoipExtraButton(text: VoipTexts.call_action_calls_list, buttonTheme: VoipTheme.call_action("voip_calls_list"), withbBoucinCounterDataSource: CallsViewModel.shared.inactiveCallsCount, onClickAction: {
			ControlsViewModel.shared.goToCallsListEvent.notifyAllObservers(with: true)
		})
		row2.addArrangedSubview(calls)
        calls.accessibilityIdentifier = "active_call_extra_buttons_calls"
		
		addArrangedSubview(row2)
		row2.matchParentSideBorders().done()
		
		ConferenceViewModel.shared.conferenceExists.readCurrentAndObserve { (isIn) in
			participants.isHidden = isIn != true
			layoutselect.isHidden = isIn != true
			transfer.isHidden = isIn == true
			addcall.isHidden = isIn == true
		}
		
		CallsViewModel.shared.inactiveCallsCount.readCurrentAndObserve { title in
			transfer.setTitle(title! < 1 ? VoipTexts.call_action_transfer_call : VoipTexts.call_context_action_attended_transfer, for: .normal)
		}
	}
	
	func refresh() {
		CallsViewModel.shared.currentCallUnreadChatMessageCount.notifyValue()
		CallsViewModel.shared.inactiveCallsCount.notifyValue()
	}
	
	func attendedTransfer() {
		var core = CallManager.instance().lc
		var currentCall = core?.currentCall

		if (currentCall == nil) {
			Log.e("[Call Controls] Can't do an attended transfer without a current call")
			return
		}
		
		if let callsNb = core?.callsNb, callsNb <= 1 {
			Log.e("[Call Controls] Need at least two calls to do an attended transfer")
			return
		}
		
		var callToTransferTo = core!.calls.last { call in
			call.state == Call.State.Paused
		}
		
		if (callToTransferTo == nil) {
			Log.e("[Call Controls] Couldn't find a call in Paused state to transfer current call to")
			return
		}

		Log.i(
			"[Call Controls] Doing an attended transfer between active call [${currentCall.remoteAddress.asStringUriOnly()}] and paused call [${callToTransferTo.remoteAddress.asStringUriOnly()}]"
		)
		
		do{
			try callToTransferTo?.transferToAnother(dest: currentCall!)
		}catch{
			Log.e("[Call Controls] Attended transfer failed!")
		}
	}
}
