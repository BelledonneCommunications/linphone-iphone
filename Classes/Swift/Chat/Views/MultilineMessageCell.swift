//
//  MultilineMessageCell.swift
//  linphone
//
//  Created by Benoît Martins on 21/02/2023.
//

import UIKit

class MultilineMessageCell: UICollectionViewCell {
	static let reuseId = "MultilineMessageCellReuseId"
	
	private let label: UILabel = UILabel(frame: .zero)
	private let contentBubble: UIView = UIView(frame: .zero)
	private let bubble: UIView = UIView(frame: .zero)
	private let imageUser: UIView = UIView(frame: .zero)
	private let chatRead = UIImageView(image: UIImage(named: "chat_read.png"))
	var constraint1 : NSLayoutConstraint? = nil
	var constraint2 : NSLayoutConstraint? = nil
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		
		let labelInset = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 0)
		
		contentView.addSubview(contentBubble)
		contentBubble.translatesAutoresizingMaskIntoConstraints = false
		contentBubble.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 0).isActive = true
		contentBubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: 0).isActive = true
		constraint1 = contentBubble.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 40)
		constraint2 = contentBubble.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -22)
		constraint1!.isActive = true
		
		contentBubble.addSubview(imageUser)
		imageUser.topAnchor.constraint(equalTo: contentView.topAnchor).isActive = true
		imageUser.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 6).isActive = true
		imageUser.backgroundColor = UIColor("D").withAlphaComponent(0.2)
		imageUser.layer.cornerRadius = 15.0
		imageUser.size(w: 30, h: 30).done()
		imageUser.isHidden = true
		
		contentBubble.addSubview(bubble)
		bubble.translatesAutoresizingMaskIntoConstraints = false
		bubble.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top).isActive = true
		bubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom).isActive = true
		bubble.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left).isActive = true
		bubble.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right).isActive = true
		
		bubble.layer.cornerRadius = 10.0
		
		label.numberOfLines = 0
		label.lineBreakMode = .byWordWrapping
		
		bubble.addSubview(label)
		label.translatesAutoresizingMaskIntoConstraints = false
		label.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top+10).isActive = true
		label.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom-10).isActive = true
		label.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left+10).isActive = true
		label.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right-10).isActive = true
		
		contentBubble.addSubview(chatRead)
		chatRead.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -2).isActive = true
		chatRead.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -8).isActive = true
		chatRead.size(w: 10, h: 10).done()
		chatRead.isHidden = true
	}
	
	required init?(coder aDecoder: NSCoder) {
		fatalError("Storyboards are quicker, easier, more seductive. Not stronger then Code.")
	}
	
	func configure(text: String?, mess: Int) {
		label.text = text
		print("Storyboards are quicker \(mess)")
		if mess == 1 {
			constraint1?.isActive = true
			constraint2?.isActive = false
			imageUser.isHidden = false
			bubble.backgroundColor = UIColor("D").withAlphaComponent(0.2)
			chatRead.isHidden = true
		}else{
			constraint1?.isActive = false
			constraint2?.isActive = true
			imageUser.isHidden = true
			bubble.backgroundColor = UIColor("A").withAlphaComponent(0.2)
			chatRead.isHidden = false
		}
	}
	
	override func preferredLayoutAttributesFitting(_ layoutAttributes: UICollectionViewLayoutAttributes) -> UICollectionViewLayoutAttributes {
		label.preferredMaxLayoutWidth = (UIScreen.main.bounds.size.width*3/4)
		layoutAttributes.bounds.size.height = systemLayoutSizeFitting(UIView.layoutFittingCompressedSize).height
		
		let cellsPerRow = 1
		let minimumInterItemSpacing = 1.0
		let marginsAndInsets = window!.safeAreaInsets.left + window!.safeAreaInsets.right + minimumInterItemSpacing * CGFloat(cellsPerRow - 1)
		layoutAttributes.bounds.size.width = ((window!.bounds.size.width - marginsAndInsets) / CGFloat(cellsPerRow)).rounded(.down)
				
		return layoutAttributes
	}
}
