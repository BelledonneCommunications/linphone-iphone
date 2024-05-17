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

// swiftlint:disable large_tuple
import SwiftUI

public extension Notification.Name {
	static let onScrollToBottom = Notification.Name("onScrollToBottom")
}

struct UIList: UIViewRepresentable {
	
	@ObservedObject var viewModel: ChatViewModel
	@ObservedObject var paginationState: PaginationState
	@ObservedObject var conversationViewModel: ConversationViewModel

	@Binding var isScrolledToBottom: Bool
	
	let showMessageMenuOnLongPress: Bool
	let geometryProxy: GeometryProxy
	let sections: [MessagesSection]

	@State private var isScrolledToTop = false

	private let updatesQueue = DispatchQueue(label: "updatesQueue", qos: .utility)
	@State private var updateSemaphore = DispatchSemaphore(value: 1)
	@State private var tableSemaphore = DispatchSemaphore(value: 0)

	func makeUIView(context: Context) -> UITableView {
		let tableView = UITableView(frame: .zero, style: .grouped)
		tableView.contentInset = UIEdgeInsets(top: -10, left: 0, bottom: -20, right: 0)
		tableView.translatesAutoresizingMaskIntoConstraints = false
		tableView.separatorStyle = .none
		tableView.dataSource = context.coordinator
		tableView.delegate = context.coordinator
		tableView.register(UITableViewCell.self, forCellReuseIdentifier: "Cell")
		tableView.transform = CGAffineTransformMakeScale(1, -1)
		
		tableView.showsVerticalScrollIndicator = true
		tableView.estimatedSectionHeaderHeight = 1
		tableView.estimatedSectionFooterHeight = UITableView.automaticDimension
		tableView.backgroundColor = UIColor(.white)
		tableView.scrollsToTop = true

		NotificationCenter.default.addObserver(forName: .onScrollToBottom, object: nil, queue: nil) { _ in
			DispatchQueue.main.async {
				if !context.coordinator.sections.isEmpty {
					tableView.scrollToRow(at: IndexPath(row: 0, section: 0), at: .bottom, animated: true)
				}
			}
		}

		return tableView
	}

	func updateUIView(_ tableView: UITableView, context: Context) {
		if context.coordinator.sections == sections {
			return
		}
		updatesQueue.async {
			updateSemaphore.wait()

			if context.coordinator.sections == sections {
				updateSemaphore.signal()
				return
			}

			let prevSections = context.coordinator.sections
			let (appliedDeletes, appliedDeletesSwapsAndEdits, deleteOperations, swapOperations, editOperations, insertOperations) = operationsSplit(oldSections: prevSections, newSections: sections)

			// step 1
			// preapare intermediate sections and operations
			//print("1 updateUIView sections:", "\n")
			//print("whole previous:\n", formatSections(prevSections), "\n")
			//print("whole appliedDeletes:\n", formatSections(appliedDeletes), "\n")
			//print("whole appliedDeletesSwapsAndEdits:\n", formatSections(appliedDeletesSwapsAndEdits), "\n")
			//print("whole final sections:\n", formatSections(sections), "\n")

			//print("operations delete:\n", deleteOperations)
			//print("operations swap:\n", swapOperations)
			//print("operations edit:\n", editOperations)
			//print("operations insert:\n", insertOperations)

			DispatchQueue.main.async {
				tableView.performBatchUpdates {
					// step 2
					// delete sections and rows if necessary
					//print("2 apply delete")
					context.coordinator.sections = appliedDeletes
					for operation in deleteOperations {
						applyOperation(operation, tableView: tableView)
					}
				} completion: { _ in
					tableSemaphore.signal()
					//print("2 finished delete")
				}
			}
			tableSemaphore.wait()

			DispatchQueue.main.async {
				tableView.performBatchUpdates {
					// step 3
					// swap places for rows that moved inside the table
					// (example of how this happens. send two messages: first m1, then m2. if m2 is delivered to server faster, then it should jump above m1 even though it was sent later)
					//print("3 apply swaps")
					context.coordinator.sections = appliedDeletesSwapsAndEdits // NOTE: this array already contains necessary edits, but won't be a problem for appplying swaps
					for operation in swapOperations {
						applyOperation(operation, tableView: tableView)
					}
				} completion: { _ in
					tableSemaphore.signal()
					//print("3 finished swaps")
				}
			}
			tableSemaphore.wait()

			DispatchQueue.main.async {
				tableView.performBatchUpdates {
					// step 4
					// check only sections that are already in the table for existing rows that changed and apply only them to table's dataSource without animation
					//print("4 apply edits")
					context.coordinator.sections = appliedDeletesSwapsAndEdits
					for operation in editOperations {
						applyOperation(operation, tableView: tableView)
					}
				} completion: { _ in
					tableSemaphore.signal()
					//print("4 finished edits")
				}
			}
			tableSemaphore.wait()
			
			if isScrolledToBottom || isScrolledToTop {
				DispatchQueue.main.sync {
					// step 5
					// apply the rest of the changes to table's dataSource, i.e. inserts
					//print("5 apply inserts")
					context.coordinator.sections = sections

					tableView.beginUpdates()
					for operation in insertOperations {
						applyOperation(operation, tableView: tableView)
					}
					tableView.endUpdates()

					updateSemaphore.signal()
				}
			} else {
				updateSemaphore.signal()
			}
		}
	}

	// MARK: - Operations

	enum Operation {
		case deleteSection(Int)
		case insertSection(Int)

		case delete(Int, Int) // delete with animation
		case insert(Int, Int) // insert with animation
		case swap(Int, Int, Int) // delete first with animation, then insert it into new position with animation. do not do anything with the second for now
		case edit(Int, Int) // reload the element without animation
	}

	func applyOperation(_ operation: Operation, tableView: UITableView) {
		switch operation {
		case .deleteSection(let section):
			tableView.deleteSections([section], with: .top)
		case .insertSection(let section):
			tableView.insertSections([section], with: .top)

		case .delete(let section, let row):
			tableView.deleteRows(at: [IndexPath(row: row, section: section)], with: .top)
		case .insert(let section, let row):
			tableView.insertRows(at: [IndexPath(row: row, section: section)], with: .top)
		case .edit(let section, let row):
			tableView.reloadRows(at: [IndexPath(row: row, section: section)], with: .none)
		case .swap(let section, let rowFrom, let rowTo):
			tableView.deleteRows(at: [IndexPath(row: rowFrom, section: section)], with: .top)
			tableView.insertRows(at: [IndexPath(row: rowTo, section: section)], with: .top)
		}
	}

	func operationsSplit(oldSections: [MessagesSection], newSections: [MessagesSection]) -> ([MessagesSection], [MessagesSection], [Operation], [Operation], [Operation], [Operation]) {
		var appliedDeletes = oldSections // start with old sections, remove rows that need to be deleted
		var appliedDeletesSwapsAndEdits = newSections // take new sections and remove rows that need to be inserted for now, then we'll get array with all the changes except for inserts
		// appliedDeletesSwapsEditsAndInserts == newSection

		var deleteOperations = [Operation]()
		var swapOperations = [Operation]()
		var editOperations = [Operation]()
		var insertOperations = [Operation]()

		// 1 compare sections

		let oldDates = oldSections.map { $0.date }
		let newDates = newSections.map { $0.date }
		let commonDates = Array(Set(oldDates + newDates)).sorted(by: >)
		for date in commonDates {
			let oldIndex = appliedDeletes.firstIndex(where: { $0.date == date } )
			let newIndex = appliedDeletesSwapsAndEdits.firstIndex(where: { $0.date == date } )
			if oldIndex == nil, let newIndex {
				// operationIndex is not the same as newIndex because appliedDeletesSwapsAndEdits is being changed as we go, but to apply changes to UITableView we should have initial index
				if let operationIndex = newSections.firstIndex(where: { $0.date == date } ) {
					appliedDeletesSwapsAndEdits.remove(at: newIndex)
					insertOperations.append(.insertSection(operationIndex))
				}
				continue
			}
			if newIndex == nil, let oldIndex {
				if let operationIndex = oldSections.firstIndex(where: { $0.date == date } ) {
					appliedDeletes.remove(at: oldIndex)
					deleteOperations.append(.deleteSection(operationIndex))
				}
				continue
			}
			guard let newIndex, let oldIndex else { continue }

			// 2 compare section rows
			// isolate deletes and inserts, and remove them from row arrays, leaving only rows that are in both arrays: 'duplicates'
			// this will allow to compare relative position changes of rows - swaps

			var oldRows = appliedDeletes[oldIndex].rows
			var newRows = appliedDeletesSwapsAndEdits[newIndex].rows
			let oldRowIDs = Set(oldRows.map { $0.id })
			let newRowIDs = Set(newRows.map { $0.id })
			let rowIDsToDelete = oldRowIDs.subtracting(newRowIDs)
			let rowIDsToInsert = newRowIDs.subtracting(oldRowIDs) // TODO is order important?
			for rowId in rowIDsToDelete {
				if let index = oldRows.firstIndex(where: { $0.id == rowId }) {
					oldRows.remove(at: index)
					deleteOperations.append(.delete(oldIndex, index)) // this row was in old section, should not be in final result
				}
			}
			for rowId in rowIDsToInsert {
				if let index = newRows.firstIndex(where: { $0.id == rowId }) {
					// this row was not in old section, should add it to final result
					insertOperations.append(.insert(newIndex, index))
				}
			}

			for rowId in rowIDsToInsert {
				if let index = newRows.firstIndex(where: { $0.id == rowId }) {
					// remove for now, leaving only 'duplicates'
					newRows.remove(at: index)
				}
			}

			// 3 isolate swaps and edits

			for row in 0..<oldRows.count {
				let oldRow = oldRows[row]
				let newRow = newRows[row]
				if oldRow.id != newRow.id { // a swap: rows in same position are not actually the same rows
					if let index = newRows.firstIndex(where: { $0.id == oldRow.id }) {
						if !swapsContain(swaps: swapOperations, section: oldIndex, index: row) ||
							!swapsContain(swaps: swapOperations, section: oldIndex, index: index) {
							swapOperations.append(.swap(oldIndex, row, index))
						}
					}
				} else if oldRow != newRow { // same ids om same positions but something changed - reload rows without animation
					editOperations.append(.edit(oldIndex, row))
				}
			}

			// 4 store row changes in sections

			appliedDeletes[oldIndex].rows = oldRows
			appliedDeletesSwapsAndEdits[newIndex].rows = newRows
		}

		return (appliedDeletes, appliedDeletesSwapsAndEdits, deleteOperations, swapOperations, editOperations, insertOperations)
	}

	func swapsContain(swaps: [Operation], section: Int, index: Int) -> Bool {
		!swaps.filter {
			if case let .swap(section, rowFrom, rowTo) = $0 {
				return section == section && (rowFrom == index || rowTo == index)
			}
			return false
		}.isEmpty
	}

	// MARK: - Coordinator

	func makeCoordinator() -> Coordinator {
		Coordinator(
			conversationViewModel: conversationViewModel,
			viewModel: viewModel,
			paginationState: paginationState,
			isScrolledToBottom: $isScrolledToBottom,
			isScrolledToTop: $isScrolledToTop,
			showMessageMenuOnLongPress: showMessageMenuOnLongPress,
			geometryProxy: geometryProxy,
			sections: sections
		)
	}
	
	class Coordinator: NSObject, UITableViewDataSource, UITableViewDelegate {

		@ObservedObject var viewModel: ChatViewModel
		@ObservedObject var paginationState: PaginationState
		@ObservedObject var conversationViewModel: ConversationViewModel

		@Binding var isScrolledToBottom: Bool
		@Binding var isScrolledToTop: Bool
		
		let showMessageMenuOnLongPress: Bool
		let geometryProxy: GeometryProxy
		var sections: [MessagesSection]

		init(conversationViewModel: ConversationViewModel, viewModel: ChatViewModel, paginationState: PaginationState, isScrolledToBottom: Binding<Bool>, isScrolledToTop: Binding<Bool>, showMessageMenuOnLongPress: Bool, geometryProxy: GeometryProxy, sections: [MessagesSection]) {
			self.conversationViewModel = conversationViewModel
			self.viewModel = viewModel
			self.paginationState = paginationState
			self._isScrolledToBottom = isScrolledToBottom
			self._isScrolledToTop = isScrolledToTop
			self.showMessageMenuOnLongPress = showMessageMenuOnLongPress
			self.geometryProxy = geometryProxy
			self.sections = sections
		}

		func numberOfSections(in tableView: UITableView) -> Int {
			sections.count
		}

		func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
			sections[section].rows.count
		}

		func tableView(_ tableView: UITableView, viewForFooterInSection section: Int) -> UIView? {
			return progressView(section)
		}
		
		func progressView(_ section: Int) -> UIView? {
			if section > conversationViewModel.conversationMessagesSection.count
				&& conversationViewModel.conversationMessagesSection[section].rows.count < conversationViewModel.displayedConversationHistorySize {
				let header = UIHostingController(rootView:
					ProgressView()
						.frame(idealWidth: .infinity, maxWidth: .infinity, alignment: .center)
				).view
				header?.backgroundColor = UIColor(.white)
				return header
			}
			return nil
		}

		func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {

			let tableViewCell = tableView.dequeueReusableCell(withIdentifier: "Cell", for: indexPath)
			tableViewCell.selectionStyle = .none
			tableViewCell.backgroundColor = UIColor(.white)

			let row = sections[indexPath.section].rows[indexPath.row]
			if #available(iOS 16.0, *) {
				tableViewCell.contentConfiguration = UIHostingConfiguration {
					ChatBubbleView(conversationViewModel: conversationViewModel, message: row, geometryProxy: geometryProxy)
						.padding(.vertical, 1)
						.padding(.horizontal, 10)
						.onTapGesture { }
				}
				.minSize(width: 0, height: 0)
				.margins(.all, 0)
			}
			
			tableViewCell.transform = CGAffineTransformMakeScale(1, -1)

			return tableViewCell
		}

		func tableView(_ tableView: UITableView, willDisplay cell: UITableViewCell, forRowAt indexPath: IndexPath) {
			let row = sections[indexPath.section].rows[indexPath.row]
			paginationState.handle(row)
		}

		func scrollViewDidScroll(_ scrollView: UIScrollView) {
			isScrolledToBottom = scrollView.contentOffset.y <= 10
			
			if isScrolledToBottom && conversationViewModel.displayedConversationUnreadMessagesCount > 0 {
				conversationViewModel.markAsRead()
			}
			
			if !isScrolledToTop && scrollView.contentOffset.y >= scrollView.contentSize.height - scrollView.frame.height - 200 {
				self.conversationViewModel.getOldMessages()
			}
			isScrolledToTop = scrollView.contentOffset.y >= scrollView.contentSize.height - scrollView.frame.height - 200
		}
	}
}

struct MessagesSection: Equatable {

	let date: Date
	var rows: [Message]

	static var formatter = {
		let formatter = DateFormatter()
		formatter.dateFormat = "EEEE, MMMM d"
		return formatter
	}()

	init(date: Date, rows: [Message]) {
		self.date = date
		self.rows = rows
	}

	var formattedDate: String {
		MessagesSection.formatter.string(from: date)
	}

	static func == (lhs: MessagesSection, rhs: MessagesSection) -> Bool {
		lhs.date == rhs.date && lhs.rows == rhs.rows
	}

}

final class PaginationState: ObservableObject {
	var onEvent: ChatPaginationClosure?
	var offset: Int

	var shouldHandlePagination: Bool {
		onEvent != nil
	}

	init(onEvent: ChatPaginationClosure? = nil, offset: Int = 0) {
		self.onEvent = onEvent
		self.offset = offset
	}

	func handle(_ message: Message) {
		guard shouldHandlePagination else {
			return
		}
	}
}

public typealias ChatPaginationClosure = (Message) -> Void

final class ChatViewModel: ObservableObject {
	
	@Published private(set) var fullscreenAttachmentItem: Optional<Attachment> = nil
	@Published var fullscreenAttachmentPresented = false

	@Published var messageMenuRow: Message?

	public var didSendMessage: (DraftMessage) -> Void = {_ in}
	
	func presentAttachmentFullScreen(_ attachment: Attachment) {
		fullscreenAttachmentItem = attachment
		fullscreenAttachmentPresented = true
	}
	
	func dismissAttachmentFullScreen() {
		fullscreenAttachmentPresented = false
		fullscreenAttachmentItem = nil
	}

	func sendMessage(_ message: DraftMessage) {
		didSendMessage(message)
	}
}

// swiftlint:enable large_tuple
