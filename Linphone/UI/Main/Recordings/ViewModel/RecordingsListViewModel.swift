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
import Combine

class RecordingsListViewModel: ObservableObject {
	private let TAG = "[RecordingsListViewModel]"
	
	@Published var recordings: [RecordingModel] = []
	@Published var searchBarVisible: Bool = false
	@Published var searchFilter: String = ""
	@Published var fetchInProgress: Bool = true

	@Published var focusSearchBarEvent: Bool? = nil
	
	var selectedRecording: RecordingModel?

	private let legacyRecordRegex = try! NSRegularExpression(pattern: ".*/(.*)_(\\d{2}-\\d{2}-\\d{4}-\\d{2}-\\d{2}-\\d{2})\\..*")
	
	init() {
		fetchInProgress = true
		CoreContext.shared.doOnCoreQueue { core in
			self.computeList(filter: "")
		}
	}

	func openSearchBar() {
		searchBarVisible = true
		focusSearchBarEvent = true
	}

	func closeSearchBar() {
		clearFilter()
		searchBarVisible = false
		focusSearchBarEvent = false
	}

	func clearFilter() {
		if searchFilter.isEmpty {
			searchBarVisible = false
			focusSearchBarEvent = false
		} else {
			searchFilter = ""
		}
	}

	func applyFilter(_ filter: String) {
		DispatchQueue.global(qos: .background).async {
			self.computeList(filter: filter)
		}
	}

	private func computeList(filter: String) {
		var list: [RecordingModel] = []
		
		let dir1 = FileUtil.sharedContainerUrl().appendingPathComponent("Library/Recordings")
		if let files = try? FileManager.default.contentsOfDirectory(at: dir1, includingPropertiesForKeys: nil) {
			for file in files {
				let path = file.path
				let name = file.lastPathComponent

				let model = RecordingModel(filePath: path, fileName: name)

				if filter.isEmpty || model.sipUri.contains(filter) {
					list.append(model)
				}
			}
		}
		
		list.sort { $0.timestamp > $1.timestamp }
		
		DispatchQueue.main.async {
			self.recordings = list
			self.fetchInProgress = false
		}
	}
}
