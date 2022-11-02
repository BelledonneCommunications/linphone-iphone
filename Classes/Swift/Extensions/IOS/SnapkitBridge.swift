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
import Photos
import linphonesw


@objc class SnapkitBridge: NSObject {
	@objc static func matchParentDimensions(view:UIView) {
		view.matchParentDimmensions().done()
	}
	
	@objc static func matchParentDimensions(view:UIView,leftInset:CGFloat) {
		view.matchParentDimmensions(insetedBy: UIEdgeInsets(top: 0, left: leftInset, bottom: 0, right: 0)).done()
	}
	
	@objc static func matchParentDimensions(view:UIView,topInset:CGFloat) {
		view.matchParentDimmensions(insetedBy: UIEdgeInsets(top: topInset, left: 0, bottom: 0, right: 0)).done()
	}
	
	@objc static func height(view:UIView,heiht:Int) {
		view.height(heiht).done()
	}
	@objc static func square(view:UIView,size:Int) {
		view.square(size).done()
	}
	
	@objc static func alignParentLeft(view:UIView) {
		view.alignParentLeft().done()
	}
	
	@objc static func centerY(view:UIView) {
		view.centerY().done()
	}

}

