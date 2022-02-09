//
//  FloatingScrollDownButton.swift
//  linphone
//
//  Created by QuentinArguillere on 27/01/2022.
//

import Foundation
import UIKit

public extension ChatConversationTableView {

	private enum Constants {
		static let trailingValue: CGFloat = 20.0
		static let leadingValue: CGFloat = 85.0
		static let buttonHeight: CGFloat = 40.0
		static let buttonWidth: CGFloat = 40.0
	}
	
	override func viewDidLoad() {
		super.viewDidLoad()
		
		tableView.tableFooterView = UIView()
		createFloatingButton()
	}
	
	override func tableView(_ tableView: UITableView, willDisplay cell: UITableViewCell, forRowAt indexPath: IndexPath) {
		if let lastCellRowIndex = tableView.indexPathsForVisibleRows?.last?.row {
			if( lastCellRowIndex != self.totalNumberOfItems() - 1) {
				self.floatingScrollButton?.isHidden = false
				self.scrollBadge?.isHidden = (self.scrollBadge?.text == nil)
			} else {
				self.floatingScrollButton?.isHidden = true
				self.scrollBadge?.text = nil
			}
		}
	}
	
	private func createFloatingButton() {
		self.floatingScrollButton = UIButton(type: .custom)
		self.floatingScrollButton?.translatesAutoresizingMaskIntoConstraints = false
		constrainFloatingButtonToWindow()
		self.floatingScrollButton?.setImage(UIImage(named: "scroll_to_bottom_default"), for: .normal)
		self.floatingScrollButton?.addTarget(self, action: #selector(scrollToBottomButtonAction(_:)), for: .touchUpInside)
		self.floatingScrollButton?.isHidden = true;
		addBadgeToButon(badge: nil)
	}
	
	private func constrainFloatingButtonToWindow() {
		DispatchQueue.main.async {
			guard let keyWindow = UIApplication.shared.keyWindow,
				  let floatingButton = self.floatingScrollButton else { return }
			keyWindow.addSubview(floatingButton)
			keyWindow.trailingAnchor.constraint(equalTo: floatingButton.trailingAnchor,
												constant: Constants.trailingValue).isActive = true
			keyWindow.bottomAnchor.constraint(equalTo: floatingButton.bottomAnchor,
											  constant: Constants.leadingValue).isActive = true
			floatingButton.widthAnchor.constraint(equalToConstant:
													Constants.buttonWidth).isActive = true
			floatingButton.heightAnchor.constraint(equalToConstant:
													Constants.buttonHeight).isActive = true
		}
	}
	
	@IBAction private func scrollToBottomButtonAction(_ sender: Any) {
		scroll(toBottom: true)
	}
	
	
	private func addBadgeToButon(badge: String?) {
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
			
			let x = (Double(scrollButton.bounds.size.width) - 10 + horizontal!)
			let y = -(Double(badgeSize.height) / 2) - 10 + vertical!
			self.scrollBadge?.frame = CGRect(x: x, y: y, width: width, height: height)
			
			self.scrollBadge!.layer.cornerRadius = self.scrollBadge!.frame.height/2
			self.scrollBadge!.layer.masksToBounds = true
			scrollButton.addSubview(self.scrollBadge!)
		}
	}
}

