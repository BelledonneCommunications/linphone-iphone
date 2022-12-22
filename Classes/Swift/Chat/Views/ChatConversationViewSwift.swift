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
	
	var activeAlertController = CustomAlertController()
	
	@objc let tableController = ChatConversationTableView()
	let refreshControl = UIRefreshControl()
	
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
			title: address ?? "Error"
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
	}
	
	override func viewWillDisappear(_ animated: Bool) {
		editModeOff()
	}
	
	override func viewDidAppear(_ animated: Bool) {
		tableController.reloadData()
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
			
		} else {
			address = chatRoom?.subject
			changeIcon = true
			
			
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
	
	@objc func pressed() {
		print("")
	}
}
