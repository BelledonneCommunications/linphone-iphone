/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
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

extension Account {
	func displayName() -> String {
		guard let address = params?.identityAddress else {
			return ""
		}
		if address.displayName != nil && !address.displayName!.isEmpty {
			return address.displayName!
		}
		if address.username != nil && !address.username!.isEmpty {
			return address.username!
		}
		return String(address.asStringUriOnly().dropFirst(4))
	}
	
	static func == (lhs: Account, rhs: Account) -> Bool {
		if lhs.params != nil && lhs.params?.identityAddress != nil && rhs.params != nil && rhs.params?.identityAddress != nil {
			return lhs.params?.identityAddress?.asString() == rhs.params?.identityAddress?.asString()
		} else {
			return false
		}
	}
}
