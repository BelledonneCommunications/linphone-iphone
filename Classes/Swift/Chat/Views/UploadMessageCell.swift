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

class UploadMessageCell: UIView {
	var circularProgressBarView = CircularProgressBarView()
	let circularProgressBarLabel = StyledLabel(VoipTheme.chat_conversation_download_progress_text)
	
	var content: Content? = nil
	var fromValue : Float = 0.0
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		self.layer.zPosition = 10
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
