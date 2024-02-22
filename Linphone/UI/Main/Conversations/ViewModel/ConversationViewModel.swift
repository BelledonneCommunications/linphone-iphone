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

class ConversationViewModel: ObservableObject {
	
	private var coreContext = CoreContext.shared
	
	@Published var displayedConversation: ChatRoom?
	
	@Published var messageText: String = ""
	
	private var chatRoomSuscriptions = Set<AnyCancellable?>()
	
	@Published var conversationMessagesList: [LinphoneCustomEventLog] = []
	
	init() {}
	
	func addConversationDelegate() {
		if displayedConversation != nil {
			self.chatRoomSuscriptions.insert(displayedConversation!.publisher?.onChatMessageSent?.postOnMainQueue { (cbValue: (chatRoom: ChatRoom, eventLog: EventLog)) in
				self.getNewMessages(eventLogs: [cbValue.eventLog])
			})
		
			self.chatRoomSuscriptions.insert(displayedConversation!.publisher?.onChatMessagesReceived?.postOnMainQueue { (cbValue: (chatRoom: ChatRoom, eventLogs: [EventLog])) in
				self.getNewMessages(eventLogs: cbValue.eventLogs)
			})
		}
	}
	
	func removeConversationDelegate() {
		self.chatRoomSuscriptions.removeAll()
	}
	
	func getMessage() {
		if self.displayedConversation != nil {
			let historyEvents = displayedConversation!.getHistoryRangeEvents(begin: conversationMessagesList.count, end: conversationMessagesList.count + 30)
			
			historyEvents.reversed().forEach { eventLog in
				conversationMessagesList.append(LinphoneCustomEventLog(eventLog: eventLog))
			}
		}
	}
	
	func getNewMessages(eventLogs: [EventLog]) {
		withAnimation {
			eventLogs.forEach { eventLog in
				conversationMessagesList.insert(LinphoneCustomEventLog(eventLog: eventLog), at: 0)
				//conversationMessagesList.append(LinphoneCustomEventLog(eventLog: eventLog))
			}
		}
	}
	
	func resetMessage() {
		conversationMessagesList = []
	}
	
	func sendMessage() {
		//val messageToReplyTo = chatMessageToReplyTo
		//val message = if (messageToReplyTo != null) {
			//Log.i("$TAG Sending message as reply to [${messageToReplyTo.messageId}]")
			//chatRoom.createReplyMessage(messageToReplyTo)
		//} else {
		let message = try? self.displayedConversation!.createEmptyMessage()
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
			for (attachment in attachments.value.orEmpty()) {
				val content = Factory.instance().createContent()

				content.type = when (attachment.mimeType) {
					FileUtils.MimeType.Image -> "image"
					FileUtils.MimeType.Audio -> "audio"
					FileUtils.MimeType.Video -> "video"
					FileUtils.MimeType.Pdf -> "application"
					FileUtils.MimeType.PlainText -> "text"
					else -> "file"
				}
				content.subtype = if (attachment.mimeType == FileUtils.MimeType.PlainText) {
					"plain"
				} else {
					FileUtils.getExtensionFromFileName(attachment.fileName)
				}
				content.name = attachment.fileName
				// Let the file body handler take care of the upload
				content.filePath = attachment.file

				message.addFileContent(content)
			}
		}
		 */

		if message != nil && !message!.contents.isEmpty {
			Log.info("[ConversationViewModel] Sending message")
			message!.send()
		}

		Log.info("[ConversationViewModel] Message sent, re-setting defaults")
		self.messageText = ""
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
