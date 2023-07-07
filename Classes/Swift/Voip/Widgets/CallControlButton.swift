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
import SwiftUI

class CallControlButton : ButtonWithStateBackgrounds {
	
	// Layout constants
	static let default_size = 50
	static let hungup_width = 65
	
	var showActivityIndicatorDataSource : MutableLiveData<Bool>? = nil {
		didSet {
			if let showActivityIndicatorDataSource = self.showActivityIndicatorDataSource {
				let spinner = UIActivityIndicatorView(style: .white)
				spinner.color = VoipTheme.primary_color
				self.addSubview(spinner)
				spinner.matchParentDimmensions().center().done()
				
				showActivityIndicatorDataSource.readCurrentAndObserve { (show) in
					if (show == true) {
						spinner.startAnimating()
						spinner.isHidden = false
						self.isEnabled = false
					} else {
						spinner.stopAnimating()
						spinner.isHidden = true
						self.isEnabled = true
					}
				}
			}
		}
	}
	
	var onClickAction : (()->Void)? = nil
	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
	init (width:Int = CallControlButton.default_size, height:Int = CallControlButton.default_size, imageInset:UIEdgeInsets = UIEdgeInsets(top: 2, left: 2, bottom: 2, right: 2), buttonTheme: ButtonTheme, onClickAction :  (()->Void)? = nil ) {
		super.init(backgroundStateColors: buttonTheme.backgroundStateColors)
		
		layer.cornerRadius = CGFloat(height/2)
		clipsToBounds = true
		contentMode = .scaleAspectFit
		
		applyTintedIcons(tintedIcons: buttonTheme.tintableStateIcons)
		
		imageView?.contentMode = .scaleAspectFit
		
		imageEdgeInsets = imageInset
		
		size(w: CGFloat(width), h: CGFloat(height)).done()
				
		self.onClickAction = onClickAction
		onClick {
			self.onClickAction?()
		}
		UIDeviceBridge.displayModeSwitched.observe { _ in
			self.applyTintedIcons(tintedIcons: buttonTheme.tintableStateIcons)
		}
		
	}
	

	
	
}
