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


import UIKit
import Foundation
import SnapKit
import linphonesw

class RemotelyRecordingView: UIView {
		
	let label = StyledLabel(VoipTheme.call_remote_recording)
	let icon = UIImageView(image: UIImage(named:"voip_remote_recording"))

	var isRemotelyRecorded: MutableLiveData<Bool>? = nil {
		didSet {
			isRemotelyRecorded?.readCurrentAndObserve(onChange: { (isRemotelyRecording) in
				self.isHidden = isRemotelyRecording != true
			})
		}
	}
	
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
	init (height:Int, text:String) {
		super.init(frame: .zero)
		backgroundColor = VoipTheme.dark_grey_color
		layer.cornerRadius = CGFloat(height/2)
		clipsToBounds = true
		
		addSubview(label)
		label.center().height(CGFloat(height)).done()
		label.text = text
		
		addSubview(icon)
		icon.square(height).toLeftOf(label).done()
		
		isHidden = true 
		
	}
	
}
