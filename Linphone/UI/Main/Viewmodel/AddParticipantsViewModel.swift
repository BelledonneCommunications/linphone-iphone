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
import Combine

class SelectedAddressModel: ObservableObject {
	var address: Address
	var avatarModel: ContactAvatarModel
	var isOrganizer: Bool = false
	
	init (addr: Address, avModel: ContactAvatarModel, isOrg: Bool = false) {
		address = addr
		avatarModel = avModel
		isOrganizer = isOrg
	}
}

class AddParticipantsViewModel: ObservableObject {
	static let TAG = "[AddParticipantsViewModel]"
	
	@Published var participantsToAdd: [SelectedAddressModel] = []
	@Published var searchField: String = ""
	
	func selectParticipant(addr: Address) {
		if let idx = participantsToAdd.firstIndex(where: {$0.address.weakEqual(address2: addr)}) {
			Log.info("[\(AddParticipantsViewModel.TAG)] Removing participant \(addr.asStringUriOnly()) from selection")
			participantsToAdd.remove(at: idx)
		} else {
			Log.info("[\(AddParticipantsViewModel.TAG)] Adding participant \(addr.asStringUriOnly()) to selection")
			ContactAvatarModel.getAvatarModelFromAddress(address: addr) { avatarResult in
				DispatchQueue.main.async {
					self.participantsToAdd.append(SelectedAddressModel(addr: addr, avModel: avatarResult))
				}
			}
		}
	}
	
	func reset() {
		participantsToAdd = []
		searchField = ""
	}
}
