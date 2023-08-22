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
import SVProgressHUD

@objc class ConferenceSchedulingSummaryView:  BackNextNavigationView, UICompositeViewDelegate, UITableViewDataSource {
	
	let CONFERENCE_CREATION_TIME_OUT_SEC = 15.0
	
	let viaChatLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_send_invite_chat_summary)
	let speakersLabel = StyledLabel(VoipTheme.conference_scheduling_font, "  "+VoipTexts.conference_schedule_speakers_list)
	let participantsLabel = StyledLabel(VoipTheme.conference_scheduling_font, "  "+VoipTexts.conference_schedule_participants_list)
	let speakersListTableView = UITableView()
	let participantsListTableView = UITableView()
	
	let datePicker = StyledDatePicker(liveValue: ConferenceSchedulingViewModel.shared.scheduledDate,pickerMode: .date, readOnly:true)
	let timeZoneValue = StyledValuePicker(liveIndex: ConferenceSchedulingViewModel.shared.scheduledTimeZone,options: ConferenceSchedulingViewModel.timeZones.map({ (tzd: TimeZoneData) -> String in tzd.descWithOffset()}), readOnly:true)
	let durationValue = StyledValuePicker(liveIndex: ConferenceSchedulingViewModel.shared.scheduledDuration,options: ConferenceSchedulingViewModel.durationList.map({ (duration: Duration) -> String in duration.display}), readOnly:true)
	let timePicker = StyledDatePicker(liveValue: ConferenceSchedulingViewModel.shared.scheduledTime,pickerMode: .time, readOnly:true)
	let descriptionLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_description_hint)
	let descriptionInput = StyledTextView(VoipTheme.conference_scheduling_font, placeHolder:VoipTexts.conference_schedule_description_hint,liveValue: ConferenceSchedulingViewModel.shared.description, readOnly:true)
	let createButton = FormButton(backgroundStateColors: VoipTheme.primary_colors_background)
	let leftColumn = UIView()
	let rightColumn = UIView()
	let scheduleForm = UIView()

	static let compositeDescription = UICompositeViewDescription(ConferenceSchedulingSummaryView.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	override func viewDidLoad() {
		
		super.viewDidLoad(
			backAction: {
				self.goBackParticipantsListSelection()
			},nextAction: {
			},
			nextActionEnableCondition: ConferenceSchedulingViewModel.shared.continueEnabled,
			title: ConferenceSchedulingViewModel.shared.getMode() == 0 ? VoipTexts.conference_schedule_summary : VoipTexts.conference_schedule_broadcast_summary)
		super.nextButton.isHidden = true
	
		let subjectLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_subject_title)
		contentView.addSubview(subjectLabel)
		subjectLabel.alignParentLeft(withMargin: form_margin).alignParentTop().done()
		
		let encryptedIcon = UIImageView(image: UIImage(named: "security_2_indicator"))
		encryptedIcon.contentMode = .scaleAspectFit
		contentView.addSubview(encryptedIcon)
		encryptedIcon.height(form_input_height).alignParentTop().alignParentTop().alignParentRight(withMargin: form_margin).alignHorizontalCenterWith(subjectLabel).done()
		ConferenceSchedulingViewModel.shared.isEncrypted.readCurrentAndObserve { (encrypt) in
			encryptedIcon.isHidden = encrypt != true
		}
		
		
		let subjectInput = StyledTextView(VoipTheme.conference_scheduling_font, placeHolder:VoipTexts.conference_schedule_subject_hint, liveValue: ConferenceSchedulingViewModel.shared.subject, readOnly:true)
		contentView.addSubview(subjectInput)
		subjectInput.alignUnder(view: subjectLabel,withMargin: form_margin).matchParentSideBorders(insetedByDx: form_margin).height(form_input_height).done()
	
		
		let schedulingStack = UIStackView()
		schedulingStack.axis = .vertical
		contentView.addSubview(schedulingStack)
		schedulingStack.alignUnder(view: subjectInput,withMargin: 3*form_margin).matchParentSideBorders(insetedByDx: form_margin).done()
		

		schedulingStack.addArrangedSubview(scheduleForm)
		ConferenceSchedulingViewModel.shared.scheduleForLater.readCurrentAndObserve { (forLater) in self.scheduleForm.isHidden = forLater != true }
		
		// Left column (Date & Time)
		scheduleForm.addSubview(leftColumn)
		leftColumn.matchParentWidthDividedBy(2.2).alignParentLeft().alignParentTop(withMargin: form_margin).done()
		
		let dateLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_date)
		leftColumn.addSubview(dateLabel)
		dateLabel.alignParentLeft().alignParentTop(withMargin: form_margin).done()
		
		leftColumn.addSubview(datePicker)
		datePicker.alignParentLeft().alignUnder(view: dateLabel,withMargin: form_margin).matchParentSideBorders().done()
		
		let timeLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_time)
		leftColumn.addSubview(timeLabel)
		timeLabel.alignParentLeft().alignUnder(view: datePicker,withMargin: form_margin).done()
		
		leftColumn.addSubview(timePicker)
		timePicker.alignParentLeft().alignUnder(view: timeLabel,withMargin: form_margin).matchParentSideBorders().done()
	
		leftColumn.wrapContentY().done()

		
		// Right column (Duration & Timezone)
		scheduleForm.addSubview(rightColumn)
		rightColumn.matchParentWidthDividedBy(2.2).alignParentRight().alignParentTop().done()
		
		let durationLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_duration)
		rightColumn.addSubview(durationLabel)
		durationLabel.alignParentLeft().alignParentTop(withMargin: form_margin).done()
		
		rightColumn.addSubview(durationValue)
		durationValue.alignParentLeft().alignUnder(view: durationLabel,withMargin: form_margin).matchParentSideBorders().done()
		
		let timeZoneLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_timezone)
		rightColumn.addSubview(timeZoneLabel)
		timeZoneLabel.alignParentLeft().alignUnder(view: durationValue,withMargin: form_margin).done()
		
		rightColumn.addSubview(timeZoneValue)
		timeZoneValue.alignParentLeft().alignUnder(view: timeZoneLabel,withMargin: form_margin).matchParentSideBorders().done()
	
		rightColumn.wrapContentY().done()
		
		// Description
		scheduleForm.addSubview(descriptionLabel)
		descriptionInput.textContainer.maximumNumberOfLines = 5
		descriptionInput.textContainer.lineBreakMode = .byWordWrapping
		scheduleForm.addSubview(descriptionInput)
		
		// Sending method
		contentView.addSubview(viaChatLabel)
		viaChatLabel.matchParentSideBorders(insetedByDx: form_margin).alignUnder(view: schedulingStack,withMargin: 2*form_margin).done()
		viaChatLabel.numberOfLines = 2
		ConferenceSchedulingViewModel.shared.sendInviteViaChat.readCurrentAndObserve { (sendChat) in
			self.viaChatLabel.isHidden = sendChat != true || ConferenceSchedulingViewModel.shared.scheduleForLater.value != true
		}
		
		// Speaker
		contentView.addSubview(speakersLabel)
		speakersLabel.matchParentSideBorders().height(form_input_height).alignUnder(view: viaChatLabel,withMargin: form_margin).done()
		speakersLabel.textAlignment = .left
		
		
		contentView.addSubview(speakersListTableView)
		speakersListTableView.isScrollEnabled = false
		speakersListTableView.dataSource = self
		speakersListTableView.register(VoipSpeakerCell.self, forCellReuseIdentifier: "VoipSpeakerCellSSchedule")
		speakersListTableView.allowsSelection = false
		if #available(iOS 15.0, *) {
			speakersListTableView.allowsFocus = false
		}
		speakersListTableView.separatorStyle = .singleLine
		speakersListTableView.backgroundColor = .clear
		
		// Participants
		contentView.addSubview(participantsLabel)
		participantsLabel.matchParentSideBorders().height(form_input_height).alignUnder(view: speakersListTableView,withMargin: form_margin).done()
		participantsLabel.textAlignment = .left
		
		
		contentView.addSubview(participantsListTableView)
		participantsListTableView.isScrollEnabled = false
		participantsListTableView.dataSource = self
		participantsListTableView.register(VoipParticipantCell.self, forCellReuseIdentifier: "VoipParticipantCellSSchedule")
		participantsListTableView.allowsSelection = false
		if #available(iOS 15.0, *) {
			participantsListTableView.allowsFocus = false
		}
		participantsListTableView.separatorStyle = .singleLine
		participantsListTableView.backgroundColor = .clear
		
		
		self.createButton.isEnabled = ConferenceSchedulingViewModel.shared.getMode() == 0 ? true : (ConferenceSchedulingViewModel.shared.selectedParticipants.value!.filter({$0.role == .Speaker}).count > 0 && ConferenceSchedulingViewModel.shared.selectedParticipants.value!.filter({$0.role == .Listener}).count > 0)
		
		ConferenceSchedulingViewModel.shared.selectedParticipants.readCurrentAndObserve { (participants) in
			self.speakersListTableView.reloadData()
			self.speakersListTableView.removeConstraints().done()
			self.speakersListTableView.matchParentSideBorders().alignUnder(view: self.speakersLabel,withMargin: self.form_margin).done()
			if ConferenceSchedulingViewModel.shared.getMode() != 0 {
				self.speakersListTableView.height((participants!.filter({$0.role == .Speaker}).count > 0 ? Double(participants!.filter({$0.role == .Speaker}).count) : 0.5) * VoipParticipantCell.cell_height).done()
			} else {
				self.speakersListTableView.height(0).done()
			}
			if participants!.count == 0 {
				let emptyLabel = UILabel(frame: CGRect(x: 0, y: 0, width: self.view.bounds.size.width, height: self.view.bounds.size.height))
				emptyLabel.text = VoipTexts.conference_schedule_speakers_list_empty
				emptyLabel.textAlignment = NSTextAlignment.center
				self.speakersListTableView.backgroundView = emptyLabel
				self.speakersListTableView.separatorStyle = UITableViewCell.SeparatorStyle.none
				self.speakersListTableView.backgroundView?.isHidden = false
			} else {
				self.speakersListTableView.backgroundView?.isHidden = true
			}
			
			self.participantsListTableView.reloadData()
			self.participantsListTableView.removeConstraints().done()
			self.participantsListTableView.matchParentSideBorders().alignUnder(view: self.participantsLabel,withMargin: self.form_margin).done()
			if ConferenceSchedulingViewModel.shared.getMode() != 0 {
				self.participantsListTableView.height((participants!.filter({$0.role == .Listener}).count > 0 ? Double(participants!.filter({$0.role == .Listener}).count) : 0.5) * VoipParticipantCell.cell_height).done()
			} else {
				self.participantsListTableView.height(Double(participants!.filter({$0.role == .Speaker}).count) * VoipParticipantCell.cell_height).done()
			}
			
			if ConferenceSchedulingViewModel.shared.getMode() != 0 && ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Listener}).count == 0 {
				self.participantsListTableView.reloadData()
				self.participantsListTableView.removeConstraints().done()
				self.participantsListTableView.matchParentSideBorders().alignUnder(view: self.participantsLabel,withMargin: self.form_margin).done()
				self.participantsListTableView.height(0.5 * VoipParticipantCell.cell_height).done()
				let emptyLabel = UILabel(frame: CGRect(x: 0, y: 0, width: self.view.bounds.size.width, height: self.view.bounds.size.height))
				emptyLabel.text = VoipTexts.conference_schedule_participants_list_empty
				emptyLabel.textAlignment = NSTextAlignment.center
				self.participantsListTableView.backgroundView = emptyLabel
				self.participantsListTableView.separatorStyle = UITableViewCell.SeparatorStyle.none
			} else {
				self.participantsListTableView.backgroundView?.isHidden = true
			}
		}
		
		// Create / Schedule
		contentView.addSubview(createButton)
		createButton.centerX().alignParentBottom(withMargin: 3*self.form_margin).alignUnder(view: participantsListTableView,withMargin: 3*self.form_margin).width(0).done()
		ConferenceSchedulingViewModel.shared.scheduleForLater.readCurrentAndObserve { _ in
			self.createButton.title = ConferenceSchedulingViewModel.shared.scheduleForLater.value == true ? ConferenceSchedulingViewModel.shared.existingConfInfo.value != nil ? VoipTexts.conference_schedule_edit.uppercased() : VoipTexts.conference_schedule_start.uppercased() : VoipTexts.conference_group_call_create.uppercased()
			self.createButton.addSidePadding()
		}
		ConferenceSchedulingViewModel.shared.existingConfInfo.readCurrentAndObserve { _ in
			self.createButton.title = ConferenceSchedulingViewModel.shared.scheduleForLater.value == true ? ConferenceSchedulingViewModel.shared.existingConfInfo.value != nil ? VoipTexts.conference_schedule_edit.uppercased() : VoipTexts.conference_schedule_start.uppercased() : VoipTexts.conference_group_call_create.uppercased()
			self.createButton.addSidePadding()
		}
		
		ConferenceSchedulingViewModel.shared.conferenceCreationInProgress.observe { progress in
			if (progress == true) {
				SVProgressHUD.show()
			} else {
				SVProgressHUD.dismiss()
			}
		}
		
		var enableCreationTimeOut = false

		ConferenceSchedulingViewModel.shared.conferenceCreationCompletedEvent.observe { pair in
			enableCreationTimeOut = false
			if (ConferenceSchedulingViewModel.shared.scheduleForLater.value == true) {
				PhoneMainView.instance().pop(toView:ScheduledConferencesView.compositeDescription)
				VoipDialog.toast(message: VoipTexts.conference_schedule_info_created)
				
			}
		}
		ConferenceSchedulingViewModel.shared.onErrorEvent.observe { error in
			VoipDialog.init(message: error!).show()
		}
		createButton.onClick {
			enableCreationTimeOut = true
			ConferenceSchedulingViewModel.shared.createConference()
			DispatchQueue.main.asyncAfter(deadline: .now() + self.CONFERENCE_CREATION_TIME_OUT_SEC) {
				if (enableCreationTimeOut) {
					enableCreationTimeOut = false
					ConferenceSchedulingViewModel.shared.conferenceCreationInProgress.value = false
					ConferenceSchedulingViewModel.shared.onErrorEvent.value = VoipTexts.call_error_server_timeout
				}
			}
		}
		ConferenceSchedulingViewModel.shared.scheduleForLater.readCurrentAndObserve { (later) in
			self.createButton.title = ConferenceSchedulingViewModel.shared.scheduleForLater.value == true ? ConferenceSchedulingViewModel.shared.existingConfInfo.value != nil ? VoipTexts.conference_schedule_edit.uppercased() : VoipTexts.conference_schedule_start.uppercased() : VoipTexts.conference_group_call_create.uppercased()
			self.viaChatLabel.isHidden = later != true || ConferenceSchedulingViewModel.shared.sendInviteViaChat.value != true
			self.viaChatLabel.removeConstraints().matchParentSideBorders(insetedByDx: self.form_margin).alignUnder(view: schedulingStack,withMargin: (self.viaChatLabel.isHidden ? 0 : 1)*self.form_margin).done()
			if (self.viaChatLabel.isHidden) {
				self.viaChatLabel.height(0).done()
			}

			self.createButton.addSidePadding()
		}
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.view.backgroundColor = VoipTheme.voipBackgroundBWColor.get()
			self.speakersLabel.backgroundColor = VoipTheme.voipFormBackgroundColor.get()
			self.speakersListTableView.separatorColor = VoipTheme.separatorColor.get()
			self.participantsLabel.backgroundColor = VoipTheme.voipFormBackgroundColor.get()
			self.participantsListTableView.separatorColor = VoipTheme.separatorColor.get()
		}
				
	}
	
	override func viewWillAppear(_ animated: Bool) {
		if ConferenceSchedulingViewModel.shared.existingConfInfo.value != nil {
			let isBroadcastExisting = ConferenceSchedulingViewModel.shared.existingConfInfo.value??.participantInfos.filter({$0.role == .Speaker}).count != 0 && ConferenceSchedulingViewModel.shared.existingConfInfo.value??.participantInfos.filter({$0.role == .Listener}).count != 0
			ConferenceSchedulingViewModel.shared.mode.value = isBroadcastExisting ? 1 : 0
		}
			
		titleLabel.text = ConferenceSchedulingViewModel.shared.getMode() == 0 ? VoipTexts.conference_schedule_summary : VoipTexts.conference_schedule_broadcast_summary
		
		datePicker.liveValue = ConferenceSchedulingViewModel.shared.scheduledDate
		timeZoneValue.setIndex(index: ConferenceSchedulingViewModel.shared.scheduledTimeZone.value!)
		durationValue.setIndex(index: ConferenceSchedulingViewModel.shared.scheduledDuration.value!)
		timePicker.liveValue = ConferenceSchedulingViewModel.shared.scheduledTime
		
		descriptionInput.text = ConferenceSchedulingViewModel.shared.description.value
		descriptionLabel.removeConstraints().alignUnder(view: leftColumn,withMargin: form_margin).alignUnder(view: rightColumn,withMargin: form_margin).matchParentSideBorders().done()
		descriptionInput.removeConstraints().alignUnder(view: descriptionLabel,withMargin: form_margin).matchParentSideBorders().height(description_height).alignParentBottom(withMargin: form_margin*2).done()
		if (ConferenceSchedulingViewModel.shared.description.value == nil || ConferenceSchedulingViewModel.shared.description.value!.count == 0) {
			descriptionLabel.height(0).done()
			descriptionInput.height(0).done()
		}
		// Wrap form
		scheduleForm.removeConstraints().matchParentSideBorders().wrapContentY().done()
		
		createButton.addSidePadding()
		
		speakersLabel.removeConstraints().matchParentSideBorders().height(form_input_height).alignUnder(view: viaChatLabel,withMargin: form_margin).done()
		
		speakersListTableView.removeConstraints().done()
		speakersListTableView.matchParentSideBorders().alignUnder(view: self.speakersLabel,withMargin: self.form_margin).done()
		speakersListTableView.height(Double((ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Speaker}).count)!) * VoipParticipantCell.cell_height).done()
		
		if ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Speaker}).count == 0 {
			self.speakersListTableView.reloadData()
			self.speakersListTableView.removeConstraints().done()
			self.speakersListTableView.matchParentSideBorders().alignUnder(view: self.speakersLabel,withMargin: self.form_margin).done()
			self.speakersListTableView.height(0.5 * VoipParticipantCell.cell_height).done()
			let emptyLabel = UILabel(frame: CGRect(x: 0, y: 0, width: self.view.bounds.size.width, height: self.view.bounds.size.height))
			emptyLabel.text = VoipTexts.conference_schedule_speakers_list_empty
			emptyLabel.textAlignment = NSTextAlignment.center
			self.speakersListTableView.backgroundView = emptyLabel
			self.speakersListTableView.separatorStyle = UITableViewCell.SeparatorStyle.none
		}
		
		if ConferenceSchedulingViewModel.shared.getMode() != 0 && ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Listener}).count == 0 {
			self.participantsListTableView.reloadData()
			self.participantsListTableView.removeConstraints().done()
			self.participantsListTableView.matchParentSideBorders().alignUnder(view: self.participantsLabel,withMargin: self.form_margin).done()
			self.participantsListTableView.height(0.5 * VoipParticipantCell.cell_height).done()
			let emptyLabel = UILabel(frame: CGRect(x: 0, y: 0, width: self.view.bounds.size.width, height: self.view.bounds.size.height))
			emptyLabel.text = VoipTexts.conference_schedule_participants_list_empty
			emptyLabel.textAlignment = NSTextAlignment.center
			self.participantsListTableView.backgroundView = emptyLabel
			self.participantsListTableView.separatorStyle = UITableViewCell.SeparatorStyle.none
		}
		
		if ConferenceSchedulingViewModel.shared.getMode() == 0 {
			speakersLabel.isHidden = true
			speakersListTableView.isHidden = true
			speakersLabel.height(0).done()
			speakersListTableView.height(0).done()
		} else {
			speakersLabel.isHidden = false
			speakersListTableView.isHidden = false
		}
		
		super.viewWillAppear(animated)
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		reloadLists()
	}
		
	
	
	func goBackParticipantsListSelection() {
		let view: ChatConversationCreateView = VIEW(ChatConversationCreateView.compositeViewDescription())
		view.unfragmentCompositeDescription()
		
		let addresses =  ConferenceSchedulingViewModel.shared.selectedParticipants.value!.map { (participant) in String(participant.address!.asStringUriOnly()) }
		view.tableController.contactsGroup = (addresses as NSArray).mutableCopy() as? NSMutableArray
		view.tableController.notFirstTime = true
		view.isForEditing = false
		view.isForVoipConference = true
		PhoneMainView.instance().pop(toView: view.compositeViewDescription())
	}
	
	// Objc - bridge, as can't access easily to the view model.
	@objc func setParticipants(addresses:[String]) {
		ConferenceSchedulingViewModel.shared.selectedParticipants.value = []
		return addresses.forEach { (address) in
			do {
				let createAddress = try Factory.Instance.createAddress(addr: address)
				if let address = try?Factory.Instance.createParticipantInfo(address: createAddress) {
					ConferenceSchedulingViewModel.shared.selectedParticipants.value?.append(address)
					address.role = ConferenceSchedulingViewModel.shared.getMode() != 0 ? .Listener : .Speaker
				}
			} catch {
				Log.e("[goBackParticipantsListSelection] unable to create ParticipantInfo \(error)")
			}
			
		}
	}
	
	// TableView datasource delegate
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		if(tableView == speakersListTableView){
			guard let speakers = ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Speaker}) else {
				return 0
			}
			return speakers.count
		} else {
			if ConferenceSchedulingViewModel.shared.getMode() != 0 {
				guard let participants = ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Listener}) else {
					return 0
				}
				return participants.count
			} else {
				guard let participants = ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Speaker}) else {
					return 0
				}
				return participants.count
			}
		}
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		if(tableView == speakersListTableView){
			let cell:VoipSpeakerCell = tableView.dequeueReusableCell(withIdentifier: "VoipSpeakerCellSSchedule") as! VoipSpeakerCell
			guard let speaker = ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Speaker})[indexPath.row] else {
				return cell
			}
			cell.selectionStyle = .none
			cell.scheduleConfSpeakerAddress = speaker.address
			cell.limeBadge.isHidden = ConferenceSchedulingViewModel.shared.isEncrypted.value != true
			
			cell.deleteButton.addTarget(self, action: #selector(deleteButtonPressed), for: .touchUpInside)
			cell.deleteButton.tag = indexPath.row
			
			return cell
		} else {
			let cell:VoipParticipantCell = tableView.dequeueReusableCell(withIdentifier: "VoipParticipantCellSSchedule") as! VoipParticipantCell
			if ConferenceSchedulingViewModel.shared.getMode() != 0 {
				guard let participant = ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Listener})[indexPath.row] else {
					return cell
				}
				cell.scheduleConfParticipantAddress = participant.address
			} else {
				guard let speaker = ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Speaker})[indexPath.row] else {
					return cell
				}
				cell.scheduleConfParticipantAddress = speaker.address
			}
			cell.selectionStyle = .none
			cell.limeBadge.isHidden = ConferenceSchedulingViewModel.shared.isEncrypted.value != true
			
			if ConferenceSchedulingViewModel.shared.getMode() == 0 {
				cell.addButton.isHidden = true
			} else {
				cell.addButton.isHidden = false
				cell.addButton.addTarget(self, action: #selector(addButtonPressed), for: .touchUpInside)
				cell.addButton.tag = indexPath.row
			}
			return cell
		}
	}
	
	@objc func addButtonPressed(sender:UIButton!) {
		if(ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Listener})[sender.tag] != nil) {
			ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Listener})[sender.tag].role = .Speaker
		}
		reloadLists()
	}
	
	@objc func deleteButtonPressed(sender:UIButton!) {
		if(ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Speaker})[sender.tag] != nil) {
				ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Speaker})[sender.tag].role = .Listener
		}
		reloadLists()
	}
	
	func reloadLists(){
		let participants = ConferenceSchedulingViewModel.shared.selectedParticipants.value
		
		self.speakersListTableView.reloadData()
		self.speakersListTableView.removeConstraints().done()
		self.speakersListTableView.matchParentSideBorders().alignUnder(view: self.speakersLabel,withMargin: self.form_margin).done()
		if ConferenceSchedulingViewModel.shared.getMode() != 0 {
			self.speakersListTableView.height((participants!.filter({$0.role == .Speaker}).count > 0 ? Double(participants!.filter({$0.role == .Speaker}).count) : 0.5) * VoipParticipantCell.cell_height).done()
		} else {
			self.speakersListTableView.height(0).done()
		}
		if participants!.filter({$0.role == .Speaker}).count == 0 {
			let emptyLabel = UILabel(frame: CGRect(x: 0, y: 0, width: self.view.bounds.size.width, height: self.view.bounds.size.height))
			emptyLabel.text = VoipTexts.conference_schedule_speakers_list_empty
			emptyLabel.textAlignment = NSTextAlignment.center
			self.speakersListTableView.backgroundView = emptyLabel
			self.speakersListTableView.separatorStyle = UITableViewCell.SeparatorStyle.none
			self.speakersListTableView.backgroundView?.isHidden = false
		} else {
			self.speakersListTableView.backgroundView?.isHidden = true
		}
		
		self.participantsListTableView.reloadData()
		self.participantsListTableView.removeConstraints().done()
		self.participantsListTableView.matchParentSideBorders().alignUnder(view: participantsLabel,withMargin: self.form_margin).done()
		if ConferenceSchedulingViewModel.shared.getMode() != 0 {
			self.participantsListTableView.height(Double(participants!.filter({$0.role == .Listener}).count) * VoipParticipantCell.cell_height).done()
		} else {
			self.participantsListTableView.height(Double(participants!.filter({$0.role == .Speaker}).count) * VoipParticipantCell.cell_height).done()
		}
		
		if ConferenceSchedulingViewModel.shared.getMode() != 0 && ConferenceSchedulingViewModel.shared.selectedParticipants.value?.filter({$0.role == .Listener}).count == 0 {
			self.participantsListTableView.reloadData()
			self.participantsListTableView.removeConstraints().done()
			self.participantsListTableView.matchParentSideBorders().alignUnder(view: self.participantsLabel,withMargin: self.form_margin).done()
			self.participantsListTableView.height(0.5 * VoipParticipantCell.cell_height).done()
			let emptyLabel = UILabel(frame: CGRect(x: 0, y: 0, width: self.view.bounds.size.width, height: self.view.bounds.size.height))
			emptyLabel.text = VoipTexts.conference_schedule_participants_list_empty
			emptyLabel.textAlignment = NSTextAlignment.center
			self.participantsListTableView.backgroundView = emptyLabel
			self.participantsListTableView.separatorStyle = UITableViewCell.SeparatorStyle.none
		} else {
			self.participantsListTableView.backgroundView?.isHidden = true
		}
		self.createButton.isEnabled = ConferenceSchedulingViewModel.shared.getMode() == 0 ? true : (ConferenceSchedulingViewModel.shared.selectedParticipants.value!.filter({$0.role == .Speaker}).count > 0 && ConferenceSchedulingViewModel.shared.selectedParticipants.value!.filter({$0.role == .Listener}).count > 0)
	}
}
