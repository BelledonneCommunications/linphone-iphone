/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
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

import linphonesw

class AccountProfileViewModel: ObservableObject {
	
	@Published var avatarModel: ContactAvatarModel?
	@Published var photoAvatarModel: String?
	
	init() {}
	
	func setAvatarModel() {
		CoreContext.shared.doOnCoreQueue { core in
			if core.defaultAccount != nil {
				let displayNameTmp = core.defaultAccount!.displayName()
				let contactAddressTmp = core.defaultAccount!.contactAddress?.asStringUriOnly() ?? ""
				DispatchQueue.main.async {
					self.avatarModel = ContactAvatarModel(friend: nil, name: displayNameTmp, address: contactAddressTmp, withPresence: false)
				}
			}
		}
	}
}
