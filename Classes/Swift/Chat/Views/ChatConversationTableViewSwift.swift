//
//  ChatConversationTableViewSwift.swift
//  linphone
//
//  Created by Benoît Martins on 20/02/2023.
//

import UIKit

let textExample = [
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed non risus. Suspendisse lectus tortor, dignissim sit amet, adipiscing nec, ultricies sed, dolor. Cras elementum ultrices diam. Maecenas ligula massa, varius a, semper congue, euismod non, mi. Proin porttitor, orci nec nonummy molestie, enim est eleifend mi, non fermentum diam nisl sit amet erat. Duis semper.",
	"Ut in risus volutpat libero pharetra tempor. Cras vestibulum bibendum augue. Praesent egestas leo in pede. Praesent blandit odio eu enim. Pellentesque sed dui ut augue blandit sodales.",
	"sed pede pellentesque fermentum. Maecenas adipiscing ante non diam sodales hendrerit.",
	"ligula massa, varius a, semper congue, euismod non, mi. Proin porttitor, orci nec nonummy molestie, enim est eleifend mi, non fermentum diam nisl sit amet erat.",
	"Maecenas ligula massa, varius a, semper congue, euismod non, mi. Proin porttitor, orci nec nonummy molestie, enim est eleifend mi, non fermentum diam nisl sit amet erat. Duis semper. Duis arcu massa, scelerisque vitae, consequat in, pretium a, enim. Pellentesque congue. Ut in risus volutpat libero pharetra tempor. Cras vestibulum bibendum augue. Praesent egestas leo in pede. Praesent blandit odio eu enim.",
	"nec nonummy molestie, enim est eleifend mi, non fermentum diam nisl sit amet erat. Duis semper. Duis arcu massa, scelerisque vitae, consequat in, pretium a, enim. Pellentesque congue. Ut in risus volutpat libero pharetra tempor. Cras vestibulum bibendum augue. Praesent egestas leo in pede. Praesent blandit odio eu enim. Pellentesque sed dui ut augue blandit sodales. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae",
	"Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Aliquam nibh. Mauris ac mauris sed pede pellentesque fermentum. Maecenas adipiscing",
	"Lorem ipsum dolor sit amet",
	"Salut Salut Salut",
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed non risus. Suspendisse lectus tortor, dignissim sit amet, adipiscing nec, ultricies sed, dolor. Cras elementum ultrices diam. Maecenas ligula massa, varius a, semper congue, euismod non, mi. Proin porttitor, orci nec nonummy molestie, enim est eleifend mi, non fermentum diam nisl sit amet erat. Duis semper. Duis arcu massa, scelerisque vitae, consequat in, pretium a, enim. Pellentesque congue. Ut in risus volutpat libero pharetra tempor. Cras vestibulum bibendum augue. Praesent egestas leo in pede. Praesent blandit odio eu enim. Pellentesque sed dui ut augue blandit sodales. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Aliquam nibh. Mauris ac mauris sed pede pellentesque fermentum. Maecenas adipiscing ante non diam sodales hendrerit."
]

class ChatConversationTableViewSwift: UIViewController, UICollectionViewDataSource, UICollectionViewDelegateFlowLayout {
	private(set) var collectionView: UICollectionView
	
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
		
		// Register Cells
		collectionView.register(MultilineMessageCell.self, forCellWithReuseIdentifier: MultilineMessageCell.reuseId)
		
		// Add `coolectionView` to display hierarchy and setup its appearance
		view.addSubview(collectionView)
		collectionView.backgroundColor = .white
		collectionView.contentInsetAdjustmentBehavior = .always
		collectionView.contentInset = UIEdgeInsets(top: 10, left: 0, bottom: 10, right: 0)
		
		// Setup Autolayout constraints
		collectionView.translatesAutoresizingMaskIntoConstraints = false
		collectionView.bottomAnchor.constraint(equalTo: view.bottomAnchor, constant: 0).isActive = true
		collectionView.leftAnchor.constraint(equalTo: view.leftAnchor, constant: 0).isActive = true
		collectionView.topAnchor.constraint(equalTo: view.topAnchor, constant: 0).isActive = true
		collectionView.rightAnchor.constraint(equalTo: view.rightAnchor, constant: 0).isActive = true
		
		// Setup `dataSource` and `delegate`
		collectionView.dataSource = self
		collectionView.delegate = self
		
		(collectionView.collectionViewLayout as! UICollectionViewFlowLayout).estimatedItemSize = UICollectionViewFlowLayout.automaticSize
		(collectionView.collectionViewLayout as! UICollectionViewFlowLayout).minimumLineSpacing = 1
	}
	
	override func viewDidAppear(_ animated: Bool) {
		/*
		DispatchQueue.main.async {
			for i in 1...100{
				let indexPath = IndexPath(row: self.collectionViewMessageItem.count, section: 0)
				self.collectionViewMessageItem.append(i)
				self.collectionViewMessage.insertItems(at: [indexPath])
			}
		}
	 	*/
		//let bottomOffset = CGPoint(x: 0, y: collectionView.contentSize.height)
		//collectionView.setContentOffset(bottomOffset, animated: false)
	}
	
	// MARK: - UICollectionViewDataSource -
	func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
		let cell = collectionView.dequeueReusableCell(withReuseIdentifier: MultilineMessageCell.reuseId, for: indexPath) as! MultilineMessageCell
		cell.configure(text: textExample[indexPath.row])
		return cell
	}
	
	func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
		return textExample.count
	}
	
	// MARK: - UICollectionViewDelegateFlowLayout -
	func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
		//let sectionInset = (collectionViewLayout as! UICollectionViewFlowLayout).sectionInset
		let referenceHeight: CGFloat = 100
		let referenceWidth: CGFloat = 100
		/*let referenceWidth = collectionView.safeAreaLayoutGuide.layoutFrame.width
			- sectionInset.left
			- sectionInset.right
			- collectionView.contentInset.left
			- collectionView.contentInset.right*/
		return CGSize(width: referenceWidth, height: referenceHeight)
	}
}
