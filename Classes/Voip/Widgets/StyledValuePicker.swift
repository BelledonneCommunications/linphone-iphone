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

import Foundation
import UIKit
import DropDown


class StyledValuePicker: UIView {
	
	// layout constants
	let chevron_margin = 10.0
	let form_input_height = 38.0

	
	let formattedLabel = StyledLabel(VoipTheme.conference_scheduling_font)
	var pickerMode:UIDatePicker.Mode = .date
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
	init (liveIndex:MutableLiveData<Int>, options:[String], readOnly:Bool = false) {
		super.init(frame: .zero)

		formattedLabel.isUserInteractionEnabled = false
		formattedLabel.backgroundColor = VoipTheme.voipFormBackgroundColor.get()
		formattedLabel.text =  "  "+options[liveIndex.value!]
		
		if (readOnly) {
			formattedLabel.textColor = formattedLabel.textColor.withAlphaComponent(0.5)
		}
		
		addSubview(formattedLabel)
		formattedLabel.alignParentLeft().alignParentRight(withMargin: (readOnly ? chevron_margin : form_input_height)).matchParentHeight().done()
		
		let chevron = UIImageView(image: UIImage(named: "chevron_list_close"))
		addSubview(chevron)
		chevron.alignParentRight(withMargin: chevron_margin).centerY().done()
		chevron.isHidden = readOnly

		setFormInputBackground(readOnly:readOnly)

		
		DropDown.appearance().textColor = VoipTheme.conference_scheduling_font.fgColor.get()
		DropDown.appearance().selectedTextColor = VoipTheme.conference_scheduling_font.fgColor.get()
		DropDown.appearance().textFont = formattedLabel.font
		DropDown.appearance().backgroundColor = .white
		DropDown.appearance().selectionBackgroundColor = VoipTheme.light_grey_color
		DropDown.appearance().cellHeight = form_input_height
		
		let dropDown = DropDown()
		dropDown.anchorView = self
		dropDown.bottomOffset = CGPoint(x: 0, y:(dropDown.anchorView?.plainView.bounds.height)!)
		dropDown.dataSource = options
		dropDown.backgroundColor = .white
		
		dropDown.selectionAction = { [unowned self] (index: Int, item: String) in
			liveIndex.value = index
			dropDown.selectRow(at: index)
			//dropDown.tableView.scrollToRow(at: IndexPath(row: index, section: 0), at: .middle, animated: true)
			formattedLabel.text = "  "+options[liveIndex.value!]
			dropDown.hide()
		}
	
		onClick {
			dropDown.show()
		}
		
		height(form_input_height).done()
		
		liveIndex.readCurrentAndObserve { (value) in
			dropDown.selectRow(value!)
		}
		isUserInteractionEnabled = !readOnly

		
		
   }
	
	


}
