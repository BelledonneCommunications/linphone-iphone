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

class VoipExtraButton : UIButton {
	
	// Layout constants
	let width = 60.0
	let image_size = 50.0
	let bouncing_label_size = 17.0
	
	var boucingCounter : BouncingCounter? = nil

	
	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}
	
	init ( text:String, buttonTheme: ButtonTheme, withbBoucinCounterDataSource:MutableLiveData<Int>? = nil, onClickAction : @escaping ()->Void ) {
		super.init(frame: .zero)
		
	
		contentMode = .scaleToFill
		
		buttonTheme.tintableStateIcons.keys.forEach { (stateRawValue) in
			let tintedIcon = buttonTheme.tintableStateIcons[stateRawValue]!
			UIImage(named:tintedIcon.name).map {
				setImage($0.tinted(with: tintedIcon.tintColor?.get()),for:  UIButton.State(rawValue: stateRawValue))
			}
			setTitleColor(tintedIcon.tintColor?.get(), for: UIButton.State(rawValue: stateRawValue))
		}
		imageView?.contentMode = .scaleAspectFit
		imageView?.size(w: image_size,h: image_size).centerX().alignParentTop().done()
		titleLabel?.alignUnder(view: imageView!).centerX().done()
			
		
		size(w: width,h: image_size).done()
		setTitle(text, for: .normal)
		applyTitleStyle(VoipTheme.voip_extra_button)
	
		onClick {
			ControlsViewModel.shared.hideExtraButtons.value = true
			onClickAction()
		}
		
		if (withbBoucinCounterDataSource != nil) {
			boucingCounter = BouncingCounter(inButton:self)
			addSubview(boucingCounter!)
			boucingCounter?.dataSource = withbBoucinCounterDataSource
		}
				
	}


}
