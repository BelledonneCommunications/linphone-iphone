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
import Combine

class ConversationsListViewModel: ObservableObject {
	
	private var coreContext = CoreContext.shared
	private var contactsManager = ContactsManager.shared
	
	private var mCoreSuscriptions = Set<AnyCancellable?>()
	
	@Published var conversationsList: [ChatRoom] = []
	@Published var unreadMessages: Int = 0
	
	init() {
		computeChatRoomsList(filter: "")
		addConversationDelegate()
	}
	
	func computeChatRoomsList(filter: String) {
		coreContext.doOnCoreQueue { core in
			let account = core.defaultAccount
			let chatRooms = account?.chatRooms != nil ? account!.chatRooms : core.chatRooms
			
			DispatchQueue.main.async {
				self.conversationsList = []
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
				
				self.updateUnreadMessagesCount()
			}
		}
	}
	
	func addConversationDelegate() {
		coreContext.doOnCoreQueue { core in
			self.mCoreSuscriptions.insert(core.publisher?.onChatRoomStateChanged?.postOnMainQueue { (cbValue: (_: Core, chatRoom: ChatRoom, state: ChatRoom.State)) in
				//Log.info("[ConversationsListViewModel] Conversation [${LinphoneUtils.getChatRoomId(chatRoom)}] state changed [$state]")
				switch cbValue.state {
				case ChatRoom.State.Created:
					self.computeChatRoomsList(filter: "")
				case ChatRoom.State.Deleted:
					self.computeChatRoomsList(filter: "")
					//ToastViewModel.shared.toastMessage = "toast_conversation_deleted"
					//ToastViewModel.shared.displayToast = true
				default:
					break
				}
			})
			
			self.mCoreSuscriptions.insert(core.publisher?.onMessageSent?.postOnMainQueue { _ in
				self.reorderChatRooms()
			})
			
			self.mCoreSuscriptions.insert(core.publisher?.onMessagesReceived?.postOnMainQueue { _ in
				self.reorderChatRooms()
			})
		}
	}
	
	func reorderChatRooms() {
		Log.info("[ConversationsListViewModel] Re-ordering conversations")
		var sortedList: [ChatRoom] = []
		sortedList.append(contentsOf: conversationsList)
		
		DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
			self.conversationsList = sortedList.sorted { $0.lastUpdateTime > $1.lastUpdateTime }
		}
		
		updateUnreadMessagesCount()
	}
	
	func updateUnreadMessagesCount() {
		coreContext.doOnCoreQueue { core in
			let account = core.defaultAccount
			if account != nil {
				let count = account?.unreadChatMessageCount != nil ? account!.unreadChatMessageCount : core.unreadChatMessageCount
				
				DispatchQueue.main.async {
					self.unreadMessages = count
				}
			} else {
				DispatchQueue.main.async {
					self.unreadMessages = 0
				}
			}
		}
	}
	
	func getContentTextMessage(message: ChatMessage) -> String {
		var fromAddressFriend = message.fromAddress != nil
		? contactsManager.getFriendWithAddress(address: message.fromAddress!)?.name ?? nil
		: nil
		
		if !message.isOutgoing && message.chatRoom != nil && !message.chatRoom!.hasCapability(mask: ChatRoom.Capabilities.OneToOne.rawValue) {
			if fromAddressFriend == nil {
				if message.fromAddress!.displayName != nil {
					fromAddressFriend = message.fromAddress!.displayName! + ": "
				} else if message.fromAddress!.username != nil {
					fromAddressFriend = message.fromAddress!.username! + ": "
				} else {
					fromAddressFriend = ""
				}
			} else {
				fromAddressFriend! += ": "
			}
			
		} else {
			fromAddressFriend = nil
		}
		
		return (fromAddressFriend ?? "") + (message.contents.first(where: {$0.isText == true})?.utf8Text ?? (message.contents.first(where: {$0.isFile == true || $0.isFileTransfer == true})?.name ?? ""))
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
}
