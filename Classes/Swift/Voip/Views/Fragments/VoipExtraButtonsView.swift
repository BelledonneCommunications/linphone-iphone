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
		
		let stats = VoipExtraButton(text: VoipTexts.call_action_statistics, buttonTheme: VoipTheme.call_action("voip_call_stats"),onClickAction: {
			ControlsViewModel.shared.callStatsVisible.value = true
		})
		row1.addArrangedSubview(stats)
		
		let chats = VoipExtraButton(text: VoipTexts.call_action_chat, buttonTheme: VoipTheme.call_action("voip_call_chat"),withbBoucinCounterDataSource:CallsViewModel.shared.currentCallUnreadChatMessageCount,  onClickAction: {
			ControlsViewModel.shared.goToChatEvent.notifyAllObservers(with: true)
		})
		row1.addArrangedSubview(chats)
			
		addArrangedSubview(row1)
		row1.matchParentSideBorders().done()

		// Second row

		let row2 = UIStackView()
		row2.axis = .horizontal
		row2.distribution = .fillEqually
		row2.alignment = .center
		
		let transfer = VoipExtraButton(text: VoipTexts.call_action_transfer_call, buttonTheme: VoipTheme.call_action("voip_call_forward"),onClickAction: {
			let view: DialerView = self.VIEW(DialerView.compositeViewDescription());
			view.setAddress("")
			CallManager.instance().nextCallIsTransfer = true
			PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
		})
		row2.addArrangedSubview(transfer)
		
		let participants = VoipExtraButton(text: VoipTexts.call_action_participants_list, buttonTheme: VoipTheme.call_action("voip_call_participants"),onClickAction: {
			ControlsViewModel.shared.goToConferenceParticipantsListEvent.notifyAllObservers(with: true)
		})
		row2.addArrangedSubview(participants)
		
		
		let addcall = VoipExtraButton(text: VoipTexts.call_action_add_call, buttonTheme: VoipTheme.call_action("voip_call_add"),onClickAction: {
			let view: DialerView = self.VIEW(DialerView.compositeViewDescription());
			view.setAddress("")
			CallManager.instance().nextCallIsTransfer = false
			PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
		})
		row2.addArrangedSubview(addcall)
		
		
		let layoutselect = VoipExtraButton(text: VoipTexts.call_action_change_conf_layout, buttonTheme: VoipTheme.call_action("voip_conference_mosaic"),onClickAction: {
			ControlsViewModel.shared.goToConferenceLayoutSettings.notifyAllObservers(with: true)
		})
		row2.addArrangedSubview(layoutselect)
			
		let calls = VoipExtraButton(text: VoipTexts.call_action_calls_list, buttonTheme: VoipTheme.call_action("voip_calls_list"), withbBoucinCounterDataSource: CallsViewModel.shared.inactiveCallsCount, onClickAction: {
			ControlsViewModel.shared.goToCallsListEvent.notifyAllObservers(with: true)
		})
		row2.addArrangedSubview(calls)
		
		addArrangedSubview(row2)
		row2.matchParentSideBorders().done()
		
		ConferenceViewModel.shared.conferenceExists.readCurrentAndObserve { (isIn) in
			participants.isHidden = isIn != true
			layoutselect.isHidden = isIn != true
			transfer.isHidden = isIn == true
			addcall.isHidden = isIn == true
		}
		
	}
	
	func refresh() {
		CallsViewModel.shared.currentCallUnreadChatMessageCount.notifyValue()
		CallsViewModel.shared.inactiveCallsCount.notifyValue()
	}
	
	

}
