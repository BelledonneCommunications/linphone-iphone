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
	var notifications_on: Bool = true
	
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
			action1Bis: {
				self.onCallClick(cChatRoom: self.chatRoom?.getCobject)
			},
			action2: {
				self.tapChooseMenuItem(self.action2Button)
			},
			title: address ?? "Error"
			//title:"benoit.martins.test1"
			//title:"Coin à champis de François"
        )
        //view.backgroundColor = VoipTheme.backgroundColor3.get()
    }
	
	override func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)
		topBar.backgroundColor = VoipTheme.voipToolbarBackgroundColor.get()
	}
    
    func goBackChatListView() {
        PhoneMainView.instance().pop(toView: ChatsListView.compositeViewDescription())
    }
	
	func tapChooseMenuItem(_ sender: UIButton) {
		menu.anchorView = sender
		menu.bottomOffset = CGPoint(x: -UIScreen.main.bounds.width * 0.6, y: sender.frame.size.height)
		menu.show()
  	}
	
	func initDataSource(groupeChat: Bool, secureLevel: Bool, cChatRoom: OpaquePointer) {
		menu.dataSource.removeAll()
		
		let config = Config.newForSharedCore(appGroupId: APP_GROUP_ID, configFilename: "linphonerc", factoryConfigFilename: "")
		let debugLevel = config?.getInt(section: "app", key: "debugenable_preference", defaultValue: LogLevel.Debug.rawValue)
		debugEnabled = (debugLevel! >= LogLevel.Debug.rawValue && debugLevel! < LogLevel.Error.rawValue)
		
		print(debugEnabled)
		
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
		if(notifications_on){
			menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_mute_notifications)
		}else{
			menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_unmute_notifications)
		}
		menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_delete_messages)
		menu.dataSource.append(VoipTexts.dropdown_menu_chat_conversation_debug_infos)
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
		changeCallIcon(groupeChat: changeIcon)
		
		let secureLevel = FastAddressBook.image(for: linphone_chat_room_get_security_level(cChatRoom))
		changeSecureLevel(secureLevel: secureLevel != nil, imageBadge: secureLevel)
		initDataSource(groupeChat: !isOneToOneChat, secureLevel: secureLevel != nil, cChatRoom: cChatRoom)
	}
	
	func onCallClick(cChatRoom: OpaquePointer?) {
		let firstParticipant = chatRoom?.participants.first
		let addr = (firstParticipant != nil) ? linphone_participant_get_address(firstParticipant?.getCobject) : linphone_chat_room_get_peer_address(cChatRoom);
		
		let isOneToOneChat = chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesOneToOne.rawValue))
		
		if (!isOneToOneChat) {
			//ConferenceViewModelBridge.startGroupCall(cChatRoom: cChatRoom!)
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
										
		self.present(alertController, animated: true, completion:{
			alertController.view.superview?.isUserInteractionEnabled = true
			alertController.view.superview?.subviews[0].addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(self.dismissOnTapOutsideOrCancel)))
		})
		
		
		alertController.ok_button_alert.addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(self.onTapOkStartGroupCall)))
			  
	}
	
	@objc func onTapOkStartGroupCall(){
		self.dismiss(animated: true, completion: nil)
		ConferenceViewModelBridge.startGroupCall(cChatRoom: (chatRoom?.getCobject)!)
	}
	
	@objc override func onTapOk() {
		self.dismiss(animated: true, completion: nil)
		let view: DevicesListView = self.VIEW(DevicesListView.compositeViewDescription())
		view.room = chatRoom?.getCobject
		PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
	}
}
