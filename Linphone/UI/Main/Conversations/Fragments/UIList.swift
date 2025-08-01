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
// swiftlint:disable line_length
// swiftlint:disable cyclomatic_complexity
// swiftlint:disable type_body_length
import SwiftUI
import linphonesw

public extension Notification.Name {
	static let onScrollToBottom = Notification.Name("onScrollToBottom")
	static let onScrollToIndex = Notification.Name("onScrollToIndex")
}

class FloatingButton: UIButton {

	var unreadMessageCount: Int = 0 {
		didSet {
			updateUnreadBadge()
		}
	}

	override init(frame: CGRect) {
		super.init(frame: frame)
		setupButton()
	}

	required init?(coder: NSCoder) {
		super.init(coder: coder)
		setupButton()
	}

	private func setupButton() {
		// Set the button's appearance
		if let originalImage = UIImage(named: "caret-double-down")?.withRenderingMode(.alwaysTemplate) {
			let newSize = CGSize(width: originalImage.size.width / 1.5, height: originalImage.size.height / 1.5)
			
			UIGraphicsBeginImageContextWithOptions(newSize, false, 0.0)
			originalImage.draw(in: CGRect(origin: .zero, size: newSize))
			var resizedImage = UIGraphicsGetImageFromCurrentImageContext()
			UIGraphicsEndImageContext()
			
			resizedImage = resizedImage?.withRenderingMode(.alwaysTemplate)
			
			self.setImage(resizedImage, for: .normal)
			self.tintColor = .white
		}
		self.backgroundColor = UIColor(Color.orangeMain500)
		self.layer.cornerRadius = 30
		self.layer.shadowColor = UIColor.black.withAlphaComponent(0.2).cgColor
		self.layer.shadowOffset = CGSize(width: 0, height: 2)
		self.layer.shadowOpacity = 1
		self.layer.shadowRadius = 4
		
		// Add target action
		self.addTarget(self, action: #selector(buttonTapped), for: .touchUpInside)
	}

	private func updateUnreadBadge() {
		// Remove old badge if exists
		self.viewWithTag(100)?.removeFromSuperview()
		
		if unreadMessageCount > 0 {
			// Create the badge view
			let badgeLabel = UILabel()
			badgeLabel.tag = 100
			badgeLabel.text = unreadMessageCount < 99 ? "\(unreadMessageCount)" : "99+"
			badgeLabel.textColor = .white
			badgeLabel.font = UIFont.systemFont(ofSize: 10)
			badgeLabel.textAlignment = .center
			badgeLabel.backgroundColor = UIColor(Color.redDanger500)
			badgeLabel.layer.cornerRadius = 9
			badgeLabel.layer.masksToBounds = true
			badgeLabel.frame = CGRect(x: self.frame.size.width - 18, y: 0, width: 18, height: 18)
			self.addSubview(badgeLabel)
		}
	}

	@objc private func buttonTapped() {
		NotificationCenter.default.post(name: .onScrollToBottom, object: nil)
	}
}

struct UIList: UIViewRepresentable {
	
	@StateObject private var viewModel = ChatViewModel()
	@StateObject private var paginationState = PaginationState()
	
	@EnvironmentObject var conversationViewModel: ConversationViewModel
	@EnvironmentObject var conversationsListViewModel: ConversationsListViewModel
	
	let geometryProxy: GeometryProxy
	let sections: [MessagesSection]
    
    @Binding var isMessageTextFocused: Bool
	
	@State private var isScrolledToTop = false
	@State private var isScrolledToBottom = true
	
	func makeUIView(context: Context) -> UIView {
		// Create a UIView to contain the UITableView and UIButton
		let containerView = UIView()

		// Create the UITableView
		let tableView = UITableView(frame: .zero, style: .grouped)
		tableView.contentInset = UIEdgeInsets(top: -10, left: 0, bottom: 0, right: 0)
		tableView.translatesAutoresizingMaskIntoConstraints = false
		tableView.separatorStyle = .none
		tableView.dataSource = context.coordinator
		tableView.delegate = context.coordinator
		tableView.register(UITableViewCell.self, forCellReuseIdentifier: "Cell")
		tableView.transform = CGAffineTransformMakeScale(1, -1)
		
		tableView.showsVerticalScrollIndicator = true
		tableView.estimatedSectionHeaderHeight = 1
		tableView.estimatedSectionFooterHeight = UITableView.automaticDimension
		tableView.keyboardDismissMode = .interactive
		tableView.backgroundColor = UIColor(.white)
		tableView.scrollsToTop = true
		
		if SharedMainViewModel.shared.displayedConversation != nil && SharedMainViewModel.shared.displayedConversation!.encryptionEnabled {
			let footerView = Self.makeFooterView()
			footerView.frame = CGRect(x: 0, y: 0, width: tableView.bounds.width, height: 80)
			footerView.transform = CGAffineTransformMakeScale(1, -1)
			tableView.tableFooterView = footerView
		}
		
		// Create the floating UIButton
		let button = FloatingButton(frame: CGRect(x: 0, y: 0, width: 60, height: 60))
		button.translatesAutoresizingMaskIntoConstraints = false
		button.isHidden = isScrolledToBottom
		
		// Add the tableView and floating button to the containerView
		containerView.addSubview(tableView)
		containerView.addSubview(button)
		
		// Set up constraints
		NSLayoutConstraint.activate([
			// TableView constraints
			tableView.topAnchor.constraint(equalTo: containerView.topAnchor),
			tableView.leadingAnchor.constraint(equalTo: containerView.leadingAnchor),
			tableView.trailingAnchor.constraint(equalTo: containerView.trailingAnchor),
			tableView.bottomAnchor.constraint(equalTo: containerView.bottomAnchor),
			
			// Floating Button constraints
			button.widthAnchor.constraint(equalToConstant: 60),
			button.heightAnchor.constraint(equalToConstant: 60),
			button.trailingAnchor.constraint(equalTo: containerView.trailingAnchor, constant: -20),
			button.bottomAnchor.constraint(equalTo: containerView.bottomAnchor, constant: -20)
		])
		
		// Set the tableView as a tag for easy access in updateUIView
		tableView.tag = 101
		// Set the button as a tag for easy access in updateUIView
		button.tag = 102
		
		context.coordinator.parent = self
		context.coordinator.tableView = tableView
		context.coordinator.floatingButton = button
		context.coordinator.geometryProxy = geometryProxy

		return containerView
	}
	
	static func makeFooterView() -> UIView {
		let host = UIHostingController(
			rootView:
				VStack {
					HStack {
						Image("lock-simple-bold")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.blueInfo500)
							.frame(width: 25, height: 25)
							.padding(10)
						
						VStack(spacing: 5) {
							Text("conversation_end_to_end_encrypted_event_title")
								.foregroundStyle(Color.blueInfo500)
								.default_text_style_700(styleSize: 14)
								.frame(maxWidth: .infinity, alignment: .leading)
								.multilineTextAlignment(.leading)
							
							Text("conversation_end_to_end_encrypted_event_subtitle")
								.foregroundStyle(Color.gray400)
								.default_text_style(styleSize: 12)
								.frame(maxWidth: .infinity, alignment: .leading)
								.multilineTextAlignment(.leading)
						}
					}
					.padding(10)
					.cornerRadius(10)
					.overlay(
						RoundedRectangle(cornerRadius: 10)
							.inset(by: 0.5)
							.stroke(Color.blueInfo500, lineWidth: 0.5)
					)
                    .padding(.horizontal, 10)
				}
				.frame(height: 80)
		)
		host.view.backgroundColor = .clear
		return host.view
	}
	
	// func updateUIView(_ tableView: UITableView, context: Context) {
	func updateUIView(_ uiView: UIView, context: Context) {
		if let button = uiView.viewWithTag(102) as? FloatingButton {
			button.unreadMessageCount = conversationViewModel.displayedConversationUnreadMessagesCount
		}
		
		if let tableView = uiView.viewWithTag(101) as? UITableView {
			if context.coordinator.sections == sections {
				return
			}
			if context.coordinator.sections == sections {
				return
			}
			
			let prevSections = context.coordinator.sections
			let (appliedDeletes, appliedDeletesSwapsAndEdits, deleteOperations, swapOperations, editOperations, insertOperations) = operationsSplit(oldSections: prevSections, newSections: sections)
			
			tableView.performBatchUpdates {
				context.coordinator.sections = appliedDeletes
				for operation in deleteOperations {
					applyOperation(operation, tableView: tableView)
				}
			}
			
			tableView.performBatchUpdates {
				context.coordinator.sections = appliedDeletesSwapsAndEdits // NOTE: this array already contains necessary edits, but won't be a problem for appplying swaps
				for operation in swapOperations {
					applyOperation(operation, tableView: tableView)
				}
			}
			
			tableView.performBatchUpdates {
				context.coordinator.sections = appliedDeletesSwapsAndEdits
				for operation in editOperations {
					applyOperation(operation, tableView: tableView)
				}
			}
			
			context.coordinator.sections = sections
			
			tableView.beginUpdates()
			for operation in insertOperations {
				applyOperation(operation, tableView: tableView)
			}
			tableView.endUpdates()
			
			if isScrolledToBottom && conversationViewModel.displayedConversationUnreadMessagesCount > 0 {
				conversationViewModel.markAsRead()
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
			tableView.deleteRows(at: [IndexPath(row: row, section: section)], with: .left)
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
		var appliedDeletes = oldSections
		var appliedDeletesSwapsAndEdits = newSections
		
		var deleteOperations = [Operation]()
		var swapOperations = [Operation]()
		var editOperations = [Operation]()
		var insertOperations = [Operation]()
		
		let oldDates = oldSections.map { $0.date }
		let newDates = newSections.map { $0.date }
		let commonDates = Array(Set(oldDates + newDates)).sorted(by: >)
		for date in commonDates {
			let oldIndex = appliedDeletes.firstIndex(where: { $0.date == date })
			let newIndex = appliedDeletesSwapsAndEdits.firstIndex(where: { $0.date == date })
			if oldIndex == nil, let newIndex {
				if let operationIndex = newSections.firstIndex(where: { $0.date == date }) {
					appliedDeletesSwapsAndEdits.remove(at: newIndex)
					insertOperations.append(.insertSection(operationIndex))
				}
				continue
			}
			if newIndex == nil, let oldIndex {
				if let operationIndex = oldSections.firstIndex(where: { $0.date == date }) {
					appliedDeletes.remove(at: oldIndex)
					deleteOperations.append(.deleteSection(operationIndex))
				}
				continue
			}
			guard let newIndex, let oldIndex else { continue }
			
			var oldRows = appliedDeletes[oldIndex].rows
			var newRows = appliedDeletesSwapsAndEdits[newIndex].rows
			let oldRowIDs = Set(oldRows.map { $0.message.id })
			let newRowIDs = Set(newRows.map { $0.message.id })
			let rowIDsToDelete = oldRowIDs.subtracting(newRowIDs)
			let rowIDsToInsert = newRowIDs.subtracting(oldRowIDs)
			
			for rowId in rowIDsToDelete {
				if let index = oldRows.firstIndex(where: { $0.message.id == rowId }) {
					oldRows.remove(at: index)
					deleteOperations.append(.delete(oldIndex, index))
				}
			}
			
			for rowId in rowIDsToInsert {
				if let index = newRows.firstIndex(where: { $0.message.id == rowId }) {
					insertOperations.append(.insert(newIndex, index))
				}
			}
			
			for rowId in rowIDsToInsert {
				if let index = newRows.firstIndex(where: { $0.message.id == rowId }) {
					newRows.remove(at: index)
				}
			}
			
			for row in 0..<oldRows.count {
				let oldRow = oldRows[row]
				let newRow = newRows[row]
				if oldRow.message.id != newRow.message.id {
					if let index = newRows.firstIndex(where: { $0.message.id == oldRow.message.id }) {
						if !swapsContain(swaps: swapOperations, section: oldIndex, index: row) ||
							!swapsContain(swaps: swapOperations, section: oldIndex, index: index) {
							swapOperations.append(.swap(oldIndex, row, index))
						}
					}
				} else if oldRow != newRow {
					editOperations.append(.edit(oldIndex, row))
				}
			}
			
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
			parent: self,
			geometryProxy: geometryProxy,
			sections: sections
		)
	}
	
	class Coordinator: NSObject, UITableViewDataSource, UITableViewDelegate {
		
		var parent: UIList
		var tableView: UITableView?
		var floatingButton: FloatingButton?
		
		var geometryProxy: GeometryProxy
		var sections: [MessagesSection]
		
		init(parent: UIList, geometryProxy: GeometryProxy, sections: [MessagesSection]) {
			self.parent = parent
			self.geometryProxy = geometryProxy
			self.sections = sections
			
			super.init()
			
			NotificationCenter.default.addObserver(forName: .onScrollToBottom, object: nil, queue: nil) { _ in
				DispatchQueue.main.async {
					guard !self.sections.isEmpty,
						  let firstSection = self.sections.first,
						  let firstConversationSection = parent.conversationViewModel.conversationMessagesSection.first,
						  let displayedConversation = SharedMainViewModel.shared.displayedConversation,
						  let tableView = self.tableView,
						  firstSection.chatRoomID == displayedConversation.id,
						  firstSection.rows.count == firstConversationSection.rows.count else {
						return
					}
					tableView.scrollToRow(at: IndexPath(row: 0, section: 0), at: .top, animated: true)
				}
			}
			
			NotificationCenter.default.addObserver(forName: .onScrollToIndex, object: nil, queue: nil) { notification in
				DispatchQueue.main.async {
					if !self.sections.isEmpty {
						if self.sections.first != nil
							&& parent.conversationViewModel.conversationMessagesSection.first != nil
							&& SharedMainViewModel.shared.displayedConversation != nil
							&& self.sections.first!.chatRoomID == SharedMainViewModel.shared.displayedConversation!.id
							&& self.sections.first!.rows.count == parent.conversationViewModel.conversationMessagesSection.first!.rows.count {
							if let dict = notification.userInfo as NSDictionary? {
								if let index = dict["index"] as? Int {
									if let animated = dict["animated"] as? Bool {
										self.tableView!.scrollToRow(at: IndexPath(row: index, section: 0), at: .bottom, animated: animated)
									}
								}
							}
						}
					}
				}
			}
		}
		
		deinit {
			NotificationCenter.default.removeObserver(self)
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
			if section < parent.conversationViewModel.conversationMessagesSection.count
				&& parent.conversationViewModel.conversationMessagesSection[section].rows.count < parent.conversationViewModel.displayedConversationHistorySize {
				let header = UIHostingController(rootView:
					ProgressView()
						.frame(height: 50)
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
			
			let pan = CustomPanRecognizer()
			pan.onBeginSwipe = { [self] in
				self.parent.conversationViewModel.isSwiping = true
				DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
					self.parent.conversationViewModel.isSwiping = false
				}
			}
			
			if !(tableViewCell.gestureRecognizers?.contains(where: { $0 is CustomPanRecognizer }) ?? false) {
				tableViewCell.addGestureRecognizer(pan)
			}
			
			if #available(iOS 16.0, *) {
				tableViewCell.contentConfiguration = UIHostingConfiguration {
					ChatBubbleView(eventLogMessage: row, geometryProxy: geometryProxy)
						.environmentObject(parent.conversationViewModel)
						.padding(.vertical, 2)
						.padding(.horizontal, 10)
						.onTapGesture { }
				}
				.minSize(width: 0, height: 50)
				.margins(.all, 0)
			}
			
			tableViewCell.transform = CGAffineTransformMakeScale(1, -1)
			
			return tableViewCell
		}
		
		func tableView(_ tableView: UITableView, willDisplay cell: UITableViewCell, forRowAt indexPath: IndexPath) {
			let row = sections[indexPath.section].rows[indexPath.row]
			parent.paginationState.handle(row.message)
		}
		
		func scrollViewDidScroll(_ scrollView: UIScrollView) {
			let isScrolledToBottomTmp = scrollView.contentOffset.y <= 10
			DispatchQueue.main.async {
				if self.parent.isScrolledToBottom != isScrolledToBottomTmp {
					self.parent.isScrolledToBottom = isScrolledToBottomTmp
					
					if self.parent.isScrolledToBottom {
						self.floatingButton!.isHidden = true
					} else {
						self.floatingButton!.isHidden = false
					}
					
					if self.parent.isScrolledToBottom && self.parent.conversationViewModel.displayedConversationUnreadMessagesCount > 0 {
						self.parent.conversationViewModel.markAsRead()
					}
				}
			}
			
			if !parent.isScrolledToTop && scrollView.contentOffset.y >= scrollView.contentSize.height - scrollView.frame.height - 500 {
				parent.conversationViewModel.getOldMessages()
			}
			
			parent.isScrolledToTop = scrollView.contentOffset.y >= scrollView.contentSize.height - scrollView.frame.height - 500
		}
		
		func tableView(_ tableView: UITableView, leadingSwipeActionsConfigurationForRowAt indexPath: IndexPath) -> UISwipeActionsConfiguration? {
			let archiveAction = UIContextualAction(style: .normal, title: "") { _, _, completionHandler in
                self.parent.conversationViewModel.replyToMessage(index: indexPath.row, isMessageTextFocused: Binding(
                    get: { self.parent.isMessageTextFocused },
                    set: { self.parent.isMessageTextFocused = $0 }
                ))
				completionHandler(true)
			}
			
			archiveAction.image = UIImage(named: "reply-reversed")!
			
			let configuration = UISwipeActionsConfiguration(actions: [archiveAction])
			
			return configuration
		}
	}
}

struct MessagesSection: Equatable {
	
	let date: Date
	let chatRoomID: String
	var rows: [EventLogMessage]
	
	static var formatter = {
		let formatter = DateFormatter()
		formatter.dateFormat = "EEEE, MMMM d"
		return formatter
	}()
	
	init(date: Date, chatRoomID: String, rows: [EventLogMessage]) {
		self.date = date
		self.chatRoomID = chatRoomID
		self.rows = rows
	}
	
	var formattedDate: String {
		MessagesSection.formatter.string(from: date)
	}
	
	static func == (lhs: MessagesSection, rhs: MessagesSection) -> Bool {
		lhs.date == rhs.date && lhs.rows == rhs.rows
	}
	
}

struct EventLogMessage: Equatable {
	
	let eventModel: EventModel
	var message: Message
	
	static var formatter = {
		let formatter = DateFormatter()
		formatter.dateFormat = "EEEE, MMMM d"
		return formatter
	}()
	
	static func == (lhs: EventLogMessage, rhs: EventLogMessage) -> Bool {
		lhs.message == rhs.message
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
	
	@Published private(set) var fullscreenAttachmentItem: Attachment?
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

class CustomPanRecognizer: UIPanGestureRecognizer, UIGestureRecognizerDelegate {
	var onBeginSwipe: (() -> Void)?

	override init(target: Any?, action: Selector?) {
		super.init(target: target, action: action)
		self.delegate = self
		self.cancelsTouchesInView = false
	}

	func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
		let velocity = self.velocity(in: self.view)
		if abs(velocity.x) > abs(velocity.y) {
			onBeginSwipe?()
		}
		return false
	}
}

// swiftlint:enable large_tuple
// swiftlint:enable line_length
// swiftlint:enable cyclomatic_complexity
// swiftlint:enable type_body_length
