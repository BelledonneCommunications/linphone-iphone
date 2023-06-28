//
//  FloatingScrollDownButton.swift
//  linphone
//
//  Created by QuentinArguillere on 27/01/2022.
//

import Foundation
import UIKit

extension ChatConversationTableViewSwift {

	private enum Constants {
		static let trailingValue: CGFloat = 30.0
		static let leadingValue: CGFloat = 85.0
		static let buttonHeight: CGFloat = 16.0
		static let buttonWidth: CGFloat = 16.0
	}
	
	/*
	override func viewDidLoad() {
		super.viewDidLoad()
		
		tableView.tableFooterView = UIView()
	}
	
	override func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)
		createFloatingButton()
	}
	 */
	
	override func viewDidDisappear(_ animated: Bool) {
		super.viewDidAppear(animated)
		self.floatingScrollButton?.removeFromSuperview()
		self.floatingScrollBackground?.removeFromSuperview()
	}
	
	func createFloatingButton() {
		self.floatingScrollButton = UIButton(type: .custom)
		self.floatingScrollBackground = UIButton(type: .custom)
		self.floatingScrollButton?.translatesAutoresizingMaskIntoConstraints = false
		self.floatingScrollBackground?.translatesAutoresizingMaskIntoConstraints = false
		constrainFloatingButtonToWindow()
		var imageFloatingScrollButton = UIImage()
		if #available(iOS 13.0, *) {
			imageFloatingScrollButton = UIImage(named: "scroll_to_bottom_default")!.withTintColor(.darkGray)
		} else {
			imageFloatingScrollButton = UIImage(named: "scroll_to_bottom_default")!
		}
		self.floatingScrollButton?.setImage(imageFloatingScrollButton, for: .normal)
		self.floatingScrollButton?.isHidden = true;
		self.floatingScrollBackground?.backgroundColor = .lightGray
		self.floatingScrollBackground?.layer.cornerRadius = 25
		self.floatingScrollBackground?.isHidden = true;
		
		self.floatingScrollButton?.onClick(action: {
			self.scrollToBottomButtonAction()
		})
		self.floatingScrollBackground?.onClick(action: {
			self.scrollToBottomButtonAction()
		})
		addBadgeToButton(badge: nil)
	}
	
	private func constrainFloatingButtonToWindow() {
		DispatchQueue.main.async {
			guard let keyWindow = self.view,
				  let floatingButton = self.floatingScrollButton else { return }
			keyWindow.addSubview(self.floatingScrollBackground!)
			keyWindow.addSubview(floatingButton)
			keyWindow.trailingAnchor.constraint(equalTo: floatingButton.trailingAnchor, constant: Constants.trailingValue).isActive = true
			
			floatingButton.bottomAnchor.constraint(equalTo: keyWindow.bottomAnchor, constant: -25).isActive = true
			
			floatingButton.widthAnchor.constraint(equalToConstant:
													Constants.buttonWidth).isActive = true
			floatingButton.heightAnchor.constraint(equalToConstant:
													Constants.buttonHeight).isActive = true
			self.floatingScrollBackground?.centerYAnchor.constraint(equalTo: floatingButton.centerYAnchor).isActive = true
			self.floatingScrollBackground?.centerXAnchor.constraint(equalTo: floatingButton.centerXAnchor).isActive = true
			self.floatingScrollBackground!.widthAnchor.constraint(equalToConstant:
																	Constants.buttonHeight*3).isActive = true
			self.floatingScrollBackground!.heightAnchor.constraint(equalToConstant:
																	Constants.buttonHeight*3).isActive = true
		}
	}
	
	@IBAction private func scrollToBottomButtonAction() {
		scrollToBottom(animated: false)
	}
	
	
	private func addBadgeToButton(badge: String?) {
		self.scrollBadge = UILabel()
		self.scrollBadge?.text = badge
		self.scrollBadge?.textColor = UIColor.white
		self.scrollBadge?.backgroundColor = UIColor.red
		self.scrollBadge?.font = UIFont.systemFont(ofSize: 12.0)
		self.scrollBadge?.sizeToFit()
		self.scrollBadge?.textAlignment = .center

		if let badgeSize = self.scrollBadge?.frame.size, let scrollButton = self.floatingScrollButton {
			let height = max(18, Double(badgeSize.height) + 5.0)
			let width = max(height, Double(badgeSize.width) + 10.0)
			
			var vertical: Double?, horizontal: Double?
			let badgeInset = UIEdgeInsets(top: 20, left: 0, bottom: 0, right: 15)
			
			vertical = Double(badgeInset.top) - Double(badgeInset.bottom)
			horizontal = Double(badgeInset.left) - Double(badgeInset.right)
			
			let x = (Double(scrollButton.bounds.size.width) + 34 + horizontal!)
			let y = -(Double(badgeSize.height) / 2) - 38 + vertical!
			self.scrollBadge?.frame = CGRect(x: x, y: y, width: width, height: height)
			
			self.scrollBadge!.layer.cornerRadius = self.scrollBadge!.frame.height/2
			self.scrollBadge!.layer.masksToBounds = true
			scrollButton.addSubview(self.scrollBadge!)
		}
	}
}

