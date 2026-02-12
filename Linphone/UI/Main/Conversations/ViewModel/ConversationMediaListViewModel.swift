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

final class ConversationMediaListViewModel: ObservableObject {

	private static let TAG = "[ConversationMediaListViewModel]"
	private static let CONTENTS_PER_PAGE = 50

	@Published var mediaList: [FileModel] = []
	@Published var operationInProgress: Bool = false

	private var totalMediaCount: Int = -1
	
	private var conversationModel: ConversationModel!
	
	init() {
		if let conversationModelTmp = SharedMainViewModel.shared.displayedConversation {
			self.conversationModel = conversationModelTmp
			loadMediaList()
		}
	}

	// MARK: - Loading

	private func loadMediaList() {
		self.operationInProgress = true
		CoreContext.shared.doOnCoreQueue { _ in
			Log.info("\(Self.TAG) Loading media contents for conversation \(self.conversationModel.chatRoom.identifier ?? "No ID")")
			
			self.totalMediaCount = self.conversationModel.chatRoom.mediaContentsSize
			Log.info("\(Self.TAG) Media contents size is [\(self.totalMediaCount)]")
			
			let contentsToLoad = min(self.totalMediaCount, Self.CONTENTS_PER_PAGE)
			let contents = self.conversationModel.chatRoom.getMediaContentsRange(begin: 0, end: contentsToLoad)
			
			Log.info("\(Self.TAG) \(contents.count) media have been fetched")
			
			DispatchQueue.main.async {
				self.mediaList = self.getFileModelsList(from: contents)
				self.operationInProgress = false
			}
		}
	}

	func loadMoreData(totalItemsCount: Int) {
		self.operationInProgress = true
		CoreContext.shared.doOnCoreQueue { _ in
			Log.info("\(Self.TAG) Loading more data, current total is \(totalItemsCount), max size is \(self.totalMediaCount)")

			guard totalItemsCount < self.totalMediaCount else {
				DispatchQueue.main.async {
					self.operationInProgress = false
				}
				return
			}

			var upperBound = totalItemsCount + Self.CONTENTS_PER_PAGE
			if upperBound > self.totalMediaCount {
				upperBound = self.totalMediaCount
			}

			let contents = self.conversationModel.chatRoom.getMediaContentsRange(begin: totalItemsCount, end: upperBound)
			Log.info("\(Self.TAG) \(contents.count) contents loaded, adding them to list")

			let newModels = self.getFileModelsList(from: contents)

			DispatchQueue.main.async {
				self.mediaList.append(contentsOf: newModels)
				self.operationInProgress = false
			}
		}
	}

	// MARK: - Mapping Content -> FileModel

	private func getFileModelsList(from contents: [Content]) -> [FileModel] {
		var list: [FileModel] = []

		for mediaContent in contents {
			
			if mediaContent.isVoiceRecording { continue }

			let isEncrypted = mediaContent.isFileEncrypted
			let originalPath = mediaContent.filePath ?? ""

			let path: String
			if isEncrypted {
				Log.info("\(Self.TAG) [VFS] Content is encrypted, requesting plain file path for file \(originalPath)")
				path = mediaContent.exportPlainFile()
			} else {
				path = originalPath
			}

			let name = mediaContent.name ?? ""
			let size = Int64(mediaContent.size)
			let timestamp = mediaContent.creationTimestamp

			if !path.isEmpty && !name.isEmpty {
				let model = FileModel(
					path: path,
					fileName: name,
					fileSize: size,
					fileCreationTimestamp: Int64(timestamp),
					isEncrypted: isEncrypted,
					originalPath: originalPath,
					isFromEphemeralMessage: conversationModel.chatRoom.ephemeralEnabled
				)
				list.append(model)
			}
		}
		
		return list
	}
}
