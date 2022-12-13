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

extension UIAlertController {
	
	//Set background color of UIAlertController
	func setBackgroundColor(color: UIColor) {
		if let bgView = self.view.subviews.first, let groupView = bgView.subviews.first, let contentView = groupView.subviews.first {
			contentView.backgroundColor = color
		}
	}
	
	func setMaxWidth(alert: UIAlertController) {
		let widthConstraints = alert.view.constraints.filter({ return $0.firstAttribute == .width })
			alert.view.removeConstraints(widthConstraints)
			// Here you can enter any width that you want
			let newWidth = UIScreen.main.bounds.width * 0.90
			// Adding constraint for alert base view
			let widthConstraint = NSLayoutConstraint(item: alert.view,
													 attribute: .width,
													 relatedBy: .equal,
													 toItem: nil,
													 attribute: .notAnAttribute,
													 multiplier: 1,
													 constant: newWidth)
			alert.view.addConstraint(widthConstraint)
			let firstContainer = alert.view.subviews[0]
			// Finding first child width constraint
			let constraint = firstContainer.constraints.filter({ return $0.firstAttribute == .width && $0.secondItem == nil })
			firstContainer.removeConstraints(constraint)
			// And replacing with new constraint equal to alert.view width constraint that we setup earlier
			alert.view.addConstraint(NSLayoutConstraint(item: firstContainer,
														attribute: .width,
														relatedBy: .equal,
														toItem: alert.view,
														attribute: .width,
														multiplier: 1.0,
														constant: 0))
			// Same for the second child with width constraint with 998 priority
			let innerBackground = firstContainer.subviews[0]
			let innerConstraints = innerBackground.constraints.filter({ return $0.firstAttribute == .width && $0.secondItem == nil })
			innerBackground.removeConstraints(innerConstraints)
			firstContainer.addConstraint(NSLayoutConstraint(item: innerBackground,
															attribute: .width,
															relatedBy: .equal,
															toItem: firstContainer,
															attribute: .width,
															multiplier: 1.0,
															constant: 0))
	}
	
	//Set title font and title color
	func setTitle(font: UIFont?, color: UIColor?) {
		guard let title = self.title else { return }
		let attributeString = NSMutableAttributedString(string: title)//1

		if let titleFont = font {
			attributeString.addAttributes([NSAttributedString.Key.font : titleFont],//2
										  range: NSMakeRange(0, title.count))
		}
		
		if let titleColor = color {
			attributeString.addAttributes([NSAttributedString.Key.foregroundColor : titleColor],//3
										  range: NSMakeRange(0, title.count))
		}
		
		self.setValue(attributeString, forKey: "attributedTitle")//4
	}
	
	//Set message font and message color
	func setMessage(font: UIFont?, color: UIColor?) {
		guard let message = self.message else { return }
		let attributeString = NSMutableAttributedString(string: message)
		if let messageFont = font {
			attributeString.addAttributes([NSAttributedString.Key.font : messageFont],
										  range: NSMakeRange(0, message.count))
		}
		
		if let messageColorColor = color {
			attributeString.addAttributes([NSAttributedString.Key.foregroundColor : messageColorColor],
										  range: NSMakeRange(0, message.count))
		}
		self.setValue(attributeString, forKey: "attributedMessage")
	}
	
	//Set tint color of UIAlertController
	func setTint(color: UIColor) {
		self.view.tintColor = color
	}
}
