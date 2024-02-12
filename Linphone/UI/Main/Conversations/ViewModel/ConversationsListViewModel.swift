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

import Foundation
import linphonesw

class ConversationsListViewModel: ObservableObject {
	
	private var coreContext = CoreContext.shared
	
	@Published var conversationsList: [ChatRoom] = []
	@Published var unreadMessages: Int = 0
	
	init() {
		computeChatRoomsList(filter: "")
		updateUnreadMessagesCount()
	}
	
	func computeChatRoomsList(filter: String) {
		coreContext.doOnCoreQueue { core in
			let account = core.defaultAccount
			let chatRooms = account?.chatRooms != nil ? account!.chatRooms : core.chatRooms
			
			chatRooms.forEach { chatRoom in
				//let disabledBecauseNotSecured = (account?.isInSecureMode() == true && !chatRoom.hasCapability) ? Capabilities.Encrypted.toInt() : 0
				
				if filter.isEmpty {
					//val model = ConversationModel(chatRoom, disabledBecauseNotSecured)
					self.conversationsList.append(chatRoom)
				}
				/*
				 else {
					val participants = chatRoom.participants
					val found = participants.find {
						// Search in address but also in contact name if exists
						val model =
						coreContext.contactsManager.getContactAvatarModelForAddress(it.address)
						model.contactName?.contains(
							filter,
							ignoreCase = true
						) == true || it.address.asStringUriOnly().contains(
							filter,
							ignoreCase = true
						)
					}
					if (
						found != null ||
						chatRoom.peerAddress.asStringUriOnly().contains(filter, ignoreCase = true) ||
						chatRoom.subject.orEmpty().contains(filter, ignoreCase = true)
					) {
						val model = ConversationModel(chatRoom, disabledBecauseNotSecured)
						list.add(model)
						count += 1
					}
				}
				 */
			}
		}
	}
	
	func getCallTime(startDate: time_t) -> String {
		let timeInterval = TimeInterval(startDate)
		
		let myNSDate = Date(timeIntervalSince1970: timeInterval)
		
		if Calendar.current.isDateInToday(myNSDate) {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
			return formatter.string(from: myNSDate)
		} else if Calendar.current.isDateInYesterday(myNSDate) {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
			return "Yesterday"
		} else if Calendar.current.isDate(myNSDate, equalTo: .now, toGranularity: .year) {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "dd/MM" : "MM/dd"
			return formatter.string(from: myNSDate)
		} else {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "dd/MM/yy" : "MM/dd/yy"
			return formatter.string(from: myNSDate)
		}
	}
	
	func updateUnreadMessagesCount() {
		coreContext.doOnCoreQueue { core in
			let account = core.defaultAccount
			if account != nil {
				let count = account?.unreadChatMessageCount != nil ? account!.unreadChatMessageCount : core.unreadChatMessageCount
				self.unreadMessages = count
			} else {
				self.unreadMessages = 0
			}
		}
	}
}
