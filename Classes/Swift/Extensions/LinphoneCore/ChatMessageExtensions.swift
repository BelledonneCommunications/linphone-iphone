/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
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


class DownloadListener {
	private var chatMessageDelegate : ChatMessageDelegateStub? = nil
	var messageId: String
	init(message:ChatMessage, ok: @escaping ()->Void, ko: ()->Void) {
		messageId = message.messageId
		chatMessageDelegate = ChatMessageDelegateStub(
			onFileTransferProgressIndication: { (message: ChatMessage, content: Content, offset: Int, total: Int) -> Void in
				if (content.name == message.audioContent()?.name && offset >= total) {
					message.removeDelegate(delegate: self.chatMessageDelegate!)
					ok()
				}
			})
		message.addDelegate(delegate: chatMessageDelegate!)
	}
}

extension Content {
	func downloaded() -> Bool {
		if (!isFileTransfer) {
			return true
		}
		let isVfs = VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId)
		if let path = isVfs ? exportPlainFile() : filePath {
			let downloaded = FileUtil.fileExistsAndIsNotEmpty(path: path)
			if (isVfs) {
				FileUtil.delete(path: path)
			}
			return downloaded
		} else {
			return false
		}
	}
}

extension ChatMessage {
	
	static var downloadListeners:[DownloadListener] = []
	
	func downloadInProgress() -> Bool {
		return state == .FileTransferInProgress
	}
	
	func downloadError() -> Bool {
		return state == .FileTransferError
	}
	
	func hasAudioContent() -> Bool {
		return audioContent() != nil
	}
	
	func audioContent() -> Content?  { // Messages can have only one voice recording.
		return contents.filter({$0.isVoiceRecording}).first
	}

	func downloadAudioContent(ok: @escaping ()->Void, ko: ()->Void) {
		if let audioContent = audioContent() {
			let downloadListener = DownloadListener(message: self, ok: {
				ChatMessage.downloadListeners.removeAll(where: {$0.messageId == self.messageId})
				ok()
			}, ko: {
				ChatMessage.downloadListeners.removeAll(where: {$0.messageId == self.messageId})
				ko()
			})
			if (!audioContent.downloaded()) {
				audioContent.filePath = LinphoneManager.imagesDirectory() + audioContent.name!
				ChatMessage.downloadListeners.append(downloadListener)
				let _ = downloadContent(content: audioContent)
			} else {
				ok()
			}
		} else {
			ok()
		}
	}
	
}

