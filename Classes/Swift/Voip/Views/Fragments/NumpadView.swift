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
import linphonesw

@objc class NumpadView: UIView {
	
	// Layout constants
	let side_margins = 10.0
	let margin_top = 100.0
    let eneteredDtmf_size = 40.0
	let button_size = 70
	let button_vertical_space = 17.0
	let button_horizontal_space = 14.0
	let digit_icon_inset = UIEdgeInsets(top: 20, left: 20, bottom: 20, right: 20)
	let corner_radius = 20.0
	let pad_height = 550
	let side_padding = 50.0

	
	init(superView:UIView, callData:CallData, marginTop:CGFloat, above:UIView, onDismissAction : @escaping ()->Void) {
		super.init(frame:.zero)
		backgroundColor = VoipTheme.voip_translucent_popup_background
		layer.cornerRadius = corner_radius
		clipsToBounds = true
		superView.addSubview(self)
		matchParentSideBorders(insetedByDx: side_margins).alignParentTop(withMargin: marginTop).alignAbove(view: above,withMargin: SharedLayoutConstants.buttons_bottom_margin).done()

		callData.callState.observe { state in
			if (state == Call.State.End) {
                //never happens
				onDismissAction()
			}
		}
		
		// Hide numpad button
		let hide = CallControlButton(buttonTheme: VoipTheme.voip_cancel_light, onClickAction: {
			onDismissAction()
		})
		addSubview(hide)
		hide.alignParentRight(withMargin: side_margins).alignParentTop(withMargin: side_margins).done()
	
		// DTMF History :
		
		let eneteredDtmf = StyledLabel(VoipTheme.dtmf_label)
		addSubview(eneteredDtmf)
        eneteredDtmf.height(eneteredDtmf_size).matchParentSideBorders().alignUnder(view:hide,withMargin:side_margins).done()
		callData.enteredDTMF.readCurrentAndObserve { (dtmfs) in
			eneteredDtmf.text = dtmfs
		}
		
		// Digit buttons
		
		let allRows = UIStackView()
		allRows.axis = .vertical
		allRows.distribution = .equalSpacing
		allRows.alignment = .center
		allRows.spacing = button_vertical_space
		allRows.layoutMargins = UIEdgeInsets(top: 0, left: side_padding, bottom: 0, right: side_padding)
		allRows.isLayoutMarginsRelativeArrangement = true
		addSubview(allRows)
		_ = allRows.matchParentSideBorders().alignUnder(view:eneteredDtmf,withMargin: side_margins)
		
		
		for key in [["1","2","3"],["4","5","6"],["7","8","9"],["*","0","#"]] {
			let newRow = addRow(allRows: allRows)
			for subkey in key {
				let digit = CallControlButton(width:button_size, height:button_size, imageInset: digit_icon_inset, buttonTheme: ButtonTheme(tintableStateIcons:[UIButton.State.normal.rawValue : TintableIcon(name: "voip_numpad_\(iconNameForDigit(digit: subkey))")],backgroundStateColors:VoipTheme.numpad_digit_background), onClickAction: {
					callData.sendDTMF(dtmf: "\(subkey)")
				})
				newRow.addArrangedSubview(digit)
			}
		}
	}
	
	func iconNameForDigit(digit:String) -> String {
		if (digit == "*") {
			return "star"
		}
		if (digit == "#") {
			return "hash"
		}
		return digit
	}
	
	func addRow(allRows:UIStackView) -> UIStackView {
		let row = UIStackView()
		row.axis = .horizontal
		row.distribution = .equalSpacing
		row.alignment = .center
		row.spacing = button_vertical_space
		row.isLayoutMarginsRelativeArrangement = true
		allRows.addArrangedSubview(row)
		return row
	}
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
}



