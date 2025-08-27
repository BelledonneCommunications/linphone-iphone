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
class ConversationModel: ObservableObject, Identifiable {
	
	private var coreContext = CoreContext.shared
	private var contactsManager = ContactsManager.shared
	
	var chatRoom: ChatRoom
	var lastMessage: ChatMessage?
	
	let isDisabledBecauseNotSecured: Bool = false
	
	static let TAG = "[Conversation Model]"
	
	let id: String
	let localSipUri: String
	let remoteSipUri: String
	let isGroup: Bool
	@Published var isReadOnly: Bool
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
	
	private var conferenceDelegate: ConferenceDelegate?
	private var chatMessageDelegate: ChatMessageDelegate?
	
	init(chatRoom: ChatRoom) {
		self.chatRoom = chatRoom
		
		self.id = LinphoneUtils.getChatRoomId(room: chatRoom)
		
		self.localSipUri = chatRoom.localAddress?.asStringUriOnly() ?? ""
		
		self.remoteSipUri = chatRoom.peerAddress?.asStringUriOnly() ?? ""
		
		self.isGroup = !chatRoom.hasCapability(mask: ChatRoom.Capabilities.OneToOne.rawValue) && chatRoom.hasCapability(mask: ChatRoom.Capabilities.Conference.rawValue)

		self.isReadOnly = chatRoom.isReadOnly
		
		let chatRoomParticipants = chatRoom.participants
		let addressFriend = (chatRoomParticipants.first != nil && chatRoomParticipants.first!.address != nil)
		? self.contactsManager.getFriendWithAddress(address: chatRoomParticipants.first?.address)
		: nil
		
		var subjectTmp = ""
		
		if self.isGroup {
			subjectTmp = chatRoom.subject!
		} else if addressFriend != nil {
			subjectTmp = addressFriend!.name!
		} else {
			if chatRoomParticipants.first != nil
				&& chatRoomParticipants.first!.address != nil {
				
				subjectTmp = chatRoomParticipants.first!.address!.displayName != nil
				? chatRoomParticipants.first!.address!.displayName!
				: (chatRoomParticipants.first!.address!.username ?? String(chatRoomParticipants.first!.address!.asStringUriOnly().dropFirst(4)))
				
			}
		}
		
		let addressTmp = addressFriend?.address?.asStringUriOnly() ?? ""
		
		let avatarModelTmp: ContactAvatarModel
		if let addressFriend = addressFriend, !self.isGroup {
			if let existingAvatarModel = ContactsManager.shared.avatarListModel.first(where: {
				$0.friend?.name == addressFriend.name &&
				$0.friend?.address?.asStringUriOnly() == addressFriend.address?.asStringUriOnly()
			}) {
				avatarModelTmp = existingAvatarModel
			} else {
				avatarModelTmp = ContactAvatarModel(
					friend: nil,
					name: subjectTmp,
					address: addressTmp,
					withPresence: false
				)
			}
		} else {
			avatarModelTmp = ContactAvatarModel(
				friend: nil,
				name: subjectTmp,
				address: chatRoom.peerAddress?.asStringUriOnly() ?? addressTmp,
				withPresence: false
			)
		}
		
		var participantsAddressTmp: [String] = []
		
		self.chatRoom.participants.forEach { participant in
			participantsAddressTmp.append(participant.address?.asStringUriOnly() ?? "")
		}
		
		self.subject = subjectTmp
		self.avatarModel = avatarModelTmp
		self.participantsAddress = participantsAddressTmp

		self.lastUpdateTime = chatRoom.lastUpdateTime

		self.isComposing = chatRoom.isRemoteComposing

		self.isMuted = chatRoom.muted

		self.isEphemeral = chatRoom.ephemeralEnabled
		
		self.encryptionEnabled = chatRoom.currentParams != nil && chatRoom.currentParams!.encryptionEnabled
		
		self.lastMessage = nil
		
		self.lastMessageText = ""
		
		self.lastMessageIsOutgoing = false
		
		self.lastMessageState = 0

		self.unreadMessagesCount = chatRoom.unreadMessagesCount
		
		getContentTextMessage(chatRoom: chatRoom)
	}
	
	func leave() {
		coreContext.doOnCoreQueue { _ in
			self.chatRoom.leave()
		}
	}
	
	func toggleMute() {
		coreContext.doOnCoreQueue { _ in
			let chatRoomMuted = self.chatRoom.muted
			self.chatRoom.muted.toggle()
			DispatchQueue.main.async {
				self.isMuted = !chatRoomMuted
			}
		}
	}
	
	func call() {
		coreContext.doOnCoreQueue { core in
			if self.chatRoom.hasCapability(mask: ChatRoom.Capabilities.OneToOne.rawValue) && !self.chatRoom.hasCapability(mask: ChatRoom.Capabilities.Conference.rawValue) {
				TelecomManager.shared.doCallOrJoinConf(address: self.chatRoom.peerAddress!)
			} else if self.chatRoom.hasCapability(mask: ChatRoom.Capabilities.OneToOne.rawValue) && self.chatRoom.hasCapability(mask: ChatRoom.Capabilities.Conference.rawValue) {
				if self.chatRoom.participants.first != nil && self.chatRoom.participants.first!.address != nil {
					TelecomManager.shared.doCallOrJoinConf(address: self.chatRoom.participants.first!.address!)
				}
			} else {
				self.createGroupCall()
			}
		}
	}
	
	func createGroupCall() {
		coreContext.doOnCoreQueue { core in
			let account = core.defaultAccount
			if account == nil {
				Log.error(
					"\(ConversationModel.TAG) No default account found, can't create group call!"
				)
				return
			}
			
			do {
				var participantsList: [Address] = []
				self.chatRoom.participants.forEach { participant in
					participantsList.append(participant.address!)
				}
				
				Log.info(
					"\(ConversationModel.TAG) Creating group call with subject \(self.chatRoom.subject ?? "Conference") and \(participantsList.count) participant(s)"
				)
				if let conference = LinphoneUtils.createGroupCall(core: core, account: account, subject: self.chatRoom.subject ?? "Conference") {
					let callParams = try? core.createCallParams(call: nil)
					if let callParams = callParams {
						callParams.videoEnabled = true
						callParams.videoDirection = .RecvOnly
						
						Log.info("\(ConversationModel.TAG) Inviting \(participantsList.count) participant(s) into newly created conference")
                        
                        self.conferenceAddDelegate(core: core, conference: conference)
						
						try conference.inviteParticipants(addresses: participantsList, params: callParams)
						
						DispatchQueue.main.async {
							TelecomManager.shared.participantsInvited = true
						}
					}
				}
			} catch let error {
				Log.error(
					"\(ConversationModel.TAG) createGroupCall: \(error)"
				)
			}
		}
	}
	
	func conferenceAddDelegate(core: Core, conference: Conference) {
		self.conferenceDelegate = ConferenceDelegateStub(onStateChanged: { (conference: Conference, state: Conference.State) in
			Log.info("\(ConversationModel.TAG) Conference state is \(state)")
			if state == .Created {
				NotificationCenter.default.post(name: Notification.Name("CallViewModelReset"), object: self)
			} else if state == .CreationFailed {
				Log.error("\(ConversationModel.TAG) Failed to create group call!")
				DispatchQueue.main.async {
					ToastViewModel.shared.toastMessage = "Failed_to_create_group_call_error"
					ToastViewModel.shared.displayToast = true
				}
			}
		})
		
		if self.conferenceDelegate != nil {
			conference.addDelegate(delegate: self.conferenceDelegate!)
		}
	}
	
	func chatMessageAddDelegate() {
		if self.lastMessage != nil && self.chatMessageDelegate != nil {
			self.lastMessage!.removeDelegate(delegate: self.chatMessageDelegate!)
		}
		
		self.chatMessageDelegate = ChatMessageDelegateStub(onMsgStateChanged: { (message: ChatMessage, msgState: ChatMessage.State) in
			let lastMessageStateTmp = msgState.rawValue
			DispatchQueue.main.async {
				self.lastMessageState = lastMessageStateTmp
			}
		})
		
		if self.lastMessage != nil && self.chatMessageDelegate != nil {
			self.lastMessage!.addDelegate(delegate: self.chatMessageDelegate!)
		}
	}
	
	func chatMessageRemoveDelegate() {
		if self.lastMessage != nil && chatMessageDelegate != nil {
			self.lastMessage!.removeDelegate(delegate: chatMessageDelegate!)
		}
	}
	
	func getContentTextMessage(chatRoom: ChatRoom) {
		let lastMessage = chatRoom.lastMessageInHistory
		if lastMessage != nil {
			if !(CoreContext.shared.imdnToEverybodyThreshold && !lastMessage!.isOutgoing) {
				self.lastMessage = lastMessage
				self.chatMessageAddDelegate()
			}
			
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
						fromAddressFriend = String(lastMessage!.fromAddress!.asStringUriOnly().dropFirst(4)) + ": "
					}
				} else {
					fromAddressFriend! += ": "
				}
				
			} else {
				fromAddressFriend = nil
			}
			
			var lastMessageTextTmp = (fromAddressFriend ?? "")
			+ (lastMessage!.contents.first(where: {$0.isText == true})?.utf8Text ?? (lastMessage!.contents.first(where: {$0.isFile == true || $0.isFileTransfer == true})?.name ?? ""))
			
			if lastMessage!.contents.first != nil && lastMessage!.contents.first!.isIcalendar == true {
				if let conferenceInfo = try? Factory.Instance.createConferenceInfoFromIcalendarContent(content: lastMessage!.contents.first!) {
					if conferenceInfo.uri != nil {
						//let meetingSubjectTmp = conferenceInfo.subject ?? ""
						if conferenceInfo.state == .New {
							lastMessageTextTmp = String(localized: "message_meeting_invitation_notification")
						} else if conferenceInfo.state == .Updated {
							lastMessageTextTmp = String(localized: "message_meeting_invitation_updated_notification")
						} else if conferenceInfo.state == .Cancelled {
							lastMessageTextTmp = String(localized: "message_meeting_invitation_cancelled_notification")
						}
					}
				}
			}
			
			let lastMessageIsOutgoingTmp = lastMessage?.isOutgoing ?? false
            
            let lastUpdateTimeTmp = lastMessage?.time ?? chatRoom.lastUpdateTime
			
			let lastMessageStateTmp = lastMessage?.state.rawValue ?? 0
			
            DispatchQueue.main.async {
                self.lastMessageText = lastMessageTextTmp
                
                self.lastMessageIsOutgoing = lastMessageIsOutgoingTmp
                
                self.lastUpdateTime = lastUpdateTimeTmp
                
                self.lastMessageState = lastMessageStateTmp
            }
            
            getUnreadMessagesCount()
		}
	}
	
	func getUnreadMessagesCount() {
		let unreadMessagesCountTmp = self.chatRoom.unreadMessagesCount
        DispatchQueue.main.async {
            self.unreadMessagesCount = unreadMessagesCountTmp
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
