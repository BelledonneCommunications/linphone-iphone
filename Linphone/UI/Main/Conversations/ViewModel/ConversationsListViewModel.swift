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
import SwiftUI

// swiftlint:disable line_length
class ConversationsListViewModel: ObservableObject {
	
	static let TAG = "[ConversationsListViewModel]"
	
	private var coreContext = CoreContext.shared
	private var contactsManager = ContactsManager.shared
	private var sharedMainViewModel = SharedMainViewModel.shared
	
	private var coreConversationDelegate: CoreDelegate?
	
	@Published var conversationsList: [ConversationModel] = []
	
	var selectedConversation: ConversationModel?
	
	var currentFilter: String = ""
	
	init() {
		computeChatRoomsList()
		addConversationDelegate()
	}
	
	func computeChatRoomsList() {
		coreContext.doOnCoreQueue { core in
			if let account = core.defaultAccount {
				let conversationsListTmp = account.filterChatRooms(filter: self.currentFilter)
				var conversationsTmp: [ConversationModel] = []
				conversationsListTmp.forEach { chatRoom in
					let model = ConversationModel(chatRoom: chatRoom)
					conversationsTmp.append(model)
				}
				
				DispatchQueue.main.async {
					self.conversationsList = conversationsTmp
				}
				
                SharedMainViewModel.shared.updateUnreadMessagesCount()
			}
		}
	}
	
	func updateChatRoomsList() {
		CoreContext.shared.doOnCoreQueue { _ in
			if !self.conversationsList.isEmpty {
				self.contactsManager.avatarListModel.forEach { contactAvatarModel in
					self.conversationsList.forEach { conversationModel in
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
				self.conversationsList.forEach { conversationModel in
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
			self.coreConversationDelegate = CoreDelegateStub(onMessagesReceived: { (core: Core, chatRoom: ChatRoom, _: [ChatMessage]) in
                if let defaultAddress = core.defaultAccount?.contactAddress,
                   let localAddress = chatRoom.localAddress,
                   defaultAddress.weakEqual(address2: localAddress) {
                    let idTmp = LinphoneUtils.getChatRoomId(room: chatRoom)
                    let model = self.conversationsList.first(where: { $0.id == idTmp }) ?? ConversationModel(chatRoom: chatRoom)
                    model.getContentTextMessage(chatRoom: chatRoom)
                    let index = self.conversationsList.firstIndex(where: { $0.id == idTmp })
                    DispatchQueue.main.async {
                        if index != nil {
                            self.conversationsList.remove(at: index!)
                        }
                        self.conversationsList.insert(model, at: 0)
                    }
                    SharedMainViewModel.shared.updateUnreadMessagesCount()
                }
			}, onMessageSent: { (_: Core, chatRoom: ChatRoom, _: ChatMessage) in
                if let defaultAddress = core.defaultAccount?.contactAddress,
                   let localAddress = chatRoom.localAddress,
                   defaultAddress.weakEqual(address2: localAddress) {
                    let idTmp = LinphoneUtils.getChatRoomId(room: chatRoom)
                    let model = self.conversationsList.first(where: { $0.id == idTmp }) ?? ConversationModel(chatRoom: chatRoom)
                    model.getContentTextMessage(chatRoom: chatRoom)
                    let index = self.conversationsList.firstIndex(where: { $0.id == idTmp })
                    if index != nil {
                        self.conversationsList[index!].chatMessageRemoveDelegate()
                    }
                    DispatchQueue.main.async {
                        if index != nil {
                            self.conversationsList.remove(at: index!)
                        }
                        self.conversationsList.insert(model, at: 0)
                    }
                    SharedMainViewModel.shared.updateUnreadMessagesCount()
                }
			}, onChatRoomRead: { (_: Core, chatRoom: ChatRoom) in
				let idTmp = LinphoneUtils.getChatRoomId(room: chatRoom)
				let model = self.conversationsList.first(where: { $0.id == idTmp }) ?? ConversationModel(chatRoom: chatRoom)
				model.getContentTextMessage(chatRoom: chatRoom)
				if let index = self.conversationsList.firstIndex(where: { $0.id == idTmp }) {
					DispatchQueue.main.async {
						self.conversationsList.remove(at: index)
						self.conversationsList.insert(model, at: index)
					}
				}
				SharedMainViewModel.shared.updateUnreadMessagesCount()
			}, onChatRoomStateChanged: { (core: Core, chatroom: ChatRoom, state: ChatRoom.State) in
				// Log.info("[ConversationsListViewModel] Conversation [${LinphoneUtils.getChatRoomId(chatRoom)}] state changed [$state]")
                if let defaultAddress = core.defaultAccount?.contactAddress,
                   let localAddress = chatroom.localAddress,
                   defaultAddress.weakEqual(address2: localAddress) {
                    if core.globalState == .On {
                        switch state {
                        case .Created:
                            self.addChatRoom(chatRoom: chatroom)
                        case .Deleted:
                            self.removeChatRoom(chatRoom: chatroom)
                        default:
                            break
                        }
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
		
		let currentList = conversationsList
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
	
	private func removeChatRoom(chatRoom: ChatRoom) {
		let currentList = conversationsList
		let identifier = chatRoom.identifier
		let foundIndex = currentList.firstIndex(where: {$0.chatRoom.identifier == identifier})
		if foundIndex != nil {
			var newList: [ConversationModel] = []
			newList.append(contentsOf: currentList)
			newList.remove(at: foundIndex!)
			Log.info("\(ConversationsListViewModel.TAG) Removing chat room with identifier \(identifier ?? "Identifier error") from list")
			
			DispatchQueue.main.async {
				self.conversationsList = newList
			}
		} else {
			Log.warn(
				"\(ConversationsListViewModel.TAG) Failed to find item in list matching deleted chat room identifier \(identifier ?? "Identifier error")"
			)
		}
	}
	
	func reorderChatRooms() {
		Log.info("[ConversationsListViewModel] Re-ordering conversations")
		var sortedList: [ConversationModel] = []
		sortedList.append(contentsOf: conversationsList)
		
		DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
			self.conversationsList = sortedList.sorted { $0.lastUpdateTime > $1.lastUpdateTime }
		}
		
        SharedMainViewModel.shared.updateUnreadMessagesCount()
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
				SharedMainViewModel.shared.updateUnreadMessagesCount()
                DispatchQueue.main.async {
                    self.selectedConversation!.unreadMessagesCount = 0
                }
			}
		}
	}
	
	func filterConversations(filter: String) {
		currentFilter = filter
		coreContext.doOnCoreQueue { core in
			if let account = core.defaultAccount {
				let conversationsListTmp = account.filterChatRooms(filter: filter)
				var conversationsTmp: [ConversationModel] = []
				conversationsListTmp.forEach { chatRoom in
					let model = ConversationModel(chatRoom: chatRoom)
					conversationsTmp.append(model)
				}
				
				DispatchQueue.main.async {
					self.conversationsList = conversationsTmp
				}
			}
		}
	}
	
	func resetFilterConversations() {
		filterConversations(filter: "")
	}
	
	func getChatRoomWithStringAddress(stringAddr: String) {
		CoreContext.shared.doOnCoreQueue { _ in
			do {
				let stringAddrCleaned = stringAddr.components(separatedBy: ";gr=")
				let address = try Factory.Instance.createAddress(addr: stringAddrCleaned[0])
				if let dispChatRoom = self.conversationsList.first(where: {$0.chatRoom.peerAddress != nil && $0.chatRoom.peerAddress!.equal(address2: address)}) {
					if self.sharedMainViewModel.displayedConversation != nil {
						if dispChatRoom.id != self.sharedMainViewModel.displayedConversation!.id {
							DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
								self.changeDisplayedChatRoom(conversationModel: dispChatRoom)
							}
						}
					} else {
						DispatchQueue.main.async {
							self.changeDisplayedChatRoom(conversationModel: dispChatRoom)
						}
					}
				}
			} catch {
			}
		}
	}
	
	func changeDisplayedChatRoom(conversationModel: ConversationModel) {
		CoreContext.shared.doOnCoreQueue { core in
			let nilParams: ConferenceParams? = nil
			if let newChatRoom = core.searchChatRoomByIdentifier(identifier: conversationModel.id) {
				if self.sharedMainViewModel.displayedConversation == nil {
					DispatchQueue.main.async {
						withAnimation {
							self.sharedMainViewModel.displayedConversation = conversationModel
						}
					}
				} else {
					DispatchQueue.main.async {
						self.sharedMainViewModel.displayedConversation = nil
						DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
							withAnimation {
								self.sharedMainViewModel.displayedConversation = conversationModel
							}
						}
					}
				}
			} else {
				Log.warn("\(ConversationsListViewModel.TAG) changeDisplayedChatRoom: no chat room found for identifier \(conversationModel.id)")
			}
		}
	}
}
// swiftlint:enable line_length
