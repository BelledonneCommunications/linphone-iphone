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

class StyledDatePicker: UIView {
	
	// layout constants
	let chevron_margin = 10
	let form_input_height = 38.0

	let datePicker = UIDatePicker()

	var liveValue:MutableLiveData<Date>? {
		didSet {
			if let liveValue = liveValue {
				datePicker.date = liveValue.value!
				self.valueChanged(datePicker: datePicker)
			}
		}
		
	}
	let formattedLabel = StyledLabel(VoipTheme.conference_scheduling_font)
	var pickerMode:UIDatePicker.Mode = .date
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
	init (liveValue:MutableLiveData<Date>? = nil, pickerMode:UIDatePicker.Mode, readOnly:Bool = false) {
		super.init(frame: .zero)
		self.pickerMode = pickerMode

		addSubview(datePicker)
		datePicker.datePickerMode = pickerMode
		datePicker.addTarget(self, action: #selector(valueChanged), for: .valueChanged)
		datePicker.matchParentDimmensions().done()
		
		formattedLabel.isUserInteractionEnabled = false
		formattedLabel.backgroundColor = VoipTheme.voipFormBackgroundColor.get()
		addSubview(formattedLabel)
		formattedLabel.matchParentDimmensions().done()
		
		let chevron = UIImageView(image: UIImage(named: "chevron_list_close"))
		addSubview(chevron)
		chevron.alignParentRight(withMargin: chevron_margin).centerY().done()
		chevron.isHidden = readOnly
		
		setFormInputBackground(readOnly:readOnly)
		height(form_input_height).done()
	
		if (readOnly) {
			formattedLabel.textColor = formattedLabel.textColor.withAlphaComponent(0.5)
		}
		isUserInteractionEnabled = !readOnly
		self.liveValue = liveValue

   }

	
	@objc func valueChanged(datePicker: UIDatePicker) {
		liveValue!.value = datePicker.date
		formattedLabel.text = "  "+(pickerMode == .date ? TimestampUtils.dateToString(date: datePicker.date) : TimestampUtils.timeToString(date: datePicker.date))
	}
}
