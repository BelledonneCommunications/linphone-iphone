//
//  ImageViewer.swift
//  linphone
//
//  Created by Benoît Martins on 21/06/2023.
//

import Foundation

@objc class ImageViewer:  BackNextNavigationView, UICompositeViewDelegate, UIScrollViewDelegate {

	static let compositeDescription = UICompositeViewDescription(ImageViewer.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	@objc var imageNameViewer = ""
	@objc var imageViewer = UIImage()
	let imageViewViewer = UIImageView()
	let imageScrollView = UIScrollView()
	
	override func viewDidLoad() {
		
		super.viewDidLoad(
			backAction: {
				PhoneMainView.instance().popView(self.compositeViewDescription())
			},nextAction: {
			},
			nextActionEnableCondition: MutableLiveData(false),
			title:"")
		super.nextButton.isHidden = true
		
		let shareButton = CallControlButton(buttonTheme:VoipTheme.nav_button("voip_export"))
		super.topBar.addSubview(shareButton)
		shareButton.alignParentRight(withMargin: side_buttons_margin).alignParentBottom(withMargin: 18).alignParentTop(withMargin: 18).done()

		shareButton.addTarget(self, action: #selector(shareTextButton), for: .touchUpInside)
		
		let vWidth = UIScreen.main.bounds.size.width
		let vHeight = UIScreen.main.bounds.size.height-32.0

		imageScrollView.delegate = self
		imageScrollView.frame = CGRectMake(0, 0, vWidth, vHeight)
		imageScrollView.showsVerticalScrollIndicator = true

		imageScrollView.minimumZoomScale = 1.0
		imageScrollView.maximumZoomScale = 10.0

		self.view.addSubview(imageScrollView)
		
		imageViewViewer.contentMode = .scaleAspectFit
		imageScrollView.addSubview(imageViewViewer)
		
		self.view.bringSubviewToFront(topBar)
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.view.backgroundColor = VoipTheme.voipBackgroundBWColor.get()
			self.imageViewViewer.frame = CGRect(x: 0, y: 0, width: UIScreen.main.bounds.size.width, height: UIScreen.main.bounds.size.height-32.0)
		}
		 
	}
	
	override func viewDidAppear(_ animated: Bool) {
		imageViewViewer.image = imageViewer
	}
	
	@IBAction func shareTextButton(_ sender: UIButton) {
		/*
		// text to share
		let text = textViewer
		
		// set up activity view controller
		let textToShare = [ text ]
		let activityViewController = UIActivityViewController(activityItems: textToShare, applicationActivities: nil)
		activityViewController.popoverPresentationController?.sourceView = self.view // so that iPads won't crash
		
		// exclude some activity types from the list (optional)
		activityViewController.excludedActivityTypes = [ UIActivity.ActivityType.airDrop, UIActivity.ActivityType.postToFacebook ]
		
		// present the view controller
		self.present(activityViewController, animated: true, completion: nil)
		*/
	}
	
	func viewForZooming(in scrollView: UIScrollView) -> UIView? {
		return self.imageViewViewer
	}
}
