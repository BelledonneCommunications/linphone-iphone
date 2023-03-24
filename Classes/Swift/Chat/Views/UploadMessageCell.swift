//
//  UploadMessageCell.swift
//  linphone
//
//  Created by Benoît Martins on 24/03/2023.
//

import Foundation
import linphonesw

class UploadMessageCell: UIView {
	var circularProgressBarView = CircularProgressBarView()
	let circularProgressBarLabel = StyledLabel(VoipTheme.chat_conversation_download_progress_text)
	
	var content: Content? = nil
	var fromValue : Float = 0.0
	
	override init(frame: CGRect) {
		super.init(frame: frame)

		addSubview(circularProgressBarView)
		circularProgressBarView.isHidden = true
		circularProgressBarLabel.text = "0%"
		circularProgressBarLabel.size(w: 30, h: 30).done()
		circularProgressBarView.addSubview(circularProgressBarLabel)
	}
	
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}
	
	func setUpCircularProgressBarView(toValue: Float) {
		
		circularProgressBarLabel.text = "\(Int(toValue*100))%"
		circularProgressBarLabel.center = CGPoint(x: 69, y: 69)
		

		circularProgressBarView.progressAnimation(fromValue: fromValue, toValue: toValue)
		fromValue = toValue
	}
}
