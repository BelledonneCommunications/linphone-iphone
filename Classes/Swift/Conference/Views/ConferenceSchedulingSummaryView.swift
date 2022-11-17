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
			title:VoipTexts.conference_schedule_summary)
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
		let viaChatLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_send_invite_chat_summary)
		contentView.addSubview(viaChatLabel)
		viaChatLabel.matchParentSideBorders(insetedByDx: form_margin).alignUnder(view: schedulingStack,withMargin: 2*form_margin).done()
		viaChatLabel.numberOfLines = 2
		ConferenceSchedulingViewModel.shared.sendInviteViaChat.readCurrentAndObserve { (sendChat) in
      viaChatLabel.isHidden = sendChat != true || ConferenceSchedulingViewModel.shared.scheduleForLater.value != true
		}
			
		// Participants
		let participantsLabel = StyledLabel(VoipTheme.conference_scheduling_font, "  "+VoipTexts.conference_schedule_participants_list)
		contentView.addSubview(participantsLabel)
		participantsLabel.matchParentSideBorders().height(form_input_height).alignUnder(view: viaChatLabel,withMargin: form_margin).done()
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
		
		ConferenceSchedulingViewModel.shared.selectedAddresses.readCurrentAndObserve { (addresses) in
			self.participantsListTableView.reloadData()
			self.participantsListTableView.removeConstraints().done()
			self.participantsListTableView.matchParentSideBorders().alignUnder(view: participantsLabel,withMargin: self.form_margin).done()
			self.participantsListTableView.height(Double(addresses!.count) * VoipParticipantCell.cell_height).done()
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
      viaChatLabel.isHidden = later != true || ConferenceSchedulingViewModel.shared.sendInviteViaChat.value != true
			viaChatLabel.removeConstraints().matchParentSideBorders(insetedByDx: self.form_margin).alignUnder(view: schedulingStack,withMargin: (viaChatLabel.isHidden ? 0 : 1)*self.form_margin).done()
			if (viaChatLabel.isHidden) {
				viaChatLabel.height(0).done()
			}

			self.createButton.addSidePadding()
		}
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.view.backgroundColor = VoipTheme.voipBackgroundBWColor.get()
			participantsLabel.backgroundColor = VoipTheme.voipFormBackgroundColor.get()
			self.participantsListTableView.separatorColor = VoipTheme.separatorColor.get()
		}
				
	}
	
	override func viewWillAppear(_ animated: Bool) {
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
		super.viewWillAppear(animated)
	}
		
	
	
	func goBackParticipantsListSelection() {
		let view: ChatConversationCreateView = VIEW(ChatConversationCreateView.compositeViewDescription())
		view.unfragmentCompositeDescription()
		let addresses =  ConferenceSchedulingViewModel.shared.selectedAddresses.value!.map { (address) in String(address.asStringUriOnly()) }
		view.tableController.contactsGroup = (addresses as NSArray).mutableCopy() as? NSMutableArray
		view.tableController.notFirstTime = true
		view.isForEditing = false
		view.isForVoipConference = true
		PhoneMainView.instance().pop(toView: view.compositeViewDescription())
	}
	
	// Objc - bridge, as can't access easily to the view model.
	@objc func setParticipants(addresses:[String]) {
		ConferenceSchedulingViewModel.shared.selectedAddresses.value = []
		return addresses.forEach { (address) in
			if let address = try?Factory.Instance.createAddress(addr: address) {
				ConferenceSchedulingViewModel.shared.selectedAddresses.value?.append(address)
			}
		}
	}
	
	// TableView datasource delegate
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		guard let participants = ConferenceSchedulingViewModel.shared.selectedAddresses.value else {
			return 0
		}
		return participants.count
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell:VoipParticipantCell = tableView.dequeueReusableCell(withIdentifier: "VoipParticipantCellSSchedule") as! VoipParticipantCell
		guard let participant = ConferenceSchedulingViewModel.shared.selectedAddresses.value?[indexPath.row] else {
			return cell
		}
		cell.selectionStyle = .none
		cell.scheduleConfParticipantAddress = participant
		cell.limeBadge.isHidden = ConferenceSchedulingViewModel.shared.isEncrypted.value != true
		return cell
	}
	
	
}
