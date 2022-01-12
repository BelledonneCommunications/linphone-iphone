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

class ScheduledConferencesCell: UITableViewCell {
	
	let corner_radius  = 7.0
	let border_width = 2.0
	
	let timeDuration = StyledLabel(VoipTheme.conference_invite_desc_font)
	let organiser = StyledLabel(VoipTheme.conference_invite_desc_font)
	let subject = StyledLabel(VoipTheme.conference_invite_subject_font)
	let participants = StyledLabel(VoipTheme.conference_invite_desc_font)
	let infoConf = UIButton()
	
	let descriptionTitle = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_description_title)
	let descriptionValue = StyledLabel(VoipTheme.conference_scheduling_font)
	let urlTitle = StyledLabel(VoipTheme.conference_scheduling_font, VoipTexts.conference_schedule_address_title)
	let urlAndCopy = UIView()
	let urlValue = StyledLabel(VoipTheme.conference_scheduling_font)
	let copyLink  = CallControlButton(buttonTheme: VoipTheme.scheduled_conference_action("voip_copy"))
	let joinEditDelete = UIView()
	let joinConf = FormButton(title:VoipTexts.conference_invite_join.uppercased(), backgroundStateColors: VoipTheme.button_green_background)
	let deleteConf = CallControlButton(buttonTheme: VoipTheme.scheduled_conference_action("voip_delete"))
	let editConf = CallControlButton(buttonTheme: VoipTheme.scheduled_conference_action("voip_edit"))
	
	var conferenceData: ScheduledConferenceData? = nil {
		didSet {
			if let data = conferenceData {
				timeDuration.text = "\(data.time) ( \(data.duration) )"
				timeDuration.addIndicatorIcon(iconName: "conference_schedule_time_default", trailing: false)
				organiser.text = VoipTexts.conference_schedule_organizer+data.organizer.value!
				subject.text = data.subject.value!
				descriptionValue.text = data.description.value!
				urlValue.text = data.address.value!
				data.expanded.readCurrentAndObserve { expanded in
					self.contentView.layer.borderWidth = expanded == true ? 2.0 : 0.0
					self.descriptionTitle.isHidden = expanded != true
					self.descriptionValue.isHidden = expanded != true
					self.urlAndCopy.isHidden = expanded != true
					self.joinEditDelete.isHidden = expanded != true
					self.infoConf.isSelected = expanded == true
					self.participants.text = expanded == true ? data.participantsExpanded.value : data.participantsShort.value
					self.participants.addIndicatorIcon(iconName: "conference_schedule_participants_default", trailing: false)
				}
			}
		}
	}

	override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
		super.init(style: style, reuseIdentifier: reuseIdentifier)
		
		contentView.layer.cornerRadius = corner_radius
		contentView.clipsToBounds = true
		contentView.backgroundColor = VoipTheme.header_background_color
		contentView.layer.borderColor = VoipTheme.primary_color.cgColor
		
		let rows = UIStackView()
		rows.axis = .vertical
		rows.addArrangedSubview(timeDuration)
		rows.addArrangedSubview(subject)

		let participantsAndInfos = UIView()
		participantsAndInfos.addSubview(participants)
		participants.alignParentLeft().done()
		participantsAndInfos.addSubview(infoConf)
		infoConf.toRightOf(participants).done()
		rows.addArrangedSubview(participantsAndInfos)
		infoConf.applyTintedIcons(tintedIcons: VoipTheme.conference_info_button)
		infoConf.onClick {
			self.conferenceData?.toggleExpand()
		}
		
		rows.addArrangedSubview(descriptionTitle)
		rows.addArrangedSubview(descriptionValue)

		rows.addArrangedSubview(urlTitle)
		urlAndCopy.addSubview(urlValue)
		urlValue.backgroundColor = .white
		urlValue.alignParentLeft().done()
		urlAndCopy.addSubview(copyLink)
		copyLink.toLeftOf(urlValue).done()
		rows.addArrangedSubview(urlAndCopy)

		joinEditDelete.addSubview(joinConf)
		joinEditDelete.addSubview(editConf)
		joinEditDelete.addSubview(deleteConf)
		deleteConf.alignParentRight().done()
		editConf.toLeftOf(deleteConf).done()
		joinConf.toLeftOf(deleteConf).done()
		
		joinConf.onClick {
			/*
			 ConferenceWaitingRoomFragment *view = VIEW(ConferenceWaitingRoomFragment);
			 [PhoneMainView.instance changeCurrentView:ConferenceWaitingRoomFragment.compositeViewDescription];
			 [view setDetailsWithSubject:@"Sujet de la conférence" url:@"toto"];
			 return;			 
			 */
		}
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
}
