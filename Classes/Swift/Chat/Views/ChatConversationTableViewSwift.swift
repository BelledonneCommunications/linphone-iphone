//
//  ChatConversationTableViewSwift.swift
//  linphone
//
//  Created by Benoît Martins on 20/02/2023.
//

import UIKit
import Foundation
import linphonesw
import DropDown

class ChatConversationTableViewSwift: UIViewController, UICollectionViewDataSource, UICollectionViewDelegateFlowLayout {
	
	let controlsView = ControlsView(showVideo: true, controlsViewModel: ChatConversationTableViewModel.sharedModel)
	
	static let compositeDescription = UICompositeViewDescription(ChatConversationTableViewSwift.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	var collectionView: UICollectionView = {
		let collectionView = UICollectionView(frame: .zero, collectionViewLayout: UICollectionViewFlowLayout())
		return collectionView
	}()
	
	var menu: DropDown = {
		let menu = DropDown()
		menu.dataSource = [""]
		var images = [
			"menu_resend_default",
			"menu_copy_text_default",
			"menu_forward_default",
			"menu_reply_default",
			"menu_info",
			"contact_add_default",
			"menu_delete",
			"menu_info"
		]
		menu.cellNib = UINib(nibName: "DropDownCell", bundle: nil)
		menu.customCellConfiguration = { index, title, cell in
			guard let cell = cell as? MyCell else {
				return
			}
			if(index < images.count){
				switch menu.dataSource[index] {
				case VoipTexts.bubble_chat_dropDown_resend:
					if #available(iOS 13.0, *) {
						cell.myImageView.image = UIImage(named: images[0])!.withTintColor(.darkGray)
					} else {
						cell.myImageView.image = UIImage(named: images[0])
					}
				case VoipTexts.bubble_chat_dropDown_copy_text:
					cell.myImageView.image = UIImage(named: images[1])
				case VoipTexts.bubble_chat_dropDown_forward:
					cell.myImageView.image = UIImage(named: images[2])
				case VoipTexts.bubble_chat_dropDown_reply:
					cell.myImageView.image = UIImage(named: images[3])
				case VoipTexts.bubble_chat_dropDown_infos:
					cell.myImageView.image = UIImage(named: images[4])
				case VoipTexts.bubble_chat_dropDown_add_to_contact:
					cell.myImageView.image = UIImage(named: images[5])
				case VoipTexts.bubble_chat_dropDown_delete:
					cell.myImageView.image = UIImage(named: images[6])
				default:
					cell.myImageView.image = UIImage(named: images[7])
				}
			}
		}
		return menu
	}()
	
	var basic :Bool = false
	
	override func viewDidLoad() {
		super.viewDidLoad()
		

		self.initView()
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.collectionView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		}
		
		NotificationCenter.default.addObserver(self, selector: #selector(self.rotated), name: UIDevice.orientationDidChangeNotification, object: nil)
		
		ChatConversationTableViewModel.sharedModel.nbEventDisplayed.observe { index in
			self.collectionView.reloadData()
		}
		
		
		collectionView.isUserInteractionEnabled = true
	}
	
	deinit {
		 NotificationCenter.default.removeObserver(self)
	}

	@objc func rotated() {
		collectionView.reloadData()
	}
	
	func initView(){
		basic = isBasicChatRoom(ChatConversationTableViewModel.sharedModel.chatRoom?.getCobject)
		
		view.addSubview(collectionView)
		collectionView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		collectionView.contentInsetAdjustmentBehavior = .always
		collectionView.contentInset = UIEdgeInsets(top: 10, left: 0, bottom: 10, right: 0)
		
		collectionView.translatesAutoresizingMaskIntoConstraints = false
		collectionView.bottomAnchor.constraint(equalTo: view.bottomAnchor, constant: 0).isActive = true
		collectionView.leftAnchor.constraint(equalTo: view.leftAnchor, constant: 0).isActive = true
		collectionView.topAnchor.constraint(equalTo: view.topAnchor, constant: 0).isActive = true
		collectionView.rightAnchor.constraint(equalTo: view.rightAnchor, constant: 0).isActive = true
		
		collectionView.dataSource = self
		collectionView.delegate = self
		collectionView.register(MultilineMessageCell.self, forCellWithReuseIdentifier: MultilineMessageCell.reuseId)
		
		(collectionView.collectionViewLayout as! UICollectionViewFlowLayout).estimatedItemSize = UICollectionViewFlowLayout.automaticSize
		(collectionView.collectionViewLayout as! UICollectionViewFlowLayout).minimumLineSpacing = 2
		
		collectionView.transform = CGAffineTransform(scaleX: 1, y: -1)
	}
	
	override func viewWillAppear(_ animated: Bool) {
		//ChatConversationTableViewModel.sharedModel.updateData()
		collectionView.reloadData()
	}
	
	override func viewDidAppear(_ animated: Bool) {
		self.collectionView.scrollToItem(at: IndexPath(item: 0, section: 0), at: .top, animated: false)
	}
    
    func scrollToMessage(message: ChatMessage){
        let messageIndex = ChatConversationTableViewModel.sharedModel.getIndexMessage(message: message)
        print("ChatConversationTableViewSwift collectionview \(messageIndex)")
        
        collectionView.reloadData()
        collectionView.layoutIfNeeded()
        collectionView.scrollToItem(at: IndexPath(row: messageIndex, section: 0), at: .bottom, animated: false)
        //Scroll twice because collection view doesn't have time to calculate cell size
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) {
            self.collectionView.scrollToItem(at: IndexPath(row: messageIndex, section: 0), at: .bottom, animated: false)
        }
    }
	
	// MARK: - UICollectionViewDataSource -
	func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
		let cell = collectionView.dequeueReusableCell(withReuseIdentifier: MultilineMessageCell.reuseId, for: indexPath) as! MultilineMessageCell

		if let message = ChatConversationTableViewModel.sharedModel.getMessage(index: indexPath.row){
			cell.configure(message: message, isBasic: basic)

			cell.onLongClickOneClick {
				self.initDataSource(message: message)
                self.tapChooseMenuItemMessage(contentViewBubble: cell.contentViewBubble, message: message, preContentSize: cell.preContentViewBubble.frame.size.height)
			}
			
			if (!cell.replyContent.isHidden && message.replyMessage != nil){
				cell.replyContent.onClick {
                    print("\n\nChatConversationTableViewSwift collectionview new")
                    print("ChatConversationTableViewSwift collectionview \(indexPath.row+1)")
                    self.scrollToMessage(message: message.replyMessage!)
				}
			}
		}
		

		
		cell.contentView.transform = CGAffineTransform(scaleX: 1, y: -1)
		return cell
	}
	
	func collectionView(_ collectionView: UICollectionView, didEndDisplaying cell: UICollectionViewCell, forItemAt indexPath: IndexPath) {
		let cell = collectionView.dequeueReusableCell(withReuseIdentifier: MultilineMessageCell.reuseId, for: indexPath) as! MultilineMessageCell
		if cell.isPlayingVoiceRecording {
			AudioPlayer.stopSharedPlayer()
		}
	}
	
	func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
		return ChatConversationTableViewModel.sharedModel.getNBMessages()
	}
	
	// MARK: - UICollectionViewDelegateFlowLayout -
	func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
		let referenceHeight: CGFloat = 100
		let referenceWidth: CGFloat = 100
		return CGSize(width: referenceWidth, height: referenceHeight)
	}
	
	func isBasicChatRoom(_ room: OpaquePointer?) -> Bool {
		if room == nil {
			return true
		}
		
		let charRoomBasic = ChatRoom.getSwiftObject(cObject: room!)
		let isBasic = charRoomBasic.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesBasic.rawValue))
		return isBasic
	}
	
    func tapChooseMenuItemMessage(contentViewBubble: UIView, message: ChatMessage, preContentSize: CGFloat) {
        
        menu.anchorView = view
        menu.width = 200

        let coordinateMin = contentViewBubble.convert(contentViewBubble.frame.origin, to: view)
        let coordinateMax = contentViewBubble.convert(CGPoint(x: contentViewBubble.frame.maxX, y: contentViewBubble.frame.maxY), to: view)
        
        print("ChatConversationTableViewSwift collectionview cellForItemAt longclick")
        
        if (coordinateMax.y + CGFloat(menu.dataSource.count * 44) - preContentSize < view.frame.maxY) {
            menu.bottomOffset = CGPoint(x: message.isOutgoing ? coordinateMax.x - 200 : coordinateMin.x, y: coordinateMax.y - preContentSize)
        } else if ((coordinateMax.y + CGFloat(menu.dataSource.count * 44) > view.frame.maxY) && coordinateMin.y > CGFloat(menu.dataSource.count * 44) + (preContentSize * 2)) {
            menu.bottomOffset = CGPoint(x: message.isOutgoing ? coordinateMax.x - 200 : coordinateMin.x, y: coordinateMin.y - (preContentSize * 2) - CGFloat(menu.dataSource.count * 44))
        } else {
            menu.bottomOffset = CGPoint(x: message.isOutgoing ? coordinateMax.x - 200 : coordinateMin.x, y: 0)
        }
        
		menu.show()
		menu.selectionAction = { [weak self] (index: Int, item: String) in
			guard let _ = self else { return }
			print(item)
			switch item {
			case VoipTexts.bubble_chat_dropDown_resend:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick resend")
				self!.resendMessage(message: message)
			case VoipTexts.bubble_chat_dropDown_copy_text:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick copy")
				self!.copyMessage(message: message)
			case VoipTexts.bubble_chat_dropDown_forward:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick forward")
				self!.forwardMessage(message: message)
			case VoipTexts.bubble_chat_dropDown_reply:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick reply")
				self!.replyMessage(message: message)
			case VoipTexts.bubble_chat_dropDown_infos:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick infos")
				self!.infoMessage(message: message)
			case VoipTexts.bubble_chat_dropDown_add_to_contact:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick add")
                self!.addToContacts(message: message)
			case VoipTexts.bubble_chat_dropDown_delete:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick delete")
				//self!.mute_unmute_notifications()
			default:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick default")
				//self!.showAddressAndIdentityPopup()
			}
			self!.menu.clearSelection()
		}
	}
	
    func initDataSource(message: ChatMessage) {
	menu = {
		let menu = DropDown()
		menu.dataSource = [""]
		let images = [
			"menu_resend_default",
			"menu_copy_text_default",
			"menu_forward_default",
			"menu_reply_default",
			"menu_info",
			"contact_add_default",
			"menu_delete",
			"menu_info"
		]
		menu.cellNib = UINib(nibName: "DropDownCell", bundle: nil)
		menu.customCellConfiguration = { index, title, cell in
			guard let cell = cell as? MyCell else {
				return
			}
			if(index < images.count){
				switch menu.dataSource[index] {
				case VoipTexts.bubble_chat_dropDown_resend:
					if #available(iOS 13.0, *) {
						cell.myImageView.image = UIImage(named: images[0])!.withTintColor(.darkGray)
					} else {
						cell.myImageView.image = UIImage(named: images[0])
					}
				case VoipTexts.bubble_chat_dropDown_copy_text:
					cell.myImageView.image = UIImage(named: images[1])
				case VoipTexts.bubble_chat_dropDown_forward:
					cell.myImageView.image = UIImage(named: images[2])
				case VoipTexts.bubble_chat_dropDown_reply:
					cell.myImageView.image = UIImage(named: images[3])
				case VoipTexts.bubble_chat_dropDown_infos:
					cell.myImageView.image = UIImage(named: images[4])
				case VoipTexts.bubble_chat_dropDown_add_to_contact:
					cell.myImageView.image = UIImage(named: images[5])
				case VoipTexts.bubble_chat_dropDown_delete:
					cell.myImageView.image = UIImage(named: images[6])
				default:
					cell.myImageView.image = UIImage(named: images[7])
				}
			}
		}
		return menu
	}()
		
		menu.dataSource.removeAll()
        let state = message.state
        
        if (state.rawValue == LinphoneChatMessageStateNotDelivered.rawValue || state.rawValue == LinphoneChatMessageStateFileTransferError.rawValue) {
            menu.dataSource.append(VoipTexts.bubble_chat_dropDown_resend)
        }
        
        if (message.utf8Text != "" && !ICSBubbleView.isConferenceInvitationMessage(cmessage: message.getCobject!)) {
            menu.dataSource.append(VoipTexts.bubble_chat_dropDown_copy_text)
        }
        
		menu.dataSource.append(VoipTexts.bubble_chat_dropDown_forward)
        
		menu.dataSource.append(VoipTexts.bubble_chat_dropDown_reply)
        
        let chatroom = message.chatRoom
        if (chatroom!.nbParticipants > 1) {
            menu.dataSource.append(VoipTexts.bubble_chat_dropDown_infos)
        }
        
        let isOneToOneChat = ChatConversationViewModel.sharedModel.chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesOneToOne.rawValue))
        if (!message.isOutgoing && FastAddressBook.getContactWith(message.fromAddress?.getCobject) == nil
            && !isOneToOneChat ) {
            menu.dataSource.append(VoipTexts.bubble_chat_dropDown_add_to_contact)
        }
        
		menu.dataSource.append(VoipTexts.bubble_chat_dropDown_delete)
	}
    
    func resendMessage(message: ChatMessage){
        if ((linphone_core_is_network_reachable(LinphoneManager.getLc()) == 0)) {
            PhoneMainView.instance().present(LinphoneUtils.networkErrorView("send a message"), animated: true)
            return;
        }else{
            message.send()
        }
    }
    
    func copyMessage(message: ChatMessage){
        UIPasteboard.general.string = message.utf8Text
    }
    
    func forwardMessage(message: ChatMessage){
        let view: ChatConversationViewSwift = self.VIEW(ChatConversationViewSwift.compositeViewDescription())
        view.pendingForwardMessage = message.getCobject
        let viewtoGo: ChatsListView = self.VIEW(ChatsListView.compositeViewDescription())
        PhoneMainView.instance().changeCurrentView(viewtoGo.compositeViewDescription())
    }
    
    func replyMessage(message: ChatMessage){
        let view: ChatConversationViewSwift = self.VIEW(ChatConversationViewSwift.compositeViewDescription())
        view.initiateReplyView(forMessage: message.getCobject)
    }
    
    func infoMessage(message: ChatMessage){
        let view: ChatConversationImdnView = self.VIEW(ChatConversationImdnView.compositeViewDescription())
        
        //let callbackMessage: OpaquePointer = linphone_chat_message_get_callbacks(message.getCobject)
        //let eventMessage: OpaquePointer = linphone_chat_message_cbs_get_user_data(callbackMessage)
        
        //view.event = eventMessage
        PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
    }
    
    func addToContacts(message: ChatMessage) {
        let addr = message.fromAddress
        addr?.clean()
        let lAddress = addr?.asStringUriOnly()
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
