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
import EventKit
import EventKitUI

@objc class ICSBubbleView: UIView, EKEventEditViewDelegate {
	
	
	let corner_radius  = 7.0
	let border_width = 2.0
	let rows_spacing = 6.0
	let inner_padding = 8.0
	let indicator_y = 3.0
	let share_size = 25
	let join_share_width = 150.0
	
	
	let inviteTitle = StyledLabel(VoipTheme.conference_invite_title_font, VoipTexts.conference_invite_title)
	let subject = StyledLabel(VoipTheme.conference_invite_subject_font)
	let participants = StyledLabel(VoipTheme.conference_invite_desc_font)
	let date = StyledLabel(VoipTheme.conference_invite_desc_font)
	let timeDuration = StyledLabel(VoipTheme.conference_invite_desc_font)
	let descriptionTitle = StyledLabel(VoipTheme.conference_invite_desc_title_font, VoipTexts.conference_description_title)
	let descriptionValue = StyledLabel(VoipTheme.conference_invite_desc_font)
	let joinShare = UIStackView()
	let join = FormButton(title:VoipTexts.conference_invite_join.uppercased(), backgroundStateColors: VoipTheme.button_green_background)
	let share = UIImageView(image:UIImage(named:"voip_export")?.tinted(with: VoipTheme.primaryTextColor.get()))
	
	
	var conferenceData: ScheduledConferenceData? = nil {
		didSet {
			if let data = conferenceData {
				subject.text = data.subject.value
				participants.text = VoipTexts.conference_invite_participants_count.replacingOccurrences(of: "%d", with: String(data.conferenceInfo.participants.count+1))
				participants.addIndicatorIcon(iconName: "conference_schedule_participants_default",padding : 0.0, y: -indicator_y, trailing: false)
				date.text = " "+TimestampUtils.dateToString(date: data.rawDate)
				date.addIndicatorIcon(iconName: "conference_schedule_calendar_default", padding: 0.0, y:-indicator_y, trailing:false)
				timeDuration.text = " \(data.time.value)" + (data.duration.value != nil ? " ( \(data.duration.value) )" : "")
				timeDuration.addIndicatorIcon(iconName: "conference_schedule_time_default",padding : 0.0, y: -indicator_y, trailing: false)
				descriptionTitle.isHidden = true // data.description.value == nil || data.description.value!.count == 0
				descriptionValue.isHidden = true // descriptionTitle.isHidden
				descriptionValue.text = data.description.value
			}
		}
	}
	
	init() {
		super.init(frame:.zero)
		
		layer.cornerRadius = corner_radius
		clipsToBounds = true
		backgroundColor = VoipTheme.voip_light_gray
		
		let rows = UIStackView()
		rows.axis = .vertical
		rows.spacing = rows_spacing
		
		addSubview(rows)
		
		rows.addArrangedSubview(inviteTitle)
		rows.addArrangedSubview(subject)
		rows.addArrangedSubview(participants)
		rows.addArrangedSubview(date)
		rows.addArrangedSubview(timeDuration)
		rows.addArrangedSubview(descriptionTitle)
		rows.addArrangedSubview(descriptionValue)
		
		
		addSubview(joinShare)
		joinShare.axis = .horizontal
		joinShare.spacing = rows_spacing
		joinShare.addArrangedSubview(share)
		share.square(share_size).done()
		joinShare.addArrangedSubview(join)
		rows.matchParentSideBorders(insetedByDx: inner_padding).alignParentTop(withMargin: inner_padding).done()
		joinShare.alignParentBottom(withMargin: inner_padding).width(join_share_width).alignParentRight(withMargin: inner_padding).done()
		
		join.onClick {
			let view : ConferenceWaitingRoomFragment = self.VIEW(ConferenceWaitingRoomFragment.compositeViewDescription())
			PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
			view.setDetails(subject: (self.conferenceData?.subject.value)!, url: (self.conferenceData?.address.value)!)
		}
		
		share.onClick {
			let eventStore = EKEventStore()
			eventStore.requestAccess( to: EKEntityType.event, completion:{(granted, error) in
				DispatchQueue.main.async {
					if (granted) && (error == nil) {
						let event = EKEvent(eventStore: eventStore)
						event.title = self.conferenceData?.subject.value
						event.startDate = self.conferenceData?.rawDate
						if let duration = self.conferenceData?.conferenceInfo.duration, duration > 0 {
							event.endDate = event.startDate.addingTimeInterval(TimeInterval(duration*60))
						} else {
							event.endDate = event.startDate.addingTimeInterval(TimeInterval(3600))
						}
						event.calendar = eventStore.defaultCalendarForNewEvents
						if let description = self.conferenceData?.description.value, description.count > 0 {
							event.notes = description + "\n\n"
						}
						event.notes = (event.notes != nil ? event.notes! : "") + "\(VoipTexts.call_action_participants_list):\n\(self.conferenceData?.participantsExpanded.value)"
						if let urlString = self.conferenceData?.conferenceInfo.uri?.asStringUriOnly() {
							event.url = URL(string:urlString)
						}
						let addController = EKEventEditViewController()
						addController.event = event
						addController.eventStore = eventStore
						PhoneMainView.instance().present(addController, animated: false)
						addController.editViewDelegate = self;
					} else {
						VoipDialog.toast(message: VoipTexts.conference_unable_to_share_via_calendar)
					}
				}
			})
		}
	}
	
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	@objc func setFromChatMessage(cmessage: OpaquePointer) {
		let message = ChatMessage.getSwiftObject(cObject: cmessage)
		message.contents.forEach { content in
			if (content.isIcalendar) {
				if let conferenceInfo = try? Factory.Instance.createConferenceInfoFromIcalendarContent(content: content) {
					self.conferenceData = ScheduledConferenceData(conferenceInfo: conferenceInfo)
				}
			}
		}
	}
	@objc static func isConferenceInvitationMessage(cmessage: OpaquePointer) -> Bool {
		var isConferenceInvitationMessage = false
		let message = ChatMessage.getSwiftObject(cObject: cmessage)
		message.contents.forEach { content in
			if (content.isIcalendar) {
				isConferenceInvitationMessage = true
			}
		}
		return isConferenceInvitationMessage
	}
	
	@objc func setLayoutConstraints(view:UIView) {
		matchDimensionsWith(view: view, insetedByDx: inner_padding).done()
	}
	
	func eventEditViewController(_ controller: EKEventEditViewController, didCompleteWith action: EKEventEditViewAction) {
		controller.dismiss(animated: true, completion: nil)
	}
	
	@objc static func getSubjectFromContent(cmessage: OpaquePointer) -> String {
		let message = ChatMessage.getSwiftObject(cObject: cmessage)
		var subject = ""
		message.contents.forEach { content in
			if (content.isIcalendar) {
				if let conferenceInfo = try? Factory.Instance.createConferenceInfoFromIcalendarContent(content: content) {
					subject = conferenceInfo.subject
				}
			}
		}
		return subject
	}
	
}
