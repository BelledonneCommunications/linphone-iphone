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

class SharedLayoutConstants {
	static var buttons_bottom_margin : Int  {
		get { UIDevice.hasNotch() && UIDevice.current.orientation == .portrait ? 30 : 15 }
	}
	static let margin_call_view_side_controls_buttons = 12
	static let bottom_margin_notch_clearance = UIDevice.hasNotch() ? 30.0 : 0.0
	static let content_inset = 12.0
}
