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

// swiftlint:disable type_body_length
class ConversationViewModel: ObservableObject {
	
	private var coreContext = CoreContext.shared
	
	@Published var displayedConversation: ConversationModel?
	@Published var displayedConversationHistorySize: Int = 0
	@Published var displayedConversationUnreadMessagesCount: Int = 0
	
	@Published var messageText: String = ""
	
	private var chatRoomSuscriptions = Set<AnyCancellable?>()
	
	@Published var conversationMessagesSection: [MessagesSection] = []
	@Published var participantConversationModel: [ContactAvatarModel] = []
	
	@Published var mediasToSend: [Attachment] = []
	var maxMediaCount = 12
	
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
	
	func removeConversationDelegate() {
		self.chatRoomSuscriptions.removeAll()
	}
	
	func getHistorySize() {
		coreContext.doOnCoreQueue { _ in
			if self.displayedConversation != nil {
				let historySize = self.displayedConversation!.chatRoom.historySize
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
			let unreadMessagesCount = self.displayedConversation!.chatRoom.unreadMessagesCount
			
			if unreadMessagesCount > 0 {
				self.displayedConversation!.chatRoom.markAsRead()
				
				DispatchQueue.main.async {
					self.displayedConversationUnreadMessagesCount = 0
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
			}
		}
	}
	
	func getMessages() {
		self.getHistorySize()
		self.getUnreadMessagesCount()
		self.getParticipantConversationModel()
		coreContext.doOnCoreQueue { _ in
			if self.displayedConversation != nil {
				let historyEvents = self.displayedConversation!.chatRoom.getHistoryRangeEvents(begin: 0, end: 30)
				
				var conversationMessage: [Message] = []
				historyEvents.enumerated().forEach { index, eventLog in
					
					var attachmentList: [Attachment] = []
					var contentText = ""
					
					if eventLog.chatMessage != nil && !eventLog.chatMessage!.contents.isEmpty {
						eventLog.chatMessage!.contents.forEach { content in
							if content.isText {
								contentText = content.utf8Text ?? ""
							} else if content.name != nil && !content.name!.isEmpty {
								if content.filePath == nil || content.filePath!.isEmpty {
									self.downloadContent(chatMessage: eventLog.chatMessage!, content: content)
								} else {
									if content.type != "video" {
										let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
										if path != nil {
											let attachment =
											Attachment(
												id: UUID().uuidString,
												url: path!,
												type: (content.name?.lowercased().hasSuffix("gif"))! ? .gif : .image
											)
											attachmentList.append(attachment)
										}
									} else if content.type == "video" {
										let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
										let pathThumbnail = URL(string: self.generateThumbnail(name: content.name ?? ""))
										if path != nil && pathThumbnail != nil {
											let attachment =
											Attachment(
												id: UUID().uuidString,
												thumbnail: pathThumbnail!,
												full: path!,
												type: .video
											)
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
					default:
						statusTmp = nil
					}
					
					if eventLog.chatMessage != nil {
						conversationMessage.append(
							Message(
								id: UUID().uuidString,
								status: statusTmp,
								isOutgoing: eventLog.chatMessage?.isOutgoing ?? false,
								dateReceived: eventLog.chatMessage?.time ?? 0,
								address: addressCleaned?.asStringUriOnly() ?? "",
								isFirstMessage: isFirstMessageTmp,
								text: contentText,
								attachments: attachmentList
							)
						)
					}
				}
				
				DispatchQueue.main.async {
					if self.conversationMessagesSection.isEmpty {
						self.conversationMessagesSection.append(MessagesSection(date: Date(), rows: conversationMessage.reversed()))
					}
				}
			}
		}
	}
	
	func getOldMessages() {
		coreContext.doOnCoreQueue { _ in
			if self.displayedConversation != nil {
				let historyEvents = self.displayedConversation!.chatRoom.getHistoryRangeEvents(begin: self.conversationMessagesSection[0].rows.count, end: self.conversationMessagesSection[0].rows.count + 30)
				var conversationMessagesTmp: [Message] = []
				
				historyEvents.enumerated().reversed().forEach { index, eventLog in
					var attachmentList: [Attachment] = []
					var contentText = ""
					
					if eventLog.chatMessage != nil && !eventLog.chatMessage!.contents.isEmpty {
						eventLog.chatMessage!.contents.forEach { content in
							if content.isText {
								contentText = content.utf8Text ?? ""
							} else if content.name != nil && !content.name!.isEmpty {
								if content.filePath == nil || content.filePath!.isEmpty {
									self.downloadContent(chatMessage: eventLog.chatMessage!, content: content)
								} else {
									if content.type != "video" {
										let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
										if path != nil {
											let attachment =
											Attachment(
												id: UUID().uuidString,
												url: path!,
												type: (content.name?.lowercased().hasSuffix("gif"))! ? .gif : .image
											)
											attachmentList.append(attachment)
										}
									} else if content.type == "video" {
										let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
										let pathThumbnail = URL(string: self.generateThumbnail(name: content.name ?? ""))
										if path != nil && pathThumbnail != nil {
											let attachment =
											Attachment(
												id: UUID().uuidString,
												thumbnail: pathThumbnail!,
												full: path!,
												type: .video
											)
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
					default:
						statusTmp = nil
					}
					
					if eventLog.chatMessage != nil {
						conversationMessagesTmp.insert(
							Message(
								id: UUID().uuidString,
								status: statusTmp,
								isOutgoing: eventLog.chatMessage?.isOutgoing ?? false,
								dateReceived: eventLog.chatMessage?.time ?? 0,
								address: addressCleaned?.asStringUriOnly() ?? "",
								isFirstMessage: isFirstMessageTmp,
								text: contentText,
								attachments: attachmentList
							), at: 0
						)
					}
				}
				
				if !conversationMessagesTmp.isEmpty {
					DispatchQueue.main.async {
						if self.conversationMessagesSection[0].rows.last?.address == conversationMessagesTmp.last?.address {
							self.conversationMessagesSection[0].rows[self.conversationMessagesSection[0].rows.count - 1].isFirstMessage = false
						}
						self.conversationMessagesSection[0].rows.append(contentsOf: conversationMessagesTmp.reversed())
					}
				}
			}
		}
	}
	
	func getNewMessages(eventLogs: [EventLog]) {
		eventLogs.enumerated().forEach { index, eventLog in
			var attachmentList: [Attachment] = []
			var contentText = ""
			
			if eventLog.chatMessage != nil && !eventLog.chatMessage!.contents.isEmpty {
				eventLog.chatMessage!.contents.forEach { content in
					if content.isText {
						contentText = content.utf8Text ?? ""
					} else {
						if content.filePath == nil || content.filePath!.isEmpty {
							self.downloadContent(chatMessage: eventLog.chatMessage!, content: content)
						} else if content.name != nil && !content.name!.isEmpty {
							if content.type != "video" {
								let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
								if path != nil {
									let attachment =
									Attachment(
										id: UUID().uuidString,
										url: path!,
										type: (content.name?.lowercased().hasSuffix("gif"))! ? .gif : .image
									)
									attachmentList.append(attachment)
								}
							} else if content.type == "video" {
								let path = URL(string: self.getNewFilePath(name: content.name ?? ""))
								let pathThumbnail = URL(string: self.generateThumbnail(name: content.name ?? ""))
								if path != nil && pathThumbnail != nil {
									let attachment =
									Attachment(
										id: UUID().uuidString,
										thumbnail: pathThumbnail!,
										full: path!,
										type: .video
									)
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
			
			let isFirstMessageIncomingTmp = index > 0
			? addressPrecCleaned?.asStringUriOnly() != addressCleaned?.asStringUriOnly()
			: (
				self.conversationMessagesSection.isEmpty || self.conversationMessagesSection[0].rows.isEmpty
				? true
				: self.conversationMessagesSection[0].rows[0].address != addressCleaned?.asStringUriOnly()
			)
			
			let isFirstMessageOutgoingTmp = index <= eventLogs.count - 2
			? addressNextCleaned?.asStringUriOnly() == addressCleaned?.asStringUriOnly()
			: (
				self.conversationMessagesSection.isEmpty || self.conversationMessagesSection[0].rows.isEmpty
				? true
				: !self.conversationMessagesSection[0].rows[0].isOutgoing || self.conversationMessagesSection[0].rows[0].address == addressCleaned?.asStringUriOnly()
			)
			
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
			default:
				statusTmp = nil
			}
			
			if eventLog.chatMessage != nil {
				let message = Message(
					id: UUID().uuidString,
					status: statusTmp,
					isOutgoing: eventLog.chatMessage?.isOutgoing ?? false,
					dateReceived: eventLog.chatMessage?.time ?? 0,
					address: addressCleaned?.asStringUriOnly() ?? "",
					isFirstMessage: isFirstMessageTmp,
					text: contentText,
					attachments: attachmentList
				)
				
				DispatchQueue.main.async {
					if !self.conversationMessagesSection.isEmpty 
						&& !self.conversationMessagesSection[0].rows.isEmpty
						&& self.conversationMessagesSection[0].rows[0].isOutgoing
						&& (self.conversationMessagesSection[0].rows[0].address == message.address) {
						self.conversationMessagesSection[0].rows[0].isFirstMessage = false
					}
					
					if self.conversationMessagesSection.isEmpty {
						self.conversationMessagesSection.append(MessagesSection(date: Date(), rows: [message]))
					} else {
						self.conversationMessagesSection[0].rows.insert(message, at: 0)
					}
					
					if !message.isOutgoing {
						self.displayedConversationUnreadMessagesCount += 1
					}
				}
			}
			
			if self.displayedConversation != nil {
				self.displayedConversation!.markAsRead()
			}
		}
	}
	
	func resetMessage() {
		conversationMessagesSection = []
	}
	
	func sendMessage() {
		coreContext.doOnCoreQueue { _ in
			//val messageToReplyTo = chatMessageToReplyTo
			//val message = if (messageToReplyTo != null) {
			//Log.i("$TAG Sending message as reply to [${messageToReplyTo.messageId}]")
			//chatRoom.createReplyMessage(messageToReplyTo)
			//} else {
			let message = try? self.displayedConversation!.chatRoom.createEmptyMessage()
			//}
			
			let toSend = self.messageText.trimmingCharacters(in: .whitespacesAndNewlines)
			if !toSend.isEmpty {
				if message != nil {
					message!.addUtf8TextContent(text: toSend)
				}
			}
			
			/*
			 if (isVoiceRecording.value == true && voiceMessageRecorder.file != null) {
			 stopVoiceRecorder()
			 val content = voiceMessageRecorder.createContent()
			 if (content != null) {
			 Log.i(
			 "$TAG Voice recording content created, file name is ${content.name} and duration is ${content.fileDuration}"
			 )
			 message.addContent(content)
			 } else {
			 Log.e("$TAG Voice recording content couldn't be created!")
			 }
			 } else {
			 */
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
					
					//content.subtype = attachment.type == .plainText ? "plain" : FileUtils.getExtensionFromFileName(attachment.fileName)
					content.subtype = attachment.full.pathExtension
					
					content.name = attachment.full.lastPathComponent
					
					if message != nil {
						
						let path = FileManager.default.temporaryDirectory.appendingPathComponent((attachment.full.lastPathComponent.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? ""))
						let newPath = URL(string: "file://" + Factory.Instance.getDownloadDir(context: nil) + (attachment.full.lastPathComponent.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? ""))
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
			//}
			
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
		}
	}
	
	func changeDisplayedChatRoom(conversationModel: ConversationModel) {
		self.displayedConversation = conversationModel
	}
	
	func downloadContent(chatMessage: ChatMessage, content: Content) {
		//Log.debug("[ConversationViewModel] Starting downloading content for file \(model.fileName)")
		if content.filePath == nil || content.filePath!.isEmpty {
			let contentName = content.name
			if contentName != nil {
				let isImage = FileUtil.isExtensionImage(path: contentName!)
				let file = FileUtil.getFileStoragePath(fileName: contentName ?? "", isImage: isImage)
				content.filePath = file
				Log.info(
					"[ConversationViewModel] File \(contentName) will be downloaded at \(content.filePath)"
				)
				self.displayedConversation?.downloadContent(chatMessage: chatMessage, content: content)
			} else {
				Log.error("[ConversationViewModel] Content name is null, can't download it!")
			}
		}
	}
	
	func getNewFilePath(name: String) -> String {
		return "file://" + Factory.Instance.getDownloadDir(context: nil) + (name.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? "")
	}
	
	func generateThumbnail(name: String, pathThumbnail: URL? = nil) -> String {
		do {
			let path = pathThumbnail == nil
			? URL(string: "file://" + Factory.Instance.getDownloadDir(context: nil) + (name.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? ""))
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
			? URL(string: "file://" + Factory.Instance.getDownloadDir(context: nil) + "preview_" + (name.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? "") + ".png")
			: pathThumbnail!.appendingPathComponent("preview_" + (name.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? "") + ".png")
			
			if urlName != nil {
				let decodedData: () = try data.write(to: urlName!)
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
			return ""
		}
	}
	
}
struct LinphoneCustomEventLog: Hashable {
	var id = UUID()
	var eventLog: EventLog
	
	func hash(into hasher: inout Hasher) {
		hasher.combine(id)
	}
}

extension LinphoneCustomEventLog {
	static func ==(lhs: LinphoneCustomEventLog, rhs: LinphoneCustomEventLog) -> Bool {
		return lhs.id == rhs.id
	}
}
// swiftlint:enable type_body_length
