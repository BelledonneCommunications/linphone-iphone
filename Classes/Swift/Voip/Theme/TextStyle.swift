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

struct TextStyle {
	var fgColor:LightDarkColor
	var bgColor:LightDarkColor
	var allCaps:Bool
	var align:NSTextAlignment
	var font:String
	var size:Float
	
	func boldEd() -> TextStyle {
		return self.font.contains("Bold") ? self : TextStyle(fgColor: self.fgColor,bgColor: self.bgColor,allCaps: self.allCaps,align: self.align,font: self.font.replacingOccurrences(of: "Regular", with: "Bold"), size: self.size)
	}
}


extension UILabel {
	
	func applyStyleColors(_ style:TextStyle) {
		textColor = style.fgColor.get()
		backgroundColor = style.bgColor.get()
	}
	
	func applyStyle(_ style:TextStyle) {
		applyStyleColors(style)
		if (style.allCaps) {
			text = self.text?.uppercased()
			tag = 1
		}
		textAlignment = style.align
		let fontSizeMultiplier: Float = (UIDevice.ipad() ? 1.25 : UIDevice.is5SorSEGen1() ? 0.9 : 1.0)
		font = UIFont.init(name: style.font, size: CGFloat(style.size*fontSizeMultiplier))
	}
	
	func addIndicatorIcon(iconName:String,  padding:CGFloat = 5.0, y:CGFloat = 4.0, trailing: Bool = true) {
		let imageAttachment = NSTextAttachment()
		imageAttachment.image = UIImage(named:iconName)?.tinted(with: VoipTheme.voipDrawableColor.get())
		imageAttachment.bounds = CGRect(x: 0.0, y: y , width: font.lineHeight - 2*padding, height: font.lineHeight - 2*padding)
		let iconString = NSMutableAttributedString(attachment: imageAttachment)
		let textXtring = NSMutableAttributedString(string: text != nil ? (!trailing ? " " : "") + text! + (trailing ? " " : "") : "")
		if (trailing) {
			textXtring.append(iconString)
			self.text = nil
			self.attributedText = textXtring
		} else {
			iconString.append(textXtring)
			self.text = nil
			self.attributedText = iconString
		}
	}
}

extension UIButton {
	func applyTitleStyle(_ style:TextStyle) {
		titleLabel?.applyStyle(style)
		if (style.allCaps) {
			setTitle(self.title(for: .normal)?.uppercased(), for: .normal)
			tag = 1
		}
		setTitleColor(style.fgColor.get(), for: .normal)
		contentHorizontalAlignment = style.align == .left ? .left : style.align == .center ? .center : style.align == .right ? .right : .left
	}
}

extension UITextView {
	
	func applyStyleColors(_ style:TextStyle) {
		textColor = style.fgColor.get()
		backgroundColor = style.bgColor.get()
	}
	
	func applyStyle(_ style:TextStyle) {
		applyStyleColors(style)
		if (style.allCaps) {
			text = self.text?.uppercased()
			tag = 1
		}
		textAlignment = style.align
		let fontSizeMultiplier: Float = (UIDevice.ipad() ? 1.25 : UIDevice.is5SorSEGen1() ? 0.9 : 1.0)
		font = UIFont.init(name: style.font, size: CGFloat(style.size*fontSizeMultiplier))
	}
	var numberOfCurrentlyDisplayedLines: Int {
		return text.components(separatedBy: "\n").count
	}
	func removeTextUntilSatisfying(maxNumberOfLines: Int) {
		while numberOfCurrentlyDisplayedLines > (maxNumberOfLines) {
			text = String(text.dropLast())
		}
	}
}

