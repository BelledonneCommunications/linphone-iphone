//
//  ChatConversationTableViewSwift.swift
//  linphone
//
//  Created by Benoît Martins on 20/02/2023.
//

import UIKit
import Foundation
import linphonesw

class ChatConversationTableViewSwift: UIViewController, UICollectionViewDataSource, UICollectionViewDelegateFlowLayout {
	var collectionView: UICollectionView
	
	// Initializers
	init() {
		// Create new `UICollectionView` and set `UICollectionViewFlowLayout` as its layout
		collectionView = UICollectionView(frame: .zero, collectionViewLayout: UICollectionViewFlowLayout())
		super.init(nibName: nil, bundle: nil)
	}
	
	required init?(coder aDecoder: NSCoder) {
		// Create new `UICollectionView` and set `UICollectionViewFlowLayout` as its layout
		collectionView = UICollectionView(frame: .zero, collectionViewLayout: UICollectionViewFlowLayout())
		super.init(coder: aDecoder)
	}
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		collectionView.register(MultilineMessageCell.self, forCellWithReuseIdentifier: MultilineMessageCell.reuseId)
		
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
		
		(collectionView.collectionViewLayout as! UICollectionViewFlowLayout).estimatedItemSize = UICollectionViewFlowLayout.automaticSize
		(collectionView.collectionViewLayout as! UICollectionViewFlowLayout).minimumLineSpacing = 2
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.collectionView.backgroundColor = VoipTheme.backgroundWhiteBlack.get()
		}
		
		NotificationCenter.default.addObserver(self, selector: #selector(self.rotated), name: UIDevice.orientationDidChangeNotification, object: nil)
	}
	
	deinit {
		 NotificationCenter.default.removeObserver(self)
	}

	@objc func rotated() {
		collectionView.reloadData()
	}
	
	override func viewWillAppear(_ animated: Bool) {
/*
		if view.subviews.count > 0
		{
			view.subviews.forEach({ $0.removeFromSuperview()})
		}
		view.addSubview(collectionView)
 */
		ChatConversationTableViewModel.sharedModel.reloadData()
		collectionView.reloadData()
	}
	
	override func viewDidAppear(_ animated: Bool) {
		let indexPath = IndexPath(item: ChatConversationTableViewModel.sharedModel.messageListHistory.count - 1, section: 0)
		self.collectionView.scrollToItem(at: indexPath, at: .bottom, animated: false)
	}

	override func viewDidLayoutSubviews() {
		super.viewDidLayoutSubviews()
		self.collectionView.scrollToItem(at: IndexPath(row: ChatConversationTableViewModel.sharedModel.messageListHistory.count-1, section: 0), at: .bottom, animated: false)
	}
	
	// MARK: - UICollectionViewDataSource -
	func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
		let cell = collectionView.dequeueReusableCell(withReuseIdentifier: MultilineMessageCell.reuseId, for: indexPath) as! MultilineMessageCell
		
		let basic = isBasicChatRoom(ChatConversationTableViewModel.sharedModel.chatRoom?.getCobject)
		cell.configure(message: ChatConversationTableViewModel.sharedModel.messageListHistory[indexPath.row], isBasic: basic)
		print("MultilineMessageCell configure ChatMessage audio \(indexPath.row)")
		return cell
	}
	
	func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
		return ChatConversationTableViewModel.sharedModel.messageListHistory.count
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
}
