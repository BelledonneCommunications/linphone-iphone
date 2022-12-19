//
//  CustomAlertController.swift
//  linphone
//
//  Created by Benoît Martins on 19/12/2022.
//

import Foundation

class CustomAlertController: UIAlertController {
	
	let cancel_button_alert = UIButton()
	let ok_button_alert = UIButton()
	let checkBoxButton = CallControlButton(buttonTheme:VoipTheme.nav_button("checkbox_unchecked"))
	var isChecked = false
	let checkBoxText = UILabel()
	
	var isSecure : Bool = false
	let isGroupChat : Bool = false
	let levelMaxSecure : Bool = false
	
	@objc func dismissOnTapOutsideOrCancel(){
		self.dismiss(animated: true, completion: nil)
	}
	
	@objc func onTapOk(){
		self.dismiss(animated: true, completion: nil)
	}
	
	@objc func switchCheckedValue(){
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
			checkBoxButton.addTarget(self, action: #selector(switchCheckedValue), for: .touchUpInside)
			checkboxView.addSubview(checkBoxButton)
			
			checkBoxText.text = VoipTexts.alert_dialog_secure_badge_button_chat_conversation_checkboxtext
			checkBoxText.textColor = .white
			checkboxView.addSubview(checkBoxText)
			checkBoxText.toRightOf(checkBoxButton, withLeftMargin: -5).height(checkboxViewHeight).done()
			checkBoxText.sizeToFit()
		}
	}
}
