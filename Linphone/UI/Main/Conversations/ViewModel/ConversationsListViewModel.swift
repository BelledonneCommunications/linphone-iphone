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
	
	static let TAG = "[ConversationsListViewModel]"
	
	private var coreContext = CoreContext.shared
	private var contactsManager = ContactsManager.shared
	
	private var coreConversationDelegate: CoreDelegate?
	
	@Published var conversationsList: [ConversationModel] = []
	var conversationsListTmp: [ConversationModel] = []
	
	@Published var unreadMessages: Int = 0
	
	var selectedConversation: ConversationModel?
	
	var currentFilter: String = ""
	
	init() {
		computeChatRoomsList(filter: "")
		addConversationDelegate()
	}
	
	func computeChatRoomsList(filter: String) {
		coreContext.doOnCoreQueue { core in
			let account = core.defaultAccount
			let chatRooms = account != nil ? account!.chatRooms : core.chatRooms
			
			self.conversationsListTmp = []
			DispatchQueue.main.async {
				self.conversationsList = []
			}
			
			var conversationsTmp: [ConversationModel] = []
			var count = 0
			print("ConversationModelInit 0000")
			chatRooms.forEach { chatRoom in
				if filter.isEmpty {
					print("ConversationModelInit 1111")
					let model = ConversationModel(chatRoom: chatRoom)
					conversationsTmp.append(model)
					count += 1
				}
			}
			
			DispatchQueue.main.async {
				self.conversationsListTmp = conversationsTmp
				self.conversationsList = conversationsTmp
			}
			
			self.updateUnreadMessagesCount()
		}
	}
	
	func updateChatRoomsList() {
		CoreContext.shared.doOnCoreQueue { _ in
			if !self.conversationsListTmp.isEmpty {
				self.contactsManager.avatarListModel.forEach { contactAvatarModel in
					self.conversationsListTmp.forEach { conversationModel in
						if conversationModel.participantsAddress.contains(contactAvatarModel.address) {
							if conversationModel.isGroup && conversationModel.participantsAddress.count > 1 {
								if let lastMessage = conversationModel.chatRoom.lastMessageInHistory, let fromAddress = lastMessage.fromAddress, fromAddress.asStringUriOnly().contains(contactAvatarModel.address) {
									var fromAddressFriend = self.contactsManager.getFriendWithAddress(address: fromAddress)?.name
									
									if !lastMessage.isOutgoing && lastMessage.chatRoom != nil && !lastMessage.chatRoom!.hasCapability(mask: ChatRoom.Capabilities.OneToOne.rawValue) {
										if fromAddressFriend == nil {
											if let displayName = fromAddress.displayName {
												fromAddressFriend = displayName + ": "
											} else if let username = fromAddress.username {
												fromAddressFriend = username + ": "
											} else {
												fromAddressFriend = String(fromAddress.asStringUriOnly().dropFirst(4)) + ": "
											}
										} else {
											fromAddressFriend! += ": "
										}
									} else {
										fromAddressFriend = nil
									}
									
									let lastMessageTextTmp = (fromAddressFriend ?? "") + (lastMessage.contents.first(where: { $0.isText })?.utf8Text ?? (lastMessage.contents.first(where: { $0.isFile || $0.isFileTransfer })?.name ?? ""))
									
									if let index = self.conversationsList.firstIndex(where: { $0.chatRoom === conversationModel.chatRoom }) {
										DispatchQueue.main.async {
											conversationModel.lastMessageText = lastMessageTextTmp
											self.conversationsList[index].lastMessageText = lastMessageTextTmp
										}
									} else {
										DispatchQueue.main.async {
											conversationModel.lastMessageText = lastMessageTextTmp
										}
									}
								}
							} else if !conversationModel.isGroup, let firstParticipantAddress = conversationModel.participantsAddress.first, firstParticipantAddress.contains(contactAvatarModel.address) {
								if let index = self.conversationsList.firstIndex(where: { $0.chatRoom === conversationModel.chatRoom }) {
									DispatchQueue.main.async {
										conversationModel.avatarModel = contactAvatarModel
										conversationModel.subject = contactAvatarModel.name
										self.conversationsList[index].avatarModel = contactAvatarModel
									}
								} else {
									DispatchQueue.main.async {
										conversationModel.avatarModel = contactAvatarModel
										conversationModel.subject = contactAvatarModel.name
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	func updateChatRoom(address: String) {
		CoreContext.shared.doOnCoreQueue { _ in
			if let contactAvatarModel = self.contactsManager.avatarListModel.first(where: { $0.addresses.contains(address) }) {
				self.conversationsListTmp.forEach { conversationModel in
					if conversationModel.participantsAddress.contains(contactAvatarModel.address) {
						if conversationModel.isGroup && conversationModel.participantsAddress.count > 1 {
							if let lastMessage = conversationModel.chatRoom.lastMessageInHistory, let fromAddress = lastMessage.fromAddress, fromAddress.asStringUriOnly().contains(contactAvatarModel.address) {
								var fromAddressFriend = self.contactsManager.getFriendWithAddress(address: fromAddress)?.name
								
								if !lastMessage.isOutgoing && lastMessage.chatRoom != nil && !lastMessage.chatRoom!.hasCapability(mask: ChatRoom.Capabilities.OneToOne.rawValue) {
									if fromAddressFriend == nil {
										if let displayName = fromAddress.displayName {
											fromAddressFriend = displayName + ": "
										} else if let username = fromAddress.username {
											fromAddressFriend = username + ": "
										} else {
											fromAddressFriend = String(fromAddress.asStringUriOnly().dropFirst(4)) + ": "
										}
									} else {
										fromAddressFriend! += ": "
									}
								} else {
									fromAddressFriend = nil
								}
								
								let lastMessageTextTmp = (fromAddressFriend ?? "") + (lastMessage.contents.first(where: { $0.isText })?.utf8Text ?? (lastMessage.contents.first(where: { $0.isFile || $0.isFileTransfer })?.name ?? ""))
								
								if let index = self.conversationsList.firstIndex(where: { $0.chatRoom === conversationModel.chatRoom }) {
									DispatchQueue.main.async {
										conversationModel.lastMessageText = lastMessageTextTmp
										self.conversationsList[index].lastMessageText = lastMessageTextTmp
									}
								} else {
									DispatchQueue.main.async {
										conversationModel.lastMessageText = lastMessageTextTmp
									}
								}
							}
						} else if !conversationModel.isGroup, let firstParticipantAddress = conversationModel.participantsAddress.first, firstParticipantAddress.contains(contactAvatarModel.address) {
							if let index = self.conversationsList.firstIndex(where: { $0.chatRoom === conversationModel.chatRoom }) {
								DispatchQueue.main.async {
									conversationModel.avatarModel = contactAvatarModel
									conversationModel.subject = contactAvatarModel.name
									self.conversationsList[index].avatarModel = contactAvatarModel
								}
							} else {
								DispatchQueue.main.async {
									conversationModel.avatarModel = contactAvatarModel
									conversationModel.subject = contactAvatarModel.name
								}
							}
						}
					}
				}
			}
		}
	}
	
	func addConversationDelegate() {
		coreContext.doOnCoreQueue { core in
			self.coreConversationDelegate = CoreDelegateStub(onMessagesReceived: { (_: Core, chatRoom: ChatRoom, _: [ChatMessage]) in
				let model = ConversationModel(chatRoom: chatRoom)
				let idTmp = LinphoneUtils.getChatRoomId(room: chatRoom)
				let index = self.conversationsList.firstIndex(where: { $0.id == idTmp })
				DispatchQueue.main.async {
					if index != nil {
						self.conversationsList.remove(at: index!)
					}
					self.conversationsList.insert(model, at: 0)
				}
				self.updateUnreadMessagesCount()
			}, onMessageSent: { (_: Core, chatRoom: ChatRoom, _: ChatMessage) in
				let model = ConversationModel(chatRoom: chatRoom)
				let idTmp = LinphoneUtils.getChatRoomId(room: chatRoom)
				let index = self.conversationsList.firstIndex(where: { $0.id == idTmp })
				DispatchQueue.main.async {
					if index != nil {
						self.conversationsList.remove(at: index!)
					}
					self.conversationsList.insert(model, at: 0)
				}
				self.updateUnreadMessagesCount()
			}, onChatRoomRead: { (_: Core, chatRoom: ChatRoom) in
				let model = ConversationModel(chatRoom: chatRoom)
				let idTmp = LinphoneUtils.getChatRoomId(room: chatRoom)
				let index = self.conversationsList.firstIndex(where: { $0.id == idTmp })
				DispatchQueue.main.async {
					if index != nil {
						self.conversationsList.remove(at: index!)
						self.conversationsList.insert(model, at: index!)
					} else {
						self.conversationsList.insert(model, at: 0)
					}
				}
				self.updateUnreadMessagesCount()
			}, onChatRoomStateChanged: { (core: Core, chatroom: ChatRoom, state: ChatRoom.State) in
				// Log.info("[ConversationsListViewModel] Conversation [${LinphoneUtils.getChatRoomId(chatRoom)}] state changed [$state]")
				if core.globalState == .On {
					switch state {
					case .Created:
						print("")
						//self.addChatRoom(chatRoom: chatroom)
						//self.computeChatRoomsList(filter: "")
					case .Deleted:
						self.computeChatRoomsList(filter: "")
					default:
						break
					}
				}
			})
			core.addDelegate(delegate: self.coreConversationDelegate!)
		}
	}
	
	private func addChatRoom(chatRoom: ChatRoom) {
		let identifier = chatRoom.identifier
		let chatRoomAccount = chatRoom.account
		let defaultAccount = LinphoneUtils.getDefaultAccount()
		
		print("ConversationModelInit 2222")
		//print("addChatRoomaddChatRoom \(chatRoomAccount?.contactAddress?.asStringUriOnly()) \(defaultAccount?.contactAddress?.asStringUriOnly())")
		if defaultAccount == nil || chatRoomAccount == nil { //} || chatRoomAccount != defaultAccount {
			Log.warn(
				"\(ConversationsListViewModel.TAG) Chat room with identifier \(identifier ?? "Identifier error") was created but not displaying it because it doesn't belong to currently default account"
			)
			return
		}
		
		let hideEmptyChatRooms = coreContext.mCore.config == nil ? true : coreContext.mCore.config!.getBool(section: "misc", key: "hide_empty_chat_rooms", defaultValue: true)
		// Hide empty chat rooms only applies to 1-1 conversations
		if (hideEmptyChatRooms && !LinphoneUtils.isChatRoomAGroup(chatRoom: chatRoom) && chatRoom.lastMessageInHistory == nil) {
			Log.warn("\(ConversationsListViewModel.TAG) Chat room with identifier \(identifier ?? "Identifier error") is empty, not adding it to match Core setting")
			return
		}
		
		let currentList = conversationsListTmp
		let found = currentList.first(where: {$0.chatRoom.identifier == identifier})
		if (found != nil) {
			Log.warn("\(ConversationsListViewModel.TAG) Created chat room with identifier \(identifier ?? "Identifier error") is already in the list, skipping")
			return
		}
		
		if !currentFilter.isEmpty {
			let filteredRooms = defaultAccount!.filterChatRooms(filter: currentFilter)
			let found = filteredRooms.first(where: {$0.identifier == chatRoom.identifier})
			if found == nil {
				return
			}
		}
		
		var newList: [ConversationModel] = []
		let model = ConversationModel(chatRoom: chatRoom)
		newList.append(model)
		newList.append(contentsOf: currentList)
		Log.info("\(ConversationsListViewModel.TAG) Adding chat room with identifier \(identifier ?? "Identifier error") to list")
		
		DispatchQueue.main.async {
			self.conversationsList = newList
		}
	}
	
	/*
	@WorkerThread
	private fun removeChatRoom(chatRoom: ChatRoom) {
		val currentList = conversations.value.orEmpty()
		val identifier = chatRoom.identifier
		val found = currentList.find {
			it.chatRoom.identifier == identifier
		}
		if (found != null) {
			val newList = arrayListOf<ConversationModel>()
			newList.addAll(currentList)
			newList.remove(found)
			found.destroy()
			Log.i("$TAG Removing chat room with identifier [$identifier] from list")
			conversations.postValue(newList)
		} else {
			Log.w(
				"$TAG Failed to find item in list matching deleted chat room identifier [$identifier]"
			)
		}
		
		showGreenToast(R.string.conversation_deleted_toast, R.drawable.chat_teardrop_text)
	}
	*/
	
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
						fromAddressFriend = String(message.fromAddress!.asStringUriOnly().dropFirst(4)) + ": "
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
		currentFilter = filter
		conversationsList.removeAll()
		conversationsListTmp.forEach { conversation in
			if conversation.subject.lowercased().contains(filter.lowercased()) || !conversation.participantsAddress.filter({ $0.lowercased().contains(filter.lowercased()) }).isEmpty {
				conversationsList.append(conversation)
			}
		}
	}
	
	func resetFilterConversations() {
		currentFilter = ""
		conversationsList = conversationsListTmp
	}
}
// swiftlint:enable line_length
