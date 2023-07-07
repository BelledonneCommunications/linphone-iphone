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

class CustomAlertController: UIAlertController {
	
	let cancel_button_alert = UIButton()
	let ok_button_alert = UIButton()
	let checkBoxButton = CallControlButton(buttonTheme:VoipTheme.nav_button("checkbox_unchecked"))
	var isChecked = false
	let checkBoxText = UILabel()
	
	@objc func dismissOnTapOutsideOrCancel(){
		self.dismiss(animated: true, completion: nil)
	}
	
	@objc func onTapOk(){
		self.dismiss(animated: true, completion: nil)
	}
	
	@objc func changeCheckValue(){
		isChecked = !isChecked
		checkBoxButton.isSelected = isChecked
	}
	
	func addButtonsAlertController(alertController: UIAlertController, buttonsViewHeightV: CGFloat, checkboxViewHeightV: CGFloat? = nil, buttonsAlertHeightV: CGFloat){
		let buttonsViewHeight = buttonsViewHeightV
		let checkboxViewHeight = checkboxViewHeightV ?? 0
		let buttonsAlertHeight = buttonsAlertHeightV
		
		let alertControllerHeight : CGFloat = (buttonsViewHeight + checkboxViewHeight + buttonsAlertHeight) * 2
		
		let buttonsView = UIView()
		alertController.view.addSubview(buttonsView)
		buttonsView.translatesAutoresizingMaskIntoConstraints = false
		buttonsView.bottomAnchor.constraint(equalTo: alertController.view.bottomAnchor, constant: -10).isActive = true
		buttonsView.rightAnchor.constraint(equalTo: alertController.view.rightAnchor, constant: -10).isActive = true
		buttonsView.leftAnchor.constraint(equalTo: alertController.view.leftAnchor, constant: 10).isActive = true
		buttonsView.heightAnchor.constraint(equalToConstant: buttonsViewHeight).isActive = true

		alertController.view.translatesAutoresizingMaskIntoConstraints = false
		alertController.view.heightAnchor.constraint(equalToConstant: alertControllerHeight).isActive = true
		
		cancel_button_alert.setTitle(VoipTexts.cancel.uppercased(), for: .normal)
		cancel_button_alert.backgroundColor = .systemRed
		cancel_button_alert.layer.cornerRadius = 5
		cancel_button_alert.addTarget(self, action: #selector(dismissOnTapOutsideOrCancel), for: .touchUpInside)
		buttonsView.addSubview(cancel_button_alert)
		cancel_button_alert.alignParentLeft(withMargin: 40).size(w: 100, h: buttonsAlertHeight).done()
		
		ok_button_alert.setTitle(VoipTexts.ok.uppercased(), for: .normal)
		ok_button_alert.backgroundColor = .systemGreen
		ok_button_alert.layer.cornerRadius = 5
		ok_button_alert.addTarget(self, action: #selector(onTapOk), for: .touchUpInside)
		buttonsView.addSubview(ok_button_alert)
		ok_button_alert.alignParentRight(withMargin: 40).size(w: 100, h: buttonsAlertHeight).done()
		
		if(checkboxViewHeight != 0){
			let checkboxView = UIView()
			alertController.view.addSubview(checkboxView)
			checkboxView.translatesAutoresizingMaskIntoConstraints = false
			checkboxView.bottomAnchor.constraint(equalTo: buttonsView.topAnchor, constant: -5).isActive = true
			checkboxView.centerXAnchor.constraint(equalTo: alertController.view.centerXAnchor).isActive = true
			checkboxView.heightAnchor.constraint(equalToConstant: checkboxViewHeight).isActive = true
			checkboxView.width(180).done()
			
			checkBoxButton.setImage(UIImage(named:"checkbox_unchecked.png"), for: .normal)
			checkBoxButton.setImage(UIImage(named:"checkbox_checked.png"), for: .selected)
			checkBoxButton.addTarget(self, action: #selector(changeCheckValue), for: .touchUpInside)
			checkboxView.addSubview(checkBoxButton)
			
			checkBoxText.text = VoipTexts.alert_dialog_secure_badge_button_chat_conversation_checkboxtext
			checkBoxText.textColor = .white
			checkboxView.addSubview(checkBoxText)
			checkBoxText.toRightOf(checkBoxButton, withLeftMargin: -5).height(checkboxViewHeight).done()
			checkBoxText.sizeToFit()
		}
	}
	
	func setBackgroundColor(color: UIColor) {
		if let bgView = self.view.subviews.first, let groupView = bgView.subviews.first, let contentView = groupView.subviews.first {
			contentView.backgroundColor = color
		}
	}
	
	func setMaxWidth(alert: UIAlertController) {
		let widthConstraints = alert.view.constraints.filter({ return $0.firstAttribute == .width })
			alert.view.removeConstraints(widthConstraints)
			let newWidth = UIScreen.main.bounds.width * 0.90
			let widthConstraint = NSLayoutConstraint(item: alert.view,
													 attribute: .width,
													 relatedBy: .equal,
													 toItem: nil,
													 attribute: .notAnAttribute,
													 multiplier: 1,
													 constant: newWidth)
			alert.view.addConstraint(widthConstraint)
			let firstContainer = alert.view.subviews[0]
			let constraint = firstContainer.constraints.filter({ return $0.firstAttribute == .width && $0.secondItem == nil })
			firstContainer.removeConstraints(constraint)
			alert.view.addConstraint(NSLayoutConstraint(item: firstContainer,
														attribute: .width,
														relatedBy: .equal,
														toItem: alert.view,
														attribute: .width,
														multiplier: 1.0,
														constant: 0))
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
	
	func setTitle(font: UIFont?, color: UIColor?) {
		guard let title = self.title else { return }
		let attributeString = NSMutableAttributedString(string: title)

		if let titleFont = font {
			attributeString.addAttributes([NSAttributedString.Key.font : titleFont],
										  range: NSMakeRange(0, title.count))
		}
		
		if let titleColor = color {
			attributeString.addAttributes([NSAttributedString.Key.foregroundColor : titleColor],
										  range: NSMakeRange(0, title.count))
		}
		
		self.setValue(attributeString, forKey: "attributedTitle")
	}
	
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
	
	func setTint(color: UIColor) {
		self.view.tintColor = color
	}
}
