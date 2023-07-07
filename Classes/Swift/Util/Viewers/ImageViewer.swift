//
//  ImageViewer.swift
//  linphone
//
//  Created by Benoît Martins on 21/06/2023.
//

import Foundation

@objc class ImageViewer:  BackNextNavigationView, UICompositeViewDelegate, UIScrollViewDelegate, QLPreviewControllerDelegate, QLPreviewControllerDataSource {
	static let compositeDescription = UICompositeViewDescription(ImageViewer.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	@objc var imageNameViewer = ""
	@objc var imagePathViewer = ""
	@objc var imageViewer = UIImage()
	var newImageView = UIImageView()
	let imageViewViewer = UIImageView()
	let imageScrollView = UIScrollView()
	var previewItems : [QLPreviewItem?] = []
	
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
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.view.backgroundColor = VoipTheme.voipBackgroundBWColor.get()
		}
		 
	}
	
	override func viewWillAppear(_ animated: Bool) {
		let vWidth = self.view.bounds.size.width
		let vHeight = self.view.bounds.size.height-66
		
		
		newImageView.removeFromSuperview()
		imageViewViewer.removeFromSuperview()
		imageScrollView.removeFromSuperview()

		imageScrollView.delegate = self
		imageScrollView.frame = CGRectMake(0, 66, vWidth, vHeight)
		imageScrollView.showsVerticalScrollIndicator = true

		imageScrollView.minimumZoomScale = 1.0
		imageScrollView.maximumZoomScale = 10.0

		self.view.addSubview(imageScrollView)
		
		imageViewViewer.contentMode = .scaleAspectFit
		imageScrollView.addSubview(imageViewViewer)
		self.imageViewViewer.frame = CGRect(x: 0, y: 0, width: vWidth, height: vHeight)
		self.view.bringSubviewToFront(topBar)
		
		let pictureTap = UITapGestureRecognizer(target: self, action: #selector(imageTapped))
		imageViewViewer.addGestureRecognizer(pictureTap)
		imageViewViewer.isUserInteractionEnabled = true
		
		imageViewViewer.image = imageViewer
		titleLabel.text = imageNameViewer
	}
	
	override func didRotate(from fromInterfaceOrientation: UIInterfaceOrientation) {
		dismissFullscreenImageRotated()
		self.viewWillAppear(true)
	}
	
	@IBAction func imageTapped(_ sender: UITapGestureRecognizer) {
		let imageView = sender.view as! UIImageView
		newImageView = UIImageView(image: imageView.image)
		newImageView.frame = UIScreen.main.bounds
		newImageView.frame = CGRectMake(0, 0, UIScreen.main.bounds.size.width, UIScreen.main.bounds.size.height-20)
		newImageView.backgroundColor = .black
		newImageView.contentMode = .scaleAspectFit
		newImageView.isUserInteractionEnabled = true
		let tap = UITapGestureRecognizer(target: self, action: #selector(dismissFullscreenImage))
		newImageView.addGestureRecognizer(tap)
		self.view.addSubview(newImageView)
		self.navigationController?.isNavigationBarHidden = true
		self.tabBarController?.tabBar.isHidden = true
		PhoneMainView.instance().hideStatusBar(true)
	}
	
	func dismissFullscreenImageRotated() {
		self.navigationController?.isNavigationBarHidden = false
		self.tabBarController?.tabBar.isHidden = false
		PhoneMainView.instance().hideStatusBar(false)
	}

	@objc func dismissFullscreenImage(_ sender: UITapGestureRecognizer) {
		self.navigationController?.isNavigationBarHidden = false
		self.tabBarController?.tabBar.isHidden = false
		PhoneMainView.instance().hideStatusBar(false)
		sender.view?.removeFromSuperview()
	}
	
	func viewForZooming(in scrollView: UIScrollView) -> UIView? {
		return self.imageViewViewer
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
		
		
		
		let previewController = QLPreviewController()
		self.previewItems = []
		
		self.previewItems.append(self.getPreviewItem(filePath: imagePathViewer))
		
		previewController.dataSource = self
		previewController.delegate = self
		PhoneMainView.instance().mainViewController.present(previewController, animated: true, completion: nil)
		
	}
	
	func numberOfPreviewItems(in controller: QLPreviewController) -> Int {
		return previewItems.count
	}
	
	func previewController(_ controller: QLPreviewController, previewItemAt index: Int) -> QLPreviewItem {
		return (previewItems[index] as QLPreviewItem?)!
	}
	
	func getPreviewItem(filePath: String) -> NSURL{
		let url = NSURL(fileURLWithPath: filePath)
		return url
	}
}
