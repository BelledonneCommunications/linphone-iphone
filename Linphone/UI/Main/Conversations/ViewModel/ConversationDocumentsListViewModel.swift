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

final class ConversationDocumentsListViewModel: ObservableObject {

	private static let TAG = "[ConversationDocumentsListViewModel]"
	private static let CONTENTS_PER_PAGE = 20

	@Published var documentsList: [FileModel] = []
	@Published var operationInProgress: Bool = false

	private var totalDocumentsCount: Int = -1
	
	private var conversationModel: ConversationModel!

	init() {
		if let conversationModelTmp = SharedMainViewModel.shared.displayedConversation {
			self.conversationModel = conversationModelTmp
			loadDocumentsList()
		}
	}

	// MARK: - Loading
	private func loadDocumentsList() {
		self.operationInProgress = true
		CoreContext.shared.doOnCoreQueue { _ in
			self.totalDocumentsCount = self.conversationModel.chatRoom.documentContentsSize
			
			let contentsToLoad = min(self.totalDocumentsCount, Self.CONTENTS_PER_PAGE)
			let contents = self.conversationModel.chatRoom.getDocumentContentsRange(begin: 0, end: contentsToLoad)
			
			let documentsListTmp = self.getFileModelsList(from: contents)
			
			DispatchQueue.main.async {
				self.documentsList = documentsListTmp
				self.operationInProgress = false
			}
		}
	}

	func loadMoreData(totalItemsCount: Int) {
		self.operationInProgress = true
		CoreContext.shared.doOnCoreQueue { _ in
			guard totalItemsCount < self.totalDocumentsCount else {
				DispatchQueue.main.async {
					self.operationInProgress = false
				}
				return
			}
			
			var upperBound = totalItemsCount + Self.CONTENTS_PER_PAGE
			if upperBound > self.totalDocumentsCount {
				upperBound = self.totalDocumentsCount
			}
			
			let contents = self.conversationModel.chatRoom.getDocumentContentsRange(begin: totalItemsCount, end: upperBound)
			let newModels = self.getFileModelsList(from: contents)
			
			DispatchQueue.main.async {
				self.documentsList.append(contentsOf: newModels)
				self.operationInProgress = false
			}
		}
		
	}

	// MARK: - Mapping Content -> FileModel
	private func getFileModelsList(from contents: [Content]) -> [FileModel] {
		var list: [FileModel] = []

		for documentContent in contents {
			let isEncrypted = documentContent.isFileEncrypted
			let originalPath = documentContent.filePath ?? ""
			let path = isEncrypted ? documentContent.exportPlainFile() : originalPath
			let name = documentContent.name ?? ""
			let size = Int64(documentContent.size)
			let timestamp = documentContent.creationTimestamp

			if path.isEmpty || name.isEmpty { continue }
			
			let ephemeral: Bool
			if let messageId = documentContent.relatedChatMessageId {
				if let chatMessage = self.conversationModel.chatRoom.findMessage(messageId: messageId) {
					ephemeral = chatMessage.isEphemeral
				} else {
					ephemeral = self.conversationModel.chatRoom.ephemeralEnabled
				}
			} else {
				ephemeral = self.conversationModel.chatRoom.ephemeralEnabled
			}

			let model = FileModel(
				path: path,
				fileName: name,
				fileSize: size,
				fileCreationTimestamp: Int64(timestamp),
				isEncrypted: isEncrypted,
				originalPath: originalPath,
				isFromEphemeralMessage: ephemeral
			)

			list.append(model)
		}

		return list
	}
}
