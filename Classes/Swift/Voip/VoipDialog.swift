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

class VoipDialog : UIView{
	
	// Layout constants
	let center_corner_radius = 7.0
	let title_margin_top = 20
	let title_margin_sides = 10.0
	let button_margin = 20.0
	let button_width = 135.0
	let button_height = 40.0
	let button_radius = 3.0
	let button_spacing = 15.0
	let center_view_sides_margin = 13.0

	
	let title = StyledLabel(VoipTheme.basic_popup_title)

	init(message:String, givenButtons:[ButtonAttributes]? = nil) {
		
		super.init(frame: .zero)
		backgroundColor = VoipTheme.voip_translucent_popup_background
		
		let centerView = UIView()
		centerView.backgroundColor = VoipTheme.dark_grey_color.withAlphaComponent(0.8)
		centerView.layer.cornerRadius = center_corner_radius
		centerView.clipsToBounds = true
		addSubview(centerView)
		
		title.numberOfLines = 0
		centerView.addSubview(title)
		title.alignParentTop(withMargin:title_margin_top).matchParentSideBorders(insetedByDx: title_margin_sides).done()
		title.text = message
		
		let buttonsView = UIStackView()
		buttonsView.axis = .horizontal
		buttonsView.spacing = button_spacing
		
		var buttons = givenButtons
		
		if (buttons == nil) { // assuming info popup, just putting an ok button
			let ok = ButtonAttributes(text:VoipTexts.ok, action: {}, isDestructive:false)
			buttons = [ok]
		}
		
		buttons?.forEach {
			let b = ButtonWithStateBackgrounds(backgroundStateColors: $0.isDestructive ? VoipTheme.primary_colors_background_gray : VoipTheme.primary_colors_background)
			b.setTitle($0.text, for: .normal)
			b.layer.cornerRadius = button_radius
			b.clipsToBounds = true
			buttonsView.addArrangedSubview(b)
			b.applyTitleStyle(VoipTheme.form_button_bold)
			let action = $0.action
			b.onClick {
				self.removeFromSuperview()
				action()
			}
			b.size(w: button_width,h: button_height).done()
		}
		centerView.addSubview(buttonsView)
		buttonsView.alignUnder(view:title,withMargin:button_margin).alignParentBottom(withMargin:button_margin).centerX().done()

		

		centerView.matchParentSideBorders(insetedByDx: center_view_sides_margin).center().done()
	}
	
	func show() {
		VoipDialog.rootVC()?.view.addSubview(self)
		matchParentDimmensions().done()
	}
	
	private static func rootVC() -> UIViewController? {
		return PhoneMainView.instance().mainViewController
	}
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
	static var toastQueue: [String] = []
	
	static func toast(message:String, timeout:CGFloat = 1.5) {
		if (toastQueue.count > 0) {
			toastQueue.append(message)
			return
		}
		let rooVc = rootVC()
		let alert = UIAlertController(title: nil, message: message, preferredStyle: .actionSheet)
		alert.popoverPresentationController?.sourceView = PhoneMainView.instance().mainViewController.statusBarView
		rooVc?.present(alert, animated: true)
		DispatchQueue.main.asyncAfter(deadline: DispatchTime.now() + timeout) {
			alert.dismiss(animated: true)
			if (toastQueue.count > 0) {
				let message = toastQueue.first
				toastQueue.remove(at: 0)
				self.toast(message: message!)
			}
		}
	}
	
}

struct ButtonAttributes {
	let text:String
	let action: (()->Void)
	let isDestructive: Bool
}
