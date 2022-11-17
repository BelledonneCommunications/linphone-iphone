/*
* Copyright (c) 2010-2020 Belledonne Communications SARL.
*
* This file is part of linhome
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
import AVFoundation

extension UIDevice {
	static func ipad() -> Bool {
		return UIDevice.current.userInterfaceIdiom == .pad
	}
	static func vibrate() {
		if (!ipad()) {
			AudioServicesPlaySystemSound(kSystemSoundID_Vibrate)
		}
	}
	static func is5SorSEGen1() -> Bool {
		return UIScreen.main.nativeBounds.height == 1136
	}
	
	static func hasNotch() -> Bool {
		if (UserDefaults.standard.bool(forKey: "hasNotch")) {
			return true
		}
		guard #available(iOS 11.0, *), let topPadding = UIApplication.shared.keyWindow?.safeAreaInsets.top, topPadding > 24 else {
			return false
		}
		UserDefaults.standard.setValue(true, forKey: "hasNotch")
		return true
	}
	
	static func notchHeight() -> CGFloat {
		guard #available(iOS 11.0, *), let topPadding = UIApplication.shared.keyWindow?.safeAreaInsets.top, let sidePadding =  UIApplication.shared.keyWindow?.safeAreaInsets.left else {
			return 0
		}
		return [.landscapeRight,.landscapeLeft].contains(UIDevice.current.orientation) ? sidePadding : topPadding
	}
	
	static func switchedDisplayMode() -> Bool {
		let displayMode = UserDefaults.standard.string(forKey: "displayMode")
		if #available(iOS 13.0, *) {
			if UITraitCollection.current.userInterfaceStyle == .light {
				UserDefaults.standard.set("light", forKey: "displayMode")
			} else {
				UserDefaults.standard.set("dark", forKey: "displayMode")
			}
		}
		return displayMode != nil && displayMode != UserDefaults.standard.string(forKey: "displayMode")
	}
	
}

@objc class UIDeviceBridge : NSObject {
	static let displayModeSwitched = MutableLiveData<Bool>()
	@objc static func switchedDisplayMode() -> Bool {
		return UIDevice.switchedDisplayMode()
	}
	@objc static func notifyDisplayModeSwitch() {
		displayModeSwitched.notifyValue()
	}
}
