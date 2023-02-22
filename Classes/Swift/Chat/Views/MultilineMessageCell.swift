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
	
	override init(frame: CGRect) {
		super.init(frame: frame)
		
		let randomMessage = Int.random(in: 1..<3)
		
		let labelInset = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 0)
		
		contentView.addSubview(contentBubble)
		contentView.backgroundColor = .orange
		contentBubble.translatesAutoresizingMaskIntoConstraints = false
		contentBubble.topAnchor.constraint(equalTo: contentView.topAnchor, constant: 0).isActive = true
		contentBubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: 0).isActive = true
		//contentBubble.width(UIScreen.main.bounds.size.width).done()
		if(randomMessage == 1){
			contentBubble.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 50).isActive = true
		}else{
			
		}
		//contentBubble.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 50).isActive = true
		contentBubble.trailingAnchor.constraint(equalTo: contentView.trailingAnchor, constant: -20).isActive = true

		//contentBubble.backgroundColor = .green
		
		contentBubble.addSubview(bubble)
		bubble.translatesAutoresizingMaskIntoConstraints = false
		bubble.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top).isActive = true
		bubble.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom).isActive = true
		bubble.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left).isActive = true
		bubble.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right).isActive = true
		
		//bubble.bounds.origin = contentView.bounds.origin
		
		bubble.layer.cornerRadius = 10.0
		bubble.backgroundColor = .systemBlue
		
		
		label.numberOfLines = 0
		label.lineBreakMode = .byWordWrapping
		
		bubble.addSubview(label)
		label.translatesAutoresizingMaskIntoConstraints = false
		label.topAnchor.constraint(equalTo: contentView.topAnchor, constant: labelInset.top+10).isActive = true
		label.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: labelInset.bottom-10).isActive = true
		label.leadingAnchor.constraint(equalTo: contentBubble.leadingAnchor, constant: labelInset.left+10).isActive = true
		label.trailingAnchor.constraint(equalTo: contentBubble.trailingAnchor, constant: labelInset.right-10).isActive = true
		
	}
	
	required init?(coder aDecoder: NSCoder) {
		fatalError("Storyboards are quicker, easier, more seductive. Not stronger then Code.")
	}
	
	func configure(text: String?) {
		label.text = text
	}
	
	override func preferredLayoutAttributesFitting(_ layoutAttributes: UICollectionViewLayoutAttributes) -> UICollectionViewLayoutAttributes {
		//bubble.transform = CGAffineTransformIdentity
		label.preferredMaxLayoutWidth = (UIScreen.main.bounds.size.width*4/5)
		
		//print("MultilineMessageCell init UIScreen label \(label.preferredMaxLayoutWidth)")
		layoutAttributes.bounds.size.height = systemLayoutSizeFitting(UIView.layoutFittingCompressedSize).height
		layoutAttributes.bounds.size.width = (UIScreen.main.bounds.size.width / CGFloat(1)).rounded(.down)
		//print("MultilineMessageCell init UIScreen \(UIScreen.main.bounds.size.width*4/5)")
		//print("MultilineMessageCell init layoutAttributes \(layoutAttributes.bounds.size.width)")
		//print("MultilineMessageCell init \((UIScreen.main.bounds.size.width*4/5) - layoutAttributes.bounds.size.width)")
		
		//For Left
		//bubble.transform = CGAffineTransformTranslate(bubble.transform, (layoutAttributes.bounds.size.width - (UIScreen.main.bounds.size.width*4/5))/2, 0.0)
		
		//For Right
		//bubble.transform = CGAffineTransformTranslate(bubble.transform, -(layoutAttributes.bounds.size.width - (UIScreen.main.bounds.size.width*4/5))/2, 0.0)
		
		return layoutAttributes
	}
}
