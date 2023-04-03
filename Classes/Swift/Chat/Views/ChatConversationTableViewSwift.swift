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
import QuickLook

class ChatConversationTableViewSwift: UIViewController, UICollectionViewDataSource, UICollectionViewDelegateFlowLayout, QLPreviewControllerDelegate, QLPreviewControllerDataSource {
	
	let controlsView = ControlsView(showVideo: true, controlsViewModel: ChatConversationTableViewModel.sharedModel)
	
	static let compositeDescription = UICompositeViewDescription(ChatConversationTableViewSwift.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	var collectionView: UICollectionView = {
		let collectionView = UICollectionView(frame: .zero, collectionViewLayout: UICollectionViewFlowLayout())
		return collectionView
	}()
	
	var menu: DropDown? = nil
	
	var basic :Bool = false
	
	var floatingScrollButton : UIButton?
	var scrollBadge : UILabel?
	var floatingScrollBackground : UIButton?
	
	var previewItems : [QLPreviewItem?] = []
	var afterPreviewIndex = -1
	
	override func viewDidLoad() {
		super.viewDidLoad()
		

		self.initView()
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.collectionView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		}
		
		NotificationCenter.default.addObserver(self, selector: #selector(self.rotated), name: UIDevice.orientationDidChangeNotification, object: nil)
        
        ChatConversationTableViewModel.sharedModel.refreshIndexPath.observe { index in
            self.collectionView.reloadData()
        }
		
		ChatConversationTableViewModel.sharedModel.onClickIndexPath.observe { index in
			self.onGridClick(indexMessage: ChatConversationTableViewModel.sharedModel.onClickMessageIndexPath, index: index!)
		}
		
		ChatConversationTableViewModel.sharedModel.editModeOn.observe { mode in
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
		
		//collectionView.transform = CGAffineTransform(scaleX: 1, y: -1)
	}
	
	override func viewWillAppear(_ animated: Bool) {
		collectionView.reloadData()
	}
	
	override func viewDidAppear(_ animated: Bool) {
		createFloatingButton()
		if ChatConversationTableViewModel.sharedModel.getNBMessages() > 0 {
			scrollToBottom(animated: false)
		}
	}
    
    func scrollToMessage(message: ChatMessage){
        let messageIndex = ChatConversationTableViewModel.sharedModel.getIndexMessage(message: message)
        collectionView.reloadData()
        collectionView.layoutIfNeeded()
        collectionView.scrollToItem(at: IndexPath(row: messageIndex, section: 0), at: .top, animated: false)
    }
	
	func scrollToBottom(animated: Bool){
        collectionView.reloadData()
		self.collectionView.scrollToItem(at: IndexPath(item: ChatConversationTableViewModel.sharedModel.getNBMessages()-1, section: 0), at: .bottom, animated: false)
		//Scroll twice because collection view doesn't have time to calculate cell size
		DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
			self.collectionView.scrollToItem(at: IndexPath(item: ChatConversationTableViewModel.sharedModel.getNBMessages()-1, section: 0), at: .bottom, animated: animated)
		}
		ChatConversationViewSwift.markAsRead(ChatConversationViewModel.sharedModel.chatRoom?.getCobject)
        self.floatingScrollButton?.isHidden = true
        self.floatingScrollBackground?.isHidden = true
		scrollBadge!.text = "0"
	}
	
	func scrollToBottomWithRelaod(){
		if (ChatConversationTableViewModel.sharedModel.getNBMessages() > 1){
			scrollToBottom(animated: true)
			if ChatConversationTableViewModel.sharedModel.editModeOn.value! {
				ChatConversationTableViewModel.sharedModel.messageListSelected.value!.insert(false, at: 0)
			}
		}else{
			collectionView.reloadData()
		}
		
	}
	
	func refreshData(){
		if (ChatConversationTableViewModel.sharedModel.getNBMessages() > 1){
			let isDisplayingBottomOfTable = collectionView.contentOffset.y >= (collectionView.contentSize.height - collectionView.bounds.height + collectionView.contentInset.bottom) - 20

			if isDisplayingBottomOfTable {
				scrollToBottom(animated: true)
			} else {
				collectionView.reloadData()
				scrollBadge!.isHidden = false
				scrollBadge!.text = "\(ChatConversationViewModel.sharedModel.chatRoom?.unreadMessagesCount ?? 0)"
				
			}
			if ChatConversationTableViewModel.sharedModel.editModeOn.value! {
				ChatConversationTableViewModel.sharedModel.messageListSelected.value!.insert(false, at: 0)
			}
		}else{
			collectionView.reloadData()
		}
	}
    
    func scrollViewDidScroll(_ scrollView: UIScrollView) {
        let contentOffsetY = scrollView.contentOffset.y
        if contentOffsetY >= (collectionView.contentSize.height - collectionView.bounds.height + collectionView.contentInset.bottom) - 20{
            floatingScrollButton?.isHidden = true
            floatingScrollBackground?.isHidden = true
            scrollBadge?.text = "0"
            ChatConversationViewSwift.markAsRead(ChatConversationViewModel.sharedModel.chatRoom?.getCobject)
        } else {
            floatingScrollButton?.isHidden = false
            floatingScrollBackground?.isHidden = false;
            if(scrollBadge?.text ==  "0"){
                scrollBadge?.isHidden = true
            }
        }
    }
	
	// MARK: - UICollectionViewDataSource -
	func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
		let cell = collectionView.dequeueReusableCell(withReuseIdentifier: MultilineMessageCell.reuseId, for: indexPath) as! MultilineMessageCell

		if let event = ChatConversationTableViewModel.sharedModel.getMessage(index: indexPath.row){

			if(ChatConversationTableViewModel.sharedModel.editModeOn.value! && indexPath.row >= ChatConversationTableViewModel.sharedModel.messageListSelected.value!.count){
				for _ in ChatConversationTableViewModel.sharedModel.messageListSelected.value!.count...indexPath.row {
					ChatConversationTableViewModel.sharedModel.messageListSelected.value!.append(false)
				}
			}
			
			cell.configure(event: event, selfIndexPathConfigure: indexPath, editMode: ChatConversationTableViewModel.sharedModel.editModeOn.value!, selected: ChatConversationTableViewModel.sharedModel.editModeOn.value! ? ChatConversationTableViewModel.sharedModel.messageListSelected.value![indexPath.row] : false)
            
			if (event.chatMessage != nil){
				cell.onLongClickOneClick {
					self.initDataSource(message: event.chatMessage!)
					self.tapChooseMenuItemMessage(contentViewBubble: cell.contentViewBubble, event: event, preContentSize: cell.preContentViewBubble.frame.size.height)
				}
			}
			
			if (!cell.replyContent.isHidden && event.chatMessage?.replyMessage != nil){
				cell.replyContent.onClick {
					self.scrollToMessage(message: (event.chatMessage?.replyMessage)!)
				}
			}
			
			if (!cell.imageViewBubble.isHidden || !cell.imageVideoViewBubble.isHidden){
				cell.imageViewBubble.onClick {
					self.onImageClick(chatMessage: event.chatMessage!, index: indexPath.row)
				}
				cell.imageVideoViewBubble.onClick {
					self.onImageClick(chatMessage: event.chatMessage!, index: indexPath.row)
				}
			}
		}
		
		//cell.contentView.transform = CGAffineTransform(scaleX: 1, y: -1)
		return cell
	}
	
	func collectionView(_ collectionView: UICollectionView, didEndDisplaying cell: UICollectionViewCell, forItemAt indexPath: IndexPath) {
		let cell = collectionView.dequeueReusableCell(withReuseIdentifier: MultilineMessageCell.reuseId, for: indexPath) as! MultilineMessageCell
		if cell.isPlayingVoiceRecording {
			AudioPlayer.stopSharedPlayer()
		}
		if cell.ephemeralTimer != nil {
			cell.ephemeralTimer?.invalidate()
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
	
    func tapChooseMenuItemMessage(contentViewBubble: UIView, event: EventLog, preContentSize: CGFloat) {
        
        menu!.anchorView = view
        menu!.width = 200

        let coordinateMin = contentViewBubble.convert(contentViewBubble.frame.origin, to: view)
        let coordinateMax = contentViewBubble.convert(CGPoint(x: contentViewBubble.frame.maxX, y: contentViewBubble.frame.maxY), to: view)
        
        if (coordinateMax.y + CGFloat(menu!.dataSource.count * 44) - preContentSize < view.frame.maxY) {
			menu!.bottomOffset = CGPoint(x: event.chatMessage!.isOutgoing ? coordinateMax.x - 200 : coordinateMin.x, y: coordinateMax.y - preContentSize)
        } else if ((coordinateMax.y + CGFloat(menu!.dataSource.count * 44) > view.frame.maxY) && coordinateMin.y > CGFloat(menu!.dataSource.count * 44) + (preContentSize * 2)) {
			menu!.bottomOffset = CGPoint(x: event.chatMessage!.isOutgoing ? coordinateMax.x - 200 : coordinateMin.x, y: coordinateMin.y - (preContentSize * 2) - CGFloat(menu!.dataSource.count * 44))
        } else {
			menu!.bottomOffset = CGPoint(x: event.chatMessage!.isOutgoing ? coordinateMax.x - 200 : coordinateMin.x, y: 0)
        }
        
		menu!.show()
		menu!.selectionAction = { [weak self] (index: Int, item: String) in
			guard let _ = self else { return }
			print(item)
			switch item {
			case VoipTexts.bubble_chat_dropDown_resend:
				self!.resendMessage(message: event.chatMessage!)
			case VoipTexts.bubble_chat_dropDown_copy_text:
				self!.copyMessage(message: event.chatMessage!)
			case VoipTexts.bubble_chat_dropDown_forward:
				self!.forwardMessage(message: event.chatMessage!)
			case VoipTexts.bubble_chat_dropDown_reply:
				self!.replyMessage(message: event.chatMessage!)
			case VoipTexts.bubble_chat_dropDown_infos:
				self!.infoMessage(event: event)
			case VoipTexts.bubble_chat_dropDown_add_to_contact:
                self!.addToContacts(message: event.chatMessage!)
			case VoipTexts.bubble_chat_dropDown_delete:
				self!.deleteMessage(message: event.chatMessage!)
			default:
				print("Error Default")
			}
			self!.menu!.clearSelection()
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
		
		menu!.dataSource.removeAll()
        let state = message.state
        
        if (state.rawValue == LinphoneChatMessageStateNotDelivered.rawValue || state.rawValue == LinphoneChatMessageStateFileTransferError.rawValue) {
            menu!.dataSource.append(VoipTexts.bubble_chat_dropDown_resend)
        }
        
        if (message.utf8Text != "" && !ICSBubbleView.isConferenceInvitationMessage(cmessage: message.getCobject!)) {
            menu!.dataSource.append(VoipTexts.bubble_chat_dropDown_copy_text)
        }
        
		menu!.dataSource.append(VoipTexts.bubble_chat_dropDown_forward)
        
		menu!.dataSource.append(VoipTexts.bubble_chat_dropDown_reply)
        
        let chatroom = message.chatRoom
        if (chatroom!.nbParticipants > 1) {
            menu!.dataSource.append(VoipTexts.bubble_chat_dropDown_infos)
        }
        
        let isOneToOneChat = ChatConversationViewModel.sharedModel.chatRoom!.hasCapability(mask: Int(LinphoneChatRoomCapabilitiesOneToOne.rawValue))
        if (!message.isOutgoing && FastAddressBook.getContactWith(message.fromAddress?.getCobject) == nil
            && !isOneToOneChat ) {
            menu!.dataSource.append(VoipTexts.bubble_chat_dropDown_add_to_contact)
        }
        
		menu!.dataSource.append(VoipTexts.bubble_chat_dropDown_delete)
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
    
    func infoMessage(event: EventLog){
        let view: ChatConversationImdnView = self.VIEW(ChatConversationImdnView.compositeViewDescription())
		view.event = event.getCobject
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
	
	func deleteMessage(message: ChatMessage){
		if ChatConversationTableViewModel.sharedModel.editModeOn.value! {
			let indexDeletedMessage = ChatConversationTableViewModel.sharedModel.getIndexMessage(message: message)
			ChatConversationTableViewModel.sharedModel.messageListSelected.value!.remove(at: indexDeletedMessage)
			ChatConversationTableViewModel.sharedModel.messageSelected.value! -= 1
		}
		message.chatRoom?.deleteMessage(message: message)
		collectionView.reloadData()
	}
    
    public func reloadCollectionViewCell(indexPath: IndexPath){
        collectionView.reloadItems(at: [indexPath])
    }
	
	func getPreviewItem(filePath: String) -> NSURL{
		let url = NSURL(fileURLWithPath: filePath)
		
		return url
	}
	
	func numberOfPreviewItems(in controller: QLPreviewController) -> Int {
		return previewItems.count
	}
	
	func previewController(_ controller: QLPreviewController, previewItemAt index: Int) -> QLPreviewItem {
		return (previewItems[index] as QLPreviewItem?)!
	}
	
	func previewControllerDidDismiss(_ controller: QLPreviewController) {
		if afterPreviewIndex > -1 {
			//collectionView.scrollToItem(at: IndexPath(row: afterPreviewIndex, section: 0), at: .centeredVertically, animated: false)
		}
	}
	
	func onImageClick(chatMessage: ChatMessage, index: Int) {

		let state = chatMessage.state
		if (state.rawValue == LinphoneChatMessageStateNotDelivered.rawValue) {
			print("Messsage not delivered")
		} else {
			if (VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) || ConfigManager.instance().lpConfigBoolForKey(key: "use_in_app_file_viewer_for_non_encrypted_files", section: "app")){
				let view: ImageView = VIEW(ImageView.compositeViewDescription())
				
				var image = UIImage()
				if chatMessage.contents.first!.type == "image" {
					if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
						var plainFile = chatMessage.contents.first!.exportPlainFile()
						
						image = UIImage(contentsOfFile: plainFile)!
						
						ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
						plainFile = ""
						
					}else {
						image = UIImage(contentsOfFile: chatMessage.contents.first!.filePath)!
					}
				}
				PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
				view.image = image
			} else {
				let previewController = QLPreviewController()
				self.previewItems = []
				
				if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
					var plainFile = chatMessage.contents.first?.exportPlainFile()
					
					self.previewItems.append(self.getPreviewItem(filePath: plainFile!))
					
					ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
					plainFile = ""
				}else {
					self.previewItems.append(self.getPreviewItem(filePath: (chatMessage.contents.first?.filePath)!))
				}
				
				afterPreviewIndex = index
				
				previewController.currentPreviewItemIndex = 0
				previewController.dataSource = self
				previewController.delegate = self
				previewController.reloadData()
				PhoneMainView.instance().mainViewController.present(previewController, animated: true, completion: nil)
			}
		}
	}
	
	func onGridClick(indexMessage: Int, index: Int) {
		let chatMessage = ChatConversationTableViewModel.sharedModel.getMessage(index: indexMessage)?.chatMessage
		let state = chatMessage!.state
		if (state.rawValue == LinphoneChatMessageStateNotDelivered.rawValue) {
			print("Messsage not delivered")
		} else {
			if (VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) || ConfigManager.instance().lpConfigBoolForKey(key: "use_in_app_file_viewer_for_non_encrypted_files", section: "app")){
				let view: ImageView = VIEW(ImageView.compositeViewDescription())
				
				var image = UIImage()
				if chatMessage!.contents[index].type == "image" {
					if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
						var plainFile = chatMessage!.contents[index].exportPlainFile()
						
						image = UIImage(contentsOfFile: plainFile)!
						
						ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
						plainFile = ""
						
					}else {
						image = UIImage(contentsOfFile: chatMessage!.contents[index].filePath)!
					}
				}
				
				PhoneMainView.instance().changeCurrentView(view.compositeViewDescription())
				view.image = image
			} else {
				let previewController = QLPreviewController()
				self.previewItems = []
				chatMessage?.contents.forEach({ content in
					if VFSUtil.vfsEnabled(groupName: kLinphoneMsgNotificationAppGroupId) {
						var plainFile = content.exportPlainFile()
						
						self.previewItems.append(self.getPreviewItem(filePath: plainFile))
						
						ChatConversationViewModel.sharedModel.removeTmpFile(filePath: plainFile)
						plainFile = ""
						
					}else {
						self.previewItems.append(self.getPreviewItem(filePath: (content.filePath)))
					}
				})
				
				afterPreviewIndex = indexMessage
				
				previewController.dataSource = self
				previewController.currentPreviewItemIndex = index
				previewController.delegate = self
				PhoneMainView.instance().mainViewController.present(previewController, animated: true, completion: nil)
			}
		}
	}
}
