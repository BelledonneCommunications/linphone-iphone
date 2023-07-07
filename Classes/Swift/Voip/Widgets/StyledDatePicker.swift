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
			if let liveValue = liveValue?.value {
				datePicker.date = liveValue
				self.valueChanged(datePicker: datePicker)
			} else {
				formattedLabel.text = nil
				datePicker.date = pickerMode == .date ? Date() : Calendar.current.startOfDay(for: Date())
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
		
		if #available(iOS 14.0, *) {
			addSubview(datePicker)
			datePicker.datePickerMode = pickerMode
			datePicker.addTarget(self, action: #selector(valueChanged), for: .valueChanged)
			datePicker.matchParentDimmensions().done()
			datePicker.preferredDatePickerStyle = .compact
			onClick {
				self.valueChanged(datePicker: self.datePicker)
			}
		} else {
			onClick {
				self.showPickerPreIOS14()
			}
		}
		
		formattedLabel.isUserInteractionEnabled = false
		addSubview(formattedLabel)
		formattedLabel.matchParentDimmensions().done()
		
		let chevron = UIImageView()
		
		
		addSubview(chevron)
		chevron.alignParentRight(withMargin: chevron_margin).centerY().done()
		chevron.isHidden = readOnly
		
		height(form_input_height).done()
		
		isUserInteractionEnabled = !readOnly
		self.liveValue = liveValue
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.setFormInputBackground(readOnly:readOnly)
			self.formattedLabel.backgroundColor = VoipTheme.voipFormBackgroundColor.get()
			chevron.image = UIImage(named: "chevron_list_close")?.tinted(with: VoipTheme.voipDrawableColor.get())
			if (readOnly) {
				self.formattedLabel.textColor = self.formattedLabel.textColor.withAlphaComponent(0.5)
			}
		}
		
	}
	
	
	@objc func valueChanged(datePicker: UIDatePicker) {
		liveValue!.value = datePicker.date
		formattedLabel.text = "  "+(pickerMode == .date ? TimestampUtils.dateToString(date: datePicker.date) : TimestampUtils.timeToString(date: datePicker.date))
	}
	
	
	func showPickerPreIOS14() {
		let alertController = UIAlertController(title: "", message: nil, preferredStyle: .alert)
		alertController.view.height(200).done()
		let picker = UIDatePicker()
		if #available(iOS 13.4, *) {
			picker.preferredDatePickerStyle = .wheels
		}
		liveValue?.value.map {picker.date = $0}
		picker.datePickerMode = pickerMode
		alertController.view.addSubview(picker)
		picker.matchParentDimmensions(insetedBy: UIEdgeInsets(top: 30, left: 50, bottom: 30, right: 50)).done()
		let ok = UIAlertAction(title: VoipTexts.ok, style: .default, handler: { (action) -> Void in
			self.valueChanged(datePicker: picker)
		})
		let cancel = UIAlertAction(title: VoipTexts.cancel, style: .cancel) { (action) -> Void in
		}
		
		alertController.addAction(cancel)
		alertController.addAction(ok)
		
		alertController.popoverPresentationController?.sourceView = PhoneMainView.instance().mainViewController.statusBarView
		PhoneMainView.instance().mainViewController.present(alertController, animated: true)
	}
	
}
