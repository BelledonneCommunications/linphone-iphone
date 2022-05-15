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
	static let button_size = 40
	
	let clockIcon = UIImageView(image: UIImage(named: "conference_schedule_time_default"))
	let timeDuration = StyledLabel(VoipTheme.conference_invite_desc_font)
	let organiser = StyledLabel(VoipTheme.conference_invite_desc_font)
	let subject = StyledLabel(VoipTheme.conference_invite_subject_font)
	let participantsIcon = UIImageView(image: UIImage(named: "conference_schedule_participants_default"))
	let participants = StyledLabel(VoipTheme.conference_invite_desc_font)
	let infoConf = UIButton()
	
	let descriptionTitle = StyledLabel(VoipTheme.conference_invite_desc_font, VoipTexts.conference_description_title)
	let descriptionValue = StyledLabel(VoipTheme.conference_invite_desc_font)
	let urlTitle = StyledLabel(VoipTheme.conference_invite_desc_font, VoipTexts.conference_schedule_address_title)
	let urlValue = StyledLabel(VoipTheme.conference_scheduling_font)
	let copyLink  = CallControlButton(width:button_size,height:button_size,buttonTheme: VoipTheme.scheduled_conference_action("voip_copy"))
	let joinConf = FormButton(title:VoipTexts.conference_invite_join.uppercased(), backgroundStateColors: VoipTheme.button_green_background)
	let deleteConf = CallControlButton(width:button_size,height:button_size,buttonTheme: VoipTheme.scheduled_conference_action("voip_delete"))
	let editConf = CallControlButton(width:button_size,height:button_size,buttonTheme: VoipTheme.scheduled_conference_action("voip_edit"))
	var owningTableView : UITableView? = nil
	let joinEditDelete = UIStackView()
	let expandedRows = UIStackView()

	var conferenceData: ScheduledConferenceData? = nil {
		didSet {
			if let data = conferenceData {
				timeDuration.text = "\(data.time.value)"+(data.duration.value != nil ? " ( \(data.duration.value) )" : "")
				organiser.text = VoipTexts.conference_schedule_organizer+data.organizer.value!
				subject.text = data.subject.value!
				descriptionValue.text = data.description.value!
				urlValue.text = data.address.value!
				data.expanded.readCurrentAndObserve { expanded in
					self.contentView.layer.borderWidth = expanded == true ? 2.0 : 0.0
					self.descriptionTitle.isHidden = expanded != true || self.descriptionValue.text?.count == 0
					self.descriptionValue.isHidden = expanded != true  || self.descriptionValue.text?.count == 0
					self.infoConf.isSelected = expanded == true
					self.participants.text = expanded == true ? data.participantsExpanded.value : data.participantsShort.value
					self.participants.numberOfLines = expanded == true ? 6 : 2
					self.expandedRows.isHidden = expanded != true
					self.joinEditDelete.isHidden = expanded != true
					if let myAddress = Core.get().defaultAccount?.params?.identityAddress {
						self.editConf.isHidden = expanded != true || data.conferenceInfo.organizer?.weakEqual(address2: myAddress) != true
					} else {
						self.editConf.isHidden = true
					}
					self.participants.removeConstraints().alignUnder(view: self.subject,withMargin: 15).toRightOf(self.participantsIcon,withLeftMargin:10).toRightOf(self.participantsIcon,withLeftMargin:10).toLeftOf(self.infoConf,withRightMargin: 15).done()
					self.joinEditDelete.removeConstraints().alignUnder(view: self.expandedRows,withMargin: 15).alignParentRight(withMargin: 10).done()
					if (expanded == true) {
						self.joinEditDelete.alignParentBottom(withMargin: 10).done()
					} else {
						self.participants.alignParentBottom(withMargin: 10).done()
					}

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
		
		
		contentView.addSubview(clockIcon)
		clockIcon.alignParentTop(withMargin: 15).square(15).alignParentLeft(withMargin: 10).done()

		contentView.addSubview(timeDuration)
		timeDuration.alignParentTop(withMargin: 15).toRightOf(clockIcon,withLeftMargin:10).alignHorizontalCenterWith(clockIcon).done()
		
		contentView.addSubview(organiser)
		organiser.alignParentTop(withMargin: 15).toRightOf(timeDuration, withLeftMargin:10).alignParentRight(withMargin:10).alignHorizontalCenterWith(clockIcon).done()
		
		contentView.addSubview(subject)
		subject.alignUnder(view: timeDuration,withMargin: 15).alignParentLeft(withMargin: 10).done()
		
		contentView.addSubview(participantsIcon)
		participantsIcon.alignUnder(view: subject,withMargin: 15).square(15).alignParentLeft(withMargin: 10).done()
		
		//infoConf.onClick {
		contentView.onClick {
			self.conferenceData?.toggleExpand()
			self.owningTableView?.reloadData()
		}
		contentView.addSubview(infoConf)
		infoConf.imageView?.contentMode = .scaleAspectFit
		infoConf.alignUnder(view: subject,withMargin: 15).square(30).alignParentRight(withMargin: 10).alignHorizontalCenterWith(participantsIcon).done()
		infoConf.applyTintedIcons(tintedIcons: VoipTheme.conference_info_button)

				
		contentView.addSubview(participants)
		participants.alignUnder(view: subject,withMargin: 15).toRightOf(participantsIcon,withLeftMargin:10).toRightOf(participantsIcon,withLeftMargin:10).toLeftOf(infoConf,withRightMargin: 15).done()
		
		expandedRows.axis = .vertical
		expandedRows.spacing = 10
		contentView.addSubview(expandedRows)
		expandedRows.alignUnder(view: participants,withMargin: 15).matchParentSideBorders(insetedByDx:10).done()

		expandedRows.addArrangedSubview(descriptionTitle)
		expandedRows.addArrangedSubview(descriptionValue)
		
		expandedRows.addArrangedSubview(urlTitle)
		let urlAndCopy = UIStackView()
		urlAndCopy.addArrangedSubview(urlValue)
		urlValue.backgroundColor = .white
		self.urlValue.isEnabled = false
		urlValue.alignParentLeft().done()
		urlAndCopy.addArrangedSubview(copyLink)
		copyLink.toRightOf(urlValue,withLeftMargin: 10).done()
		expandedRows.addArrangedSubview(urlAndCopy)
		copyLink.onClick {
			UIPasteboard.general.string = self.conferenceData?.address.value!
			VoipDialog.toast(message: VoipTexts.conference_schedule_address_copied_to_clipboard)
		}
		
		joinEditDelete.axis = .horizontal
		joinEditDelete.spacing = 10
		joinEditDelete.distribution = .equalSpacing

		contentView.addSubview(joinEditDelete)
		joinEditDelete.alignUnder(view: expandedRows,withMargin: 15).alignParentRight(withMargin: 10).done()

		
		joinEditDelete.addArrangedSubview(joinConf)
		joinConf.width(150).done()
		joinConf.onClick {
			let view : ConferenceWaitingRoomFragment = self.VIEW(ConferenceWaitingRoomFragment.compositeViewDescription())
			PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
			view.setDetails(subject: (self.conferenceData?.subject.value)!, url: (self.conferenceData?.address.value)!)
		}
		
		joinEditDelete.addArrangedSubview(editConf)
		editConf.onClick {
			// TODO
			VoipDialog.toast(message: "not available yet")
		}
		
		joinEditDelete.addArrangedSubview(deleteConf)
		deleteConf.onClick {
			let delete = ButtonAttributes(text:VoipTexts.conference_info_confirm_removal_delete, action: {
				Core.get().deleteConferenceInformation(conferenceInfo: self.conferenceData!.conferenceInfo)
				ScheduledConferencesViewModel.shared.computeConferenceInfoList()
				self.owningTableView?.reloadData()
			}, isDestructive:false)
			let cancel = ButtonAttributes(text:VoipTexts.cancel, action: {}, isDestructive:true)
			VoipDialog(message:VoipTexts.conference_info_confirm_removal, givenButtons:  [cancel,delete]).show()
		}
				

	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
}
