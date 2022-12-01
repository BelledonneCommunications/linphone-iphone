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
import linphonesw

@objc class ParticipantsListView: DismissableView, UITableViewDataSource {
	
	// Layout constants
	let side_margin = 10.0
	
	let participantsListTableView =  UITableView()
	let noParticipantsLabel = StyledLabel(VoipTheme.empty_list_font,VoipTexts.conference_empty)
	
	
	var callsDataObserver : MutableLiveDataOnChangeClosure<[CallData]>? = nil
	
	init() {
		super.init(title: VoipTexts.call_action_participants_list)
		
		
		let edit = CallControlButton(buttonTheme: VoipTheme.voip_edit, onClickAction: {
			self.removeFromSuperview()
			self.gotoParticipantsListSelection()
		})
		super.headerView.addSubview(edit)
		edit.centerY().done()
		super.dismiss?.toRightOf(edit,withLeftMargin: dismiss_right_margin).centerY().done()
		
		
		// ParticipantsList
		super.contentView.addSubview(participantsListTableView)
		participantsListTableView.matchParentDimmensions().done()
		participantsListTableView.dataSource = self
		participantsListTableView.register(VoipParticipantCell.self, forCellReuseIdentifier: "VoipParticipantCell")
		participantsListTableView.allowsSelection = false
		if #available(iOS 15.0, *) {
			participantsListTableView.allowsFocus = false
		}
		participantsListTableView.separatorStyle = .singleLine
		participantsListTableView.separatorColor = .white
	
	
		ConferenceViewModel.shared.conferenceParticipants.readCurrentAndObserve{ _ in
			self.participantsListTableView.reloadData()
			self.noParticipantsLabel.isHidden =  ConferenceViewModel.shared.conferenceParticipants.value?.count ?? 0 > 0
		}
		
		ConferenceViewModel.shared.isMeAdmin.readCurrentAndObserve { (meAdmin) in
			edit.isHidden = meAdmin != true
		}
		
		super.contentView.addSubview(noParticipantsLabel)
		noParticipantsLabel.center().matchParentSideBorders(insetedByDx: side_margin).done()
		noParticipantsLabel.isHidden =  ConferenceViewModel.shared.conferenceParticipants.value?.count ?? 0 > 0
		noParticipantsLabel.numberOfLines = 2
		self.participantsListTableView.backgroundColor = VoipTheme.voipBackgroundBWColor.get()

		UIDeviceBridge.displayModeSwitched.observe { _ in
			self.participantsListTableView.backgroundColor = VoipTheme.voipBackgroundBWColor.get()
			self.participantsListTableView.reloadData()
		}
	}

	
	// TableView datasource delegate
	
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		guard let participants = ConferenceViewModel.shared.conferenceParticipants.value else {
			return 0
		}
		return participants.count
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell:VoipParticipantCell = tableView.dequeueReusableCell(withIdentifier: "VoipParticipantCell") as! VoipParticipantCell
		guard let participantData = ConferenceViewModel.shared.conferenceParticipants.value?[indexPath.row] else {
			return cell
		}
		cell.selectionStyle = .none
		cell.participantData = participantData
		cell.owningParticpantsListView = self
		return cell
	}
	
	// View controller
	
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	func gotoParticipantsListSelection() {
		let view: ChatConversationCreateView = self.VIEW(ChatConversationCreateView.compositeViewDescription())
		view.unfragmentCompositeDescription()
		let addresses =  ConferenceViewModel.shared.conferenceParticipants.value!.map { (data) in String(data.participant.address!.asStringUriOnly()) }
		view.tableController.contactsGroup = (addresses as NSArray).mutableCopy() as? NSMutableArray
		view.isForEditing = false
		view.isForVoipConference = true
		view.isForOngoingVoipConference = true
		view.tableController.notFirstTime = true
		view.isGroupChat = true
		PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
	}
	
	
	
	
}
