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
import AVFoundation

// swiftlint:disable line_length
// swiftlint:disable type_body_length
// swiftlint:disable cyclomatic_complexity
class ConversationViewModel: ObservableObject {
	
	private var coreContext = CoreContext.shared
	
	@Published var displayedConversation: ConversationModel?
	@Published var displayedConversationHistorySize: Int = 0
	@Published var displayedConversationUnreadMessagesCount: Int = 0
	
	@Published var messageText: String = ""
	
	private var chatRoomSuscriptions = Set<AnyCancellable?>()
	private var chatMessageSuscriptions = Set<AnyCancellable?>()
	
	@Published var conversationMessagesSection: [MessagesSection] = []
	@Published var participantConversationModel: [ContactAvatarModel] = []
	
	@Published var mediasToSend: [Attachment] = []
	var maxMediaCount = 12
	
	var oldMessageReceived = false
	
	@Published var isShowSelectedMessageToDisplayDetails: Bool = false
	@Published var selectedMessageToDisplayDetails: EventLogMessage?
	@Published var selectedMessageToPlayVoiceRecording: EventLogMessage?
	@Published var selectedMessage: EventLogMessage?
	@Published var messageToReply: EventLogMessage?
	
	@Published var sheetCategories: [SheetCategory] = []
	
	var vrpManager: VoiceRecordPlayerManager?
	@Published var isPlaying = false
	@Published var progress: Double = 0.0
	
	struct SheetCategory: Identifiable {
		let id = UUID()
		let name: String
		let innerCategory: [InnerSheetCategory]
	}
	
	struct InnerSheetCategory: Identifiable {
		let id = UUID()
		let contact: ContactAvatarModel
		let detail: String
		var isMe: Bool = false
	}
	
	init() {}
	
	func addConversationDelegate() {
		coreContext.doOnCoreQueue { _ in
			if self.displayedConversation != nil {
				self.chatRoomSuscriptions.insert(self.displayedConversation?.chatRoom.publisher?.onChatMessageSending?.postOnCoreQueue { (cbValue: (chatRoom: ChatRoom, eventLog: EventLog)) in
					self.getNewMessages(eventLogs: [cbValue.eventLog])
				})
				
				self.chatRoomSuscriptions.insert(self.displayedConversation?.chatRoom.publisher?.onChatMessagesReceived?.postOnCoreQueue { (cbValue: (chatRoom: ChatRoom, eventLogs: [EventLog])) in
					self.getNewMessages(eventLogs: cbValue.eventLogs)
				})
			}
		}
	}
	
	func addChatMessageDelegate(message: ChatMessage) {
		DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
			if self.displayedConversation != nil {
				var statusTmp: Message.Status? = .sending
				switch message.state {
				case .InProgress:
					statusTmp = .sending
				case .Delivered:
					statusTmp = .sent
				case .DeliveredToUser:
					statusTmp = .received
				case .Displayed:
					statusTmp = .read
				case .NotDelivered:
					statusTmp = .error
				default:
					statusTmp = .sending
				}
				
				if !self.conversationMessagesSection.isEmpty && !self.conversationMessagesSection[0].rows.isEmpty {
					if let indexMessage = self.conversationMessagesSection[0].rows.firstIndex(where: {$0.eventLog.chatMessage?.messageId == message.messageId}) {
						if indexMessage < self.conversationMessagesSection[0].rows.count && self.conversationMessagesSection[0].rows[indexMessage].message.status != statusTmp {
							DispatchQueue.main.async {
								self.objectWillChange.send()
								self.conversationMessagesSection[0].rows[indexMessage].message.status = statusTmp
							}
						}
					}
				}
				
				self.coreContext.doOnCoreQueue { _ in
					self.chatMessageSuscriptions.insert(message.publisher?.onMsgStateChanged?.postOnCoreQueue {(cbValue: (message: ChatMessage, state: ChatMessage.State)) in
						var statusTmp: Message.Status? = .sending
						switch cbValue.message.state {
						case .InProgress:
							statusTmp = .sending
						case .Delivered:
							statusTmp = .sent
						case .DeliveredToUser:
							statusTmp = .received
						case .Displayed:
							statusTmp = .read
						case .NotDelivered:
							statusTmp = .error
						default:
							statusTmp = .sending
						}
						
						let indexMessage = self.conversationMessagesSection[0].rows.firstIndex(where: {$0.eventLog.chatMessage?.messageId == message.messageId})
						
						DispatchQueue.main.async {
							if indexMessage != nil {
								self.objectWillChange.send()
								self.conversationMessagesSection[0].rows[indexMessage!].message.status = statusTmp
							}
						}
					})
					
					self.chatMessageSuscriptions.insert(message.publisher?.onNewMessageReaction?.postOnCoreQueue {(cbValue: (message: ChatMessage, reaction: ChatMessageReaction)) in
						let indexMessage = self.conversationMessagesSection[0].rows.firstIndex(where: {$0.eventLog.chatMessage?.messageId == message.messageId})
						var reactionsTmp: [String] = []
						cbValue.message.reactions.forEach({ chatMessageReaction in
							reactionsTmp.append(chatMessageReaction.body)
						})
						
						DispatchQueue.main.async {
							if indexMessage != nil {
								self.objectWillChange.send()
								self.conversationMessagesSection[0].rows[indexMessage!].message.reactions = reactionsTmp
							}
						}
					})
					
					self.chatMessageSuscriptions.insert(message.publisher?.onReactionRemoved?.postOnCoreQueue {(cbValue: (message: ChatMessage, address: Address)) in
						let indexMessage = self.conversationMessagesSection[0].rows.firstIndex(where: {$0.eventLog.chatMessage?.messageId == message.messageId})
						var reactionsTmp: [String] = []
						cbValue.message.reactions.forEach({ chatMessageReaction in
							reactionsTmp.append(chatMessageReaction.body)
						})
						
						DispatchQueue.main.async {
							if indexMessage != nil {
								self.objectWillChange.send()
								self.conversationMessagesSection[0].rows[indexMessage!].message.reactions = reactionsTmp
							}
						}
					})
				}
			}
		}
	}
	
	func removeConversationDelegate() {
		self.chatRoomSuscriptions.removeAll()
		self.chatMessageSuscriptions.removeAll()
	}
	
	func getHistorySize() {
		coreContext.doOnCoreQueue { _ in
			if self.displayedConversation != nil {
				let historySize = self.displayedConversation!.chatRoom.historyEventsSize
				DispatchQueue.main.async {
					self.displayedConversationHistorySize = historySize
				}
			}
		}
	}
	
	func getUnreadMessagesCount() {
		coreContext.doOnCoreQueue { _ in
			if self.displayedConversation != nil {
				let unreadMessagesCount = self.displayedConversation!.chatRoom.unreadMessagesCount
				DispatchQueue.main.async {
					self.displayedConversationUnreadMessagesCount = unreadMessagesCount
				}
			}
		}
	}
	
	func markAsRead() {
		coreContext.doOnCoreQueue { _ in
			if self.displayedConversation != nil {
				let unreadMessagesCount = self.displayedConversation!.chatRoom.unreadMessagesCount
				
				if unreadMessagesCount > 0 {
					self.displayedConversation!.chatRoom.markAsRead()
					
					DispatchQueue.main.async {
						self.displayedConversationUnreadMessagesCount = 0
					}
				}
			}
		}
	}
	
	func getParticipantConversationModel() {
		coreContext.doOnCoreQueue { _ in
			if self.displayedConversation != nil {
				self.displayedConversation!.chatRoom.participants.forEach { participant in
					if participant.address != nil {
						ContactAvatarModel.getAvatarModelFromAddress(address: participant.address!) { avatarResult in
							let avatarModelTmp = avatarResult
							DispatchQueue.main.async {
								self.participantConversationModel.append(avatarModelTmp)
							}
						}
					}
				}
				
				if self.displayedConversation!.chatRoom.me != nil {
					ContactAvatarModel.getAvatarModelFromAddress(address: self.displayedConversation!.chatRoom.me!.address!) { avatarResult in
						let avatarModelTmp = avatarResult
						DispatchQueue.main.async {
							self.participantConversationModel.append(avatarModelTmp)
						}
					}
				}
			}
		}
	}
	
	func addParticipantConversationModel(address: Address) {
		coreContext.doOnCoreQueue { _ in
			ContactAvatarModel.getAvatarModelFromAddress(address: address) { avatarResult in
				let avatarModelTmp = avatarResult
				DispatchQueue.main.async {
					self.participantConversationModel.append(avatarModelTmp)
				}
			}
		}
	}
	
	func getMessages() {
		self.getHistorySize()
		self.getUnreadMessagesCount()
		self.getParticipantConversationModel()
		
		self.mediasToSend.removeAll()
		self.messageToReply = nil
		
		coreContext.doOnCoreQueue { _ in
			if self.displayedConversation != nil {
				let historyEvents = self.displayedConversation!.chatRoom.getHistoryRangeEvents(begin: 0, end: 30)
				
				var conversationMessage: [EventLogMessage] = []
				historyEvents.enumerated().forEach { index, eventLog in
					
					var attachmentNameList: String = ""
					var attachmentList: [Attachment] = []
					var contentText = ""
					
					if eventLog.chatMessage != nil && !eventLog.chatMessage!.contents.isEmpty {
						eventLog.chatMessage!.contents.forEach { content in
							if content.isText {
								contentText = content.utf8Text ?? ""
							} else if content.name != nil && !content.name!.isEmpty {
								if content.filePath == nil || content.filePath!.isEmpty {
									// self.downloadContent(chatMessage: eventLog.chatMessage!, content: content)
									let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
									
									if path != nil {
										let attachment =
										Attachment(
											id: UUID().uuidString,
											name: content.name!,
											url: path!,
											type: .fileTransfer
										)
										attachmentNameList += ", \(content.name!)"
										attachmentList.append(attachment)
									}
								} else {
									if content.type != "video" {
										let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
										var typeTmp: AttachmentType = .other
										
										switch content.type {
										case "image":
											typeTmp = (content.name?.lowercased().hasSuffix("gif"))! ? .gif : .image
										case "audio":
											typeTmp = content.isVoiceRecording ? .voiceRecording : .audio
										case "application":
											typeTmp = content.subtype.lowercased() == "pdf" ? .pdf : .other
										case "text":
											typeTmp = .text
										default:
											typeTmp = .other
										}
										
										if path != nil {
											let attachment =
											Attachment(
												id: UUID().uuidString,
												name: content.name!,
												url: path!,
												type: typeTmp,
												duration: typeTmp == . voiceRecording ? content.fileDuration : 0
											)
											attachmentNameList += ", \(content.name!)"
											attachmentList.append(attachment)
										}
									} else if content.type == "video" {
										let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
										let pathThumbnail = URL(string: self.generateThumbnail(name: content.name ?? ""))
										
										if path != nil && pathThumbnail != nil {
											let attachment =
											Attachment(
												id: UUID().uuidString,
												name: content.name!,
												thumbnail: pathThumbnail!,
												full: path!,
												type: .video
											)
											attachmentNameList += ", \(content.name!)"
											attachmentList.append(attachment)
										}
									}
								}
							}
						}
					}
					
					let addressPrecCleaned = index > 0 ? historyEvents[index - 1].chatMessage?.fromAddress?.clone() : eventLog.chatMessage?.fromAddress?.clone()
					addressPrecCleaned?.clean()
					
					let addressNextCleaned = index <= historyEvents.count - 2 ? historyEvents[index + 1].chatMessage?.fromAddress?.clone() : eventLog.chatMessage?.fromAddress?.clone()
					addressNextCleaned?.clean()
					
					let addressCleaned = eventLog.chatMessage?.fromAddress?.clone()
					addressCleaned?.clean()
					
					if addressCleaned != nil && self.participantConversationModel.first(where: {$0.address == addressCleaned!.asStringUriOnly()}) == nil {
						self.addParticipantConversationModel(address: addressCleaned!)
					}
					
					let isFirstMessageIncomingTmp = index > 0 ? addressPrecCleaned?.asStringUriOnly() != addressCleaned?.asStringUriOnly() : true
					let isFirstMessageOutgoingTmp = index <= historyEvents.count - 2 ? addressNextCleaned?.asStringUriOnly() != addressCleaned?.asStringUriOnly() : true
					
					let isFirstMessageTmp = (eventLog.chatMessage?.isOutgoing ?? false) ? isFirstMessageOutgoingTmp : isFirstMessageIncomingTmp
					
					var statusTmp: Message.Status? = .sending
					switch eventLog.chatMessage?.state {
					case .InProgress:
						statusTmp = .sending
					case .Delivered:
						statusTmp = .sent
					case .DeliveredToUser:
						statusTmp = .received
					case .Displayed:
						statusTmp = .read
					case .NotDelivered:
						statusTmp = .error
					default:
						statusTmp = .sending
					}
					
					var reactionsTmp: [String] = []
					eventLog.chatMessage?.reactions.forEach({ chatMessageReaction in
						reactionsTmp.append(chatMessageReaction.body)
					})
					
					if !attachmentNameList.isEmpty {
						attachmentNameList = String(attachmentNameList.dropFirst(2))
					}
					
					var replyMessageTmp: ReplyMessage?
					if eventLog.chatMessage?.replyMessage != nil {
						let addressReplyCleaned = eventLog.chatMessage?.replyMessage?.fromAddress?.clone()
						addressReplyCleaned?.clean()
						
						if addressReplyCleaned != nil && self.participantConversationModel.first(where: {$0.address == addressReplyCleaned!.asStringUriOnly()}) == nil {
							self.addParticipantConversationModel(address: addressReplyCleaned!)
						}
						
						let contentReplyText = eventLog.chatMessage?.replyMessage?.utf8Text ?? ""
						
						var attachmentNameReplyList: String = ""
						
						eventLog.chatMessage?.replyMessage?.contents.forEach { content in
							if !content.isText {
								attachmentNameReplyList += ", \(content.name!)"
							}
						}
						
						if !attachmentNameReplyList.isEmpty {
							attachmentNameReplyList = String(attachmentNameReplyList.dropFirst(2))
						}
						
						replyMessageTmp = ReplyMessage(
							id: eventLog.chatMessage?.replyMessage!.messageId ?? UUID().uuidString,
							address: addressReplyCleaned?.asStringUriOnly() ?? "",
							isFirstMessage: false,
							text: contentReplyText,
							isOutgoing: false,
							dateReceived: 0,
							attachmentsNames: attachmentNameReplyList,
							attachments: []
						)
					}
					
					if eventLog.chatMessage != nil {
						conversationMessage.append(
							EventLogMessage(
								eventLog: eventLog,
								message: Message(
									id: !eventLog.chatMessage!.messageId.isEmpty ? eventLog.chatMessage!.messageId : UUID().uuidString,
									status: statusTmp,
									isOutgoing: eventLog.chatMessage?.isOutgoing ?? false,
									dateReceived: eventLog.chatMessage?.time ?? 0,
									address: addressCleaned?.asStringUriOnly() ?? "",
									isFirstMessage: isFirstMessageTmp,
									text: contentText,
									attachmentsNames: attachmentNameList,
									attachments: attachmentList,
									replyMessage: replyMessageTmp,
									isForward: eventLog.chatMessage?.isForward ?? false,
									ownReaction: eventLog.chatMessage?.ownReaction?.body ?? "",
									reactions: reactionsTmp
								)
							)
						)
						
						self.addChatMessageDelegate(message: eventLog.chatMessage!)
					} else {
						conversationMessage.insert(
							EventLogMessage(
								eventLog: eventLog,
								message: Message(
									id: UUID().uuidString,
									status: nil,
									isOutgoing: false,
									dateReceived: 0,
									address: "",
									isFirstMessage: false,
									text: "",
									attachments: [],
									ownReaction: "",
									reactions: []
								)
							), at: 0
						)
					}
				}
				
				DispatchQueue.main.async {
					if self.conversationMessagesSection.isEmpty && self.displayedConversation != nil {
						self.conversationMessagesSection.append(MessagesSection(date: Date(), chatRoomID: self.displayedConversation!.id, rows: conversationMessage.reversed()))
					}
				}
			}
		}
	}
	
	func getOldMessages() {
		coreContext.doOnCoreQueue { _ in
			if self.displayedConversation != nil && !self.conversationMessagesSection.isEmpty
				&& self.displayedConversationHistorySize > self.conversationMessagesSection[0].rows.count && !self.oldMessageReceived {
				self.oldMessageReceived = true
				let historyEvents = self.displayedConversation!.chatRoom.getHistoryRangeEvents(begin: self.conversationMessagesSection[0].rows.count, end: self.conversationMessagesSection[0].rows.count + 30)
				var conversationMessagesTmp: [EventLogMessage] = []
				
				historyEvents.enumerated().reversed().forEach { index, eventLog in
					var attachmentNameList: String = ""
					var attachmentList: [Attachment] = []
					var contentText = ""
					
					if eventLog.chatMessage != nil && !eventLog.chatMessage!.contents.isEmpty {
						eventLog.chatMessage!.contents.forEach { content in
							if content.isText {
								contentText = content.utf8Text ?? ""
							} else if content.name != nil && !content.name!.isEmpty {
								if content.filePath == nil || content.filePath!.isEmpty {
									// self.downloadContent(chatMessage: eventLog.chatMessage!, content: content)
									let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
									
									if path != nil {
										let attachment =
										Attachment(
											id: UUID().uuidString,
											name: content.name!,
											url: path!,
											type: .fileTransfer
										)
										attachmentNameList += ", \(content.name!)"
										attachmentList.append(attachment)
									}
								} else {
									if content.type != "video" {
										let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
										var typeTmp: AttachmentType = .other
										
										switch content.type {
										case "image":
											typeTmp = (content.name?.lowercased().hasSuffix("gif"))! ? .gif : .image
										case "audio":
											typeTmp = content.isVoiceRecording ? .voiceRecording : .audio
										case "application":
											typeTmp = content.subtype.lowercased() == "pdf" ? .pdf : .other
										case "text":
											typeTmp = .text
										default:
											typeTmp = .other
										}
										
										if path != nil {
											let attachment =
											Attachment(
												id: UUID().uuidString,
												name: content.name!,
												url: path!,
												type: typeTmp,
												duration: typeTmp == . voiceRecording ? content.fileDuration : 0
											)
											attachmentNameList += ", \(content.name!)"
											attachmentList.append(attachment)
										}
									} else if content.type == "video" {
										let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
										let pathThumbnail = URL(string: self.generateThumbnail(name: content.name ?? ""))
										
										if path != nil && pathThumbnail != nil {
											let attachment =
											Attachment(
												id: UUID().uuidString,
												name: content.name!,
												thumbnail: pathThumbnail!,
												full: path!,
												type: .video
											)
											attachmentNameList += ", \(content.name!)"
											attachmentList.append(attachment)
										}
									}
								}
							}
						}
					}
					
					let addressPrecCleaned = index > 0 ? historyEvents[index - 1].chatMessage?.fromAddress?.clone() : eventLog.chatMessage?.fromAddress?.clone()
					addressPrecCleaned?.clean()
					
					let addressNextCleaned = index <= historyEvents.count - 2 ? historyEvents[index + 1].chatMessage?.fromAddress?.clone() : eventLog.chatMessage?.fromAddress?.clone()
					addressNextCleaned?.clean()
					
					let addressCleaned = eventLog.chatMessage?.fromAddress?.clone()
					addressCleaned?.clean()
					
					if addressCleaned != nil && self.participantConversationModel.first(where: {$0.address == addressCleaned!.asStringUriOnly()}) == nil {
						self.addParticipantConversationModel(address: addressCleaned!)
					}
					
					let isFirstMessageIncomingTmp = index > 0 ? addressPrecCleaned?.asStringUriOnly() != addressCleaned?.asStringUriOnly() : true
					let isFirstMessageOutgoingTmp = index <= historyEvents.count - 2 ? addressNextCleaned?.asStringUriOnly() != addressCleaned?.asStringUriOnly() : true
					
					let isFirstMessageTmp = (eventLog.chatMessage?.isOutgoing ?? false) ? isFirstMessageOutgoingTmp : isFirstMessageIncomingTmp
					
					var statusTmp: Message.Status? = .sending
					switch eventLog.chatMessage?.state {
					case .InProgress:
						statusTmp = .sending
					case .Delivered:
						statusTmp = .sent
					case .DeliveredToUser:
						statusTmp = .received
					case .Displayed:
						statusTmp = .read
					case .NotDelivered:
						statusTmp = .error
					default:
						statusTmp = .sending
					}
					
					var reactionsTmp: [String] = []
					eventLog.chatMessage?.reactions.forEach({ chatMessageReaction in
						reactionsTmp.append(chatMessageReaction.body)
					})
					
					if !attachmentNameList.isEmpty {
						attachmentNameList = String(attachmentNameList.dropFirst(2))
					}
					
					var replyMessageTmp: ReplyMessage?
					if eventLog.chatMessage?.replyMessage != nil {
						let addressReplyCleaned = eventLog.chatMessage?.replyMessage?.fromAddress?.clone()
						addressReplyCleaned?.clean()
						
						if addressReplyCleaned != nil && self.participantConversationModel.first(where: {$0.address == addressReplyCleaned!.asStringUriOnly()}) == nil {
							self.addParticipantConversationModel(address: addressReplyCleaned!)
						}
						
						let contentReplyText = eventLog.chatMessage?.replyMessage?.utf8Text ?? ""
						
						var attachmentNameReplyList: String = ""
						
						eventLog.chatMessage?.replyMessage?.contents.forEach { content in
							if !content.isText {
								attachmentNameReplyList += ", \(content.name!)"
							}
						}
						
						if !attachmentNameReplyList.isEmpty {
							attachmentNameReplyList = String(attachmentNameReplyList.dropFirst(2))
						}
						
						replyMessageTmp = ReplyMessage(
							id: eventLog.chatMessage?.replyMessage!.messageId ?? UUID().uuidString,
							address: addressReplyCleaned?.asStringUriOnly() ?? "",
							isFirstMessage: false,
							text: contentReplyText,
							isOutgoing: false,
							dateReceived: 0,
							attachmentsNames: attachmentNameReplyList,
							attachments: []
						)
					}
					
					if eventLog.chatMessage != nil {
						conversationMessagesTmp.insert(
							EventLogMessage(
								eventLog: eventLog,
								message: Message(
									id: !eventLog.chatMessage!.messageId.isEmpty ? eventLog.chatMessage!.messageId : UUID().uuidString,
									status: statusTmp,
									isOutgoing: eventLog.chatMessage?.isOutgoing ?? false,
									dateReceived: eventLog.chatMessage?.time ?? 0,
									address: addressCleaned?.asStringUriOnly() ?? "",
									isFirstMessage: isFirstMessageTmp,
									text: contentText,
									attachmentsNames: attachmentNameList,
									attachments: attachmentList,
									replyMessage: replyMessageTmp,
									isForward: eventLog.chatMessage?.isForward ?? false,
									ownReaction: eventLog.chatMessage?.ownReaction?.body ?? "",
									reactions: reactionsTmp
								)
							), at: 0
						)
						
						self.addChatMessageDelegate(message: eventLog.chatMessage!)
					} else {
						conversationMessagesTmp.insert(
							EventLogMessage(
								eventLog: eventLog,
								message: Message(
									id: UUID().uuidString,
									status: nil,
									isOutgoing: false,
									dateReceived: 0,
									address: "",
									isFirstMessage: false,
									text: "",
									attachments: [],
									ownReaction: "",
									reactions: []
								)
							), at: 0
						)
					}
				}
				
				if !conversationMessagesTmp.isEmpty {
					DispatchQueue.main.async {
						if self.conversationMessagesSection[0].rows.last?.message.address == conversationMessagesTmp.last?.message.address {
							self.conversationMessagesSection[0].rows[self.conversationMessagesSection[0].rows.count - 1].message.isFirstMessage = false
						}
						self.conversationMessagesSection[0].rows.append(contentsOf: conversationMessagesTmp.reversed())
						self.oldMessageReceived = false
					}
				}
			}
		}
	}
	
	func getNewMessages(eventLogs: [EventLog]) {
		eventLogs.enumerated().forEach { index, eventLog in
			var attachmentNameList: String = ""
			var attachmentList: [Attachment] = []
			var contentText = ""
			
			if eventLog.chatMessage != nil && !eventLog.chatMessage!.contents.isEmpty {
				eventLog.chatMessage!.contents.forEach { content in
					if content.isText {
						contentText = content.utf8Text ?? ""
					} else {
						if content.filePath == nil || content.filePath!.isEmpty {
							// self.downloadContent(chatMessage: eventLog.chatMessage!, content: content)
							let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
							
							if path != nil {
								let attachment =
								Attachment(
									id: UUID().uuidString,
									name: content.name ?? "???",
									url: path!,
									type: .fileTransfer
								)
								attachmentNameList += ", \(content.name ?? "???")"
								attachmentList.append(attachment)
							}
						} else if content.name != nil && !content.name!.isEmpty {
							if content.type != "video" {
								let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
								var typeTmp: AttachmentType = .other
								
								switch content.type {
								case "image":
									typeTmp = (content.name?.lowercased().hasSuffix("gif"))! ? .gif : .image
								case "audio":
									typeTmp = content.isVoiceRecording ? .voiceRecording : .audio
								case "application":
									typeTmp = content.subtype.lowercased() == "pdf" ? .pdf : .other
								case "text":
									typeTmp = .text
								default:
									typeTmp = .other
								}
								
								if path != nil {
									let attachment =
									Attachment(
										id: UUID().uuidString,
										name: content.name!,
										url: path!,
										type: typeTmp,
										duration: typeTmp == . voiceRecording ? content.fileDuration : 0
									)
									attachmentNameList += ", \(content.name!)"
									attachmentList.append(attachment)
								}
							} else if content.type == "video" {
								let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
								
								let pathThumbnail = URL(string: self.generateThumbnail(name: content.name ?? ""))
								if path != nil && pathThumbnail != nil {
									let attachment =
									Attachment(
										id: UUID().uuidString,
										name: content.name!,
										thumbnail: pathThumbnail!,
										full: path!,
										type: .video
									)
									attachmentNameList += ", \(content.name!)"
									attachmentList.append(attachment)
								}
							}
						}
					}
				}
			}
			
			let addressPrecCleaned = index > 0 ? eventLogs[index - 1].chatMessage?.fromAddress?.clone() : eventLog.chatMessage?.fromAddress?.clone()
			addressPrecCleaned?.clean()
			
			let addressNextCleaned = index <= eventLogs.count - 2 ? eventLogs[index + 1].chatMessage?.fromAddress?.clone() : eventLog.chatMessage?.fromAddress?.clone()
			addressNextCleaned?.clean()
			
			let addressCleaned = eventLog.chatMessage?.fromAddress?.clone()
			addressCleaned?.clean()
			
			if addressCleaned != nil && self.participantConversationModel.first(where: {$0.address == addressCleaned!.asStringUriOnly()}) == nil {
				self.addParticipantConversationModel(address: addressCleaned!)
			}
			
			let isFirstMessageIncomingTmp = index > 0
			? addressPrecCleaned?.asStringUriOnly() != addressCleaned?.asStringUriOnly()
			: (
				self.conversationMessagesSection.isEmpty || self.conversationMessagesSection[0].rows.isEmpty
				? true
				: self.conversationMessagesSection[0].rows[0].message.address != addressCleaned?.asStringUriOnly()
			)
			
			let isFirstMessageOutgoingTmp = index <= eventLogs.count - 2
			? addressNextCleaned?.asStringUriOnly() == addressCleaned?.asStringUriOnly()
			: (
				self.conversationMessagesSection.isEmpty || self.conversationMessagesSection[0].rows.isEmpty
				? true
				: !self.conversationMessagesSection[0].rows[0].message.isOutgoing || self.conversationMessagesSection[0].rows[0].message.address == addressCleaned?.asStringUriOnly()
			)
			
			let isFirstMessageTmp = (eventLog.chatMessage?.isOutgoing ?? false) ? isFirstMessageOutgoingTmp : isFirstMessageIncomingTmp
			
			let unreadMessagesCount = self.displayedConversation != nil ? self.displayedConversation!.chatRoom.unreadMessagesCount : 0
			
			var statusTmp: Message.Status? = .sending
			switch eventLog.chatMessage?.state {
			case .InProgress:
				statusTmp = .sending
			case .Delivered:
				statusTmp = .sent
			case .DeliveredToUser:
				statusTmp = .received
			case .Displayed:
				statusTmp = .read
			case .NotDelivered:
				statusTmp = .error
			default:
				statusTmp = .sending
			}
			
			var reactionsTmp: [String] = []
			eventLog.chatMessage?.reactions.forEach({ chatMessageReaction in
				reactionsTmp.append(chatMessageReaction.body)
			})
			
			if !attachmentNameList.isEmpty {
				attachmentNameList = String(attachmentNameList.dropFirst(2))
			}
			
			var replyMessageTmp: ReplyMessage?
			if eventLog.chatMessage?.replyMessage != nil {
				let addressReplyCleaned = eventLog.chatMessage?.replyMessage?.fromAddress?.clone()
				addressReplyCleaned?.clean()
				
				if addressReplyCleaned != nil && self.participantConversationModel.first(where: {$0.address == addressReplyCleaned!.asStringUriOnly()}) == nil {
					self.addParticipantConversationModel(address: addressReplyCleaned!)
				}
				
				let contentReplyText = eventLog.chatMessage?.replyMessage?.utf8Text ?? ""
				
				var attachmentNameReplyList: String = ""
				
				eventLog.chatMessage?.replyMessage?.contents.forEach { content in
					if !content.isText {
						attachmentNameReplyList += ", \(content.name!)"
					}
				}
				
				if !attachmentNameReplyList.isEmpty {
					attachmentNameReplyList = String(attachmentNameReplyList.dropFirst(2))
				}
				
				replyMessageTmp = ReplyMessage(
					id: eventLog.chatMessage?.replyMessage!.messageId ?? UUID().uuidString,
					address: addressReplyCleaned?.asStringUriOnly() ?? "",
					isFirstMessage: false,
					text: contentReplyText,
					isOutgoing: false,
					dateReceived: 0,
					attachmentsNames: attachmentNameReplyList,
					attachments: []
				)
			}
			
			if eventLog.chatMessage != nil {
				let message = EventLogMessage(
					eventLog: eventLog,
					message: Message(
						id: !eventLog.chatMessage!.messageId.isEmpty ? eventLog.chatMessage!.messageId : UUID().uuidString,
						appData: eventLog.chatMessage!.appdata ?? "",
						status: statusTmp,
						isOutgoing: eventLog.chatMessage?.isOutgoing ?? false,
						dateReceived: eventLog.chatMessage?.time ?? 0,
						address: addressCleaned?.asStringUriOnly() ?? "",
						isFirstMessage: isFirstMessageTmp,
						text: contentText,
						attachmentsNames: attachmentNameList,
						attachments: attachmentList,
						replyMessage: replyMessageTmp,
						isForward: eventLog.chatMessage?.isForward ?? false,
						ownReaction: eventLog.chatMessage?.ownReaction?.body ?? "",
						reactions: reactionsTmp
					)
				)
				
				self.addChatMessageDelegate(message: eventLog.chatMessage!)
				
				DispatchQueue.main.async {
					if !self.conversationMessagesSection.isEmpty 
						&& !self.conversationMessagesSection[0].rows.isEmpty
						&& self.conversationMessagesSection[0].rows[0].message.isOutgoing
						&& (self.conversationMessagesSection[0].rows[0].message.address == message.message.address) {
						self.conversationMessagesSection[0].rows[0].message.isFirstMessage = false
					}
					
					if self.conversationMessagesSection.isEmpty && self.displayedConversation != nil {
						self.conversationMessagesSection.append(MessagesSection(date: Date(), chatRoomID: self.displayedConversation!.id, rows: [message]))
					} else {
						self.conversationMessagesSection[0].rows.insert(message, at: 0)
					}
					
					if !message.message.isOutgoing {
						self.displayedConversationUnreadMessagesCount = unreadMessagesCount
					}
				}
			} else {
				let message = EventLogMessage(
					eventLog: eventLog,
					message: Message(
						id: UUID().uuidString,
						status: nil,
						isOutgoing: false,
						dateReceived: 0,
						address: "",
						isFirstMessage: false,
						text: "",
						attachments: [],
						ownReaction: "",
						reactions: []
					)
				)
				
				DispatchQueue.main.async {
					if self.conversationMessagesSection.isEmpty && self.displayedConversation != nil {
						self.conversationMessagesSection.append(MessagesSection(date: Date(), chatRoomID: self.displayedConversation!.id, rows: [message]))
					} else {
						self.conversationMessagesSection[0].rows.insert(message, at: 0)
					}
				}
			}
		}
	}
	
	func resetMessage() {
		conversationMessagesSection = []
	}
	
	func replyToMessage(index: Int) {
		coreContext.doOnCoreQueue { _ in
			let messageToReplyTmp = self.conversationMessagesSection[0].rows[index]
			DispatchQueue.main.async {
				withAnimation(.linear(duration: 0.15)) {
					self.messageToReply = messageToReplyTmp
				}
			}
		}
	}
	
	func scrollToMessage(message: Message) {
		coreContext.doOnCoreQueue { _ in
			if message.replyMessage != nil {
				if let indexMessage = self.conversationMessagesSection[0].rows.firstIndex(where: {$0.eventLog.chatMessage?.messageId == message.replyMessage!.id}) {
					NotificationCenter.default.post(name: NSNotification.Name(rawValue: "onScrollToIndex"), object: nil, userInfo: ["index": indexMessage, "animated": true])
				} else {
					if self.conversationMessagesSection[0].rows.last != nil {
						let firstEventLog = self.displayedConversation?.chatRoom.getHistoryRangeEvents(
							begin: self.conversationMessagesSection[0].rows.count - 1,
							end: self.conversationMessagesSection[0].rows.count
						)
						let lastEventLog = self.displayedConversation!.chatRoom.findEventLog(messageId: message.replyMessage!.id)
						
						var historyEvents = self.displayedConversation!.chatRoom.getHistoryRangeBetween(
							firstEvent: firstEventLog!.first,
							lastEvent: lastEventLog,
							filters: UInt(ChatRoom.HistoryFilter([.ChatMessage, .InfoNoDevice]).rawValue)
						)
						
						let historyEventsAfter = self.displayedConversation!.chatRoom.getHistoryRangeEvents(
							begin: self.conversationMessagesSection[0].rows.count + historyEvents.count + 1,
							end: self.conversationMessagesSection[0].rows.count + historyEvents.count + 30
						)
						
						if lastEventLog != nil {
							historyEvents.insert(lastEventLog!, at: 0)
						}
						
						historyEvents.insert(contentsOf: historyEventsAfter, at: 0)
						
						var conversationMessagesTmp: [EventLogMessage] = []
						
						historyEvents.enumerated().reversed().forEach { index, eventLog in
							var attachmentNameList: String = ""
							var attachmentList: [Attachment] = []
							var contentText = ""
							
							if eventLog.chatMessage != nil && !eventLog.chatMessage!.contents.isEmpty {
								eventLog.chatMessage!.contents.forEach { content in
									if content.isText {
										contentText = content.utf8Text ?? ""
									} else if content.name != nil && !content.name!.isEmpty {
										if content.filePath == nil || content.filePath!.isEmpty {
											// self.downloadContent(chatMessage: eventLog.chatMessage!, content: content)
											let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
											
											if path != nil {
												let attachment =
												Attachment(
													id: UUID().uuidString,
													name: content.name!,
													url: path!,
													type: .fileTransfer
												)
												attachmentNameList += ", \(content.name!)"
												attachmentList.append(attachment)
											}
										} else {
											if content.type != "video" {
												let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
												var typeTmp: AttachmentType = .other
												
												switch content.type {
												case "image":
													typeTmp = (content.name?.lowercased().hasSuffix("gif"))! ? .gif : .image
												case "audio":
													typeTmp = content.isVoiceRecording ? .voiceRecording : .audio
												case "application":
													typeTmp = content.subtype.lowercased() == "pdf" ? .pdf : .other
												case "text":
													typeTmp = .text
												default:
													typeTmp = .other
												}
												
												if path != nil {
													let attachment =
													Attachment(
														id: UUID().uuidString,
														name: content.name!,
														url: path!,
														type: typeTmp,
														duration: typeTmp == . voiceRecording ? content.fileDuration : 0
													)
													attachmentNameList += ", \(content.name!)"
													attachmentList.append(attachment)
												}
											} else if content.type == "video" {
												let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
												let pathThumbnail = URL(string: self.generateThumbnail(name: content.name ?? ""))
												
												if path != nil && pathThumbnail != nil {
													let attachment =
													Attachment(
														id: UUID().uuidString,
														name: content.name!,
														thumbnail: pathThumbnail!,
														full: path!,
														type: .video
													)
													attachmentNameList += ", \(content.name!)"
													attachmentList.append(attachment)
												}
											}
										}
									}
								}
							}
							
							let addressPrecCleaned = index > 0 ? historyEvents[index - 1].chatMessage?.fromAddress?.clone() : eventLog.chatMessage?.fromAddress?.clone()
							addressPrecCleaned?.clean()
							
							let addressNextCleaned = index <= historyEvents.count - 2 ? historyEvents[index + 1].chatMessage?.fromAddress?.clone() : eventLog.chatMessage?.fromAddress?.clone()
							addressNextCleaned?.clean()
							
							let addressCleaned = eventLog.chatMessage?.fromAddress?.clone()
							addressCleaned?.clean()
							
							if addressCleaned != nil && self.participantConversationModel.first(where: {$0.address == addressCleaned!.asStringUriOnly()}) == nil {
								self.addParticipantConversationModel(address: addressCleaned!)
							}
							
							let isFirstMessageIncomingTmp = index > 0 ? addressPrecCleaned?.asStringUriOnly() != addressCleaned?.asStringUriOnly() : true
							let isFirstMessageOutgoingTmp = index <= historyEvents.count - 2 ? addressNextCleaned?.asStringUriOnly() != addressCleaned?.asStringUriOnly() : true
							
							let isFirstMessageTmp = (eventLog.chatMessage?.isOutgoing ?? false) ? isFirstMessageOutgoingTmp : isFirstMessageIncomingTmp
							
							var statusTmp: Message.Status? = .sending
							switch eventLog.chatMessage?.state {
							case .InProgress:
								statusTmp = .sending
							case .Delivered:
								statusTmp = .sent
							case .DeliveredToUser:
								statusTmp = .received
							case .Displayed:
								statusTmp = .read
							case .NotDelivered:
								statusTmp = .error
							default:
								statusTmp = .sending
							}
							
							var reactionsTmp: [String] = []
							eventLog.chatMessage?.reactions.forEach({ chatMessageReaction in
								reactionsTmp.append(chatMessageReaction.body)
							})
							
							if !attachmentNameList.isEmpty {
								attachmentNameList = String(attachmentNameList.dropFirst(2))
							}
							
							var replyMessageTmp: ReplyMessage?
							if eventLog.chatMessage?.replyMessage != nil {
								let addressReplyCleaned = eventLog.chatMessage?.replyMessage?.fromAddress?.clone()
								addressReplyCleaned?.clean()
								
								if addressReplyCleaned != nil && self.participantConversationModel.first(where: {$0.address == addressReplyCleaned!.asStringUriOnly()}) == nil {
									self.addParticipantConversationModel(address: addressReplyCleaned!)
								}
								
								let contentReplyText = eventLog.chatMessage?.replyMessage?.utf8Text ?? ""
								
								var attachmentNameReplyList: String = ""
								
								eventLog.chatMessage?.replyMessage?.contents.forEach { content in
									if !content.isText {
										attachmentNameReplyList += ", \(content.name!)"
									}
								}
								
								if !attachmentNameReplyList.isEmpty {
									attachmentNameReplyList = String(attachmentNameReplyList.dropFirst(2))
								}
								
								replyMessageTmp = ReplyMessage(
									id: eventLog.chatMessage?.replyMessage!.messageId ?? UUID().uuidString,
									address: addressReplyCleaned?.asStringUriOnly() ?? "",
									isFirstMessage: false,
									text: contentReplyText,
									isOutgoing: false,
									dateReceived: 0,
									attachmentsNames: attachmentNameReplyList,
									attachments: []
								)
							}
							
							if eventLog.chatMessage != nil {
								conversationMessagesTmp.insert(
									EventLogMessage(
										eventLog: eventLog,
										message: Message(
											id: !eventLog.chatMessage!.messageId.isEmpty ? eventLog.chatMessage!.messageId : UUID().uuidString,
											status: statusTmp,
											isOutgoing: eventLog.chatMessage?.isOutgoing ?? false,
											dateReceived: eventLog.chatMessage?.time ?? 0,
											address: addressCleaned?.asStringUriOnly() ?? "",
											isFirstMessage: isFirstMessageTmp,
											text: contentText,
											attachmentsNames: attachmentNameList,
											attachments: attachmentList,
											replyMessage: replyMessageTmp,
											isForward: eventLog.chatMessage?.isForward ?? false,
											ownReaction: eventLog.chatMessage?.ownReaction?.body ?? "",
											reactions: reactionsTmp
										)
									), at: 0
								)
								
								self.addChatMessageDelegate(message: eventLog.chatMessage!)
							} else {
								conversationMessagesTmp.insert(
									EventLogMessage(
										eventLog: eventLog,
										message: Message(
											id: UUID().uuidString,
											status: nil,
											isOutgoing: false,
											dateReceived: 0,
											address: "",
											isFirstMessage: false,
											text: "",
											attachments: [],
											ownReaction: "",
											reactions: []
										)
									), at: 0
								)
							}
						}
						
						if !conversationMessagesTmp.isEmpty {
							DispatchQueue.main.async {
								if self.conversationMessagesSection[0].rows.last?.message.address == conversationMessagesTmp.last?.message.address {
									self.conversationMessagesSection[0].rows[self.conversationMessagesSection[0].rows.count - 1].message.isFirstMessage = false
								}
								self.conversationMessagesSection[0].rows.append(contentsOf: conversationMessagesTmp.reversed())
								
								NotificationCenter.default.post(
									name: NSNotification.Name(rawValue: "onScrollToIndex"),
									object: nil,
									userInfo: ["index": self.conversationMessagesSection[0].rows.count - historyEventsAfter.count - 1, "animated": true]
								)
							}
						}
					}
				}
			}
		}
	}
	
	func sendMessage(audioRecorder: AudioRecorder? = nil) {
		coreContext.doOnCoreQueue { _ in
			do {
				var message: ChatMessage?
				if self.messageToReply != nil {
					let chatMessageToReply = self.messageToReply!.eventLog.chatMessage
					if chatMessageToReply != nil {
						message = try self.displayedConversation!.chatRoom.createReplyMessage(message: chatMessageToReply!)
					}
					self.messageToReply = nil
				} else {
					message = try self.displayedConversation!.chatRoom.createEmptyMessage()
				}
				
				let toSend = self.messageText.trimmingCharacters(in: .whitespacesAndNewlines)
				if !toSend.isEmpty {
					if message != nil {
						message!.addUtf8TextContent(text: toSend)
					}
				}
				
				if audioRecorder != nil {
					do {
						audioRecorder!.stopVoiceRecorder()
						let content = try audioRecorder!.linphoneAudioRecorder.createContent()
						Log.info(
							"[ConversationViewModel] Voice recording content created, file name is \(content.name ?? "") and duration is \(content.fileDuration)"
						)
						
						if message != nil {
							message!.addContent(content: content)
						}
					}
				} else {
					self.mediasToSend.forEach { attachment in
						do {
							let content = try Factory.Instance.createContent()
							
							switch attachment.type {
							case .image:
								content.type = "image"
								/*
								 case .audio:
								 content.type = "audio"
								 */
							case .video:
								content.type = "video"
								/*
								 case .pdf:
								 content.type = "application"
								 case .plainText:
								 content.type = "text"
								 */
							default:
								content.type = "file"
							}
							
							// content.subtype = attachment.type == .plainText ? "plain" : FileUtils.getExtensionFromFileName(attachment.fileName)
							content.subtype = attachment.full.pathExtension
							
							content.name = attachment.full.lastPathComponent
							
							if message != nil {
								
								let path = FileManager.default.temporaryDirectory.appendingPathComponent((attachment.full.lastPathComponent.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? ""))
								let newPath = URL(string: FileUtil.sharedContainerUrl().appendingPathComponent("Library/Images").absoluteString
												  + (attachment.full.lastPathComponent.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? ""))
								/*
								 let data = try Data(contentsOf: path)
								 let decodedData: () = try data.write(to: path)
								 */
								
								do {
									if FileManager.default.fileExists(atPath: newPath!.path) {
										try FileManager.default.removeItem(atPath: newPath!.path)
									}
									try FileManager.default.moveItem(atPath: path.path, toPath: newPath!.path)
									
									let filePathTmp = newPath?.absoluteString
									content.filePath = String(filePathTmp!.dropFirst(7))
									message!.addFileContent(content: content)
								} catch {
									Log.error(error.localizedDescription)
								}
							}
						} catch {
						}
					}
				}
				
				if message != nil && !message!.contents.isEmpty {
					Log.info("[ConversationViewModel] Sending message")
					message!.send()
				}
				
				Log.info("[ConversationViewModel] Message sent, re-setting defaults")
				
				DispatchQueue.main.async {
					withAnimation {
						self.mediasToSend.removeAll()
					}
					self.messageText = ""
				}
				
				/*
				 isReplying.postValue(false)
				 isFileAttachmentsListOpen.postValue(false)
				 isParticipantsListOpen.postValue(false)
				 isEmojiPickerOpen.postValue(false)
				 
				 if (::voiceMessageRecorder.isInitialized) {
				 stopVoiceRecorder()
				 }
				 isVoiceRecording.postValue(false)
				 
				 // Warning: do not delete files
				 val attachmentsList = arrayListOf<FileModel>()
				 attachments.postValue(attachmentsList)
				 
				 chatMessageToReplyTo = null
				 */
			} catch {
				
			}
		}
	}
	
	func changeDisplayedChatRoom(conversationModel: ConversationModel) {
		self.displayedConversation = conversationModel
	}
	
	func resetDisplayedChatRoom(conversationsList: [ConversationModel]) {
		removeConversationDelegate()
		
		if self.displayedConversation != nil {
			conversationsList.forEach { conversation in
				if conversation.id == self.displayedConversation!.id {
					self.displayedConversation = conversation
					
					self.chatRoomSuscriptions.insert(conversation.chatRoom.publisher?.onChatMessageSending?.postOnCoreQueue { (cbValue: (chatRoom: ChatRoom, eventLog: EventLog)) in
						self.getNewMessages(eventLogs: [cbValue.eventLog])
					})
					
					self.chatRoomSuscriptions.insert(conversation.chatRoom.publisher?.onChatMessagesReceived?.postOnCoreQueue { (cbValue: (chatRoom: ChatRoom, eventLogs: [EventLog])) in
						self.getNewMessages(eventLogs: cbValue.eventLogs)
					})
				}
			}
		}
	}
	
	func downloadContent(chatMessage: ChatMessage, content: Content) {
		// Log.debug("[ConversationViewModel] Starting downloading content for file \(model.fileName)")
		if !chatMessage.isFileTransferInProgress && (content.filePath == nil || content.filePath!.isEmpty) {
			if let contentName = content.name {
				// let isImage = FileUtil.isExtensionImage(path: contentName)
				let file = FileUtil.sharedContainerUrl().appendingPathComponent("Library/Images").absoluteString + (contentName.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? "")
				// let file = FileUtil.getFileStoragePath(fileName: contentName ?? "", isImage: isImage)
				content.filePath = String(file.dropFirst(7))
				Log.info(
					"[ConversationViewModel] File \(contentName) will be downloaded at \(content.filePath ?? "NIL")"
				)
				self.displayedConversation?.downloadContent(chatMessage: chatMessage, content: content)
			} else {
				Log.error("[ConversationViewModel] Content name is null, can't download it!")
			}
		}
	}
	
	func getNewFilePath(name: String) -> String {
		return FileUtil.sharedContainerUrl().appendingPathComponent("Library/Images").absoluteString + (name.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? "")
	}
	
	func generateThumbnail(name: String, pathThumbnail: URL? = nil) -> String {
		do {
			let path = pathThumbnail == nil
			? URL(string: "file://" + FileUtil.sharedContainerUrl().appendingPathComponent("Library/Images").absoluteString + (name.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? ""))
			: pathThumbnail!.appendingPathComponent((name.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? ""))
			let asset = AVURLAsset(url: path!, options: nil)
			let imgGenerator = AVAssetImageGenerator(asset: asset)
			imgGenerator.appliesPreferredTrackTransform = true
			let cgImage = try imgGenerator.copyCGImage(at: CMTimeMake(value: 0, timescale: 1), actualTime: nil)
			let thumbnail = UIImage(cgImage: cgImage)
			
			guard let data = thumbnail.jpegData(compressionQuality: 1) ?? thumbnail.pngData() else {
				return ""
			}
			
			let urlName = pathThumbnail == nil
			? URL(string: "file://" 
				  + FileUtil.sharedContainerUrl().appendingPathComponent("Library/Images").absoluteString
				  + "preview_"
				  + (name.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? "")
				  + ".png"
			)
			: pathThumbnail!.appendingPathComponent("preview_" + (name.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? "") + ".png")
			
			if urlName != nil {
				_ = try data.write(to: urlName!)
			}
			
			return urlName!.absoluteString
		} catch let error {
			print("*** Error generating thumbnail: \(error.localizedDescription)")
			return ""
		}
	}
	
	func getMessageTime(startDate: time_t) -> String {
		let timeInterval = TimeInterval(startDate)
		
		let myNSDate = Date(timeIntervalSince1970: timeInterval)
		
		if Calendar.current.isDateInToday(myNSDate) {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
			return formatter.string(from: myNSDate)
		} else if Calendar.current.isDate(myNSDate, equalTo: .now, toGranularity: .year) {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "dd/MM HH:mm" : "MM/dd h:mm a"
			return formatter.string(from: myNSDate)
		} else {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "dd/MM/yy HH:mm" : "MM/dd/yy h:mm a"
			return formatter.string(from: myNSDate)
		}
	}
	
	func getImageIMDN(status: Message.Status) -> String {
		switch status {
		case .sending:
			return ""
		case .sent:
			return "envelope-simple"
		case .received:
			return "check"
		case .read:
			return "checks"
		case .error:
			return "warning-circle"
		}
	}
	
	func removeReaction() {
		coreContext.doOnCoreQueue { _ in
			if self.selectedMessageToDisplayDetails != nil {
				Log.info("[ConversationViewModel] Remove reaction to message with ID \(self.selectedMessageToDisplayDetails!.message.id)")
				let messageToSendReaction = self.selectedMessageToDisplayDetails!.eventLog.chatMessage
				if messageToSendReaction != nil {
					do {
						let reaction = try messageToSendReaction!.createReaction(utf8Reaction: "")
						reaction.send()
						
						let indexMessageSelected = self.conversationMessagesSection[0].rows.firstIndex(of: self.selectedMessageToDisplayDetails!)
						
						DispatchQueue.main.async {
							if indexMessageSelected != nil {
								self.conversationMessagesSection[0].rows[indexMessageSelected!].message.ownReaction = ""
							}
							self.selectedMessageToDisplayDetails = nil
							self.isShowSelectedMessageToDisplayDetails = false
						}
					} catch {
						Log.info("[ConversationViewModel] Error: Can't remove reaction to message with ID \(self.selectedMessageToDisplayDetails!.message.id)")
					}
				}
			}
		}
	}
	
	func sendReaction(emoji: String) {
		coreContext.doOnCoreQueue { _ in
			if self.selectedMessage != nil {
				Log.info("[ConversationViewModel] Sending reaction \(emoji) to message with ID \(self.selectedMessage!.message.id)")
				let messageToSendReaction = self.selectedMessage!.eventLog.chatMessage
				if messageToSendReaction != nil {
					do {
						let reaction = try messageToSendReaction!.createReaction(utf8Reaction: messageToSendReaction?.ownReaction?.body == emoji ? "" : emoji)
						reaction.send()
						
						let indexMessageSelected = self.conversationMessagesSection[0].rows.firstIndex(of: self.selectedMessage!)
						
						DispatchQueue.main.async {
							if indexMessageSelected != nil {
								self.conversationMessagesSection[0].rows[indexMessageSelected!].message.ownReaction = messageToSendReaction?.ownReaction?.body == emoji ? "" : emoji
							}
							self.selectedMessage = nil
						}
					} catch {
						Log.info("[ConversationViewModel] Error: Can't send reaction \(emoji) to message with ID \(self.selectedMessage!.message.id)")
					}
				}
			}
		}
	}
	
	func resend() {
		coreContext.doOnCoreQueue { _ in
			if self.selectedMessage != nil && self.selectedMessage!.eventLog.chatMessage != nil {
				Log.info("[ConversationViewModel] Re-sending message with ID \(self.selectedMessage!.eventLog.chatMessage!)")
				self.selectedMessage!.eventLog.chatMessage!.send()
			}
		}
	}
	
	func prepareBottomSheetForDeliveryStatus() {
		self.sheetCategories.removeAll()
		coreContext.doOnCoreQueue { _ in
			if self.selectedMessageToDisplayDetails != nil && self.selectedMessageToDisplayDetails!.eventLog.chatMessage != nil {
				
				let participantsImdnDisplayed = self.selectedMessageToDisplayDetails!.eventLog.chatMessage!.getParticipantsByImdnState(state: .Displayed)
				var participantListDisplayed: [InnerSheetCategory] = []
				participantsImdnDisplayed.forEach({ participantImdn in
					if participantImdn.participant != nil && participantImdn.participant!.address != nil {
						ContactAvatarModel.getAvatarModelFromAddress(address: participantImdn.participant!.address!) { avatarResult in
							let innerSheetCat = InnerSheetCategory(contact: avatarResult, detail: self.getMessageTime(startDate: participantImdn.stateChangeTime))
							participantListDisplayed.append(innerSheetCat)
						}
					}
				})
				
				let participantsImdnDeliveredToUser = self.selectedMessageToDisplayDetails!.eventLog.chatMessage!.getParticipantsByImdnState(state: .DeliveredToUser)
				var participantListDeliveredToUser: [InnerSheetCategory] = []
				participantsImdnDeliveredToUser.forEach({ participantImdn in
					if participantImdn.participant != nil && participantImdn.participant!.address != nil {
						ContactAvatarModel.getAvatarModelFromAddress(address: participantImdn.participant!.address!) { avatarResult in
							let innerSheetCat = InnerSheetCategory(contact: avatarResult, detail: self.getMessageTime(startDate: participantImdn.stateChangeTime))
							participantListDeliveredToUser.append(innerSheetCat)
						}
					}
				})
				
				let participantsImdnDelivered = self.selectedMessageToDisplayDetails!.eventLog.chatMessage!.getParticipantsByImdnState(state: .Delivered)
				var participantListDelivered: [InnerSheetCategory] = []
				participantsImdnDelivered.forEach({ participantImdn in
					if participantImdn.participant != nil && participantImdn.participant!.address != nil {
						ContactAvatarModel.getAvatarModelFromAddress(address: participantImdn.participant!.address!) { avatarResult in
							let innerSheetCat = InnerSheetCategory(contact: avatarResult, detail: self.getMessageTime(startDate: participantImdn.stateChangeTime))
							participantListDelivered.append(innerSheetCat)
						}
					}
				})
				
				let participantsImdnNotDelivered = self.selectedMessageToDisplayDetails!.eventLog.chatMessage!.getParticipantsByImdnState(state: .NotDelivered)
				var participantListNotDelivered: [InnerSheetCategory] = []
				participantsImdnNotDelivered.forEach({ participantImdn in
					if participantImdn.participant != nil && participantImdn.participant!.address != nil {
						ContactAvatarModel.getAvatarModelFromAddress(address: participantImdn.participant!.address!) { avatarResult in
							let innerSheetCat = InnerSheetCategory(contact: avatarResult, detail: self.getMessageTime(startDate: participantImdn.stateChangeTime))
							participantListNotDelivered.append(innerSheetCat)
						}
					}
				})
				
				DispatchQueue.main.async {
					self.sheetCategories.append(SheetCategory(name: NSLocalizedString("message_delivery_info_read_title", comment: "") + " \(participantListDisplayed.count)", innerCategory: participantListDisplayed))
					self.sheetCategories.append(SheetCategory(name: NSLocalizedString("message_delivery_info_received_title", comment: "") + " \(participantListDeliveredToUser.count)", innerCategory: participantListDeliveredToUser))
					self.sheetCategories.append(SheetCategory(name: NSLocalizedString("message_delivery_info_sent_title", comment: "") + " \(participantListDelivered.count)", innerCategory: participantListDelivered))
					self.sheetCategories.append(SheetCategory(name: NSLocalizedString("message_delivery_info_error_title", comment: "") + " \(participantListNotDelivered.count)", innerCategory: participantListNotDelivered))
					
					self.isShowSelectedMessageToDisplayDetails = true
				}
			}
		}
	}
	
	func prepareBottomSheetForReactions() {
		self.sheetCategories.removeAll()
		coreContext.doOnCoreQueue { core in
			if self.selectedMessageToDisplayDetails != nil && self.selectedMessageToDisplayDetails!.eventLog.chatMessage != nil {
				let dispatchGroup = DispatchGroup()
				
				var sheetCategoriesTmp: [SheetCategory] = []
				
				var participantList: [[InnerSheetCategory]] = [[]]
				var reactionList: [String] = []
				
				self.selectedMessageToDisplayDetails!.eventLog.chatMessage!.reactions.forEach { chatMessageReaction in
					if chatMessageReaction.fromAddress != nil {
						dispatchGroup.enter()
						ContactAvatarModel.getAvatarModelFromAddress(address: chatMessageReaction.fromAddress!) { avatarResult in
							if core.defaultAccount != nil && core.defaultAccount!.contactAddress != nil && core.defaultAccount!.contactAddress!.asStringUriOnly().contains(avatarResult.address) {
								let innerSheetCat = InnerSheetCategory(contact: avatarResult, detail: chatMessageReaction.body, isMe: true)
								participantList[0].append(innerSheetCat)
							} else {
								let innerSheetCat = InnerSheetCategory(contact: avatarResult, detail: chatMessageReaction.body)
								participantList[0].append(innerSheetCat)
							}
							
							if !reactionList.contains(where: {$0 == chatMessageReaction.body}) {
								reactionList.append(chatMessageReaction.body)
							}
							
							dispatchGroup.leave()
						}
					}
				}
				
				dispatchGroup.notify(queue: .main) {
					reactionList.forEach { reaction in
						participantList.append([])
						participantList[0].forEach { innerSheetCategory in
							if innerSheetCategory.detail == reaction {
								participantList[participantList.count - 1].append(innerSheetCategory)
							}
						}
					}
					
					sheetCategoriesTmp.append(SheetCategory(name: NSLocalizedString("message_reactions_info_all_title", comment: "") + " \(participantList.first!.count)", innerCategory: participantList.first!))
					
					reactionList.enumerated().forEach { index, reaction in
						sheetCategoriesTmp.append(SheetCategory(name: reaction + " \(participantList[index + 1].count)", innerCategory: participantList[index + 1]))
					}
					
					DispatchQueue.main.async {
						self.sheetCategories = sheetCategoriesTmp
						self.isShowSelectedMessageToDisplayDetails = true
					}
				}
			}
		}
	}
	
	func startVoiceRecordPlayer(voiceRecordPath: URL) {
		coreContext.doOnCoreQueue { core in
			if self.vrpManager == nil || self.vrpManager!.voiceRecordPath != voiceRecordPath {
				self.vrpManager = VoiceRecordPlayerManager(core: core, voiceRecordPath: voiceRecordPath)
			}
			
			if self.vrpManager != nil {
				self.vrpManager!.startVoiceRecordPlayer()
			}
		}
	}
	
	func getPositionVoiceRecordPlayer(voiceRecordPath: URL) -> Double {
		if self.vrpManager != nil && self.vrpManager!.voiceRecordPath == voiceRecordPath {
			return self.vrpManager!.positionVoiceRecordPlayer()
		} else {
			return 0
		}
	}
	
	func isPlayingVoiceRecordPlayer(voiceRecordPath: URL) -> Bool {
		if self.vrpManager != nil && self.vrpManager!.voiceRecordPath == voiceRecordPath {
			return true
		} else {
			return false
		}
	}
	
	func pauseVoiceRecordPlayer() {
		coreContext.doOnCoreQueue { _ in
			if self.vrpManager != nil {
				self.vrpManager!.pauseVoiceRecordPlayer()
			}
		}
	}
	
	func stopVoiceRecordPlayer() {
		coreContext.doOnCoreQueue { _ in
			if self.vrpManager != nil {
				self.vrpManager!.stopVoiceRecordPlayer()
			}
		}
	}
}
// swiftlint:enable line_length
// swiftlint:enable type_body_length
// swiftlint:enable cyclomatic_complexity

class VoiceRecordPlayerManager {
	private var core: Core
	var voiceRecordPath: URL
	private var voiceRecordPlayer: Player?
	//private var isPlayingVoiceRecord = false
	private var voiceRecordAudioFocusRequest: AVAudioSession?
	//private var voiceRecordPlayerPosition: Double = 0
	//private var voiceRecordingDuration: TimeInterval = 0
	
	init(core: Core, voiceRecordPath: URL) {
		self.core = core
		self.voiceRecordPath = voiceRecordPath
	}
	
	private func initVoiceRecordPlayer() {
		print("Creating player for voice record")
		do {
			voiceRecordPlayer = try core.createLocalPlayer(soundCardName: getSpeakerSoundCard(core: core), videoDisplayName: nil, windowId: nil)
		} catch {
			print("Couldn't create local player!")
		}
		
		print("Voice record player created")
		print("Opening voice record file [\(voiceRecordPath.absoluteString)]")
		
		do {
			try voiceRecordPlayer!.open(filename: String(voiceRecordPath.absoluteString.dropFirst(7)))
			print("Player opened file at [\(voiceRecordPath.absoluteString)]")
		} catch {
			print("Player failed to open file at [\(voiceRecordPath.absoluteString)]")
		}
	}
	
	func startVoiceRecordPlayer() {
		if voiceRecordAudioFocusRequest == nil {
			voiceRecordAudioFocusRequest = AVAudioSession.sharedInstance()
			if let request = voiceRecordAudioFocusRequest {
				try? request.setActive(true)
			}
		}
		
		if isPlayerClosed() {
			print("Player closed, let's open it first")
			initVoiceRecordPlayer()
			
			if voiceRecordPlayer!.state == .Closed {
				print("It seems the player fails to open the file, abort playback")
				// Handle the failure (e.g. show a toast)
				return
			}
		}
		
		do {
			try voiceRecordPlayer!.start()
			print("Playing voice record")
		} catch {
			print("Player failed to start voice recording")
		}
	}
	
	func positionVoiceRecordPlayer() -> Double {
		if !isPlayerClosed() {
			return Double(voiceRecordPlayer!.currentPosition) / Double(voiceRecordPlayer!.duration) * 100
		} else {
			return 0.0
		}
	}
	
	func pauseVoiceRecordPlayer() {
		if !isPlayerClosed() {
			print("Pausing voice record")
			try? voiceRecordPlayer?.pause()
		}
	}
	
	private func isPlayerClosed() -> Bool {
		return voiceRecordPlayer == nil || voiceRecordPlayer?.state == .Closed
	}
	
	func stopVoiceRecordPlayer() {
		if !isPlayerClosed() {
			print("Stopping voice record")
			try? voiceRecordPlayer?.pause()
			try? voiceRecordPlayer?.seek(timeMs: 0)
			voiceRecordPlayer?.close()
		}
		
		if let request = voiceRecordAudioFocusRequest {
			try? request.setActive(false)
			voiceRecordAudioFocusRequest = nil
		}
	}
	
	func getSpeakerSoundCard(core: Core) -> String? {
		var speakerCard: String? = nil
		var earpieceCard: String? = nil
		core.audioDevices.forEach { device in
			if (device.hasCapability(capability: .CapabilityPlay)) {
				if (device.type == .Speaker) {
					speakerCard = device.id
				} else if (device.type == .Earpiece) {
					earpieceCard = device.id
				}
			}
		}
		return speakerCard != nil ? speakerCard : earpieceCard
	}
	
	func changeRouteToSpeaker() {
		core.outputAudioDevice = core.audioDevices.first { $0.type == AudioDevice.Kind.Speaker }
		UIDevice.current.isProximityMonitoringEnabled = false
	}
}

class AudioRecorder: NSObject, ObservableObject {
	var linphoneAudioRecorder: Recorder!
	var recordingSession: AVAudioSession?
	@Published var isRecording = false
	@Published var audioFilename: URL?
	@Published var audioFilenameAAC: URL?
	@Published var recordingTime: TimeInterval = 0
	@Published var soundPower: Float = 0
	
	var timer: Timer?
	
	func startRecording() {
		recordingSession = AVAudioSession.sharedInstance()
		CoreContext.shared.doOnCoreQueue { core in
			core.activateAudioSession(activated: true)
		}
		
		if recordingSession != nil {
			do {
				try recordingSession!.setCategory(.playAndRecord, mode: .default)
				try recordingSession!.setActive(true)
				recordingSession!.requestRecordPermission { allowed in
					if allowed {
						self.initVoiceRecorder()
					} else {
						print("Permission to record not granted.")
					}
				}
			} catch {
				print("Failed to setup recording session.")
			}
		}
	}
	
	private func initVoiceRecorder() {
		CoreContext.shared.doOnCoreQueue { core in
			Log.info("[ConversationViewModel] [AudioRecorder] Creating voice message recorder")
			let recorderParams = try? core.createRecorderParams()
			if recorderParams != nil {
				recorderParams!.fileFormat = MediaFileFormat.Mkv
				
				let recordingAudioDevice = self.getAudioRecordingDeviceIdForVoiceMessage()
				recorderParams!.audioDevice = recordingAudioDevice
				Log.info(
					"[ConversationViewModel] [AudioRecorder] Using device \(recorderParams!.audioDevice?.id ?? "Error id") to make the voice message recording"
				)
				
				self.linphoneAudioRecorder = try? core.createRecorder(params: recorderParams!)
				Log.info("[ConversationViewModel] [AudioRecorder] Voice message recorder created")
				
				self.startVoiceRecorder()
			}
		}
	}
	
	func startVoiceRecorder() {
		switch linphoneAudioRecorder.state {
		case .Running:
			Log.warn("[ConversationViewModel] [AudioRecorder] Recorder is already recording")
		case .Paused:
			Log.warn("[ConversationViewModel] [AudioRecorder] Recorder is paused, resuming recording")
			try? linphoneAudioRecorder.start()
		case .Closed:
			var extensionFileFormat: String = ""
			switch linphoneAudioRecorder.params?.fileFormat {
			case .Smff:
				extensionFileFormat = "smff"
			case .Mkv:
				extensionFileFormat = "mka"
			default:
				extensionFileFormat = "wav"
			}
			
			let tempFileName = "voice-recording-\(Int(Date().timeIntervalSince1970)).\(extensionFileFormat)"
			audioFilename = FileUtil.sharedContainerUrl().appendingPathComponent("Library/Images").appendingPathComponent(tempFileName)
			
			if audioFilename != nil {
				Log.warn("[ConversationViewModel] [AudioRecorder] Recorder is closed, starting recording in \(audioFilename!.absoluteString)")
				try? linphoneAudioRecorder.open(file: String(audioFilename!.absoluteString.dropFirst(7)))
				try? linphoneAudioRecorder.start()
			}
			
			startTimer()
			
			DispatchQueue.main.async {
				self.isRecording = true
			}
		}
	}

	func stopVoiceRecorder() {
		if linphoneAudioRecorder.state == .Running {
			Log.info("[ConversationViewModel] [AudioRecorder] Closing voice recorder")
			try? linphoneAudioRecorder.pause()
			linphoneAudioRecorder.close()
		}
		
		stopTimer()
		
		DispatchQueue.main.async {
			self.isRecording = false
		}
		
		if let request = recordingSession {
			Log.info("[ConversationViewModel] [AudioRecorder] Releasing voice recording audio focus request")
			try? request.setActive(false)
			recordingSession = nil
			CoreContext.shared.doOnCoreQueue { core in
				core.activateAudioSession(activated: false)
			}
		}
	}
	
	func startTimer() {
		DispatchQueue.main.async {
			self.recordingTime = 0
			let maxVoiceRecordDuration = Config.voiceRecordingMaxDuration
			self.timer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { _ in  // More frequent updates
				self.recordingTime += 0.1
				self.updateSoundPower()
				let duration = self.linphoneAudioRecorder.duration
				if duration >= maxVoiceRecordDuration {
					print("[ConversationViewModel] [AudioRecorder] Max duration for voice recording exceeded (\(maxVoiceRecordDuration)ms), stopping.")
					self.stopVoiceRecorder()
				}
			}
		}
	}
	
	func stopTimer() {
		self.timer?.invalidate()
		self.timer = nil
	}
	
	func updateSoundPower() {
		let soundPowerTmp = linphoneAudioRecorder.captureVolume * 1000	// Capture sound power
		soundPower = soundPowerTmp < 10 ? 0 : (soundPowerTmp > 100 ? 100 : (soundPowerTmp - 10))
	}
	
	func getAudioRecordingDeviceIdForVoiceMessage() -> AudioDevice? {
		// In case no headset/hearing aid/bluetooth is connected, use microphone sound card
		// If none are available, default one will be used
		var headsetCard: AudioDevice?
		var bluetoothCard: AudioDevice?
		var microphoneCard: AudioDevice?
		
		CoreContext.shared.doOnCoreQueue { core in
			for device in core.audioDevices {
				if device.hasCapability(capability: .CapabilityRecord) {
					switch device.type {
					case .Headphones, .Headset:
						headsetCard = device
					case .Bluetooth, .HearingAid:
						bluetoothCard = device
					case .Microphone:
						microphoneCard = device
					default:
						break
					}
				}
			}
		}

		Log.info("Found headset/headphones/hearingAid sound card [\(String(describing: headsetCard))], "
				 + "bluetooth sound card [\(String(describing: bluetoothCard))] and microphone card [\(String(describing: microphoneCard))]")

		return headsetCard ?? bluetoothCard ?? microphoneCard
	}
}
