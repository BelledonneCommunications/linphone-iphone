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

@objc class LightDarkColor : NSObject {
	var light: UIColor
	var dark : UIColor
	init(_ l:UIColor,_ d:UIColor){
		light = l
		dark = d
	}
	
	@objc func get() -> UIColor {
		if #available(iOS 13.0, *) {
			if UITraitCollection.current.userInterfaceStyle == .light {
				return light
			} else {
				return dark
			}
		} else {
			return light
		}
	}
	
}
