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
import SwiftUI

class ParticipantModel: ObservableObject {
	
	static let TAG = "[Participant Model]"
	
	let address: Address
	@Published var sipUri: String
	@Published var name: String
	@Published var avatarModel: ContactAvatarModel
	@Published var isJoining: Bool
	@Published var onPause: Bool
	@Published var isMuted: Bool
	@Published var isAdmin: Bool
	@Published var isSpeaking: Bool
	
	init(address: Address, isJoining: Bool = false, onPause: Bool = false, isMuted: Bool = false, isAdmin: Bool = false, isSpeaking: Bool = false) {
		self.address = address
		
		self.sipUri = address.asStringUriOnly()
		
		self.name = ""
		
		self.avatarModel = ContactAvatarModel(friend: nil, name: "", address: address.asStringUriOnly(), withPresence: false)
		
		self.isJoining = isJoining
		self.onPause = onPause
		self.isMuted = isMuted
		self.isAdmin = isAdmin
		self.isSpeaking = isSpeaking
		
		ContactsManager.shared.getFriendWithAddressInCoreQueue(address: self.address) { friendResult in
			if let addressFriend = friendResult {
				self.name = addressFriend.name!
			} else {
				if address.displayName != nil {
					self.name = address.displayName!
				} else if address.username != nil {
					self.name = address.username!
				} else {
					self.name = String(address.asStringUriOnly().dropFirst(4))
				}
			}
		}
		
		ContactAvatarModel.getAvatarModelFromAddress(address: self.address) { avatarResult in
			self.avatarModel = avatarResult
		}
	}
}
