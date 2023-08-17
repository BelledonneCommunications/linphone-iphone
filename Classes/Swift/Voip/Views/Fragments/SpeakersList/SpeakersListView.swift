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

@objc class SpeakersListView: DismissableView, UITableViewDataSource {
	
	// Layout constants
	let side_margin = 10.0
	
	let speakersListTableView =  UITableView()
	let noSpeakersLabel = StyledLabel(VoipTheme.empty_list_font,VoipTexts.conference_empty)
	
	
	var callsDataObserver : MutableLiveDataOnChangeClosure<[CallData]>? = nil
	
	init() {
		super.init(title: VoipTexts.call_action_speakers_list)
		
		
		let edit = CallControlButton(buttonTheme: VoipTheme.voip_edit, onClickAction: {
			self.removeFromSuperview()
			self.gotoSpeakersListSelection()
		})
		super.headerView.addSubview(edit)
		edit.centerY().done()
		super.dismiss?.toRightOf(edit,withLeftMargin: dismiss_right_margin).centerY().done()
		
		
		// SpeakersList
		super.contentView.addSubview(speakersListTableView)
		speakersListTableView.matchParentDimmensions().done()
		speakersListTableView.dataSource = self
		speakersListTableView.register(VoipSpeakerCell.self, forCellReuseIdentifier: "VoipSpeakerCell")
		speakersListTableView.allowsSelection = false
		if #available(iOS 15.0, *) {
			speakersListTableView.allowsFocus = false
		}
		speakersListTableView.separatorStyle = .singleLine
		speakersListTableView.separatorColor = .white
	
	
		ConferenceViewModel.shared.conferenceSpeakers.readCurrentAndObserve{ _ in
			self.speakersListTableView.reloadData()
			self.noSpeakersLabel.isHidden =  ConferenceViewModel.shared.conferenceSpeakers.value?.count ?? 0 > 0
		}
		
		ConferenceViewModel.shared.isMeAdmin.readCurrentAndObserve { (meAdmin) in
			edit.isHidden = meAdmin != true
		}
		
		super.contentView.addSubview(noSpeakersLabel)
		noSpeakersLabel.center().matchParentSideBorders(insetedByDx: side_margin).done()
		noSpeakersLabel.isHidden =  ConferenceViewModel.shared.conferenceSpeakers.value?.count ?? 0 > 0
		noSpeakersLabel.numberOfLines = 2
		self.speakersListTableView.backgroundColor = VoipTheme.voipBackgroundBWColor.get()

		UIDeviceBridge.displayModeSwitched.observe { _ in
			self.speakersListTableView.backgroundColor = VoipTheme.voipBackgroundBWColor.get()
			self.speakersListTableView.reloadData()
		}
	}

	
	// TableView datasource delegate
	
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		guard let speakers = ConferenceViewModel.shared.conferenceSpeakers.value else {
			return 0
		}
		return speakers.count
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell:VoipSpeakerCell = tableView.dequeueReusableCell(withIdentifier: "VoipSpeakerCell") as! VoipSpeakerCell
		guard let speakerData = ConferenceViewModel.shared.conferenceSpeakers.value?[indexPath.row] else {
			return cell
		}
		cell.selectionStyle = .none
		cell.speakerData = speakerData
		cell.owningParticpantsListView = self
		return cell
	}
	
	// View controller
	
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	func gotoSpeakersListSelection() {
		let view: ChatConversationCreateView = self.VIEW(ChatConversationCreateView.compositeViewDescription())
		view.unfragmentCompositeDescription()
		let addresses =  ConferenceViewModel.shared.conferenceSpeakers.value!.map { (data) in String(data.speaker.address!.asStringUriOnly()) }
		view.tableController.contactsGroup = (addresses as NSArray).mutableCopy() as? NSMutableArray
		view.isForEditing = false
		view.isForVoipConference = true
		view.isForOngoingVoipConference = true
		view.tableController.notFirstTime = true
		view.isGroupChat = true
		PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
	}
	
	
	
	
}
