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

@objc class ConferenceSchedulingView:  BackNextNavigationView, UICompositeViewDelegate {
	
	
	static let compositeDescription = UICompositeViewDescription(ConferenceSchedulingView.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	let datePicker = StyledDatePicker(liveValue: ConferenceSchedulingViewModel.shared.scheduledDate,pickerMode: .date)
	let timeZoneValue = StyledValuePicker(liveIndex: ConferenceSchedulingViewModel.shared.scheduledTimeZone,options: ConferenceSchedulingViewModel.timeZones.map({ (tzd: TimeZoneData) -> String in tzd.descWithOffset()}))
	let durationValue = StyledValuePicker(liveIndex: ConferenceSchedulingViewModel.shared.scheduledDuration,options: ConferenceSchedulingViewModel.durationList.map({ (duration: Duration) -> String in duration.display}))
	let timePicker = StyledDatePicker(liveValue: ConferenceSchedulingViewModel.shared.scheduledTime,pickerMode: .time)
	let descriptionInput = StyledTextView(VoipTheme.conference_scheduling_font, placeHolder:VoipTexts.conference_schedule_description_hint,liveValue: ConferenceSchedulingViewModel.shared.description)

	
	override func viewDidLoad() {
		
		super.viewDidLoad(
			backAction: {
				PhoneMainView.instance().popView(self.compositeViewDescription())
			},nextAction: {
				self.gotoParticipantsListSelection()
			},
			nextActionEnableCondition: ConferenceSchedulingViewModel.shared.continueEnabled,
			title:VoipTexts.conference_schedule_title)
	
		let subjectLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_subject_title)
		subjectLabel.addIndicatorIcon(iconName: "voip_mandatory")
		contentView.addSubview(subjectLabel)
		subjectLabel.alignParentLeft(withMargin: form_margin).alignParentTop().done()
		
		let subjectInput = StyledTextView(VoipTheme.conference_scheduling_font, placeHolder:VoipTexts.conference_schedule_subject_hint, liveValue: ConferenceSchedulingViewModel.shared.subject,maxLines:1)
		contentView.addSubview(subjectInput)
		subjectInput.alignUnder(view: subjectLabel,withMargin: form_margin).matchParentSideBorders(insetedByDx: form_margin).height(form_input_height).done()
		
		let schedulingStack = UIStackView()
		schedulingStack.axis = .vertical
		schedulingStack.backgroundColor = VoipTheme.voipFormBackgroundColor.get()
		contentView.addSubview(schedulingStack)
		schedulingStack.alignUnder(view: subjectInput,withMargin: form_margin).matchParentSideBorders(insetedByDx: form_margin).done()
		
		let scheduleForLater = UIView()
		schedulingStack.addArrangedSubview(scheduleForLater)
		scheduleForLater.matchParentSideBorders().height(schdule_for_later_height).done()

		let laterSwitch = StyledSwitch(liveValue: ConferenceSchedulingViewModel.shared.scheduleForLater)
		scheduleForLater.addSubview(laterSwitch)
		laterSwitch.alignParentTop(withMargin: form_margin*2.5).alignParentLeft(withMargin: form_margin).centerY().done()
		
		let laterLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_later)
		laterLabel.numberOfLines = 2
		scheduleForLater.addSubview(laterLabel)
		laterLabel.alignParentTop(withMargin: form_margin*2).toRightOf(laterSwitch, withLeftMargin: form_margin*2).alignParentRight(withMargin: form_margin).done()

		let scheduleForm = UIView()
		schedulingStack.addArrangedSubview(scheduleForm)
		scheduleForm.matchParentSideBorders().done()
		ConferenceSchedulingViewModel.shared.scheduleForLater.readCurrentAndObserve { (forLater) in scheduleForm.isHidden = forLater != true }
		
		// Left column (Date & Time)
		let leftColumn = UIView()
		scheduleForm.addSubview(leftColumn)
		leftColumn.matchParentWidthDividedBy(2.2).alignParentLeft(withMargin: form_margin).alignParentTop(withMargin: form_margin).done()
		
		let dateLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_date)
		dateLabel.addIndicatorIcon(iconName: "voip_mandatory")
		leftColumn.addSubview(dateLabel)
		dateLabel.alignParentLeft().alignParentTop(withMargin: form_margin).done()
		
		leftColumn.addSubview(datePicker)
		datePicker.alignParentLeft().alignUnder(view: dateLabel,withMargin: form_margin).matchParentSideBorders().done()
		
		let timeLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_time)
		timeLabel.addIndicatorIcon(iconName: "voip_mandatory")
		leftColumn.addSubview(timeLabel)
		timeLabel.alignParentLeft().alignUnder(view: datePicker,withMargin: form_margin).done()
		
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
		
		rightColumn.addSubview(durationValue)
		durationValue.alignParentLeft().alignUnder(view: durationLabel,withMargin: form_margin).matchParentSideBorders().done()
		
		let timeZoneLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_timezone)
		rightColumn.addSubview(timeZoneLabel)
		timeZoneLabel.alignParentLeft().alignUnder(view: durationValue,withMargin: form_margin).done()
		
		rightColumn.addSubview(timeZoneValue)
		timeZoneValue.alignParentLeft().alignUnder(view: timeZoneLabel,withMargin: form_margin).matchParentSideBorders().done()
	
		rightColumn.wrapContentY().done()
		
		// Description
		let descriptionLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_description_title)
		scheduleForm.addSubview(descriptionLabel)
		descriptionLabel.alignUnder(view: leftColumn,withMargin: form_margin).alignUnder(view: rightColumn,withMargin: form_margin).matchParentSideBorders(insetedByDx: form_margin).done()
		
		descriptionInput.textContainer.maximumNumberOfLines = 5
		descriptionInput.textContainer.lineBreakMode = .byWordWrapping
		scheduleForm.addSubview(descriptionInput)
		descriptionInput.alignUnder(view: descriptionLabel,withMargin: form_margin).matchParentSideBorders(insetedByDx: form_margin).height(description_height).alignParentBottom(withMargin: form_margin*2).done()

		scheduleForm.wrapContentY().done()
		
		// Sending methods
		let viaChatSwitch = StyledCheckBox(liveValue: ConferenceSchedulingViewModel.shared.sendInviteViaChat)
		contentView.addSubview(viaChatSwitch)
		viaChatSwitch.alignParentLeft(withMargin: form_margin).alignUnder(view: schedulingStack,withMargin: 2*form_margin).done()
		
		let viaChatLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_send_invite_chat)
		contentView.addSubview(viaChatLabel)
		viaChatLabel.toRightOf(viaChatSwitch,withLeftMargin: form_margin).alignUnder(view: schedulingStack,withMargin: 2*form_margin).alignHorizontalCenterWith(viaChatSwitch).done()
		
		let viaMailSwitch = StyledCheckBox(liveValue: ConferenceSchedulingViewModel.shared.sendInviteViaEmail)
		contentView.addSubview(viaMailSwitch)
		viaMailSwitch.alignParentLeft(withMargin: form_margin).alignUnder(view: viaChatSwitch,withMargin: 2*form_margin).done()
		
		let viaMailLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_send_invite_email)
		contentView.addSubview(viaMailLabel)
		viaMailLabel.toRightOf(viaMailSwitch,withLeftMargin: form_margin).alignUnder(view: viaChatLabel,withMargin: 2*form_margin).alignHorizontalCenterWith(viaMailSwitch).done()

		
		// Encryption
		let encryptLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_encryption)
		contentView.addSubview(encryptLabel)
		encryptLabel.alignUnder(view: viaMailLabel,withMargin: 4*form_margin).centerX().done()
		
		let encryptCombo = UIStackView()
		contentView.addSubview(encryptCombo)
		encryptCombo.alignUnder(view: encryptLabel,withMargin: form_margin).centerX().height(form_input_height).done()
		
		let unencryptedIcon = UIImageView(image: UIImage(named: "security_toggle_icon_grey"))
		unencryptedIcon.contentMode = .scaleAspectFit
		encryptCombo.addArrangedSubview(unencryptedIcon)

		let encryptSwitch = StyledSwitch(liveValue: ConferenceSchedulingViewModel.shared.isEncrypted)
		encryptCombo.addArrangedSubview(encryptSwitch)
		encryptSwitch.centerY().alignParentTop(withMargin: form_margin).done()
		
		let encryptedIcon = UIImageView(image: UIImage(named: "security_toggle_icon_green"))
		encryptedIcon.contentMode = .scaleAspectFit
		encryptCombo.addArrangedSubview(encryptedIcon)
				
		
		// Mandatory label
		
		let mandatoryLabel = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_mandatory_field)
		mandatoryLabel.addIndicatorIcon(iconName: "voip_mandatory", trailing: false)
		contentView.addSubview(mandatoryLabel)
		mandatoryLabel.alignUnder(view: encryptCombo,withMargin: 4*form_margin).centerX().matchParentSideBorders().done()
		mandatoryLabel.textAlignment = .center
		
		mandatoryLabel.alignParentBottom().done()
						
	}
	
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		datePicker.liveValue = ConferenceSchedulingViewModel.shared.scheduledDate
		timeZoneValue.setIndex(index: ConferenceSchedulingViewModel.shared.scheduledTimeZone.value!)
		durationValue.setIndex(index: ConferenceSchedulingViewModel.shared.scheduledDuration.value!)
		timePicker.liveValue = ConferenceSchedulingViewModel.shared.scheduledTime
		descriptionInput.text = ConferenceSchedulingViewModel.shared.description.value
	}
	
	func gotoParticipantsListSelection() {
		let view: ChatConversationCreateView = self.VIEW(ChatConversationCreateView.compositeViewDescription());
		let addresses =  ConferenceSchedulingViewModel.shared.selectedAddresses.value!.map { (address) in String(address.asStringUriOnly()) }
		view.tableController.contactsGroup = (addresses as NSArray).mutableCopy() as? NSMutableArray
		view.isForEditing = false
		view.isForVoipConference = true
		view.isForOngoingVoipConference = false
		view.tableController.notFirstTime = true
		view.isGroupChat = true
		PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
	}
	
	@objc func resetViewModel() {
		ConferenceSchedulingViewModel.shared.reset()
	}
}
