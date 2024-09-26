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
class StartConversationViewModel: ObservableObject {
	
	static let TAG = "[StartConversationViewModel]"
	
	private var coreContext = CoreContext.shared
	
	@Published var searchField: String = ""
	
	var domain: String = ""
	
	@Published var messageText: String = ""
	
	@Published var participants: [SelectedAddressModel] = []
	
	@Published var operationInProgress: Bool = false
	@Published var displayedConversation: ConversationModel?
	
	private var chatRoomDelegate: ChatRoomDelegate?
	
	init() {
		coreContext.doOnCoreQueue { core in
			self.domain = core.defaultAccount?.params?.domain ?? ""
		}
	}
	
	func addParticipants(participantsToAdd: [SelectedAddressModel]) {
		var list = participants
		for selectedAddr in participantsToAdd {
			if let found = list.first(where: { $0.address.weakEqual(address2: selectedAddr.address) }) {
				Log.info("\(StartConversationViewModel.TAG) Participant \(found.address.asStringUriOnly()) already in list, skipping")
				continue
			}
			
			list.append(selectedAddr)
			Log.info("\(StartConversationViewModel.TAG) Added participant \(selectedAddr.address.asStringUriOnly())")
		}
		Log.info("\(StartConversationViewModel.TAG) [\(list.count - participants.count) participants added, now there are \(list.count) participants in list")
		
		participants = list
	}
	
	func createGroupChatRoom() {
		coreContext.doOnCoreQueue { core in
			let account = core.defaultAccount
			if account == nil {
				Log.error(
					"\(StartConversationViewModel.TAG) No default account found, can't create group conversation!"
				)
				return
			}
			
			DispatchQueue.main.async {
				self.operationInProgress = true
			}
			
			let groupChatRoomSubject = self.messageText
			do {
				let params: ChatRoomParams = try core.createDefaultChatRoomParams()
				params.groupEnabled = true
				params.subject = groupChatRoomSubject
				params.backend = ChatRoom.Backend.FlexisipChat
				params.encryptionEnabled = true
				
				var participantsTmp: [Address] = []
				self.participants.forEach { participant in
					participantsTmp.append(participant.address)
				}
				
				if account!.params != nil {
					let localAddress = account!.params!.identityAddress
					
					let chatRoom = try core.createChatRoom(
						params: params,
						localAddr: localAddress,
						participants: participantsTmp
					)
					
					if params.backend == ChatRoom.Backend.FlexisipChat {
						if chatRoom.state == ChatRoom.State.Created {
							let id = LinphoneUtils.getChatRoomId(room: chatRoom)
							Log.info(
								"\(StartConversationViewModel.TAG) Group conversation \(id) \(groupChatRoomSubject) has been created"
							)
							
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
							Log.info(
								"\(StartConversationViewModel.TAG) Conversation \(groupChatRoomSubject) isn't in Created state yet, wait for it"
							)
							self.chatRoomAddDelegate(core: core, chatRoom: chatRoom)
						}
					} else {
						let id = LinphoneUtils.getChatRoomId(room: chatRoom)
						Log.info("\(StartConversationViewModel.TAG) Conversation successfully created \(id) \(groupChatRoomSubject)")
						
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
				}
			} catch let error {
				Log.error("\(StartConversationViewModel.TAG) Failed to create group conversation \(groupChatRoomSubject)!")
				Log.error("\(StartConversationViewModel.TAG) \(error)")
				
				DispatchQueue.main.async {
					self.operationInProgress = false
					ToastViewModel.shared.toastMessage = "Failed_to_create_conversation_error"
					ToastViewModel.shared.displayToast = true
				}
			}
		}
	}
	
	func createOneToOneChatRoomWith(remote: Address) {
		coreContext.doOnCoreQueue { core in
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
	
	public static func isEndToEndEncryptionMandatory() -> Bool {
		return false // TODO: Will be done later in SDK
	}
}

// swiftlint:enable line_length
