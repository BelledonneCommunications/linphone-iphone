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


import UIKit
import Foundation
import linphonesw
import DropDown

@objc class ChatConversationViewSwift: BackActionsNavigationView, UICompositeViewDelegate { // Replaces ChatConversationView
	
	let controlsView = ControlsView(showVideo: true, controlsViewModel: ChatConversationViewModel.sharedModel)
	
	static let compositeDescription = UICompositeViewDescription(ChatConversationViewSwift.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	let APP_GROUP_ID = "group.belledonne-communications.linphone.widget"
	var debugEnabled = false
	
	var chatRoom: ChatRoom? = nil
	var chatRoomDelegate: ChatRoomDelegate? = nil
	var address: String? = nil
	var participants: String? = nil
	
	var activeAlertController = CustomAlertController()
	
	@objc let tableController = ChatConversationTableView()
	let refreshControl = UIRefreshControl()
	
	var replyBubble = UIChatReplyBubbleView()
	
	var composingVisible = false
	var contentOriginY = 0.0
	var composingOriginY = 0.0
	
	let menu: DropDown = {
		let menu = DropDown()
		menu.dataSource = [""]
		var images = [
			"contact_add_default.png",
			"contacts_all_default.png",
			"menu_voip_meeting_schedule",
			"menu_security_default.png",
			"ephemeral_messages_default.png",
			"menu_notifications_off.png",
			"menu_notifications_on.png",
			"delete_default.png",
			"chat_group_informations.png"
		]
		menu.cellNib = UINib(nibName: "DropDownCell", bundle: nil)
		menu.customCellConfiguration = { index, title, cell in
			guard let cell = cell as? MyCell else {
				return
			}
			if(index < images.count){
				switch menu.dataSource[index] {
				case VoipTexts.dropdown_menu_chat_conversation_add_to_contact:
					cell.myImageView.image = UIImage(named: images[0])
				case VoipTexts.dropdown_menu_chat_conversation_go_to_contact:
					cell.myImageView.image = UIImage(named: images[1])
				case VoipTexts.conference_schedule_start:
					cell.myImageView.image = UIImage(named: images[2])
				case VoipTexts.dropdown_menu_chat_conversation_conversation_device:
					cell.myImageView.image = UIImage(named: images[3])
				case VoipTexts.dropdown_menu_chat_conversation_ephemeral_messages:
					cell.myImageView.image = UIImage(named: images[4])
				case VoipTexts.dropdown_menu_chat_conversation_mute_notifications:
					cell.myImageView.image = UIImage(named: images[5])
				case VoipTexts.dropdown_menu_chat_conversation_unmute_notifications:
					cell.myImageView.image = UIImage(named: images[6])
				case VoipTexts.dropdown_menu_chat_conversation_delete_messages:
					cell.myImageView.image = UIImage(named: images[7])
				default:
					cell.myImageView.image = UIImage(named: images[8])
				}
			}
		}
		return menu
	}()
	
	override func viewDidLoad() {
		super.viewDidLoad(
			backAction: {
				self.goBackChatListView()
			},
			action1: {
				self.onCallClick(cChatRoom: self.chatRoom?.getCobject)
			},
			action2: {
				self.tapChooseMenuItem(self.action2Button)
			},
			action3: {
				self.alertActionGoToDevicesList()
			},
			action4: {
				(LinphoneManager.instance().lpConfigInt(forKey: "debugenable_preference") == 1) ? self.showAddressAndIdentityPopup() : self.tapChooseMenuItem(self.action2Button)
			},
			title: address ?? "Error",
			participants: participants ?? "Error"
		)
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		topBar.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
		self.contentView.addSubview(tableController.tableView)
		tableController.chatRoom = chatRoom?.getCobject
		refreshControl.addTarget(self, action: #selector(refreshData), for: .valueChanged)
		tableController.refreshControl = refreshControl
		tableController.toggleSelectionButton = action1SelectAllButton
		messageView.sendButton.onClickAction = onSendClick
		
		
		chatRoomDelegate = ChatRoomDelegateStub(
			onIsComposingReceived: { (room: ChatRoom, remoteAddress: Address, isComposing: Bool) -> Void in
				self.on_chat_room_is_composing_received(room, remoteAddress, isComposing)
			}, onChatMessageReceived: { (room: ChatRoom, event: EventLog) -> Void in
				self.on_chat_room_chat_message_received(room, event)
			},
			onChatMessageSending: { (room: ChatRoom, event: EventLog) -> Void in
				self.on_chat_room_chat_message_sending(room, event)
			}
		)
		
		chatRoom?.addDelegate(delegate: chatRoomDelegate!)
		tableController.tableView.separatorColor = .clear
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		chatRoom?.removeDelegate(delegate: chatRoomDelegate!)
		editModeOff()
	}
	
	override func viewDidAppear(_ animated: Bool) {
		tableController.reloadData()
		messageView.ephemeralIndicator.isHidden = (linphone_chat_room_ephemeral_enabled(chatRoom?.getCobject) == 0)
	}
	
	func goBackChatListView() {
		PhoneMainView.instance().pop(toView: ChatsListView.compositeViewDescription())
	}
	
	func tapChooseMenuItem(_ sender: UIButton) {
		menu.anchorView = sender
		menu.bottomOffset = CGPoint(x: -UIScreen.main.bounds.width * 0.6, y: sender.frame.size.height)
		menu.show()
		menu.selectionAction = { [weak self] (index: Int, item: String) in
			guard let _ = self else { return }
			print(item)
			switch item {
			case VoipTexts.dropdown_menu_chat_conversation_add_to_contact:
				self!.addOrGoToContact()
			case VoipTexts.dropdown_menu_chat_conversation_go_to_contact:
				self!.addOrGoToContact()
			case VoipTexts.conference_schedule_start:
				self!.conferenceSchedule()
			case VoipTexts.dropdown_menu_chat_conversation_group_infos:
				self!.displayGroupInfo()
			case VoipTexts.dropdown_menu_chat_conversation_conversation_device:
				self!.goToDevicesList()
			case VoipTexts.dropdown_menu_chat_conversation_ephemeral_messages:
				self!.goToEphemeralSettings()
			case VoipTexts.dropdown_menu_chat_conversation_mute_notifications:
				self!.mute_unmute_notifications()
				self?.menu.dataSource[index] = VoipTexts.dropdown_menu_chat_conversation_unmute_notifications
			case VoipTexts.dropdown_menu_chat_conversation_unmute_notifications:
				self!.mute_unmute_notifications()
				self?.menu.dataSource[index] = VoipTexts.dropdown_menu_chat_conversation_mute_notifications
			case VoipTexts.dropdown_menu_chat_conversation_delete_messages:
				self!.onEditionChangeClick()
			default:
				self!.showAddressAndIdentityPopup()
			}
			self!.menu.clearSelection()
		}
	}
	
	func goToDevicesList() {
		let view: DevicesListView = self.VIEW(DevicesListView.compositeViewDescription())
		view.room = chatRoom?.getCobject
		PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
	}
	
	func addOrGoToContact() {
		let firstParticipant = chatRoom?.participants.first
		let addr = (firstParticipant != nil) ? linphone_participant_get_address(firstParticipant?.getCobject) : linphone_chat_room_get_peer_address(chatRoom?.getCobject)
		
		let contact = FastAddressBook.getContactWith(addr)
		
		if let contact {
			let view: ContactDetailsView = self.VIEW(ContactDetailsView.compositeViewDescription())
			ContactSelection.setSelectionMode(ContactSelectionModeNone)
			MagicSearchSingleton.instance().currentFilter = ""
			view.contact = contact
			PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
		} else {
			
			let lAddress = linphone_address_as_string_uri_only(addr)
			if let lAddress {
				var normSip = String(utf8String: lAddress)
				normSip = normSip?.hasPrefix("sip:") ?? false ? (normSip as NSString?)?.substring(from: 4) : normSip
				normSip = normSip?.hasPrefix("sips:") ?? false ? (normSip as NSString?)?.substring(from: 5) : normSip
				ContactSelection.setAddAddress(normSip)
				ContactSelection.setSelectionMode(ContactSelectionModeEdit)
				ContactSelection.enableSipFilter(false)
				PhoneMainView.instance().changeCurrentView(ContactsListView.compositeViewDescription())
			}
		}
	}
	
	func displayGroupInfo() {
		let contactsArray: NSMutableArray = []
		let admins: NSMutableArray = []
		let participants = chatRoom?.participants
		participants?.forEach{ participant in
			let curi = linphone_address_as_string_uri_only(linphone_participant_get_address(participant.getCobject))
			let uri = String(utf8String: curi!)
			contactsArray.add(uri!)
			if (linphone_participant_is_admin(participant.getCobject) != 0) {
				admins.add(uri!)
			}
		}
		
		let view: ChatConversationInfoView = self.VIEW(ChatConversationInfoView.compositeViewDescription())
		view.create = false
		view.contacts = contactsArray
		view.oldContacts = contactsArray
		view.admins = admins
		view.oldAdmins = admins
		view.oldSubject = String(utf8String: linphone_chat_room_get_subject(chatRoom?.getCobject)) ?? LINPHONE_DUMMY_SUBJECT
		view.room = chatRoom?.getCobject
		
		let localAddress = linphone_address_as_string(linphone_chat_room_get_local_address(chatRoom?.getCobject))
		let peerAddress = linphone_address_as_string(linphone_chat_room_get_peer_address(chatRoom?.getCobject))
		view.peerAddress = UnsafePointer(peerAddress)
		view.localAddress = UnsafePointer(localAddress)
		PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
	}
	
	func goToEphemeralSettings(){
		let view: EphemeralSettingsView = self.VIEW(EphemeralSettingsView.compositeViewDescription())
		view.room = chatRoom?.getCobject
		PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
	}
	
	func conferenceSchedule(){
		ConferenceViewModelBridge.scheduleFromGroupChat(cChatRoom: (chatRoom?.getCobject)!)
		PhoneMainView.instance().pop(toView: ConferenceSchedulingView.compositeViewDescription())
	}
	
	func mute_unmute_notifications(){
		LinphoneManager.setChatroomPushEnabled(chatRoom?.getCobject, withPushEnabled: !LinphoneManager.getChatroomPushEnabled(chatRoom?.getCobject))
	}
	
	func onEditionChangeClick() {
		editModeOn()
	}
	
	func showAddressAndIdentityPopup() {
		
		let localAddress = String(utf8String: linphone_address_as_string(linphone_chat_room_get_local_address(chatRoom?.getCobject)))
		let peerAddress = String(utf8String: linphone_address_as_string(linphone_chat_room_get_peer_address(chatRoom?.getCobject)))
		
		var infoMsg: String? = nil
		if let peerAddress, let localAddress {
			infoMsg = "Chat room id:\n\(peerAddress)\nLocal account:\n\(localAddress)"
		}
		
		let popupView = UIAlertController(title: NSLocalizedString("Chatroom debug infos", comment: ""), message: infoMsg, preferredStyle: .alert)
		
		let defaultAction = UIAlertAction(
			title: NSLocalizedString("Copy to clipboard", comment: ""),
			style: .default,
			handler: { action in
				let pasteboard = UIPasteboard.general
				pasteboard.string = infoMsg
			})
		popupView.addAction(defaultAction)
		present(popupView, animated: true, completion:{
			popupView.view.superview?.isUserInteractionEnabled = true
			popupView.view.superview?.addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(self.dismissOnTapOutsideOrCancel)))
		})
		
	}
	
	func initDataSource(groupeChat: Bool, secureLevel: Bool, cChatRoom: OpaquePointer) {
		menu.dataSource.removeAll()
		
		if(groupeChat){
			menu.dataSource.append(VoipTexts.conference_schedule_start)
			menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_group_infos)
		}else{
			var contact: Contact? = nil
			let firstParticipant = chatRoom?.participants.first
			let addr = (firstParticipant != nil) ? linphone_participant_get_address(firstParticipant?.getCobject) : linphone_chat_room_get_peer_address(cChatRoom)
			
			contact = FastAddressBook.getContactWith(addr)
			
			if (contact == nil) {
				menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_add_to_contact)
			} else {
				menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_go_to_contact)
			}
		}
		if(secureLevel){
			menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_conversation_device)
			menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_ephemeral_messages)
		}
		if(LinphoneManager.getChatroomPushEnabled(chatRoom?.getCobject)){
			menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_mute_notifications)
		}else{
			menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_unmute_notifications)
		}
		menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_delete_messages)
		
		messageView.ephemeralIndicator.isHidden = (linphone_chat_room_ephemeral_enabled(chatRoom?.getCobject) == 0)
	}
	
	@objc func initChatRoom(cChatRoom:OpaquePointer) {
		chatRoom = ChatRoom.getSwiftObject(cObject: cChatRoom)
		PhoneMainView.instance().currentRoom = cChatRoom
		address = chatRoom?.peerAddress?.asString()
		
		var changeIcon = false
		let isOneToOneChat = chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesOneToOne.rawValue))
		
		if (isOneToOneChat) {
			
			let firstParticipant = chatRoom?.participants.first
			let addr = (firstParticipant != nil) ? linphone_participant_get_address(firstParticipant?.getCobject) : linphone_chat_room_get_peer_address(cChatRoom);
			address = FastAddressBook.displayName(for: addr) ?? "unknow"
			changeIcon = false
			titleParticipants.isHidden = true
			
		} else {
			address = chatRoom?.subject
			changeIcon = true
			
			titleParticipants.isHidden = false
			
			let participants = chatRoom?.participants
			participantsGroupLabel.text = ""
			participants?.forEach{ participant in
				if participantsGroupLabel.text != "" {
					participantsGroupLabel.text = participantsGroupLabel.text! + ", "
				}
				participantsGroupLabel.text = participantsGroupLabel.text! + FastAddressBook.displayName(for: linphone_participant_get_address(participant.getCobject))
			}
			
		}
		
		changeTitle(titleString: address ?? "Error")
		changeCallIcon(groupChat: changeIcon)
		
		let secureLevel = FastAddressBook.image(for: linphone_chat_room_get_security_level(cChatRoom))
		changeSecureLevel(secureLevel: secureLevel != nil, imageBadge: secureLevel)
		initDataSource(groupeChat: !isOneToOneChat, secureLevel: secureLevel != nil, cChatRoom: cChatRoom)
	}
	
	func onCallClick(cChatRoom: OpaquePointer?) {
		let firstParticipant = chatRoom?.participants.first
		let addr = (firstParticipant != nil) ? linphone_participant_get_address(firstParticipant?.getCobject) : linphone_chat_room_get_peer_address(cChatRoom);
		
		let isOneToOneChat = chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesOneToOne.rawValue))
		
		if (!isOneToOneChat) {
			alertActionConferenceCall(cChatRoom: cChatRoom)
		} else {
			LinphoneManager.instance().call(addr)
		}
	}
	
	func alertActionConferenceCall(cChatRoom: OpaquePointer?) {
		
		let alertController = CustomAlertController(title: VoipTexts.conference_start_group_call_dialog_message, message: nil, preferredStyle: .alert)
		
		alertController.setBackgroundColor(color: .darkGray)
		alertController.setTitle(font: nil, color: .white)
		alertController.setTint(color: .white)
		alertController.setMaxWidth(alert: alertController)
		
		alertController.addButtonsAlertController(alertController: alertController, buttonsViewHeightV: 60, buttonsAlertHeightV: 40)
		
		activeAlertController = alertController
		
		self.present(alertController, animated: true, completion:{
			alertController.view.superview?.isUserInteractionEnabled = true
			alertController.view.superview?.subviews[0].addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(self.dismissOnTapOutsideOrCancel)))
		})
		
		
		alertController.ok_button_alert.addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(self.onTapOkStartGroupCall)))
		
	}
	
	@objc func alertActionGoToDevicesList() {
		
		let notAskAgain = ConfigManager.instance().lpConfigBoolForKey(key: "confirmation_dialog_before_sas_call_not_ask_again");
		if(!notAskAgain){
			let alertController = CustomAlertController(title: VoipTexts.alert_dialog_secure_badge_button_chat_conversation_title, message: nil, preferredStyle: .alert)
			
			alertController.setBackgroundColor(color: .darkGray)
			alertController.setTitle(font: nil, color: .white)
			alertController.setTint(color: .white)
			alertController.setMaxWidth(alert: alertController)
			
			alertController.addButtonsAlertController(alertController: alertController, buttonsViewHeightV: 60, checkboxViewHeightV: 50, buttonsAlertHeightV: 40)
			
			activeAlertController = alertController
			
			self.present(alertController, animated: true, completion:{
				alertController.view.superview?.isUserInteractionEnabled = true
				alertController.view.superview?.subviews[0].addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(self.dismissOnTapOutsideOrCancel)))
			})
			
			alertController.ok_button_alert.addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(self.onTapOkGoToDevicesList)))
		}else{
			let view: DevicesListView = self.VIEW(DevicesListView.compositeViewDescription())
			view.room = chatRoom?.getCobject
			PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
		}
		
	}
	
	@objc func onTapOkStartGroupCall(){
		self.dismiss(animated: true, completion: nil)
		ConferenceViewModelBridge.startGroupCall(cChatRoom: (chatRoom?.getCobject)!)
	}
	
	@objc func onTapOkGoToDevicesList() {
		self.dismiss(animated: true, completion: nil)
		if(activeAlertController.isChecked){
			ConfigManager.instance().lpConfigSetBool(value: activeAlertController.isChecked, key: "confirmation_dialog_before_sas_call_not_ask_again")
		}
		let view: DevicesListView = self.VIEW(DevicesListView.compositeViewDescription())
		view.room = chatRoom?.getCobject
		PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
	}
	
	@objc func dismissOnTapOutsideOrCancel(){
		self.dismiss(animated: true, completion: nil)
	}
	
	@objc func refreshData() {
		tableController.refreshData()
		refreshControl.endRefreshing()
		if tableController.totalNumberOfItems() == 0 {
			return
		}
		tableController.loadData()
		tableController.tableView.scrollToRow(
			at: IndexPath(row: tableController.currentIndex, section: 0),
			at: .top,
			animated: false)
	}
	
	override func editModeOn(){
		super.editModeOn()
		tableController.setEditing(true, animated: false)
	}
	
	override func editModeOff(){
		super.editModeOff()
		tableController.setEditing(false, animated: false)
	}
	
	override func selectDeselectAll(){
		super.selectDeselectAll()
		if(action1SelectAllButton.isHidden){
			tableController.onSelectionToggle(action1SelectAllButton)
		}else{
			tableController.onSelectionToggle(action1SelectAllButton)
		}
	}
	
	override func deleteSelected(){
		super.deleteSelected()
		onDeleteClick()
	}
	
	func onDeleteClick() {
		let msg = NSLocalizedString("Do you want to delete the selected messages?", comment: "")
		UIConfirmationDialog.show(
			withMessage: msg,
			cancelMessage: nil,
			confirmMessage: nil,
			onCancelClick: { [self] in
				onEditionChangeClick()},
			onConfirmationClick: {
				self.tableController.removeSelection(nil)
				self.editModeOff()
				self.tableController.loadData()
			}
		)
	}
	
	func sendMessage(message: String?, withExterlBodyUrl externalUrl: URL?, rootMessage: ChatMessage?) -> Bool {
		if chatRoom == nil {
			return false
		}
		
		let msg = rootMessage
		let basic = ChatConversationView.isBasicChatRoom(chatRoom?.getCobject)
		let params = linphone_account_get_params(linphone_core_get_default_account(LinphoneManager.getLc()))
		let cpimEnabled = linphone_account_params_cpim_in_basic_chat_room_enabled(params)
		
		if (!basic || (cpimEnabled != 0)) && (message != nil) && message!.count > 0 {
			linphone_chat_message_add_utf8_text_content(msg?.getCobject, message)
		}
		
		if (externalUrl != nil) {
			linphone_chat_message_set_external_body_url(msg?.getCobject, externalUrl!.absoluteString)
		}
		
		let contentList = linphone_chat_message_get_contents(msg?.getCobject)
		if bctbx_list_size(contentList) > 0 {
			linphone_chat_message_send(msg?.getCobject)
		}
		
		if basic && (cpimEnabled == 0) && (message != nil) && message!.count > 0 {
			linphone_chat_message_send(linphone_chat_room_create_message_from_utf8(chatRoom?.getCobject, message))
		}
		
		return true
	}
	
	func sendMessageInMessageField(rootMessage: ChatMessage?) {
		if sendMessage(message: messageView.messageText.text, withExterlBodyUrl: nil, rootMessage: rootMessage) {
			messageView.messageText.text = ""
		}
	}
	
	func onSendClick() {
		//let rootMessage = replyBubble ? linphone_chat_room_create_reply_message(chatRoom?.getCobject, replyBubble.message) : linphone_chat_room_create_empty_message(chatRoom?.getCobject)
		let rootMessage = linphone_chat_room_create_empty_message(chatRoom?.getCobject)
		/*
		 if replyBubble != nil {
		 closePendingReply()
		 }
		 if isPendingVoiceRecord && voiceRecorder && linphone_recorder_get_file(voiceRecorder) {
		 let voiceContent = linphone_recorder_create_content(voiceRecorder)
		 isPendingVoiceRecord = false
		 cancelVoiceRecording()
		 stopVoiceRecordPlayer()
		 linphone_chat_message_add_content(rootMessage, voiceContent)
		 }
		 
		 if fileContext.count() > 0 {
		 if linphone_chat_room_get_capabilities(chatRoom?.getCobject) & LinphoneChatRoomCapabilitiesConference != 0 {
		 startMultiFilesUpload(rootMessage)
		 } else {
		 var i = 0
		 for i in 0..<(fileContext.count() - 1) {
		 startUploadData(fileContext.datasArray[i], withType: fileContext.typesArray[i], withName: fileContext.namesArray[i], andMessage: nil, rootMessage: nil)
		 }
		 if isOneToOne {
		 startUploadData(fileContext.datasArray[i], withType: fileContext.typesArray[i], withName: fileContext.namesArray[i], andMessage: nil, rootMessage: nil)
		 if messageView.messageText.text != "" {
		 sendMessage(message: messageView.messageText.text, withExterlBodyUrl: nil, rootMessage: rootMessage)
		 }
		 } else {
		 startUploadData(fileContext.datasArray[i], withType: fileContext.typesArray[i], withName: fileContext.namesArray[i], andMessage: messageField.text(), rootMessage: rootMessage)
		 }
		 }
		 
		 clearMessageView()
		 return
		 }
		 */
		let result = ChatMessage.getSwiftObject(cObject: rootMessage!)
		sendMessageInMessageField(rootMessage: result)
	}
	
	func on_chat_room_chat_message_received(_ cr: ChatRoom?, _ event_log: EventLog?) {
		let chat = event_log?.chatMessage
		if chat == nil {
			return
		}
		
		var hasFile = false
		// if auto_download is available and file is downloaded
		if (linphone_core_get_max_size_for_auto_download_incoming_files(LinphoneManager.getLc()) > -1) && (chat?.fileTransferInformation != nil) {
			hasFile = true
		}
		
		var returnValue = false;
		chat?.contents.forEach({ content in
			if !content.isFileTransfer && !content.isText && !content.isVoiceRecording && !hasFile {
				returnValue = true
			}
		})
		
		if returnValue {
			return
		}
		
		let from = chat?.fromAddress
		if from == nil {
			return
		}
		
		let isDisplayingBottomOfTable = tableController.tableView.indexPathsForVisibleRows?.last?.row == (tableController.totalNumberOfItems() ) - 1
		tableController.addEventEntry(event_log?.getCobject)
		
		if isDisplayingBottomOfTable {
			tableController.scroll(toBottom: true)
			tableController.scrollBadge!.text = nil
			tableController.scrollBadge!.isHidden = true
		} else {
			tableController.scrollBadge!.isHidden = false
			let unread_msg = linphone_chat_room_get_unread_messages_count(cr?.getCobject)
			tableController.scrollBadge!.text = "\(unread_msg)"
		}
	}
	
	func on_chat_room_chat_message_sending(_ cr: ChatRoom?, _ event_log: EventLog?) {
		tableController.addEventEntry(event_log?.getCobject)
		tableController.scroll(toBottom: true)
	}
	
	func on_chat_room_is_composing_received(_ cr: ChatRoom?, _ remoteAddr: Address?, _ isComposing: Bool) {
		let composing = (linphone_chat_room_is_remote_composing(cr?.getCobject) != 0) || bctbx_list_size(linphone_chat_room_get_composing_addresses(cr?.getCobject)) > 0
		setComposingVisible(composing, withDelay: 0.3)
	}
	
	func setComposingVisible(_ visible: Bool, withDelay delay: CGFloat) {
		let shouldAnimate = composingVisible != visible
		if visible {
			let addresses = chatRoom!.composingAddresses
			var composingAddresses : String? = ""
			if addresses.count == 1 {

				composingAddresses = FastAddressBook.displayName(for: addresses.first?.getCobject)
				isComposingTextView.text = String.localizedStringWithFormat(NSLocalizedString("%@ is writing...", comment: ""), composingAddresses!)
			} else {
				addresses.forEach({ addressItem in
					if composingAddresses != "" {
						composingAddresses = composingAddresses! + ", "
					}
					composingAddresses = composingAddresses! + FastAddressBook.displayName(for: addressItem.getCobject)
				})

				isComposingTextView.text = String.localizedStringWithFormat(NSLocalizedString("%@ are writing...", comment: ""), composingAddresses!)
			}
		}
		
		composingVisible = visible
		if !shouldAnimate {
			return
		}
		
		var isBottomOfView = false
		if (tableController.tableView.contentOffset.y + self.isComposingView.frame.size.height >= (tableController.tableView.contentSize.height - tableController.tableView.frame.size.height)) {
			isBottomOfView = true
		}
		
		UIView.animate(
			withDuration: delay,
			animations: {
				self.contentOriginY = self.contentView.frame.origin.y
				self.composingOriginY = self.isComposingView.frame.origin.y
				
				if visible {
					if(isBottomOfView){
						self.contentView.transform = self.contentView.transform.translatedBy(x: 0, y: -self.top_bar_height/2)
						self.contentView.transform = CGAffineTransform(translationX: 0, y: -self.top_bar_height/2)
					}
					self.isComposingView.transform = self.isComposingView.transform.translatedBy(x: 0, y: -self.top_bar_height/2)
					self.isComposingView.transform = CGAffineTransform(translationX: 0, y: -self.top_bar_height/2)
				}else{
					if(isBottomOfView){
						self.contentView.transform = self.contentView.transform.translatedBy(x: 0, y: 0)
						self.contentView.transform = CGAffineTransform(translationX: 0, y: 0)
					}
					self.isComposingView.transform = self.isComposingView.transform.translatedBy(x: 0, y: 0)
					self.isComposingView.transform = CGAffineTransform(translationX: 0, y: 0)
				}
			})
	}
	
	class func autoDownload(_ message: ChatMessage?) {
		let content = linphone_chat_message_get_file_transfer_information(message?.getCobject)
		let name = String(utf8String: linphone_content_get_name(content))
		let fileType = String(utf8String: linphone_content_get_type(content))
		let key = ChatConversationView.getKeyFromFileType(fileType, fileName: name)

		LinphoneManager.setValueInMessageAppData(name, forKey: key, in: message?.getCobject)
		DispatchQueue.main.async(execute: {
			if !VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) && ConfigManager.instance().lpConfigBoolForKey(key: "auto_write_to_gallery_preference") {
				ChatConversationViewSwift.writeMediaToGallery(name: name, fileType: fileType)
			}
		})
	}
	
	class func writeMediaToGallery(name: String?, fileType: String?) {
		let filePath = LinphoneManager.validFilePath(name)
		let fileManager = FileManager.default
		if fileManager.fileExists(atPath: filePath!) {
			let data = NSData(contentsOfFile: filePath!) as Data?
			let block: (() -> Void)? = {
				if fileType == "image" {
					// we're finished, save the image and update the message
					let image = UIImage(data: data!)
					if image == nil {
						showFileDownloadError()
						return
					}
					var placeHolder: PHObjectPlaceholder? = nil
					PHPhotoLibrary.shared().performChanges({
						let request = PHAssetCreationRequest.creationRequestForAsset(from: image!)
						placeHolder = request.placeholderForCreatedAsset
					}) { success, error in
						DispatchQueue.main.async(execute: {
							if error != nil {
								Log.e("Cannot save image data downloaded \(error!.localizedDescription)")
								let errView = UIAlertController(
									title: NSLocalizedString("Transfer error", comment: ""),
									message: NSLocalizedString("Cannot write image to photo library", comment: ""),
									preferredStyle: .alert)

								let defaultAction = UIAlertAction(
									title: "OK",
									style: .default,
									handler: { action in
									})

								errView.addAction(defaultAction)
								PhoneMainView.instance()!.present(errView, animated: true)
							} else {
								Log.i("Image saved to \(placeHolder!.localIdentifier)")
							}
						})
					}
				} else if fileType == "video" {
				var placeHolder: PHObjectPlaceholder?
				PHPhotoLibrary.shared().performChanges({
					let request = PHAssetCreationRequest.creationRequestForAssetFromVideo(atFileURL: URL(fileURLWithPath: filePath!))
					placeHolder = request?.placeholderForCreatedAsset
					}) { success, error in
						DispatchQueue.main.async(execute: {
							if error != nil {
								Log.e("Cannot save video data downloaded \(error!.localizedDescription)")
								let errView = UIAlertController(
									title: NSLocalizedString("Transfer error", comment: ""),
									message: NSLocalizedString("Cannot write video to photo library", comment: ""),
									preferredStyle: .alert)
								let defaultAction = UIAlertAction(
									title: "OK",
									style: .default,
									handler: { action in
									})

								errView.addAction(defaultAction)
								PhoneMainView.instance()!.present(errView, animated: true)
							} else {
								Log.i("video saved to \(placeHolder!.localIdentifier)")
							}
		 				})
	 				}
 				}
			}
			if PHPhotoLibrary.authorizationStatus() == .authorized {
				block!()
			} else {
				PHPhotoLibrary.requestAuthorization({ status in
					DispatchQueue.main.async(execute: {
						if PHPhotoLibrary.authorizationStatus() == .authorized {
							block!()
						} else {
							UIAlertView(title: NSLocalizedString("Photo's permission", comment: ""), message: NSLocalizedString("Photo not authorized", comment: ""), delegate: nil, cancelButtonTitle: "", otherButtonTitles: "Continue").show()
						}
					})
				})
			}
		}
	}
	
	class func showFileDownloadError() {
		let errView = UIAlertController(
			title: NSLocalizedString("File download error", comment: ""),
			message: NSLocalizedString(
				"""
					Error while downloading the file.\n\
					The file is probably encrypted.\n\
					Please retry to download this file after activating LIME.
					""",
				comment: ""),
			preferredStyle: .alert)

		let defaultAction = UIAlertAction(
			title: "OK",
			style: .default,
			handler: { action in
			})

		errView.addAction(defaultAction)
		PhoneMainView.instance()!.present(errView, animated: true)
	}
}
