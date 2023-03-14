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
		
		
		initDataSource()
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

			cell.contentViewBubble.onLongClickOneClick {
				self.initDataSource()
				self.tapChooseMenuItemMessage(contentViewBubble: cell.contentViewBubble, message: message)
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
	
	func tapChooseMenuItemMessage(contentViewBubble: UIView, message: ChatMessage) {
		
		print("ChatConversationTableViewSwift collectionview cellForItemAt longclick view \(contentViewBubble.frame.size.height)")
		print("ChatConversationTableViewSwift collectionview cellForItemAt longclick view \(view.frame.size.height)")
		
		if(contentViewBubble.frame.size.height >= view.frame.size.height){
			menu.anchorView = view
			menu.width = 200
			menu.bottomOffset = CGPoint(x: 0, y: (view.frame.size.height))
		}else{
			menu.anchorView = contentViewBubble
			menu.width = 200
			menu.bottomOffset = CGPoint(x: 0, y: (contentViewBubble.frame.maxY))
		}
		
		/*
		 menu.anchorView = view
		 menu.width = 200
		 menu.bottomOffset = CGPoint(x: 0, y: (contentViewBubble.frame.maxY))
		 */
		

		
		//menu.bottomOffset = sender.location(in: view)
		
		
		
		
		
		//menu.bottomOffset = point
		//menu.topOffset = point
		menu.show()
		menu.selectionAction = { [weak self] (index: Int, item: String) in
			guard let _ = self else { return }
			print(item)
			switch item {
			case VoipTexts.bubble_chat_dropDown_resend:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick resend")
				//self!.addOrGoToContact()
			case VoipTexts.bubble_chat_dropDown_copy_text:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick copy")
				//self!.addOrGoToContact()
			case VoipTexts.bubble_chat_dropDown_forward:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick forward")
				//self!.conferenceSchedule()
			case VoipTexts.bubble_chat_dropDown_reply:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick reply")
				//self!.displayGroupInfo()
			case VoipTexts.bubble_chat_dropDown_infos:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick infos")
				//self!.goToDevicesList()
			case VoipTexts.bubble_chat_dropDown_add_to_contact:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick add")
				//self!.goToEphemeralSettings()
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
	
	func tapChooseMenuItem(_ sender: UILongPressGestureRecognizer, message: OpaquePointer) {
		/*
		if(sender.frame.height >= UIScreen.main.bounds.size.height*0.8){
			menu.bottomOffset = CGPoint(x: 0, y: 0)
		}else{
			menu.anchorView = sender
			menu.bottomOffset = CGPoint(x: 0, y: sender.frame.size.height)
		}
		 */
		
		//menu.anchorView = sender.view
		//menu.bottomOffset = CGPoint(x: 0, y: (sender.view?.frame.size.height)!)
		
		let chatMessage = ChatMessage.getSwiftObject(cObject: message)
		
		menu.anchorView = view
		menu.width = 200
		menu.bottomOffset = CGPoint(x: (sender.view?.frame.minX)!, y: sender.location(in: view).y)
		
		//menu.bottomOffset = sender.location(in: view)
		
		print("ChatConversationTableViewSwift collectionview cellForItemAt longclick sender.view \(sender.location(in: sender.view))")
		print("ChatConversationTableViewSwift collectionview cellForItemAt longclick view \(sender.location(in: view))")
		print("ChatConversationTableViewSwift collectionview cellForItemAt longclick view \(chatMessage.isOutgoing)")
		
		//menu.bottomOffset = point
		//menu.topOffset = point
		menu.show()
		menu.selectionAction = { [weak self] (index: Int, item: String) in
			guard let _ = self else { return }
			print(item)
			switch item {
			case VoipTexts.bubble_chat_dropDown_resend:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick resend")
				//self!.addOrGoToContact()
			case VoipTexts.bubble_chat_dropDown_copy_text:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick copy")
				//self!.addOrGoToContact()
			case VoipTexts.bubble_chat_dropDown_forward:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick forward")
				//self!.conferenceSchedule()
			case VoipTexts.bubble_chat_dropDown_reply:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick reply")
				//self!.displayGroupInfo()
			case VoipTexts.bubble_chat_dropDown_infos:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick infos")
				//self!.goToDevicesList()
			case VoipTexts.bubble_chat_dropDown_add_to_contact:
				print("ChatConversationTableViewSwift collectionview cellForItemAt longclick add")
				//self!.goToEphemeralSettings()
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
	
	func initDataSource() {
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
		menu.dataSource.append(VoipTexts.bubble_chat_dropDown_resend)
		menu.dataSource.append(VoipTexts.bubble_chat_dropDown_copy_text)
		menu.dataSource.append(VoipTexts.bubble_chat_dropDown_forward)
		menu.dataSource.append(VoipTexts.bubble_chat_dropDown_reply)
		menu.dataSource.append(VoipTexts.bubble_chat_dropDown_infos)
		menu.dataSource.append(VoipTexts.bubble_chat_dropDown_add_to_contact)
		menu.dataSource.append(VoipTexts.bubble_chat_dropDown_delete)
	}
}
