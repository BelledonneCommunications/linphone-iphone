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

extension Address {
	
	func initials() -> String? {
		var initials = Address.initials(displayName: addressBookEnhancedDisplayName())
		if (initials == nil || initials!.isEmpty) {
			initials = String(username.prefix(1))
		}
		return initials
	}
	
	static func initials(displayName: String?) -> String?  { // Basic ImproveMe
		let separator =  displayName?.contains(" ") == true ? " " : "."
		return displayName?.components(separatedBy: separator)
			.reduce("") {
				($0.isEmpty ? "" : "\($0.first?.uppercased() ?? "")") +
				($1.isEmpty ? "" : "\($1.first?.uppercased() ?? "")")
			}
	}
	
	func addressBookEnhancedDisplayName() -> String?  {
		if let contact = FastAddressBook.getContactWith(getCobject) {
			return contact.displayName
		} else if (!displayName.isEmpty) {
			return displayName
		} else {
			return username
		}
	}
	
	func contact() -> Contact? {
		return  FastAddressBook.getContactWith(getCobject)
	}
	
	func isMe() -> Bool {
		guard let accountAddress = Core.get().defaultAccount?.params?.identityAddress else {
			return false
		}
		return weakEqual(address2: accountAddress)
	}
	
}
