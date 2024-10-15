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

// swiftlint:disable line_length
class ConversationForwardMessageViewModel: ObservableObject {
	
	static let TAG = "[ConversationForwardMessageViewModel]"
	
	@Published var searchField: String = ""
	
	@Published var operationInProgress: Bool = false
	
	@Published var selectedMessage: EventLogMessage?
	
	@Published var conversationsList: [ConversationModel] = []
	var conversationsListTmp: [ConversationModel] = []
	
	@Published var displayedConversation: ConversationModel?
	
	private var chatRoomDelegate: ChatRoomDelegate?
	
	init() {}
	
	func initConversationsLists(convsList: [ConversationModel]) {
		conversationsListTmp = convsList
		conversationsList = convsList
		searchField = ""
		operationInProgress = false
		selectedMessage = nil
	}
	
	func filterConversations() {
		conversationsList.removeAll()
		conversationsListTmp.forEach { conversation in
			if conversation.subject.lowercased().contains(searchField.lowercased()) || !conversation.participantsAddress.filter({ $0.lowercased().contains(searchField.lowercased()) }).isEmpty {
				conversationsList.append(conversation)
			}
		}
	}
	
	func resetFilterConversations() {
		conversationsList = conversationsListTmp
	}
	
	func changeChatRoom(model: ConversationModel) {
		if 	self.operationInProgress == false {
			DispatchQueue.main.async {
				self.operationInProgress = true
			}
			
			DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
				self.operationInProgress = false
				self.displayedConversation = model
			}
		} else {
			DispatchQueue.main.async {
				self.operationInProgress = false
				self.displayedConversation = model
			}
		}
	}
	
	func createOneToOneChatRoomWith(remote: Address) {
		CoreContext.shared.doOnCoreQueue { core in
			let account = core.defaultAccount
			if account == nil {
				Log.error(
					"\(StartConversationViewModel.TAG) No default account found, can't create conversation with \(remote.asStringUriOnly())!"
				)
				return
			}
			
			DispatchQueue.main.async {
				self.operationInProgress = true
			}
			
			do {
				let params: ChatRoomParams = try core.createDefaultChatRoomParams()
				params.groupEnabled = false
				params.subject = "Dummy subject"
				params.ephemeralLifetime = 0 // Make sure ephemeral is disabled by default
				
				let sameDomain = remote.domain == account?.params?.domain ?? ""
				if StartConversationViewModel.isEndToEndEncryptionMandatory() && sameDomain {
					Log.info("\(StartConversationViewModel.TAG) Account is in secure mode & domain matches, creating a E2E conversation")
					params.backend = ChatRoom.Backend.FlexisipChat
					params.encryptionEnabled = true
				} else if !StartConversationViewModel.isEndToEndEncryptionMandatory() {
					if LinphoneUtils.isEndToEndEncryptedChatAvailable(core: core) {
						Log.info(
							"\(StartConversationViewModel.TAG) Account is in interop mode but LIME is available, creating a E2E conversation"
						)
						params.backend = ChatRoom.Backend.FlexisipChat
						params.encryptionEnabled = true
					} else {
						Log.info(
							"\(StartConversationViewModel.TAG) Account is in interop mode but LIME isn't available, creating a SIP simple conversation"
						)
						params.backend = ChatRoom.Backend.Basic
						params.encryptionEnabled = false
					}
				} else {
					Log.error(
						"\(StartConversationViewModel.TAG) Account is in secure mode, can't chat with SIP address of different domain \(remote.asStringUriOnly())"
					)
					DispatchQueue.main.async {
						self.operationInProgress = false
						ToastViewModel.shared.toastMessage = "Failed_to_create_conversation_invalid_participant_error"
						ToastViewModel.shared.displayToast = true
					}
					return
				}
				
				let participants = [remote]
				let localAddress = account?.params?.identityAddress
				let existingChatRoom = core.searchChatRoom(params: params, localAddr: localAddress, remoteAddr: nil, participants: participants)
				if existingChatRoom == nil {
					Log.info(
						"\(StartConversationViewModel.TAG) No existing 1-1 conversation between local account "
						+ "\(localAddress?.asStringUriOnly() ?? "") and remote \(remote.asStringUriOnly()) was found for given parameters, let's create it"
					)
					let chatRoom = try core.createChatRoom(params: params, localAddr: localAddress, participants: participants)
					if params.backend == ChatRoom.Backend.FlexisipChat {
						if chatRoom.state == ChatRoom.State.Created {
							let id = LinphoneUtils.getChatRoomId(room: chatRoom)
							Log.info("\(StartConversationViewModel.TAG) 1-1 conversation \(id) has been created")
							
							let model = ConversationModel(chatRoom: chatRoom)
							if 	self.operationInProgress == false {
								DispatchQueue.main.async {
									self.operationInProgress = true
								}
								
								DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
									self.operationInProgress = false
									self.displayedConversation = model
								}
							} else {
								DispatchQueue.main.async {
									self.operationInProgress = false
									self.displayedConversation = model
								}
							}
						} else {
							Log.info("\(StartConversationViewModel.TAG) Conversation isn't in Created state yet, wait for it")
							self.chatRoomAddDelegate(core: core, chatRoom: chatRoom)
						}
					} else {
						let id = LinphoneUtils.getChatRoomId(room: chatRoom)
						Log.info("\(StartConversationViewModel.TAG) Conversation successfully created \(id)")
						
						let model = ConversationModel(chatRoom: chatRoom)
						if 	self.operationInProgress == false {
							DispatchQueue.main.async {
								self.operationInProgress = true
							}
							
							DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
								self.operationInProgress = false
								self.displayedConversation = model
							}
						} else {
							DispatchQueue.main.async {
								self.operationInProgress = false
								self.displayedConversation = model
							}
						}
					}
				} else {
					Log.warn(
						"\(StartConversationViewModel.TAG) A 1-1 conversation between local account \(localAddress?.asStringUriOnly() ?? "") and remote \(remote.asStringUriOnly()) for given parameters already exists!"
					)
					
					let model = ConversationModel(chatRoom: existingChatRoom!)
					if 	self.operationInProgress == false {
						DispatchQueue.main.async {
							self.operationInProgress = true
						}
						
						DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
							self.operationInProgress = false
							self.displayedConversation = model
						}
					} else {
						DispatchQueue.main.async {
							self.operationInProgress = false
							self.displayedConversation = model
						}
					}
				}
			} catch {
				DispatchQueue.main.async {
					self.operationInProgress = false
					ToastViewModel.shared.toastMessage = "Failed_to_create_conversation_error"
					ToastViewModel.shared.displayToast = true
				}
				Log.error("\(StartConversationViewModel.TAG) Failed to create 1-1 conversation with \(remote.asStringUriOnly())!")
			}
		}
	}
	
	func chatRoomAddDelegate(core: Core, chatRoom: ChatRoom) {
		self.chatRoomDelegate = ChatRoomDelegateStub(onStateChanged: { (chatRoom: ChatRoom, state: ChatRoom.State) in
			let state = chatRoom.state
			let id = LinphoneUtils.getChatRoomId(room: chatRoom)
			if state == ChatRoom.State.CreationFailed {
				Log.error("\(StartConversationViewModel.TAG) Conversation \(id) creation has failed!")
				if let chatRoomDelegate = self.chatRoomDelegate {
					chatRoom.removeDelegate(delegate: chatRoomDelegate)
				}
				self.chatRoomDelegate = nil
				DispatchQueue.main.async {
					self.operationInProgress = false
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
				if let chatRoomDelegate = self.chatRoomDelegate {
					chatRoom.removeDelegate(delegate: chatRoomDelegate)
				}
				self.chatRoomDelegate = nil
				
				let model = ConversationModel(chatRoom: chatRoom)
				if 	self.operationInProgress == false {
					DispatchQueue.main.async {
						self.operationInProgress = true
					}
					
					DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
						self.operationInProgress = false
						self.displayedConversation = model
					}
				} else {
					DispatchQueue.main.async {
						self.operationInProgress = false
						self.displayedConversation = model
					}
				}
			} else if state == ChatRoom.State.CreationFailed {
				Log.error("\(StartConversationViewModel.TAG) Conversation \(id) creation has failed!")
				if let chatRoomDelegate = self.chatRoomDelegate {
					chatRoom.removeDelegate(delegate: chatRoomDelegate)
				}
				self.chatRoomDelegate = nil
				DispatchQueue.main.async {
					self.operationInProgress = false
					ToastViewModel.shared.toastMessage = "Failed_to_create_conversation_error"
					ToastViewModel.shared.displayToast = true
				}
			}
		})
		chatRoom.addDelegate(delegate: self.chatRoomDelegate!)
	}
	
	func forwardMessage() {
		CoreContext.shared.doOnCoreQueue { _ in
			if self.displayedConversation != nil && self.selectedMessage != nil {
				let chatMessageToDisplay = self.displayedConversation!.chatRoom.findEventLog(messageId: self.selectedMessage!.eventModel.eventLogId)?.chatMessage
				if let messageToForward = chatMessageToDisplay {
					DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
						do {
							let forwardedMessage = try self.displayedConversation!.chatRoom.createForwardMessage(message: messageToForward)
							Log.info("\(ConversationForwardMessageViewModel.TAG) Sending forwarded message")
							forwardedMessage.send()
							
						} catch let error {
							print("\(#function) - Failed to create forward message: \(error)")
						}
						
						self.selectedMessage = nil
						self.displayedConversation = nil
					}
					/*
					showGreenToastEvent.postValue(
						Event(Pair(R.string.conversation_message_forwarded_toast, R.drawable.forward))
					)
					 */
				}
			}
		}
 }
}

// swiftlint:enable line_length
