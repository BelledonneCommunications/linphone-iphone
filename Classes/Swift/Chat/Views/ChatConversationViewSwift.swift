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
import PhotosUI
import AVFoundation
import EmojiPicker
import IQKeyboardManager

class ChatConversationViewSwift: BackActionsNavigationView, PHPickerViewControllerDelegate, UIDocumentPickerDelegate, UICompositeViewDelegate, UICollectionViewDataSource, UICollectionViewDelegate, UIImagePickerControllerDelegate, EmojiPickerDelegate, CoreDelegate & UINavigationControllerDelegate{ // Replaces ChatConversationView
	
	static let compositeDescription = UICompositeViewDescription(ChatConversationViewSwift.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	
	@objc var linphoneChatRoom: OpaquePointer? = nil
	@objc var tableControllerSwift = ChatConversationTableViewSwift()
	@objc var pendingForwardMessage : OpaquePointer? = nil
	@objc var sharingMedia : Bool = false
	@objc var markAsRead : Bool = false
	
	var activeAlertController = CustomAlertController()
	let refreshControl = UIRefreshControl()
	let loadingView = UIView()
	let loading = RotatingSpinner(color: VoipTheme.primary_color)
	let loadingText = StyledLabel(VoipTheme.chat_conversation_operation_in_progress_wait)
    
    var friend: Friend? = nil
    var friendDelegate: FriendDelegate? = nil
	
	let field = UITextField()
	
	var collectionViewMedia: UICollectionView = {
		let top_bar_height = 66.0
		let width = UIScreen.main.bounds.width * 0.9
		let layout: UICollectionViewFlowLayout = UICollectionViewFlowLayout()
		layout.itemSize = CGSize(width: top_bar_height*2-8, height: top_bar_height*2-8)

		layout.sectionInset = UIEdgeInsets(top: 4, left: 4, bottom: 4, right: 4)
		layout.scrollDirection = .horizontal
		layout.minimumLineSpacing = 4
		layout.minimumInteritemSpacing = 20

		let collectionView = UICollectionView(frame: CGRect(x: 0, y: 0, width: UIScreen.main.bounds.width, height: top_bar_height*2), collectionViewLayout: layout)
		collectionView.translatesAutoresizingMaskIntoConstraints = false
		collectionView.backgroundColor = .clear
		return collectionView
	}()
	
	var collectionViewReply: UICollectionView = {
		let collection_view_reply_height = 66.0
		let layout: UICollectionViewFlowLayout = UICollectionViewFlowLayout()
		layout.itemSize = CGSize(width: collection_view_reply_height, height: collection_view_reply_height)

		layout.sectionInset = UIEdgeInsets(top: 4, left: 4, bottom: 4, right: 4)
		layout.scrollDirection = .horizontal
		layout.minimumLineSpacing = 4
		layout.minimumInteritemSpacing = 20

		let collectionViewReply = UICollectionView(frame: CGRect(x: 0, y: 0, width: UIScreen.main.bounds.width - 60, height: collection_view_reply_height), collectionViewLayout: layout)
		collectionViewReply.translatesAutoresizingMaskIntoConstraints = false
		collectionViewReply.backgroundColor = .clear
		return collectionViewReply
	}()
	
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
				self.onCallClick(cChatRoom: ChatConversationViewModel.sharedModel.chatRoom?.getCobject)
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
			title: ChatConversationViewModel.sharedModel.address ?? "Error",
			participants: ChatConversationViewModel.sharedModel.participants ?? "Error"
		)
		setupViews()
		markAsRead = true
		
		//PhoneMainView.instance()!.mainViewController.view.makeSecure(field: field)
		/*
		NotificationCenter.default.addObserver(forName: UIApplication.userDidTakeScreenshotNotification, object: nil, queue: OperationQueue.main) { notification in
			if (ConfigManager.instance().lpConfigBoolForKey(key: "screenshot_preference") == false && self.floatingButton.isHidden == false) {
				let popupView = UIAlertController(title: VoipTexts.screenshot_restrictions, message: nil, preferredStyle: .alert)
				
				let defaultAction = UIAlertAction(
					title: NSLocalizedString("Ok", comment: ""),
					style: .default)
				popupView.addAction(defaultAction)
				self.present(popupView, animated: true, completion:{
					popupView.view.superview?.isUserInteractionEnabled = true
					popupView.view.superview?.addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(self.dismissOnTapOutsideOrCancel)))
				})
			}
		}
		 */
		
		ChatConversationViewModel.sharedModel.isComposing.observe { compose in
			if((compose! && self.isComposingView.isHidden)||(!compose! && !self.isComposingView.isHidden)){
				self.setComposingVisible(compose!, withDelay: 0.3)
			}
		}
		
		ChatConversationViewModel.sharedModel.messageReceived.observe { message in
			self.tableControllerSwift.refreshData(isOutgoing: false)
		}

		ChatConversationViewModel.sharedModel.stateChanged.observe { state in
			self.configureMessageField()
			self.action1BisButton.isEnabled = !ChatConversationViewModel.sharedModel.chatRoom!.isReadOnly
			self.initDataSource(groupeChat: !ChatConversationViewModel.sharedModel.isOneToOneChat, secureLevel: ChatConversationViewModel.sharedModel.secureLevel != nil, cChatRoom: (state?.getCobject)!)
		}
		
		ChatConversationViewModel.sharedModel.secureLevelChanged.observe { secure in
			self.updateParticipantLabel()
			self.tableControllerSwift.refreshData(isOutgoing: false)
			self.changeSecureLevel(secureLevel: ChatConversationViewModel.sharedModel.secureLevel != nil, imageBadge: ChatConversationViewModel.sharedModel.secureLevel)
		}
		
		ChatConversationViewModel.sharedModel.subjectChanged.observe { subject in
            if let subjectVM = ChatConversationViewModel.sharedModel.subject {
				self.titleGroupLabel.text = subjectVM
				self.titleLabel.text = subjectVM
				self.tableControllerSwift.refreshData(isOutgoing: false)
			}
		}

		ChatConversationViewModel.sharedModel.eventLog.observe { event in
			if (event?.chatMessage != nil || event?.chatMessage?.isOutgoing != nil) {
				self.tableControllerSwift.refreshData(isOutgoing: (event?.chatMessage!.isOutgoing)!)
			}else{
				self.tableControllerSwift.refreshData(isOutgoing: false)
			}
		}
		
		ChatConversationViewModel.sharedModel.indexPathVM.observe { index in
			self.collectionViewMedia.reloadData()
			if(ChatConversationViewModel.sharedModel.mediaCollectionView.count > 0){
				self.messageView.sendButton.isEnabled = true
			}
			self.loadingView.isHidden = true
			self.messageView.isLoading = false
			self.loading.stopRotation()
			
			self.messageView.sendButton.isEnabled = true
			self.messageView.pictureButton.isEnabled = true
		}
		
		ChatConversationViewModel.sharedModel.shareFileName.observe { name in
			self.messageView.messageText.text = ChatConversationViewModel.sharedModel.shareFileMessage
			self.confirmShare(ChatConversationViewModel.sharedModel.nsDataRead(), url: nil, fileName: name)
		}
		
		ChatConversationViewModel.sharedModel.shareFileURL.observe { url in
			self.messageView.messageText.text = ChatConversationViewModel.sharedModel.shareFileMessage
			self.confirmShare(ChatConversationViewModel.sharedModel.nsDataRead(), url: url, fileName: nil)
		}
		
		ChatConversationTableViewModel.sharedModel.messageSelected.observe { result in
			if ChatConversationTableViewModel.sharedModel.messageSelected.value! > 0 {
				self.action1SelectAllButton.isHidden = true
				self.action1DeselectAllButton.isHidden = false
				self.action2Delete.isEnabled = true
			}else{
				self.action1SelectAllButton.isHidden = false
				self.action1DeselectAllButton.isHidden = true
				self.action2Delete.isEnabled = false
			}
		}
		
		let notificationCenter = NotificationCenter.default
		notificationCenter.addObserver(self, selector: #selector(appMovedToForeground), name: UIApplication.willEnterForegroundNotification, object: nil)
	}
	
	@objc func appMovedToForeground() {
		if (tableControllerSwift.menu != nil && !tableControllerSwift.menu!.isHidden) {
			tableControllerSwift.menu!.hide()
		}
		if(PhoneMainView.instance().currentView == ChatConversationViewSwift.compositeViewDescription()){
			let lc: Core = Core.getSwiftObject(cObject: LinphoneManager.getLc())
			if(lc.globalState.rawValue == LinphoneGlobalOn.rawValue){
				do {
					let peerAddress = try Factory.Instance.createAddress(addr: (ChatConversationViewModel.sharedModel.chatRoom?.peerAddress?.asStringUriOnly())!)
					let localAddress = try Factory.Instance.createAddress(addr: (ChatConversationViewModel.sharedModel.chatRoom?.localAddress?.asStringUriOnly())!)
					if (peerAddress.isValid && localAddress.isValid) {
						ChatConversationViewModel.sharedModel.chatRoom = lc.searchChatRoom(params: nil, localAddr: localAddress, remoteAddr: peerAddress, participants: nil)
						if (ChatConversationViewModel.sharedModel.chatRoom != nil) {
							ChatConversationViewModel.sharedModel.createChatConversation()
							PhoneMainView.instance().currentRoom = ChatConversationViewModel.sharedModel.chatRoom?.getCobject
							tableControllerSwift.refreshDataAfterForeground()
						}
					}
				}catch{
					
				}
			}
		}
	}
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		ChatConversationViewModel.sharedModel.createChatConversation()
	
		topBar.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
		self.contentView.addSubview(tableControllerSwift.view)
		
		// Setup Autolayout constraints
		tableControllerSwift.view.translatesAutoresizingMaskIntoConstraints = false
		tableControllerSwift.view.bottomAnchor.constraint(equalTo: self.contentView.bottomAnchor, constant: 0).isActive = true
		tableControllerSwift.view.leftAnchor.constraint(equalTo: self.contentView.leftAnchor, constant: 0).isActive = true
		tableControllerSwift.view.topAnchor.constraint(equalTo: self.contentView.topAnchor, constant: 0).isActive = true
		tableControllerSwift.view.rightAnchor.constraint(equalTo: self.contentView.rightAnchor, constant: 0).isActive = true
		
		ChatConversationTableViewModel.sharedModel.chatRoom = ChatConversationViewModel.sharedModel.chatRoom
		
		messageView.sendButton.onClickAction = onSendClick
		messageView.pictureButton.onClickAction = alertAction
		messageView.voiceRecordButton.onClickAction = onVrStart
		messageView.emojisButton.addTarget(self,action:#selector(openEmojiPickerModule),
										   for:.touchUpInside)
		recordingDeleteButton.onClickAction = cancelVoiceRecording
		recordingPlayButton.onClickAction = onvrPlayPauseStop
		recordingStopButton.onClickAction = onvrPlayPauseStop
		
		if !ChatConversationViewModel.sharedModel.chatRoom!.isReadOnly {
			messageView.ephemeralIndicator.isHidden = !ChatConversationViewModel.sharedModel.chatRoom!.ephemeralEnabled
		}
	
		handlePendingTransferIfAny()
		configureMessageField()
		ChatConversationViewModel.sharedModel.shareFile()
		
		field.isUserInteractionEnabled = false
		
		/*
		let keyWindow = UIApplication.shared.windows.filter {$0.isKeyWindow}.first
		if keyWindow != nil {
			if ConfigManager.instance().lpConfigBoolForKey(key: "screenshot_preference") == false && floatingButton.isHidden == false {
				PhoneMainView.instance()!.mainViewController.view.changeSecure(field: field, isSecure: true)
			}else{
				PhoneMainView.instance()!.mainViewController.view.changeSecure(field: field, isSecure: false)
			}
		}
		 */
	}
	
	override func viewDidAppear(_ animated: Bool) {
		IQKeyboardManager.shared().isEnabled = false
	}
    
    override func viewWillDisappear(_ animated: Bool) {
        if friendDelegate != nil {
            friend?.removeDelegate(delegate: friendDelegate!)
        }
        AvatarBridge.removeAllObserver()
		
		/*
		let keyWindow = UIApplication.shared.windows.filter {$0.isKeyWindow}.first
		if keyWindow != nil {
			PhoneMainView.instance()!.mainViewController.view.changeSecure(field: field, isSecure: false)
		}
		 */
		
		field.isUserInteractionEnabled = true
    }
	
	override func viewDidDisappear(_ animated: Bool) {
		resetView()
	}
	
	@objc func resetView(){
		ChatConversationViewModel.sharedModel.resetViewModel()
		linphoneChatRoom = nil
		editModeOff()
		if(self.isComposingView.isHidden == false){
			self.isComposingView.isHidden = true
		}
		if(self.mediaSelector.isHidden == false){
			self.mediaSelector.isHidden = true
		}
		if(self.replyBubble.isHidden == false){
			self.replyBubble.isHidden = true
		}
		
		cancelVoiceRecording()

		ChatConversationViewModel.sharedModel.mediaCollectionView = []
		ChatConversationViewModel.sharedModel.replyCollectionView.removeAll()
		self.messageView.fileContext = false
		ChatConversationViewModel.sharedModel.imageT = []
		self.collectionViewMedia.reloadData()
		self.collectionViewReply.reloadData()
		if messageView.messageText.textColor == UIColor.lightGray || self.messageView.messageText.text.isEmpty{
			self.messageView.sendButton.isEnabled = false
		} else {
			self.messageView.sendButton.isEnabled = true
		}
		self.messageView.pictureButton.isEnabled = true
		
		isComposingTextView.text = ""
	}
	
	func goBackChatListView() {
		sharingMedia = false
		PhoneMainView.instance().pop(toView: ChatsListView.compositeViewDescription())
	}
	
	func tapChooseMenuItem(_ sender: UIButton) {
		menu.anchorView = sender
		menu.bottomOffset = CGPoint(x: -UIScreen.main.bounds.width * 0.6, y: sender.frame.size.height)
		menu.show()
		menu.selectionAction = { [weak self] (index: Int, item: String) in
			guard let _ = self else { return }
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
		view.room = ChatConversationViewModel.sharedModel.chatRoom?.getCobject
		PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
	}
	
	func addOrGoToContact() {
		let firstParticipant = ChatConversationViewModel.sharedModel.chatRoom?.participants.first
		let addr = (firstParticipant != nil) ? linphone_participant_get_address(firstParticipant?.getCobject) : linphone_chat_room_get_peer_address(ChatConversationViewModel.sharedModel.chatRoom?.getCobject)
		
		if let contact = FastAddressBook.getContactWith(addr) {
			let view: ContactDetailsView = self.VIEW(ContactDetailsView.compositeViewDescription())
			ContactSelection.setSelectionMode(ContactSelectionModeNone)
			MagicSearchSingleton.instance().currentFilter = ""
			view.contact = contact
			PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
		} else {
			if let lAddress = linphone_address_as_string_uri_only(addr) {
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
		let contactsArrayCopy: NSMutableArray = []
		let admins: NSMutableArray = []
		let adminsCopy: NSMutableArray = []
		let participants = ChatConversationViewModel.sharedModel.chatRoom?.participants
		participants?.forEach{ participant in
			let curi = linphone_address_as_string_uri_only(linphone_participant_get_address(participant.getCobject))
			let uri = String(utf8String: curi!)
			contactsArray.add(uri!)
			contactsArrayCopy.add(uri!)
			if (linphone_participant_is_admin(participant.getCobject) != 0) {
				admins.add(uri!)
				adminsCopy.add(uri!)
			}
		}
		
		let view: ChatConversationInfoView = self.VIEW(ChatConversationInfoView.compositeViewDescription())
		view.create = false
		view.contacts = contactsArray
		view.oldContacts = contactsArrayCopy
		view.admins = admins
		view.oldAdmins = adminsCopy
		view.oldSubject = String(utf8String: linphone_chat_room_get_subject(ChatConversationViewModel.sharedModel.chatRoom?.getCobject)) ?? LINPHONE_DUMMY_SUBJECT
		view.room = ChatConversationViewModel.sharedModel.chatRoom?.getCobject
		
		let localAddress = linphone_address_as_string(linphone_chat_room_get_local_address(ChatConversationViewModel.sharedModel.chatRoom?.getCobject))
		let peerAddress = linphone_address_as_string(linphone_chat_room_get_peer_address(ChatConversationViewModel.sharedModel.chatRoom?.getCobject))
		view.peerAddress = UnsafePointer(peerAddress)
		view.localAddress = UnsafePointer(localAddress)
		PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
	}
	
	func goToEphemeralSettings(){
		let view: EphemeralSettingsView = self.VIEW(EphemeralSettingsView.compositeViewDescription())
		view.room = ChatConversationViewModel.sharedModel.chatRoom?.getCobject
		PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
	}
	
	func conferenceSchedule(){
		ConferenceViewModelBridge.scheduleFromGroupChat(cChatRoom: (ChatConversationViewModel.sharedModel.chatRoom?.getCobject)!)
		PhoneMainView.instance().pop(toView: ConferenceSchedulingView.compositeViewDescription())
	}
	
	func mute_unmute_notifications(){
		LinphoneManager.setChatroomPushEnabled(ChatConversationViewModel.sharedModel.chatRoom?.getCobject, withPushEnabled: !LinphoneManager.getChatroomPushEnabled(ChatConversationViewModel.sharedModel.chatRoom?.getCobject))
	}
	
	func onEditionChangeClick() {
		editModeOn()
	}
	
	func showAddressAndIdentityPopup() {
		
		let localAddress = String(utf8String: linphone_address_as_string(linphone_chat_room_get_local_address(ChatConversationViewModel.sharedModel.chatRoom?.getCobject)))
		let peerAddress = String(utf8String: linphone_address_as_string(linphone_chat_room_get_peer_address(ChatConversationViewModel.sharedModel.chatRoom?.getCobject)))
		
		var infoMsg: String? = nil
		if (peerAddress != nil && localAddress != nil) {
			infoMsg = "Chat room id:\n\(peerAddress ?? "nil")\nLocal account:\n\(localAddress ?? "nil")"
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
		let defaultAccount = Core.getSwiftObject(cObject: LinphoneManager.getLc()).defaultAccount
		if(groupeChat){
			if !ChatConversationViewModel.sharedModel.chatRoom!.isReadOnly && (defaultAccount != nil) && (defaultAccount!.params!.audioVideoConferenceFactoryAddress != nil) {
				menu.dataSource.append(VoipTexts.conference_schedule_start)
			}
			menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_group_infos)
		}else{
			var contact: Contact? = nil
			let firstParticipant = ChatConversationViewModel.sharedModel.chatRoom?.participants.first
			let addr = (firstParticipant != nil) ? linphone_participant_get_address(firstParticipant?.getCobject) : linphone_chat_room_get_peer_address(cChatRoom)
			
			contact = FastAddressBook.getContactWith(addr)
            
            if (contact == nil && !ConfigManager.instance().lpConfigBoolForKey(key: "read_only_native_address_book")) {
				menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_add_to_contact)
            } else if (contact != nil) {
				menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_go_to_contact)
			}
		}
		if(secureLevel){
			menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_conversation_device)
			if !ChatConversationViewModel.sharedModel.chatRoom!.isReadOnly {
				menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_ephemeral_messages)
			}
		}
		if(LinphoneManager.getChatroomPushEnabled(ChatConversationViewModel.sharedModel.chatRoom?.getCobject)){
			menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_mute_notifications)
		}else{
			menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_unmute_notifications)
		}
		menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_delete_messages)
		
		if !ChatConversationViewModel.sharedModel.chatRoom!.isReadOnly {
			messageView.ephemeralIndicator.isHidden = !ChatConversationViewModel.sharedModel.chatRoom!.ephemeralEnabled
		}
	}
	
	@objc func initChatRoom(cChatRoom:OpaquePointer) {
		ChatConversationViewModel.sharedModel.chatRoom = ChatRoom.getSwiftObject(cObject: cChatRoom)
		linphoneChatRoom = cChatRoom
		PhoneMainView.instance().currentRoom = cChatRoom
		ChatConversationViewModel.sharedModel.address = ChatConversationViewModel.sharedModel.chatRoom?.peerAddress?.asString()
		
		var changeIcon = false
		let isOneToOneChat = ChatConversationViewModel.sharedModel.chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesOneToOne.rawValue))
		
		if (isOneToOneChat) {
			
			let firstParticipant = ChatConversationViewModel.sharedModel.chatRoom?.participants.first
			let addr = (firstParticipant != nil) ? linphone_participant_get_address(firstParticipant?.getCobject) : linphone_chat_room_get_peer_address(cChatRoom);
			ChatConversationViewModel.sharedModel.address = FastAddressBook.displayName(for: addr) ?? "unknow"
			changeIcon = false
            updateParticipantLabel()
			
		} else {
			ChatConversationViewModel.sharedModel.address = ChatConversationViewModel.sharedModel.chatRoom?.subject
			changeIcon = true
            updateParticipantLabel()
			
		}
		
		changeTitle(titleString: ChatConversationViewModel.sharedModel.address ?? "Error")
		
		if !ChatConversationViewModel.sharedModel.chatRoom!.isReadOnly{
			changeCallIcon(groupChat: changeIcon)
			action1BisButton.isEnabled = true
		}else{
			action1Button.isHidden = true
			action1BisButton.isHidden = false
			action1BisButton.isEnabled = false
		}
		let secureLevel = FastAddressBook.image(for: linphone_chat_room_get_security_level(cChatRoom))
		changeSecureLevel(secureLevel: secureLevel != nil, imageBadge: secureLevel)
		initDataSource(groupeChat: !isOneToOneChat, secureLevel: secureLevel != nil, cChatRoom: cChatRoom)
	}
	
	func updateParticipantLabel(){
		let participants = ChatConversationViewModel.sharedModel.chatRoom?.participants
		participantsGroupLabel.text = ""
        if participants!.count > 1 {
            participants?.forEach{ participant in
                if participantsGroupLabel.text != "" {
                    participantsGroupLabel.text = participantsGroupLabel.text! + ", "
                }
                participantsGroupLabel.text = participantsGroupLabel.text! + FastAddressBook.displayName(for: linphone_participant_get_address(participant.getCobject))
            }
            titleParticipants.isHidden = false
        }else if participants?.first?.address?.contact() != nil {
            let participantAddress = participants?.first?.address
            participantsGroupLabel.text = participantAddress?.asStringUriOnly()
            
            
            let participantFriend = participants?.first?.address?.contact()?.friend
            friend = Friend.getSwiftObject(cObject: participantFriend!)
            
            var presenceModel : PresenceModel?
            var hasPresence : Bool? = false
            
            if friend?.address?.asStringUriOnly() != nil {
				presenceModel = friend!.presenceModel
                hasPresence = presenceModel != nil && presenceModel!.basicStatus == PresenceBasicStatus.Open
            }
            
            if (hasPresence! && presenceModel?.consolidatedPresence == ConsolidatedPresence.Online) {
                participantsGroupLabel.text = VoipTexts.chat_room_presence_online;
            } else  if (hasPresence! && presenceModel?.consolidatedPresence == ConsolidatedPresence.Busy){
                
                let timeInterval = TimeInterval(presenceModel!.latestActivityTimestamp)
                let myNSDate = Date(timeIntervalSince1970: timeInterval)
                
                if timeInterval == -1 {
                    participantsGroupLabel.text = VoipTexts.chat_room_presence_away;
                } else if Calendar.current.isDateInToday(myNSDate) {
                    let dateFormatter = DateFormatter()
                    dateFormatter.dateFormat = "HH:mm"
                    let dateString = dateFormatter.string(from: myNSDate)
                    participantsGroupLabel.text = VoipTexts.chat_room_presence_last_seen_online_today + dateString;
                } else if Calendar.current.isDateInYesterday(myNSDate) {
                    let dateFormatter = DateFormatter()
                    dateFormatter.dateFormat = "HH:mm"
                    let dateString = dateFormatter.string(from: myNSDate)
                    participantsGroupLabel.text = VoipTexts.chat_room_presence_last_seen_online_yesterday + dateString;
                } else {
                    let dateFormatter = DateFormatter()
                    dateFormatter.dateStyle = .short
                    let dateString = dateFormatter.string(from: myNSDate)
                    participantsGroupLabel.text = VoipTexts.chat_room_presence_last_seen_online + dateString;
                }
            }

            friendDelegate = FriendDelegateStub(
                onPresenceReceived: { (linphoneFriend: Friend) -> Void in
                    self.friend?.removeDelegate(delegate: self.friendDelegate!)
                    self.updateParticipantLabel()
                }
            )
            friend?.addDelegate(delegate: friendDelegate!)
            
            titleParticipants.isHidden = false
        }else{
            titleParticipants.isHidden = true
        }
	}
	
	func onCallClick(cChatRoom: OpaquePointer?) {
		let firstParticipant = ChatConversationViewModel.sharedModel.chatRoom?.participants.first
		let addr = (firstParticipant != nil) ? linphone_participant_get_address(firstParticipant?.getCobject) : linphone_chat_room_get_peer_address(cChatRoom);
		
		let isOneToOneChat = ChatConversationViewModel.sharedModel.chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesOneToOne.rawValue))
		
		AudioPlayer.initSharedPlayer()
		
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
	
	func alertActionGoToDevicesList() {
		
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
			view.room = ChatConversationViewModel.sharedModel.chatRoom?.getCobject
			PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
		}
		
	}
	
	@objc func onTapOkStartGroupCall(){
		self.dismiss(animated: true, completion: nil)
		ConferenceViewModelBridge.startGroupCall(cChatRoom: (ChatConversationViewModel.sharedModel.chatRoom?.getCobject)!)
	}
	
	@objc func onTapOkGoToDevicesList() {
		self.dismiss(animated: true, completion: nil)
		if(activeAlertController.isChecked){
			ConfigManager.instance().lpConfigSetBool(value: activeAlertController.isChecked, key: "confirmation_dialog_before_sas_call_not_ask_again")
		}
		let view: DevicesListView = self.VIEW(DevicesListView.compositeViewDescription())
		view.room = ChatConversationViewModel.sharedModel.chatRoom?.getCobject
		PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
	}
	
	@objc func dismissOnTapOutsideOrCancel(){
		self.dismiss(animated: true, completion: nil)
	}

	override func editModeOn(){
		super.editModeOn()
		ChatConversationTableViewModel.sharedModel.changeEditMode(editMode: true)
	}
	
	override func editModeOff(){
		super.editModeOff()
		ChatConversationTableViewModel.sharedModel.messageListSelected.value?.removeAll()
		ChatConversationTableViewModel.sharedModel.changeEditMode(editMode: false)
	}
	
	override func selectDeselectAll(){
		super.selectDeselectAll()
		if(action1SelectAllButton.isHidden){
			ChatConversationTableViewModel.sharedModel.selectAllMessages()
		}else{
			ChatConversationTableViewModel.sharedModel.unSelectAllMessages()
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
				//onEditionChangeClick()
			},
			onConfirmationClick: {
				ChatConversationTableViewModel.sharedModel.deleteMessagesSelected()
				self.editModeOff()
			}
		)
	}
	

	
	func sendMessageInMessageField(rootMessage: ChatMessage?) {
		if ChatConversationViewModel.sharedModel.sendMessage(message: messageView.messageText.textColor != UIColor.lightGray ? messageView.messageText.text.trimmingCharacters(in: .whitespacesAndNewlines) : "", withExterlBodyUrl: nil, rootMessage: rootMessage) {
			if !messageView.messageText.isFirstResponder{
				messageView.messageText.textColor = UIColor.lightGray
				messageView.messageText.text = "Message"
			} else {
				messageView.messageText.text = ""
			}
			messageView.emojisButton.isHidden = false
			messageView.isComposing = false
		}
		
		setSendButtonState()
	}
	
	func onSendClick() {
		let rootMessage = !replyBubble.isHidden ? linphone_chat_room_create_reply_message(ChatConversationViewModel.sharedModel.chatRoom?.getCobject, ChatConversationViewModel.sharedModel.replyMessage) : linphone_chat_room_create_empty_message(ChatConversationViewModel.sharedModel.chatRoom?.getCobject)
		
		if ChatConversationViewModel.sharedModel.isVoiceRecording {
			stopVoiceRecording()
		}
		
		if ChatConversationViewModel.sharedModel.isPendingVoiceRecord && (ChatConversationViewModel.sharedModel.voiceRecorder != nil) && (linphone_recorder_get_file(ChatConversationViewModel.sharedModel.voiceRecorder?.getCobject) != nil) {
			let voiceContent = linphone_recorder_create_content(ChatConversationViewModel.sharedModel.voiceRecorder?.getCobject)
			ChatConversationViewModel.sharedModel.isPendingVoiceRecord = false
		 	cancelVoiceRecording()
			let conference = ChatConversationViewModel.sharedModel.chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesConference.rawValue))
			if (linphone_chat_room_get_capabilities(ChatConversationViewModel.sharedModel.chatRoom?.getCobject) != 0) && conference {
				linphone_chat_message_add_content(rootMessage, voiceContent)
			}else{
				if messageView.messageText.textColor != UIColor.lightGray {
					let rootMessageText = !replyBubble.isHidden ? linphone_chat_room_create_reply_message(ChatConversationViewModel.sharedModel.chatRoom?.getCobject, ChatConversationViewModel.sharedModel.replyMessage) : linphone_chat_room_create_empty_message(ChatConversationViewModel.sharedModel.chatRoom?.getCobject)
					let result = ChatMessage.getSwiftObject(cObject: rootMessageText!)
					sendMessageInMessageField(rootMessage: result)
					
					linphone_chat_message_add_content(rootMessage, voiceContent)
				}else{
					linphone_chat_message_add_content(rootMessage, voiceContent)
				}
			}
		}
		if ChatConversationViewModel.sharedModel.fileContext.count > 0 {
			let conference = ChatConversationViewModel.sharedModel.chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesConference.rawValue))
			if (linphone_chat_room_get_capabilities(ChatConversationViewModel.sharedModel.chatRoom?.getCobject) != 0) && conference {
				let result = ChatMessage.getSwiftObject(cObject: rootMessage!)
				let _ = startMultiFilesUpload(result)
			} else {
				for i in 0..<(ChatConversationViewModel.sharedModel.fileContext.count) {
					startUploadData(ChatConversationViewModel.sharedModel.fileContext[i], withType: FileType.init(ChatConversationViewModel.sharedModel.mediaURLCollection[i].pathExtension)?.getGroupTypeFromFile(), withName: ChatConversationViewModel.sharedModel.mediaURLCollection[i].lastPathComponent, andMessage: nil, rootMessage: nil)
				}
				if messageView.messageText.textColor != UIColor.lightGray {
					let result = ChatMessage.getSwiftObject(cObject: rootMessage!)
					sendMessageInMessageField(rootMessage: result)
				}
			}
			
			ChatConversationViewModel.sharedModel.fileContext = []
			messageView.fileContext = false
			ChatConversationViewModel.sharedModel.mediaCollectionView = []
			ChatConversationViewModel.sharedModel.mediaURLCollection = []
			if(self.mediaSelector.isHidden == false){
				self.mediaSelector.isHidden = true
			}
			if(self.replyBubble.isHidden == false){
				self.replyBubble.isHidden = true
			}
	 		return
 		}
		if(self.mediaSelector.isHidden == false){
			self.mediaSelector.isHidden = true
		}
		if(self.replyBubble.isHidden == false){
			self.replyBubble.isHidden = true
		}
		let result = ChatMessage.getSwiftObject(cObject: rootMessage!)
		sendMessageInMessageField(rootMessage: result)
	}
	
	func startMultiFilesUpload(_ rootMessage: ChatMessage?) -> Bool {
		let fileTransfer = FileTransferDelegate()
		if messageView.messageText.textColor != UIColor.lightGray {
			fileTransfer.text = messageView.messageText.text
		} else {
			fileTransfer.text = ""
		}
		fileTransfer.uploadFileContent(forSwift: ChatConversationViewModel.sharedModel.fileContext, urlList: ChatConversationViewModel.sharedModel.mediaURLCollection, for: ChatConversationViewModel.sharedModel.chatRoom?.getCobject, rootMessage: rootMessage?.getCobject)
		if fileTransfer.text.isEmpty && !messageView.messageText.isFirstResponder{
			messageView.messageText.textColor = UIColor.lightGray
			messageView.messageText.text = "Message"
			messageView.emojisButton.isHidden = false
		} else {
			messageView.messageText.text = ""
			messageView.emojisButton.isHidden = false
		}
		
		messageView.sendButton.isEnabled = false

		tableControllerSwift.refreshData(isOutgoing: true)
		return true
	}
	
	@objc class func writeFileInImagesDirectory(_ data: Data?, name: String?) {
		let filePath = URL(fileURLWithPath: LinphoneManager.imagesDirectory()).appendingPathComponent(name ?? "").path
		if name != nil || (name == "") {
			Log.i("try to write file in \(filePath)")
		}
		FileManager.default.createFile(
			atPath: filePath,
			contents: data,
			attributes: nil)
	}
	
	func startUploadData(_ data: Data?, withType type: String?, withName name: String?, andMessage message: String?, rootMessage: ChatMessage?){
		ChatConversationViewModel.sharedModel.startUploadData(data, withType: type, withName: name, andMessage: message, rootMessage: rootMessage)
		tableControllerSwift.refreshData(isOutgoing: true)
	}
	
	func setComposingVisible(_ visible: Bool, withDelay delay: CGFloat) {
		if visible {
			let addresses = ChatConversationViewModel.sharedModel.chatRoom!.composingAddresses
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
		UIView.animate(withDuration: 0.3, animations: {
			self.isComposingView.isHidden = !self.isComposingView.isHidden
	   	})
	}
	
	func selectionMedia() {
		UIView.animate(withDuration: 0.3, animations: {
			self.mediaSelector.isHidden = !self.mediaSelector.isHidden
		})
	}
	
	func setRecordingVisible(visible : Bool) {
		UIView.animate(withDuration: 0.3, animations: {
			self.recordingView.isHidden = visible
		})
	}
	
	func initReplyView(_ visible: Bool, message: OpaquePointer?) {
		if visible {
			let addresses = ChatMessage.getSwiftObject(cObject: message!).fromAddress
			let composingAddresses : String? = FastAddressBook.displayName(for: addresses?.getCobject)
			replyLabelTextView.text = String.localizedStringWithFormat(NSLocalizedString("%@", comment: ""), composingAddresses!)
			
			let isIcal = ICSBubbleView.isConferenceInvitationMessage(cmessage: message!)
			let content : String? = (isIcal ? ICSBubbleView.getSubjectFromContent(cmessage: message!) : ChatMessage.getSwiftObject(cObject: message!).utf8Text)

			replyContentTextView.text = content
			replyContentForMeetingTextView.text = content
			backgroundReplyColor.backgroundColor = (linphone_chat_message_is_outgoing(message) != 0) ? UIColor("A").withAlphaComponent(0.2) : UIColor("D").withAlphaComponent(0.2)
			
			replyDeleteButton.isHidden = false
			replyDeleteButton.onClickAction = {
				self.replyDeleteButton.isHidden = true
				self.initReplyView(false, message: nil)
				ChatConversationViewModel.sharedModel.replyURLCollection.removeAll()
				ChatConversationViewModel.sharedModel.replyCollectionView.removeAll()
				self.collectionViewReply.reloadData()
			}
			
			let contentList = linphone_chat_message_get_contents(message)

			if(isIcal){
				replyMeetingSchedule.image = UIImage(named: "voip_meeting_schedule")
				replyMeetingSchedule.isHidden = false
				replyContentForMeetingTextView.isHidden = false
				replyContentTextView.isHidden = true
				mediaSelectorReply.isHidden = true
				replyContentTextSpacing.isHidden = true
			}else{

				if(bctbx_list_size(contentList) > 1 || content == ""){
					mediaSelectorReply.isHidden = false
					replyContentTextSpacing.isHidden = true
					ChatMessage.getSwiftObject(cObject: message!).contents.forEach({ content in
						if(content.isFile){
							let indexPath = IndexPath(row: ChatConversationViewModel.sharedModel.replyCollectionView.count, section: 0)
							
							if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
								var plainFile = content.exportPlainFile()
								
								ChatConversationViewModel.sharedModel.replyURLCollection.append(URL(string: plainFile.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed)!)!)
								ChatConversationViewModel.sharedModel.replyCollectionView.append(ChatConversationViewModel.sharedModel.getImageFrom(content.getCobject, filePath: plainFile, forReplyBubble: true)!)
								
								ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
								plainFile = ""
								
							}else{
								ChatConversationViewModel.sharedModel.replyURLCollection.append(URL(string: content.filePath.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed)!)!)
								ChatConversationViewModel.sharedModel.replyCollectionView.append(ChatConversationViewModel.sharedModel.getImageFrom(content.getCobject, filePath: content.filePath, forReplyBubble: true)!)
							}
							
							collectionViewReply.reloadData()
						}else if(content.isText){
							replyContentTextSpacing.isHidden = false
			 			}
					})
					
				}else{
					mediaSelectorReply.isHidden = true
				}
				replyMeetingSchedule.isHidden = true
				replyContentForMeetingTextView.isHidden = true
				replyContentTextView.isHidden = false
				
			}
			
		}
		UIView.animate(withDuration: 0.3, animations: {
			self.replyBubble.isHidden = !self.replyBubble.isHidden
	   	})
	}
	
	@objc class func getKeyFromFileType(_ fileType: String?, fileName name: String?) -> String? {
		if fileType == "video" {
			return "localvideo"
		} else if (fileType == "image") || name?.hasSuffix("JPG") ?? false || name?.hasSuffix("PNG") ?? false || name?.hasSuffix("jpg") ?? false || name?.hasSuffix("png") ?? false {
			return "localimage"
		}
		return "localfile"
	}
	
	@objc class func writeMediaToGalleryFromName(_ name: String?, fileType: String?) {
		ChatConversationViewModel.sharedModel.writeMediaToGalleryFromName(name, fileType: fileType)
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
	
	
	func alertAction() {

		let alertController = UIAlertController(title: VoipTexts.image_picker_view_alert_action_title, message: nil, preferredStyle: .actionSheet)
		
		let alert_action_camera = UIAlertAction(title: VoipTexts.image_picker_view_alert_action_camera, style: .default, handler: { (action) -> Void in
			self.imageCamera()
		})
		let alert_action_photo_library = UIAlertAction(title: VoipTexts.image_picker_view_alert_action_photo_library, style: .default, handler: { (action) -> Void in
			self.pickPhotos()
		})
		let alert_action_document = UIAlertAction(title: VoipTexts.image_picker_view_alert_action_document, style: .default, handler: { (action) -> Void in
			self.openDocumentPicker()
		})
		
		let cancel = UIAlertAction(title: VoipTexts.cancel, style: .cancel) { (action) -> Void in
		}
		
		
		alertController.addAction(cancel)
		alertController.addAction(alert_action_camera)
		alertController.addAction(alert_action_photo_library)
		alertController.addAction(alert_action_document)
		
		alertController.popoverPresentationController?.sourceView = PhoneMainView.instance().mainViewController.statusBarView
		PhoneMainView.instance().mainViewController.present(alertController, animated: true)
	}
	
	func imageCamera(){
		let imagePicker = UIImagePickerController()
		imagePicker.sourceType = .camera
		imagePicker.mediaTypes = ["public.image", "public.movie"]
		imagePicker.modalPresentationStyle = .overFullScreen
		imagePicker.delegate = self
		PhoneMainView.instance().mainViewController.present(imagePicker, animated: true)
			
	}
	
	func pickPhotos()
	{
		if #available(iOS 14.0, *) {
			var config = PHPickerConfiguration()
			config.selectionLimit = 0
			let pickerViewController = PHPickerViewController(configuration: config)
			pickerViewController.delegate = self
			PhoneMainView.instance().mainViewController.present(pickerViewController, animated: true)
		} else {
			let imagePicker = UIImagePickerController()
			imagePicker.sourceType = .photoLibrary
			imagePicker.mediaTypes = ["public.image", "public.movie"]
			imagePicker.delegate = self
			PhoneMainView.instance().mainViewController.present(imagePicker, animated: true)
		}
	}
	
	func openDocumentPicker() {
		let documentPicker = UIDocumentPickerViewController(documentTypes: ["public.jpeg","com.compuserve.gif","public.url","public.movie","com.apple.mapkit.map-item","com.adobe.pdf","public.png","public.image", "public.data", "public.text"], in: .import)
		   	documentPicker.delegate = self
		   	documentPicker.modalPresentationStyle = .overFullScreen
			documentPicker.allowsMultipleSelection = true
		   	PhoneMainView.instance().mainViewController.present(documentPicker, animated: true)
	}
	
	func setupViews() {
		mediaSelector.addSubview(collectionViewMedia)
		collectionViewMedia.dataSource = self
		collectionViewMedia.delegate = self
		collectionViewMedia.register(UICollectionViewCell.self, forCellWithReuseIdentifier: "cell")
		
		
		loadingView.backgroundColor = UIColor(red: 0.77, green: 0.77, blue: 0.77, alpha: 0.80)
		mediaSelector.addSubview(loadingView)
		loadingView.matchParentEdges().done()
		
		loadingText.text = VoipTexts.operation_in_progress_wait
		loadingView.addSubview(loading)
		loadingView.addSubview(loadingText)
		loadingText.alignParentLeft(withMargin: 10).alignParentRight(withMargin: 10).alignParentBottom(withMargin: 30).alignVerticalCenterWith(loadingView).done()
		loading.square(Int(top_bar_height)).alignVerticalCenterWith(loadingView).alignParentTop(withMargin: 20).done()
		
		mediaSelectorReply.addSubview(collectionViewReply)
		collectionViewReply.dataSource = self
		collectionViewReply.delegate = self
		collectionViewReply.register(UICollectionViewCell.self, forCellWithReuseIdentifier: "cellReply")
	}

	func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
		if(collectionView == collectionViewMedia){
			return ChatConversationViewModel.sharedModel.mediaCollectionView.count
		}
		return ChatConversationViewModel.sharedModel.replyCollectionView.count
	}

	@objc(collectionView:cellForItemAtIndexPath:) func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
		if(collectionView == collectionViewMedia){
			let cell = collectionView.dequeueReusableCell(withReuseIdentifier: "cell", for: indexPath)
			let viewCell: UIView = UIView(frame: cell.contentView.frame)
			cell.addSubview(viewCell)
			
			let deleteButton = CallControlButton(width: 22, height: 22, buttonTheme:VoipTheme.nav_black_button("reply_cancel"))
			
			deleteButton.onClickAction = {
				ChatConversationViewModel.sharedModel.mediaCollectionView.remove(at: indexPath.row)
				ChatConversationViewModel.sharedModel.mediaURLCollection.remove(at: indexPath.row)
				ChatConversationViewModel.sharedModel.fileContext.remove(at: indexPath.row)
				ChatConversationViewModel.sharedModel.urlFile.remove(at: indexPath.row)
				ChatConversationViewModel.sharedModel.imageT.remove(at: indexPath.row)
				ChatConversationViewModel.sharedModel.data.remove(at: indexPath.row)
				if(ChatConversationViewModel.sharedModel.mediaCollectionView.count == 0){
					self.messageView.fileContext = false
					self.selectionMedia()
					self.setSendButtonState()
				}
				self.collectionViewMedia.reloadData()
			}
			
			let imageCell = ChatConversationViewModel.sharedModel.mediaCollectionView[indexPath.row]
			var myImageView = UIImageView()
			
			if(FileType.init(ChatConversationViewModel.sharedModel.mediaURLCollection[indexPath.row].pathExtension)?.getGroupTypeFromFile() == FileType.file_picture_default.rawValue || FileType.init(ChatConversationViewModel.sharedModel.mediaURLCollection[indexPath.row].pathExtension)?.getGroupTypeFromFile() == FileType.file_video_default.rawValue){
				myImageView = UIImageView(image: imageCell)
			}else{
				let fileNameText = ChatConversationViewModel.sharedModel.mediaURLCollection[indexPath.row].lastPathComponent
				let fileName = SwiftUtil.textToImage(drawText:fileNameText, inImage:imageCell, forReplyBubble:true)
				myImageView = UIImageView(image: fileName)
			}
			
			myImageView.size(w: (viewCell.frame.width * 0.9)-2, h: (viewCell.frame.height * 0.9)-2).done()
			viewCell.addSubview(myImageView)
			myImageView.alignParentBottom(withMargin: 4).alignParentLeft(withMargin: 4).done()
			
			if(FileType.init(ChatConversationViewModel.sharedModel.mediaURLCollection[indexPath.row].pathExtension)?.getGroupTypeFromFile() == FileType.file_video_default.rawValue){
				var imagePlay = UIImage()
				if #available(iOS 13.0, *) {
					imagePlay = (UIImage(named: "vr_play")!.withTintColor(.white))
				} else {
					imagePlay = UIImage(named: "vr_play")!
				}
				let myImagePlayView = UIImageView(image: imagePlay)
				viewCell.addSubview(myImagePlayView)
				myImagePlayView.size(w: viewCell.frame.width/4, h: viewCell.frame.height/4).done()
				myImagePlayView.alignHorizontalCenterWith(viewCell).alignVerticalCenterWith(viewCell).done()
			}
			myImageView.contentMode = .scaleAspectFill
			myImageView.clipsToBounds = true

			viewCell.addSubview(deleteButton)
			deleteButton.alignParentRight().done()
			
			return cell
		}else{
			let cell = collectionView.dequeueReusableCell(withReuseIdentifier: "cellReply", for: indexPath)
			let viewCell: UIView = UIView(frame: cell.contentView.frame)
			cell.addSubview(viewCell)
			
			let imageCell = ChatConversationViewModel.sharedModel.replyCollectionView[indexPath.row]
			var myImageView = UIImageView()
			
			if(FileType.init(ChatConversationViewModel.sharedModel.replyURLCollection[indexPath.row].pathExtension)?.getGroupTypeFromFile() == FileType.file_picture_default.rawValue || FileType.init(ChatConversationViewModel.sharedModel.replyURLCollection[indexPath.row].pathExtension)?.getGroupTypeFromFile() == FileType.file_video_default.rawValue){
				myImageView = UIImageView(image: imageCell)
			}else{
				let fileNameText = ChatConversationViewModel.sharedModel.replyURLCollection[indexPath.row].lastPathComponent
				let fileName = SwiftUtil.textToImage(drawText:fileNameText, inImage:imageCell, forReplyBubble:true)
				myImageView = UIImageView(image: fileName)
			}
			
			myImageView.size(w: (viewCell.frame.width), h: (viewCell.frame.height)).done()
			viewCell.addSubview(myImageView)
			
			if(FileType.init(ChatConversationViewModel.sharedModel.replyURLCollection[indexPath.row].pathExtension)?.getGroupTypeFromFile() == FileType.file_video_default.rawValue){
				var imagePlay = UIImage()
				if #available(iOS 13.0, *) {
					imagePlay = (UIImage(named: "vr_play")!.withTintColor(.white))
				} else {
					imagePlay = UIImage(named: "vr_play")!
				}
				let myImagePlayView = UIImageView(image: imagePlay)
				viewCell.addSubview(myImagePlayView)
				myImagePlayView.size(w: viewCell.frame.width/4, h: viewCell.frame.height/4).done()
				myImagePlayView.alignHorizontalCenterWith(viewCell).alignVerticalCenterWith(viewCell).done()
			}
			myImageView.contentMode = .scaleAspectFill
			myImageView.clipsToBounds = true
			
			return cell
		}
	}
	
	@available(iOS 14.0, *)
	func picker(_ picker: PHPickerViewController, didFinishPicking results: [PHPickerResult]) {
		initListMedia(sequenceCount: results.count)
		
		
		picker.dismiss(animated: true, completion: nil)
		let itemProviders = results.map(\.itemProvider)
		for item in itemProviders {
			if item.hasItemConformingToTypeIdentifier(UTType.image.identifier) {
				ChatConversationViewModel.sharedModel.progress.append(item.loadFileRepresentation(forTypeIdentifier: UTType.image.identifier) { urlFile, error in
					if(ChatConversationViewModel.sharedModel.workItem!.isCancelled){
						return
					} else {
						ChatConversationViewModel.sharedModel.createCollectionViewItem(urlFile: urlFile, type: "public.image")
					}
				})
			}else if item.hasItemConformingToTypeIdentifier(UTType.movie.identifier) {
				ChatConversationViewModel.sharedModel.progress.append(item.loadFileRepresentation(forTypeIdentifier: UTType.movie.identifier) { urlFile, error in
					if(ChatConversationViewModel.sharedModel.workItem!.isCancelled){
						return
					} else {
						ChatConversationViewModel.sharedModel.createCollectionViewItem(urlFile: urlFile, type: "public.movie")
					}
				})
			}
		}
	}
	
	func imagePickerControllerDidCancel(_ picker: UIImagePickerController) {
	  picker.dismiss(animated: true, completion: nil)
	}
	
	func imagePickerController(_ picker: UIImagePickerController, didFinishPickingMediaWithInfo info: [UIImagePickerController.InfoKey : Any]) {
		initListMedia(sequenceCount: 1)
		let mediaType = info[UIImagePickerController.InfoKey.mediaType] as! String
		switch mediaType {
		case "public.image":
			let image = info[UIImagePickerController.InfoKey.originalImage] as! UIImage
			let date = Date()
			let df = DateFormatter()
			df.dateFormat = "yyyy-MM-dd-HHmmss"
			let dateString = df.string(from: date)
			
			let fileUrl = URL(string: dateString + ".jpeg")
			
			let data  = image.jpegData(compressionQuality: 1)
			
			ChatConversationViewModel.sharedModel.data.append(data)
			if let image = UIImage(data: data!) {
				ChatConversationViewModel.sharedModel.imageT.append(image)
			}else{
				ChatConversationViewModel.sharedModel.imageT.append(UIImage(named: "chat_error"))
			}
			
			ChatConversationViewModel.sharedModel.urlFile.append(fileUrl)
			DispatchQueue.main.async(execute: ChatConversationViewModel.sharedModel.workItem!)
  		case "public.movie":
			let videoUrl = info[UIImagePickerController.InfoKey.mediaURL] as! URL
			
			ChatConversationViewModel.sharedModel.createCollectionViewItem(urlFile: videoUrl, type: "public.movie")
		default:
			Log.i("Mismatched type: \(mediaType)")
	  	}
	  	picker.dismiss(animated: true, completion: nil)
	}
	
	public func documentPickerWasCancelled(_ controller: UIDocumentPickerViewController) {
		controller.dismiss(animated: true)
	}
	
	
	public func documentPicker(_ controller: UIDocumentPickerViewController, didPickDocumentsAt urls: [URL]) {
		initListMedia(sequenceCount: urls.count)
		
		if(controller.documentPickerMode == .import){
			urls.forEach { url in
				let imageExtension = ["png", "jpg", "jpeg", "bmp", "heic"]
				let videoExtension = ["mkv", "avi", "mov", "mp4"]
				if(imageExtension.contains(url.pathExtension.lowercased())){
					ChatConversationViewModel.sharedModel.createCollectionViewItem(urlFile: url, type: "public.image")
				}else if(videoExtension.contains(url.pathExtension.lowercased())){
					ChatConversationViewModel.sharedModel.createCollectionViewItem(urlFile: url, type: "public.movie")
				}else{
					ChatConversationViewModel.sharedModel.createCollectionViewItem(urlFile: url, type: "public.data")
			   	}
			}
		}
		
		controller.dismiss(animated: true)
	}
	
	public func initListMedia(sequenceCount : Int){
		if(ChatConversationViewModel.sharedModel.mediaCollectionView.count == 0 && sequenceCount >= 1){
			self.selectionMedia()
			self.messageView.sendButton.isEnabled = !messageView.isLoading
			self.messageView.fileContext = true
			ChatConversationViewModel.sharedModel.urlFile = []
			ChatConversationViewModel.sharedModel.imageT = []
			ChatConversationViewModel.sharedModel.data = []
		}
		if(ChatConversationViewModel.sharedModel.mediaCollectionView.count > 0){
			self.messageView.sendButton.isEnabled = !messageView.isLoading
		}
		
		if(sequenceCount >= 1){
			loadingView.isHidden = false
			messageView.isLoading = true
			loading.startRotation()
			
			self.messageView.sendButton.isEnabled = false
			self.messageView.pictureButton.isEnabled = false
			
			ChatConversationViewModel.sharedModel.mediaCount = ChatConversationViewModel.sharedModel.mediaCollectionView.count
			ChatConversationViewModel.sharedModel.newMediaCount = sequenceCount
		}
	}
	
	func handlePendingTransferIfAny() {
		if (pendingForwardMessage != nil) {
			let message = pendingForwardMessage
			pendingForwardMessage = nil
			let d = UIConfirmationDialog.show(
				withMessage: NSLocalizedString("Transfer this message to this conversation ?", comment: ""),
				cancelMessage: nil,
				confirmMessage: NSLocalizedString("TRANSFER", comment: ""),
				onCancelClick: {
				},
				onConfirmationClick: {
					linphone_chat_message_send(linphone_chat_room_create_forward_message(ChatConversationViewModel.sharedModel.chatRoom?.getCobject, message))

				})
			d?.forwardImage.isHidden = false
			d?.setSpecialColor()
		}
	}
	
	func confirmShare(_ data: Data?, url: String?, fileName: String?) {
		let sheet = DTActionSheet(title: NSLocalizedString("Select or create a conversation to share the file(s)", comment: ""))
		DispatchQueue.main.async(execute: { [self] in
			sheet!.addButton(
				withTitle: NSLocalizedString("Send to this conversation", comment: "")) { [self] in
					do{
						if messageView.messageText.textColor != UIColor.lightGray {
							try sendMessageInMessageField(rootMessage: ChatConversationViewModel.sharedModel.chatRoom?.createEmptyMessage())
						}
						if let sUrl = url {
							_ = try ChatConversationViewModel.sharedModel.sendMessage(message: sUrl, withExterlBodyUrl: nil, rootMessage: ChatConversationViewModel.sharedModel.chatRoom?.createEmptyMessage())
						} else {
							try startFileUpload(data, withName: fileName, rootMessage: ChatConversationViewModel.sharedModel.chatRoom?.createEmptyMessage())
						}
					}catch{
						Log.e(error.localizedDescription)
					}
			}

			sheet!.addCancelButton(withTitle: NSLocalizedString("Cancel", comment: ""), block: nil)
			sheet!.show(in: PhoneMainView.instance().view)
		})
	}
	
	func startFileUpload(_ data: Data?, withName name: String?, rootMessage: ChatMessage?){
		ChatConversationViewModel.sharedModel.startFileUpload(data, withName: name, rootMessage: rootMessage)
		tableControllerSwift.refreshData(isOutgoing: true)
	}
	
	@objc class func getFileUrl(_ name: String?) -> URL? {
		let filePath = LinphoneManager.validFilePath(name)
		return URL(fileURLWithPath: filePath!)
	}
	
	@objc func initiateReplyView(forMessage: OpaquePointer?) {
		if(replyBubble.isHidden == false){
			replyBubble.isHidden = true
		}
		ChatConversationViewModel.sharedModel.replyURLCollection.removeAll()
		ChatConversationViewModel.sharedModel.replyCollectionView.removeAll()
		self.collectionViewReply.reloadData()
		ChatConversationViewModel.sharedModel.replyMessage = forMessage
		initReplyView(true, message: forMessage)
	}
	
	@objc class func isBasicChatRoom(_ room: OpaquePointer?) -> Bool {
		if room == nil {
			return true
		}
		
		let charRoomBasic = ChatRoom.getSwiftObject(cObject: room!)
		let isBasic = charRoomBasic.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesBasic.rawValue))
		return isBasic
	}
	
	func onVrStart() {
		self.recordingWaveImageMask.isHidden = false
		recordingWaveView.progress = 0.0
		recordingWaveView.setProgress(recordingWaveView.progress, animated: false)
		self.messageView.sendButton.isEnabled = true
		if ChatConversationViewModel.sharedModel.isVoiceRecording {
			stopVoiceRecording()
		} else {
			startVoiceRecording()
		}
	}
	
	@objc private func openEmojiPickerModule(sender: UIButton) {
		messageView.messageText.resignFirstResponder()
		let viewController = EmojiPickerViewController()
		viewController.delegate = self
		viewController.sourceView = sender
		viewController.isDismissedAfterChoosing = false
		present(viewController, animated: true, completion: nil)
	}
	
	func didGetEmoji(emoji: String) {
		if messageView.messageText.textColor != UIColor.lightGray {
			messageView.messageText.text = messageView.messageText.text + emoji
		} else {
			messageView.messageText.textColor = UIColor.black
			messageView.messageText.text = emoji
		}
	}
	
	func startVoiceRecording() {
		ChatConversationViewModel.sharedModel.startVoiceRecording()
		setRecordingVisible(visible: false)
		messageView.voiceRecordButton.isSelected = true
		recordingStopButton.isHidden = false
		recordingPlayButton.isHidden = true
		self.recordingWaveImageMask.transform = CGAffineTransform.identity
		recordingDurationTextView.isHidden = false
		recordingDurationTextView.text = ChatConversationViewModel.sharedModel.formattedDuration(Int(linphone_recorder_get_duration(ChatConversationViewModel.sharedModel.voiceRecorder?.getCobject)))
		ChatConversationViewModel.sharedModel.vrRecordTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { timer in
			self.voiceRecordTimerUpdate()
		}
	}
	
	func voiceRecordTimerUpdate() {
		let recorderDuration = linphone_recorder_get_duration(ChatConversationViewModel.sharedModel.voiceRecorder?.getCobject)
		if recorderDuration > LinphoneManager.instance().lpConfigInt(forKey: "voice_recording_max_duration", withDefault: 59999) {
			Log.i("[Chat Message Sending] Max duration for voice recording exceeded, stopping. (max = \(LinphoneManager.instance().lpConfigInt(forKey: "voice_recording_max_duration", withDefault: 59999))")
			stopVoiceRecording()
		} else {
			recordingDurationTextView.text = ChatConversationViewModel.sharedModel.formattedDuration(Int(linphone_recorder_get_duration(ChatConversationViewModel.sharedModel.voiceRecorder?.getCobject)))
			
			UIView.animate(withDuration: 10.0, delay: 0.0, options: [.repeat], animations: {
				self.recordingWaveImageMask.transform = CGAffineTransform(translationX: 98, y: 0).scaledBy(x: 0.01, y: 1)
			})
			
		}
	}
	
	func stopVoiceRecording() {
		ChatConversationViewModel.sharedModel.stopVoiceRecording()
		if (ChatConversationViewModel.sharedModel.voiceRecorder != nil) && linphone_recorder_get_state(ChatConversationViewModel.sharedModel.voiceRecorder?.getCobject) == LinphoneRecorderRunning {
			recordingDurationTextView.text = ChatConversationViewModel.sharedModel.formattedDuration(Int(linphone_recorder_get_duration(ChatConversationViewModel.sharedModel.voiceRecorder?.getCobject)))
		}
		if LinphoneManager.instance().lpConfigBool(forKey: "voice_recording_send_right_away", withDefault: false) {
			onSendClick()
		}
		recordingStopButton.isHidden = true
		recordingPlayButton.isHidden = false
		
		messageView.voiceRecordButton.isSelected = false
		recordingWaveImageMask.layer.removeAllAnimations()
		
		setSendButtonState()

	}
	
	func cancelVoiceRecording() {
		setRecordingVisible(visible: true)
		recordingStopButton.isHidden = false
		recordingPlayButton.isHidden = true
		recordingWaveImageMask.layer.removeAllAnimations()
		messageView.voiceRecordButton.isSelected = false
		
		ChatConversationViewModel.sharedModel.cancelVoiceRecordingVM()
		
		stopVoiceRecordPlayer()
		setSendButtonState()
	}
	
	func setSendButtonState() {
		self.messageView.sendButton.isEnabled = ((ChatConversationViewModel.sharedModel.isPendingVoiceRecord && linphone_recorder_get_duration(ChatConversationViewModel.sharedModel.voiceRecorder?.getCobject) > 0) || (messageView.messageText.textColor != UIColor.lightGray && self.messageView.messageText.text.count > 0) || ChatConversationViewModel.sharedModel.fileContext.count > 0)
	}
	
	func onvrPlayPauseStop() {
		if ChatConversationViewModel.sharedModel.isVoiceRecording {
			stopVoiceRecording()
		} else {
			if ChatConversationViewModel.sharedModel.isPlayingVoiceRecording {
				stopVoiceRecordPlayer()
			} else {
				playRecordedMessage()
			}
		}
	}
	 
	func playRecordedMessage() {
		self.recordingWaveImageMask.isHidden = true
		self.recordingPlayButton.isHidden = true
		self.recordingStopButton.isHidden = false
		
		ChatConversationViewModel.sharedModel.initSharedPlayer()
		AudioPlayer.sharedModel.fileChanged.value = ChatConversationViewModel.sharedModel.voiceRecorder?.file
		ChatConversationViewModel.sharedModel.startSharedPlayer(ChatConversationViewModel.sharedModel.voiceRecorder?.file)
		
		recordingWaveView.progress = 0.0
		ChatConversationViewModel.sharedModel.isPlayingVoiceRecording = true
		
		AudioPlayer.sharedModel.fileChanged.observe { file in
			if (file != ChatConversationViewModel.sharedModel.voiceRecorder?.file && ChatConversationViewModel.sharedModel.isPlayingVoiceRecording) {
				self.stopVoiceRecordPlayer()
			}
		}
		
		recordingWaveView.progress = 1.0
		UIView.animate(withDuration: TimeInterval(Double(AudioPlayer.getSharedPlayer()!.duration) / 1000.00), delay: 0.0, options: .curveLinear, animations: {
			self.recordingWaveView.layoutIfNeeded()
		}, completion: { (finished: Bool) in
			if (ChatConversationViewModel.sharedModel.isPlayingVoiceRecording) {
				self.stopVoiceRecordPlayer()
			}
		})
	}
	
	func stopVoiceRecordPlayer() {
		recordingView.subviews.forEach({ view in
			view.removeFromSuperview()
		})
		resetRecordingProgressBar()
		self.recordingWaveView.progress = 0.0
		self.recordingWaveView.setProgress(self.recordingWaveView.progress, animated: false)
		ChatConversationViewModel.sharedModel.stopSharedPlayer()
		self.recordingWaveImageMask.isHidden = false
		self.recordingPlayButton.isHidden = false
		self.recordingStopButton.isHidden = true
		ChatConversationViewModel.sharedModel.isPlayingVoiceRecording = false
	}
	
	func configureMessageField() {
		let isOneToOneChat = ChatConversationViewModel.sharedModel.chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesOneToOne.rawValue))
		if isOneToOneChat {
			messageView.isHidden = false
			if ChatConversationViewModel.sharedModel.chatRoom!.isReadOnly {
				ChatConversationViewModel.sharedModel.chatRoom!.addParticipant(addr: (ChatConversationViewModel.sharedModel.chatRoom?.me?.address)!)
			}
		} else {
			messageView.isHidden = ChatConversationViewModel.sharedModel.chatRoom!.isReadOnly
		}
	}
		
	@objc class func markAsRead(_ chatRoom: OpaquePointer?) {
		if ChatConversationViewModel.sharedModel.chatRoom == nil {
			return
		}
		let chatRoomSwift = ChatRoom.getSwiftObject(cObject: chatRoom!)
		chatRoomSwift.markAsRead()
		PhoneMainView.instance().updateApplicationBadgeNumber()
	}
}

extension UIView {
	func makeSecure(field: UITextField) {
		DispatchQueue.main.async {
			field.isSecureTextEntry = false
			self.addSubview(field)
			field.centerYAnchor.constraint(equalTo: self.centerYAnchor).isActive = true
			field.centerXAnchor.constraint(equalTo: self.centerXAnchor).isActive = true
			self.layer.superlayer?.addSublayer(field.layer)
			field.layer.sublayers?.first?.addSublayer(self.layer)
		}
	}
	
	func changeSecure(field: UITextField, isSecure: Bool){
		field.isSecureTextEntry = isSecure
	}
}
