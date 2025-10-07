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

import linphonesw
import Combine
import SwiftUI

// swiftlint:disable line_length
class ContactsListViewModel: ObservableObject {
	@Published var selectedEditFriend: ContactAvatarModel?
	
	var stringToCopy: String = ""
    
    var selectedFriend: ContactAvatarModel?
    var selectedFriendToShare: ContactAvatarModel?
	var selectedFriendToDelete: ContactAvatarModel?
	
	@Published var displayedConversation: ConversationModel?
	
	private var contactChatRoomDelegate: ChatRoomDelegate?
	
	private let nativeAddressBookFriendList = "Native address-book"
	let linphoneAddressBookFriendList = "Linphone address-book"
	let tempRemoteAddressBookFriendList = "TempRemoteDirectoryContacts address-book"
	
	init() {}
	
	func createOneToOneChatRoomWith(remote: Address) {
		CoreContext.shared.doOnCoreQueue { core in
			let account = core.defaultAccount
			if account == nil {
				Log.error(
					"\(ConversationForwardMessageViewModel.TAG) No default account found, can't create conversation with \(remote.asStringUriOnly())"
				)
				return
			}
			
			DispatchQueue.main.async {
				SharedMainViewModel.shared.operationInProgress = true
			}
			
			do {
				let params = try core.createConferenceParams(conference: nil)
				params.chatEnabled = true
				params.groupEnabled = false
				params.subject = NSLocalizedString("conversation_one_to_one_hidden_subject", comment: "")
				params.account = account
				
				guard let chatParams = params.chatParams else { return }
				chatParams.ephemeralLifetime = 0 // Make sure ephemeral is disabled by default
				
				let sameDomain = remote.domain == CorePreferences.defaultDomain && remote.domain == account!.params?.domain
				if account!.params != nil && (account!.params!.instantMessagingEncryptionMandatory && sameDomain) {
					Log.info("\(ConversationForwardMessageViewModel.TAG) Account is in secure mode & domain matches, creating an E2E encrypted conversation")
					chatParams.backend = ChatRoom.Backend.FlexisipChat
					params.securityLevel = Conference.SecurityLevel.EndToEnd
				} else if account!.params != nil && (!account!.params!.instantMessagingEncryptionMandatory) {
					if LinphoneUtils.isEndToEndEncryptedChatAvailable(core: core) {
						Log.info(
							"\(ConversationForwardMessageViewModel.TAG) Account is in interop mode but LIME is available, creating an E2E encrypted conversation"
						)
						chatParams.backend = ChatRoom.Backend.FlexisipChat
						params.securityLevel = Conference.SecurityLevel.EndToEnd
					} else {
						Log.info(
							"\(ConversationForwardMessageViewModel.TAG) Account is in interop mode but LIME isn't available, creating a SIP simple conversation"
						)
						chatParams.backend = ChatRoom.Backend.Basic
						params.securityLevel = Conference.SecurityLevel.None
					}
				} else {
					Log.error(
						"\(ConversationForwardMessageViewModel.TAG) Account is in secure mode, can't chat with SIP address of different domain \(remote.asStringUriOnly())"
					)
					
					DispatchQueue.main.async {
						SharedMainViewModel.shared.operationInProgress = false
						ToastViewModel.shared.toastMessage = "Failed_to_create_conversation_error"
						ToastViewModel.shared.displayToast = true
					}
					return
				}
				
				let participants = [remote]
				let localAddress = account?.params?.identityAddress
				let existingChatRoom = core.searchChatRoom(params: params, localAddr: localAddress, remoteAddr: nil, participants: participants)
				if existingChatRoom == nil {
					Log.info(
						"\(ConversationForwardMessageViewModel.TAG) No existing 1-1 conversation between local account \(localAddress?.asStringUriOnly() ?? "") and remote \(remote.asStringUriOnly()) was found for given parameters, let's create it"
					)
					
					do {
						let chatRoom = try core.createChatRoom(params: params, participants: participants)
						if chatParams.backend == ChatRoom.Backend.FlexisipChat {
							let state = chatRoom.state
							if state == ChatRoom.State.Created {
								let chatRoomId = LinphoneUtils.getConversationId(chatRoom: chatRoom)
								Log.info("\(ConversationForwardMessageViewModel.TAG) 1-1 conversation \(chatRoomId) has been created")
								
								let model = ConversationModel(chatRoom: chatRoom)
								DispatchQueue.main.async {
									self.displayedConversation = model
									SharedMainViewModel.shared.operationInProgress = false
								}
							} else {
								Log.info("\(ConversationForwardMessageViewModel.TAG) Conversation isn't in Created state yet (state is \(state)), wait for it")
								self.chatRoomAddDelegate(core: core, chatRoom: chatRoom)
							}
						} else {
							let chatRoomId = LinphoneUtils.getConversationId(chatRoom: chatRoom)
							Log.info("\(ConversationForwardMessageViewModel.TAG) Conversation successfully created \(chatRoomId)")
							
							let model = ConversationModel(chatRoom: chatRoom)
							DispatchQueue.main.async {
								self.displayedConversation = model
								SharedMainViewModel.shared.operationInProgress = false
							}
						}
					} catch {
						Log.error("\(ConversationForwardMessageViewModel.TAG) Failed to create 1-1 conversation with \(remote.asStringUriOnly())")
						
						DispatchQueue.main.async {
							SharedMainViewModel.shared.operationInProgress = false
							ToastViewModel.shared.toastMessage = "Failed_to_create_conversation_error"
							ToastViewModel.shared.displayToast = true
						}
					}
				} else {
					Log.warn(
						"\(ConversationForwardMessageViewModel.TAG) A 1-1 conversation between local account \(localAddress?.asStringUriOnly() ?? "") and remote \(remote.asStringUriOnly()) for given parameters already exists!"
					)
					let model = ConversationModel(chatRoom: existingChatRoom!)
					DispatchQueue.main.async {
						self.displayedConversation = model
						SharedMainViewModel.shared.operationInProgress = false
					}
				}
			} catch {
			}
		}
	}
	
	func chatRoomAddDelegate(core: Core, chatRoom: ChatRoom) {
		contactChatRoomDelegate = ChatRoomDelegateStub(onStateChanged: { (chatRoom: ChatRoom, state: ChatRoom.State) in
			let state = chatRoom.state
			let id = LinphoneUtils.getChatRoomId(room: chatRoom)
			if state == ChatRoom.State.CreationFailed {
				Log.error("\(StartConversationViewModel.TAG) Conversation \(id) creation has failed!")
				if let delegate = self.contactChatRoomDelegate {
					chatRoom.removeDelegate(delegate: delegate)
					self.contactChatRoomDelegate = nil
				}
				DispatchQueue.main.async {
					SharedMainViewModel.shared.operationInProgress = false
					ToastViewModel.shared.toastMessage = "Failed_to_create_conversation_error"
					ToastViewModel.shared.displayToast = true
				}
			}
		}, onConferenceJoined: { (chatRoom: ChatRoom, _: EventLog) in
			let state = chatRoom.state
			let id = LinphoneUtils.getChatRoomId(room: chatRoom)
			Log.info("\(StartConversationViewModel.TAG) Conversation \(id) \(chatRoom.subject ?? "") state changed: \(state)")
			if state == ChatRoom.State.Created {
				Log.info("\(StartConversationViewModel.TAG) Conversation \(id) successfully created")
				if let delegate = self.contactChatRoomDelegate {
					chatRoom.removeDelegate(delegate: delegate)
					self.contactChatRoomDelegate = nil
				}
				let model = ConversationModel(chatRoom: chatRoom)
				if 	SharedMainViewModel.shared.operationInProgress == false {
					DispatchQueue.main.async {
						SharedMainViewModel.shared.operationInProgress = true
					}
					
					DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
						SharedMainViewModel.shared.operationInProgress = false
						self.displayedConversation = model
					}
				} else {
					DispatchQueue.main.async {
						SharedMainViewModel.shared.operationInProgress = false
						self.displayedConversation = model
					}
				}
			} else if state == ChatRoom.State.CreationFailed {
				Log.error("\(StartConversationViewModel.TAG) Conversation \(id) creation has failed!")
				
				if let delegate = self.contactChatRoomDelegate {
					chatRoom.removeDelegate(delegate: delegate)
					self.contactChatRoomDelegate = nil
				}
				DispatchQueue.main.async {
					SharedMainViewModel.shared.operationInProgress = false
					ToastViewModel.shared.toastMessage = "Failed_to_create_conversation_error"
					ToastViewModel.shared.displayToast = true
				}
			}
		})
		chatRoom.addDelegate(delegate: contactChatRoomDelegate!)
	}
	
	func deleteSelectedContact() {
		CoreContext.shared.doOnCoreQueue { core in
			if self.selectedFriendToDelete != nil && self.selectedFriendToDelete!.friend != nil {
				if SharedMainViewModel.shared.displayedFriend != nil {
					DispatchQueue.main.async {
						withAnimation {
							SharedMainViewModel.shared.displayedFriend = nil
						}
					}
				}
				self.selectedFriendToDelete!.friend!.remove()
			} else if SharedMainViewModel.shared.displayedFriend != nil {
				DispatchQueue.main.async {
					withAnimation {
						SharedMainViewModel.shared.displayedFriend = nil
					}
				}
				SharedMainViewModel.shared.displayedFriend!.friend!.remove()
			}
			
			MagicSearchSingleton.shared.searchForContacts()
		}
	}
	
	func changeSelectedFriendToDelete() {
		if selectedFriend != nil {
			self.selectedFriendToDelete = self.selectedFriend
		}
	}
	
	func toggleStarredSelectedFriend() {
		CoreContext.shared.doOnCoreQueue { core in
			if let contactAvatar = self.selectedFriend, let friend = contactAvatar.friend {
				friend.edit()
				friend.starred.toggle()
				friend.done()
				
				let starredTmp = friend.starred
				
				DispatchQueue.main.async {
					contactAvatar.starred = starredTmp
				}
			} else if let displayedFriend = SharedMainViewModel.shared.displayedFriend, let friend = displayedFriend.friend {
				friend.edit()
				friend.starred.toggle()
				friend.done()
				
				let starredTmp = friend.starred
				
				DispatchQueue.main.async {
					displayedFriend.starred = starredTmp
				}
			}
		}
	}
}
// swiftlint:enable line_length
