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

// swiftlint:disable line_length
class ConversationsListViewModel: ObservableObject {
	
	private var coreContext = CoreContext.shared
	private var contactsManager = ContactsManager.shared
	
	private var coreConversationDelegate: CoreDelegate?
	
	@Published var conversationsList: [ConversationModel] = []
	var conversationsListTmp: [ConversationModel] = []
	
	@Published var unreadMessages: Int = 0
	
	var selectedConversation: ConversationModel?
	
	init() {
		computeChatRoomsList(filter: "")
		addConversationDelegate()
	}
	
	func computeChatRoomsList(filter: String) {
		coreContext.doOnCoreQueue { core in
			let account = core.defaultAccount
			let chatRooms = account != nil ? account!.chatRooms : core.chatRooms
			
			self.conversationsListTmp = []
			
			chatRooms.forEach { chatRoom in
				if filter.isEmpty {
					let model = ConversationModel(chatRoom: chatRoom)
					self.conversationsListTmp.append(model)
				}
			}
			
			DispatchQueue.main.async {
				self.conversationsList = self.conversationsListTmp
				NotificationCenter.default.post(name: NSNotification.Name("ChatRoomsComputed"), object: nil)
			}
			
			self.updateUnreadMessagesCount()
		}
	}
	
	func addConversationDelegate() {
		coreContext.doOnCoreQueue { core in
			let account = core.defaultAccount
			let chatRoomsCounter = account?.chatRooms != nil ? account!.chatRooms.count : core.chatRooms.count
			
			self.coreConversationDelegate = CoreDelegateStub(onMessagesReceived: { (_: Core, _: ChatRoom, _: [ChatMessage]) in
				self.computeChatRoomsList(filter: "")
			}, onMessageSent: { (_: Core, _: ChatRoom, _: ChatMessage) in
				self.computeChatRoomsList(filter: "")
			}, onChatRoomRead: { (_: Core, _: ChatRoom) in
				self.computeChatRoomsList(filter: "")
			}, onChatRoomStateChanged: { (core: Core, chatRoom: ChatRoom, state: ChatRoom.State) in
				// Log.info("[ConversationsListViewModel] Conversation [${LinphoneUtils.getChatRoomId(chatRoom)}] state changed [$state]")
				if core.globalState == .On {
					switch state {
					case .Created, .Deleted, .Terminated:
						self.computeChatRoomsList(filter: "")
					default:
						break
					}
				}
			})
			core.addDelegate(delegate: self.coreConversationDelegate!)
		}
	}
	
	func reorderChatRooms() {
		Log.info("[ConversationsListViewModel] Re-ordering conversations")
		var sortedList: [ConversationModel] = []
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
					UIApplication.shared.applicationIconBadgeNumber = count
				}
			} else {
				DispatchQueue.main.async {
					self.unreadMessages = 0
					UIApplication.shared.applicationIconBadgeNumber = 0
				}
			}
		}
	}
	
	func getContentTextMessage(message: ChatMessage, completion: @escaping (String) -> Void) {
		contactsManager.getFriendWithAddressInCoreQueue(address: message.fromAddress) { friendResult in
			var fromAddressFriend = message.fromAddress != nil
			? friendResult?.name ?? nil
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
			
			completion(
				(fromAddressFriend ?? "") + (message.contents.first(where: {$0.isText == true})?.utf8Text ?? (message.contents.first(where: {$0.isFile == true || $0.isFileTransfer == true})?.name ?? ""))
			)
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
	
	func refreshContactAvatarModel() {
		conversationsList.forEach { conversationModel in
			conversationModel.refreshAvatarModel()
		}
		
		reorderChatRooms()
	}
	
	func markAsReadSelectedConversation() {
		coreContext.doOnCoreQueue { _ in
			let unreadMessagesCount = self.selectedConversation!.chatRoom.unreadMessagesCount
			
			if unreadMessagesCount > 0 {
				self.selectedConversation!.chatRoom.markAsRead()
				self.selectedConversation!.unreadMessagesCount = 0
			}
		}
	}
	
	func filterConversations(filter: String) {
		conversationsList.removeAll()
		conversationsListTmp.forEach { conversation in
			if conversation.subject.lowercased().contains(filter.lowercased()) || !conversation.participantsAddress.filter({ $0.lowercased().contains(filter.lowercased()) }).isEmpty {
				conversationsList.append(conversation)
			}
		}
	}
	
	func resetFilterConversations() {
		conversationsList = conversationsListTmp
	}
}
// swiftlint:enable line_length
