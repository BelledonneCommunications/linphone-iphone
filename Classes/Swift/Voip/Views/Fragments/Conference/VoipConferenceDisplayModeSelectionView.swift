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

@objc class VoipConferenceDisplayModeSelectionView: DismissableView, UITableViewDataSource, UITableViewDelegate{
	
	// Layout constants
	let buttons_distance_from_center_x = 38
	let buttons_size = 60
	
	let optionsListView =  UITableView()
	
	init() {
		super.init(title: VoipTexts.call_action_change_conf_layout)
		
		super.contentView.addSubview(optionsListView)
		optionsListView.matchParentDimmensions().done()
		optionsListView.dataSource = self
		optionsListView.delegate = self
		optionsListView.register(ConferenceDisplayModeSelectionCell.self, forCellReuseIdentifier: "ConferenceDisplayModeSelectionCell")
		optionsListView.separatorStyle = .singleLine
		optionsListView.separatorColor = VoipTheme.light_grey_color
		optionsListView.isScrollEnabled = false
	}
	
	// TableView datasource delegate
	
	func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
		return 3
	}
	
	func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
		let cell:ConferenceDisplayModeSelectionCell = tableView.dequeueReusableCell(withIdentifier: "ConferenceDisplayModeSelectionCell") as! ConferenceDisplayModeSelectionCell
		cell.selectionStyle = .none
		if (indexPath.row == 0) {
			cell.setOption(title: VoipTexts.conference_display_mode_mosaic, onSelectAction:  {
				ConferenceViewModel.shared.changeLayout(layout: .Grid)
				ConferenceViewModel.shared.conferenceDisplayMode.value = .Grid
			}, image:(UIImage(named: "voip_conference_mosaic")?.tinted(with: VoipTheme.voipDrawableColor.get())!)!)
			cell.isUserInteractionEnabled = ConferenceViewModel.shared.conferenceParticipantDevices.value!.count <= ConferenceViewModel.shared.maxParticipantsForMosaicLayout
		}
		if (indexPath.row == 1) {
			cell.setOption(title: VoipTexts.conference_display_mode_active_speaker, onSelectAction:  {
				ConferenceViewModel.shared.changeLayout(layout: .ActiveSpeaker)
				ConferenceViewModel.shared.conferenceDisplayMode.value = .ActiveSpeaker
			}, image:(UIImage(named: "voip_conference_active_speaker")?.tinted(with: VoipTheme.voipDrawableColor.get())!)!)
			cell.isUserInteractionEnabled = true
		}
		
		if (indexPath.row == 2) {
			cell.setOption(title: VoipTexts.conference_display_mode_audio_only, onSelectAction:  {
				ConferenceViewModel.shared.changeLayout(layout: .AudioOnly)
				ConferenceViewModel.shared.conferenceDisplayMode.value = .AudioOnly
			}, image:(UIImage(named: "voip_conference_audio_only")?.tinted(with: VoipTheme.voipDrawableColor.get())!)!)
			cell.isUserInteractionEnabled = true
		}
	
		cell.separatorInset = .zero
		cell.selectionStyle = .none
		return cell
	}
	
	func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
		let cell = tableView.cellForRow(at: indexPath) as! ConferenceDisplayModeSelectionCell
		cell.onSelectAction?()
		cell.isSelected = true
		if (indexPath.row == 0) {
			tableView.deselectRow(at: IndexPath(row: 1, section: 0), animated: false)
			tableView.deselectRow(at: IndexPath(row: 2, section: 0), animated: false)
		}
		if (indexPath.row == 1) {
			tableView.deselectRow(at: IndexPath(row: 0, section: 0), animated: false)
			tableView.deselectRow(at: IndexPath(row: 2, section: 0), animated: false)
		}
		if (indexPath.row == 2) {
			tableView.deselectRow(at: IndexPath(row: 0, section: 0), animated: false)
			tableView.deselectRow(at: IndexPath(row: 1, section: 0), animated: false)
		}
	}
	
	func tableView(_ tableView: UITableView, didDeselectRowAt indexPath: IndexPath) {
		let cell = tableView.cellForRow(at: indexPath) as! ConferenceDisplayModeSelectionCell
		cell.isSelected = false
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
}

class ConferenceDisplayModeSelectionCell : UITableViewCell {
	
	let cell_height = 60.0
	let icon_size = 40.0
	let side_margins = 20.0
	
	let radio = CallControlButton(buttonTheme: VoipTheme.radio_button)
	let label = StyledLabel(VoipTheme.conference_mode_title)
	let icon = UIImageView()

	var onSelectAction : (()->Void)? = nil
	
	override var isSelected: Bool {
			didSet {
				radio.isSelected = isSelected
				label.applyStyle(isSelected ? VoipTheme.conference_mode_title_selected : VoipTheme.conference_mode_title)
			}
		}
	
	
	func setOption(title:String, onSelectAction:@escaping ()->Void, image:UIImage) {
		self.onSelectAction = onSelectAction
		label.text = title
		icon.image = image
	}
	
	override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
		super.init(style: style, reuseIdentifier: reuseIdentifier)
		contentView.height(cell_height).matchParentSideBorders().done()
		contentView.addSubview(radio)
		radio.alignParentLeft(withMargin: side_margins).centerY().done()
		contentView.addSubview(label)
		label.toRightOf(radio).centerY().done()
		contentView.addSubview(icon)
		icon.size(w: icon_size, h: icon_size).alignParentRight(withMargin: side_margins).centerY().done()
		radio.isUserInteractionEnabled = false
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
}
