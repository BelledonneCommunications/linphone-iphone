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

import linphonesw

class CallViewModel: ObservableObject {
	
	var coreContext = CoreContext.shared
	var telecomManager = TelecomManager.shared
	
	@Published var displayName: String = "Example Linphone"
	@Published var direction: Call.Dir = .Outgoing
	@Published var remoteAddressString: String = "example.linphone@sip.linphone.org"
	@Published var remoteAddress: Address?
	@Published var avatarModel: ContactAvatarModel?
	
	init() {
		coreContext.doOnCoreQueue { core in
			if core.currentCall != nil && core.currentCall!.remoteAddress != nil {
				DispatchQueue.main.async {
					self.direction = .Incoming
					self.remoteAddressString = String(core.currentCall!.remoteAddress!.asStringUriOnly().dropFirst(4))
					self.remoteAddress = core.currentCall!.remoteAddress!
					
					let friend = ContactsManager.shared.getFriendWithAddress(address: core.currentCall!.remoteAddress!)
					if friend != nil && friend!.address != nil && friend!.address!.displayName != nil {
						self.displayName = friend!.address!.displayName!
					} else {
						if core.currentCall!.remoteAddress!.displayName != nil {
							self.displayName = core.currentCall!.remoteAddress!.displayName!
						} else if core.currentCall!.remoteAddress!.username != nil {
							self.displayName = core.currentCall!.remoteAddress!.username!
						}
					}
				}
			}
		}
	}
}
