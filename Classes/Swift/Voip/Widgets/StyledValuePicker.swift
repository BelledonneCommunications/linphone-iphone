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
	let dropdown_width = 250.0
	let dropDown = DropDown()

	
	let formattedLabel = StyledLabel(VoipTheme.conference_scheduling_font)
	var pickerMode:UIDatePicker.Mode = .date
	var options : [String]
	
	required init?(coder: NSCoder) {
		self.options = []
		super.init(coder: coder)
	}
	
	init (liveIndex:MutableLiveData<Int>, options:[String], readOnly:Bool = false) {
		self.options = options
		super.init(frame: .zero)

		formattedLabel.isUserInteractionEnabled = false
		liveIndex.value.map { formattedLabel.text =  "  "+options[$0] }

		if (readOnly) {
			formattedLabel.textColor = formattedLabel.textColor.withAlphaComponent(0.5)
		}
		
		addSubview(formattedLabel)
		formattedLabel.alignParentLeft().alignParentRight(withMargin: (readOnly ? chevron_margin : form_input_height)).matchParentHeight().done()
		
		let chevron = UIImageView()
		
		addSubview(chevron)
		chevron.alignParentRight(withMargin: chevron_margin).centerY().done()
		chevron.isHidden = readOnly

		
		
		DropDown.appearance().textFont = formattedLabel.font
		DropDown.appearance().cellHeight = form_input_height
		
		dropDown.anchorView = self
		dropDown.bottomOffset = CGPoint(x: 0, y:(dropDown.anchorView?.plainView.bounds.height)!)
		dropDown.dataSource = options
		dropDown.width = dropdown_width
		
		dropDown.selectionAction = { [unowned self] (index: Int, item: String) in
			liveIndex.value = index
			dropDown.selectRow(at: index)
			formattedLabel.text = "  "+options[liveIndex.value!]
			dropDown.hide()
		}

		onClick {
			self.dropDown.anchorView = self.superview
			self.dropDown.tableView.scrollToRow(at: IndexPath(row: liveIndex.value!, section: 0), at: .top, animated: true) // Change visibility to public instead of fileprivate in DropDown.swift
			self.dropDown.show()
		}
		
		height(form_input_height).done()
		
		liveIndex.readCurrentAndObserve { (value) in
			self.dropDown.selectRow(value!)
		}
		isUserInteractionEnabled = !readOnly
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			chevron.image = UIImage(named: "chevron_list_close")?.tinted(with: VoipTheme.voipDrawableColor.get())
			self.formattedLabel.backgroundColor = VoipTheme.voipFormBackgroundColor.get()
			DropDown.appearance().textColor = VoipTheme.conference_scheduling_font.fgColor.get()
			DropDown.appearance().selectedTextColor = VoipTheme.conference_scheduling_font.fgColor.get()
			DropDown.appearance().backgroundColor = VoipTheme.voipFormBackgroundColor.get()
			DropDown.appearance().selectionBackgroundColor = VoipTheme.backgroundWhiteBlack.get()
			self.dropDown.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
			self.setFormInputBackground(readOnly:readOnly)
		}
		
   }
	
	func setIndex(index: Int) {
		self.dropDown.selectRow(index)
		formattedLabel.text = "  "+options[index]
	}

}
