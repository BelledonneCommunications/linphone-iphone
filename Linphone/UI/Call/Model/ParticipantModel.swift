/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
 *
 * This file is part of Linphone
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

class ParticipantModel: ObservableObject {
	
	static let TAG = "[Participant Model]"
	
	let address: Address
	@Published var sipUri: String
	@Published var name: String
	@Published var avatarModel: ContactAvatarModel
	@Published var isJoining: Bool
	@Published var isMuted: Bool
	
	init(address: Address, isJoining: Bool, isMuted: Bool) {
		self.address = address
		
		self.sipUri = address.asStringUriOnly()
		
		let addressFriend = ContactsManager.shared.getFriendWithAddress(address: self.address)
		
		var nameTmp = ""
		
		if addressFriend != nil {
			nameTmp = addressFriend!.name!
		} else {
			nameTmp = address.displayName != nil
			? address.displayName!
			: address.username!
		}
		
		self.name = nameTmp
		
		self.avatarModel = addressFriend != nil
		? ContactsManager.shared.avatarListModel.first(where: {
			$0.friend!.name == addressFriend!.name
			&& $0.friend!.address!.asStringUriOnly() == address.asStringUriOnly()
		}) ?? ContactAvatarModel(friend: nil, name: nameTmp, withPresence: false)
		: ContactAvatarModel(friend: nil, name: nameTmp, withPresence: false)
		
		self.isJoining = isJoining
		self.isMuted = isMuted
	}
}
