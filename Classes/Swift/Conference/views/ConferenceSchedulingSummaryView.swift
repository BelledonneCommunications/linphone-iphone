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
	
	let viewModel = ConferenceSchedulingViewModel.shared
	let participantsListTableView = UITableView()

	static let compositeDescription = UICompositeViewDescription(ConferenceSchedulingSummaryView.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	override func viewDidLoad() {
		
		super.viewDidLoad(
			backAction: {
				self.goBackParticipantsListSelection()
			},nextAction: {
			},
			nextActionEnableCondition: viewModel.continueEnabled,
			title:VoipTexts.conference_schedule_summary)
		super.nextButton.isHidden = true
	
		let subjectLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_subject_title)
		contentView.addSubview(subjectLabel)
		subjectLabel.alignParentLeft(withMargin: form_margin).alignParentTop().done()
		
		let encryptedIcon = UIImageView(image: UIImage(named: "security_2_indicator"))
		encryptedIcon.contentMode = .scaleAspectFit
		contentView.addSubview(encryptedIcon)
		encryptedIcon.height(form_input_height).alignParentTop().alignParentTop().alignParentRight(withMargin: form_margin).alignHorizontalCenterWith(subjectLabel).done()
		viewModel.isEncrypted.readCurrentAndObserve { (encrypt) in
			encryptedIcon.isHidden = encrypt != true
		}
		
		
		let subjectInput = StyledTextView(VoipTheme.conference_scheduling_font, placeHolder:VoipTexts.conference_schedule_subject_hint, liveValue: viewModel.subject, readOnly:true)
		contentView.addSubview(subjectInput)
		subjectInput.alignUnder(view: subjectLabel,withMargin: form_margin).matchParentSideBorders(insetedByDx: form_margin).height(form_input_height).done()
	
		
		let schedulingStack = UIStackView()
		schedulingStack.axis = .vertical
		contentView.addSubview(schedulingStack)
		schedulingStack.alignUnder(view: subjectInput,withMargin: 3*form_margin).matchParentSideBorders(insetedByDx: form_margin).done()
		

		let scheduleForm = UIView()
		schedulingStack.addArrangedSubview(scheduleForm)
		scheduleForm.matchParentSideBorders().done()
		viewModel.scheduleForLater.readCurrentAndObserve { (forLater) in scheduleForm.isHidden = forLater != true }
		
		// Left column (Date & Time)
		let leftColumn = UIView()
		scheduleForm.addSubview(leftColumn)
		leftColumn.matchParentWidthDividedBy(2.2).alignParentLeft(withMargin: form_margin).alignParentTop(withMargin: form_margin).done()
		
		let dateLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_date)
		leftColumn.addSubview(dateLabel)
		dateLabel.alignParentLeft().alignParentTop(withMargin: form_margin).done()
		
		let datePicker = StyledDatePicker(liveValue: viewModel.scheduledDate,pickerMode: .date, readOnly:true)
		leftColumn.addSubview(datePicker)
		datePicker.alignParentLeft().alignUnder(view: dateLabel,withMargin: form_margin).matchParentSideBorders().done()
		
		let timeLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_time)
		leftColumn.addSubview(timeLabel)
		timeLabel.alignParentLeft().alignUnder(view: datePicker,withMargin: form_margin).done()
		
		let timePicker = StyledDatePicker(liveValue: viewModel.scheduledTime,pickerMode: .time, readOnly:true)
		leftColumn.addSubview(timePicker)
		timePicker.alignParentLeft().alignUnder(view: timeLabel,withMargin: form_margin).matchParentSideBorders().done()
	
		leftColumn.wrapContentY().done()

		
		// Right column (Duration & Timezone)
		let rightColumn = UIView()
		scheduleForm.addSubview(rightColumn)
		rightColumn.matchParentWidthDividedBy(2.2).alignParentRight(withMargin: form_margin).alignParentTop().done()
		
		let durationLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_duration)
		rightColumn.addSubview(durationLabel)
		durationLabel.alignParentLeft().alignParentTop(withMargin: form_margin).done()
		
		let durationValue = StyledValuePicker(liveIndex: viewModel.scheduledDuration,options: ConferenceSchedulingViewModel.durationList.map({ (duration: Duration) -> String in duration.display}), readOnly:true)
		rightColumn.addSubview(durationValue)
		durationValue.alignParentLeft().alignUnder(view: durationLabel,withMargin: form_margin).matchParentSideBorders().done()
		
		let timeZoneLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_timezone)
		rightColumn.addSubview(timeZoneLabel)
		timeZoneLabel.alignParentLeft().alignUnder(view: durationValue,withMargin: form_margin).done()
		
		let timeZoneValue = StyledValuePicker(liveIndex: viewModel.scheduledTimeZone,options: ConferenceSchedulingViewModel.timeZones.map({ (tzd: TimeZoneData) -> String in tzd.descWithOffset()}), readOnly:true)
		rightColumn.addSubview(timeZoneValue)
		timeZoneValue.alignParentLeft().alignUnder(view: timeZoneLabel,withMargin: form_margin).matchParentSideBorders().done()
	
		rightColumn.wrapContentY().done()
		
		// Description
		let descriptionLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_description_title)
		scheduleForm.addSubview(descriptionLabel)
		descriptionLabel.alignUnder(view: leftColumn,withMargin: form_margin).alignUnder(view: rightColumn,withMargin: form_margin).matchParentSideBorders(insetedByDx: form_margin).done()
		
		let descriptionInput = StyledTextView(VoipTheme.conference_scheduling_font, placeHolder:VoipTexts.conference_schedule_description_hint,liveValue: viewModel.description, readOnly:true)
		descriptionInput.textContainer.maximumNumberOfLines = 5
		descriptionInput.textContainer.lineBreakMode = .byWordWrapping
		scheduleForm.addSubview(descriptionInput)
		descriptionInput.alignUnder(view: descriptionLabel,withMargin: form_margin).matchParentSideBorders(insetedByDx: form_margin).height(description_height).alignParentBottom(withMargin: form_margin*2).done()

		scheduleForm.wrapContentY().done()
		
		// Sending method
		let viaChatLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_send_invite_chat_summary)
		contentView.addSubview(viaChatLabel)
		viaChatLabel.matchParentSideBorders(insetedByDx: form_margin).alignUnder(view: schedulingStack,withMargin: 2*form_margin).done()
		viewModel.sendInviteViaChat.readCurrentAndObserve { (sendChat) in
			viaChatLabel.isHidden = sendChat != true
		}
			
		// Participants
		let participantsLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_participants_list)
		participantsLabel.backgroundColor = VoipTheme.voipFormBackgroundColor.get()
		contentView.addSubview(participantsLabel)
		participantsLabel.matchParentSideBorders().height(form_input_height).alignUnder(view: viaChatLabel,withMargin: form_margin*2).done()
		participantsLabel.textAlignment = .center
		
		
		contentView.addSubview(participantsListTableView)
		participantsListTableView.isScrollEnabled = false
		participantsListTableView.dataSource = self
		participantsListTableView.register(VoipParticipantCell.self, forCellReuseIdentifier: "VoipParticipantCellSSchedule")
		participantsListTableView.allowsSelection = false
		if #available(iOS 15.0, *) {
			participantsListTableView.allowsFocus = false
		}
		participantsListTableView.separatorStyle = .singleLine
		participantsListTableView.separatorColor = VoipTheme.light_grey_color
		
		viewModel.selectedAddresses.readCurrentAndObserve { (addresses) in
			self.participantsListTableView.reloadData()
			self.participantsListTableView.removeConstraints().done()
			self.participantsListTableView.matchParentSideBorders().alignUnder(view: participantsLabel,withMargin: self.form_margin).done()
			self.participantsListTableView.height(Double(addresses!.count) * VoipParticipantCell.cell_height).done()
		}
		
		// Create / Schedule
		let createButton = FormButton(backgroundStateColors: VoipTheme.primary_colors_background)
		contentView.addSubview(createButton)
		viewModel.scheduleForLater.readCurrentAndObserve { _ in
			createButton.title = self.viewModel.scheduleForLater.value == true ? VoipTexts.conference_schedule.uppercased() : VoipTexts.conference_schedule_create.uppercased()
			createButton.addSidePadding()
		}
		
		self.viewModel.conferenceCreationInProgress.observe { progress in
			if (progress == true) {
				SVProgressHUD.show()
			} else {
				SVProgressHUD.dismiss()
			}
		}
		
		var enableCreationTimeOut = false

		viewModel.conferenceCreationCompletedEvent.observe { pair in
			enableCreationTimeOut = false
			if (self.viewModel.scheduleForLater.value == true) {
				PhoneMainView.instance().pop(toView:ScheduledConferencesView.compositeDescription)
			} else {
				let view: ConferenceWaitingRoomFragment = self.VIEW(ConferenceWaitingRoomFragment.compositeViewDescription());
				PhoneMainView.instance().pop(toView:view.compositeViewDescription())
				view.setDetails(subject: pair!.second!, url: pair!.first!)
			}
		}
		viewModel.onErrorEvent.observe { error in
			VoipDialog.init(message: error!).show()
		}
		createButton.onClick {
			enableCreationTimeOut = true
			self.viewModel.createConference()
			DispatchQueue.main.asyncAfter(deadline: .now() + self.CONFERENCE_CREATION_TIME_OUT_SEC) {
				if (enableCreationTimeOut) {
					enableCreationTimeOut = false
					self.viewModel.conferenceCreationInProgress.value = false
					self.viewModel.onErrorEvent.value = VoipTexts.call_error_server_timeout
				}
			}
		}
		viewModel.scheduleForLater.readCurrentAndObserve { _ in
			createButton.title = self.viewModel.scheduleForLater.value == true ? VoipTexts.conference_schedule.uppercased() : VoipTexts.conference_schedule_create.uppercased()
			createButton.addSidePadding()
		}
		
		createButton.centerX().alignParentBottom(withMargin: 3*self.form_margin).alignUnder(view: participantsListTableView,withMargin: 3*self.form_margin).done()
		
	}
		
	
	
	func goBackParticipantsListSelection() {
		let view: ChatConversationCreateView = VIEW(ChatConversationCreateView.compositeViewDescription())
		let addresses =  viewModel.selectedAddresses.value!.map { (address) in String(address.asStringUriOnly()) }
		view.tableController.contactsGroup = (addresses as NSArray).mutableCopy() as? NSMutableArray
		view.tableController.notFirstTime = true
		view.isForEditing = false
		view.isForVoipConference = true
		PhoneMainView.instance().pop(toView: view.compositeViewDescription())
	}
	
	// Objc - bridge, as can't access easily to the view model.
	@objc func setParticipants(addresses:[String]) {
		viewModel.selectedAddresses.value = []
		return addresses.forEach { (address) in
			if let address = try?Factory.Instance.createAddress(addr: address) {
				viewModel.selectedAddresses.value?.append(address)
			}
		}
	}
	
	// TableView datasource delegate
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		guard let participants = viewModel.selectedAddresses.value else {
			return 0
		}
		return participants.count
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell:VoipParticipantCell = tableView.dequeueReusableCell(withIdentifier: "VoipParticipantCellSSchedule") as! VoipParticipantCell
		guard let participant = viewModel.selectedAddresses.value?[indexPath.row] else {
			return cell
		}
		cell.selectionStyle = .none
		cell.scheduleConfParticipantAddress = participant
		cell.limeBadge.isHidden = viewModel.isEncrypted.value != true
		return cell
	}
	
	
}
