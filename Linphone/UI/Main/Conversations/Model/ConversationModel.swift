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
import Combine

// swiftlint:disable line_length
class ConversationModel: ObservableObject {
	
	private var coreContext = CoreContext.shared
	private var contactsManager = ContactsManager.shared
	
	let chatRoom: ChatRoom
	let isDisabledBecauseNotSecured: Bool = false
	
	static let TAG = "[Conversation Model]"
	
	let id: String
	let localSipUri: String
	let remoteSipUri: String
	let isGroup: Bool
	let isReadOnly: Bool
	@Published var subject: String
	@Published var participantsAddress: [String] = []
	@Published var isComposing: Bool
	@Published var lastUpdateTime: time_t
	@Published var isMuted: Bool
	@Published var isEphemeral: Bool
	@Published var encryptionEnabled: Bool
	@Published var lastMessageText: String
	@Published var lastMessageIsOutgoing: Bool
	@Published var lastMessageState: Int
	@Published var unreadMessagesCount: Int
	@Published var avatarModel: ContactAvatarModel
	
	init(chatRoom: ChatRoom) {
		self.chatRoom = chatRoom
		
		self.id = LinphoneUtils.getChatRoomId(room: chatRoom)
		
		self.localSipUri = chatRoom.localAddress?.asStringUriOnly() ?? ""
		
		self.remoteSipUri = chatRoom.peerAddress?.asStringUriOnly() ?? ""
		
		self.isGroup = !chatRoom.hasCapability(mask: ChatRoom.Capabilities.OneToOne.rawValue) && chatRoom.hasCapability(mask: ChatRoom.Capabilities.Conference.rawValue)

		self.isReadOnly = chatRoom.isReadOnly

		self.subject = chatRoom.subject ?? ""

		self.lastUpdateTime = chatRoom.lastUpdateTime

		self.isComposing = chatRoom.isRemoteComposing

		self.isMuted = chatRoom.muted

		self.isEphemeral = chatRoom.ephemeralEnabled
		
		self.encryptionEnabled = chatRoom.currentParams != nil && chatRoom.currentParams!.encryptionEnabled
		
		self.lastMessageText = ""
		
		self.lastMessageIsOutgoing = false
		
		self.lastMessageState = 0

		self.unreadMessagesCount = 0

		self.avatarModel = ContactAvatarModel(friend: nil, name: chatRoom.subject ?? "", address: chatRoom.peerAddress?.asStringUriOnly() ?? "", withPresence: false)
		
		getContentTextMessage()
		getChatRoomSubject()
		getUnreadMessagesCount()
	}
	
	func leave() {
		coreContext.doOnCoreQueue { _ in
			self.chatRoom.leave()
		}
	}
	
	func toggleMute() {
		coreContext.doOnCoreQueue { _ in
			self.chatRoom.muted.toggle()
			self.isMuted = self.chatRoom.muted
		}
	}
	
	func call() {
		coreContext.doOnCoreQueue { _ in
			if self.chatRoom.peerAddress != nil {
				TelecomManager.shared.doCallOrJoinConf(address: self.chatRoom.peerAddress!)
			}
		}
	}
	
	func getContentTextMessage() {
		coreContext.doOnCoreQueue { _ in
			let lastMessage = self.chatRoom.lastMessageInHistory
			if lastMessage != nil {
				var fromAddressFriend = lastMessage!.fromAddress != nil
				? self.contactsManager.getFriendWithAddress(address: lastMessage!.fromAddress)?.name ?? nil
				: nil
				
				if !lastMessage!.isOutgoing && lastMessage!.chatRoom != nil && !lastMessage!.chatRoom!.hasCapability(mask: ChatRoom.Capabilities.OneToOne.rawValue) {
					if fromAddressFriend == nil {
						if lastMessage!.fromAddress!.displayName != nil {
							fromAddressFriend = lastMessage!.fromAddress!.displayName! + ": "
						} else if lastMessage!.fromAddress!.username != nil {
							fromAddressFriend = lastMessage!.fromAddress!.username! + ": "
						} else {
							fromAddressFriend = ""
						}
					} else {
						fromAddressFriend! += ": "
					}
					
				} else {
					fromAddressFriend = nil
				}
				
				let lastMessageTextTmp = (fromAddressFriend ?? "")
				+ (lastMessage!.contents.first(where: {$0.isText == true})?.utf8Text ?? (lastMessage!.contents.first(where: {$0.isFile == true || $0.isFileTransfer == true})?.name ?? ""))
				
				let lastMessageIsOutgoingTmp = lastMessage?.isOutgoing ?? false
				
				let lastMessageStateTmp = lastMessage?.state.rawValue ?? 0
				
				DispatchQueue.main.async {
					self.lastMessageText = lastMessageTextTmp
					
					self.lastMessageIsOutgoing = lastMessageIsOutgoingTmp
					
					self.lastMessageState = lastMessageStateTmp
				}
			}
		}
	}
	
	func getChatRoomSubject() {
		coreContext.doOnCoreQueue { _ in
			let addressFriend = (self.chatRoom.participants.first != nil && self.chatRoom.participants.first!.address != nil)
			? self.contactsManager.getFriendWithAddress(address: self.chatRoom.participants.first?.address)
			: nil
			
			var subjectTmp = ""
			
			if self.isGroup {
				subjectTmp = self.chatRoom.subject!
			} else if addressFriend != nil {
				subjectTmp = addressFriend!.name!
			} else {
				if self.chatRoom.participants.first != nil
					&& self.chatRoom.participants.first!.address != nil {
					
					subjectTmp = self.chatRoom.participants.first!.address!.displayName != nil
					? self.chatRoom.participants.first!.address!.displayName!
					: self.chatRoom.participants.first!.address!.username!
					
				}
			}
			
			let addressTmp = addressFriend?.address?.asStringUriOnly() ?? ""
			
			let avatarModelTmp = addressFriend != nil && !self.isGroup
			? ContactsManager.shared.avatarListModel.first(where: {
				$0.friend!.name == addressFriend!.name
				&& $0.friend!.address!.asStringUriOnly() == addressFriend!.address!.asStringUriOnly()
			})
			?? ContactAvatarModel(
				friend: nil,
				name: subjectTmp,
				address: addressTmp,
				withPresence: false
			)
			: ContactAvatarModel(
				friend: nil,
				name: subjectTmp,
				address: self.chatRoom.peerAddress?.asStringUriOnly() ?? addressTmp,
				withPresence: false
			)
			
			var participantsAddressTmp: [String] = []
			
			self.chatRoom.participants.forEach { participant in
				participantsAddressTmp.append(participant.address?.asStringUriOnly() ?? "")
			}
			
			DispatchQueue.main.async {
				self.subject = subjectTmp
				self.avatarModel = avatarModelTmp
				self.participantsAddress = participantsAddressTmp
			}
		}
	}
	
	func getUnreadMessagesCount() {
		coreContext.doOnCoreQueue { _ in
			self.unreadMessagesCount = self.chatRoom.unreadMessagesCount
		}
	}
	
	func refreshAvatarModel() {
		coreContext.doOnCoreQueue { _ in
			if !self.isGroup {
				if self.chatRoom.participants.first != nil && self.chatRoom.participants.first!.address != nil {
					ContactAvatarModel.getAvatarModelFromAddress(address: self.chatRoom.participants.first!.address!) { avatarResult in
						let avatarModelTmp = avatarResult
						let subjectTmp = avatarModelTmp.name
						
						DispatchQueue.main.async {
							self.avatarModel = avatarModelTmp
							self.subject = subjectTmp
						}
					}
				}
			}
		}
	}
	
	func downloadContent(chatMessage: ChatMessage, content: Content) {
		coreContext.doOnCoreQueue { _ in
			if !chatMessage.downloadContent(content: content) {
				Log.error("\(ConversationModel.TAG) An error occured when downloading content of chat message. MessageID=\(chatMessage.messageId)")
			}
		}
	}
	
	func deleteChatRoom() {
		CoreContext.shared.doOnCoreQueue { core in
			core.deleteChatRoom(chatRoom: self.chatRoom)
	   }
	}
}
// swiftlint:enable line_length
