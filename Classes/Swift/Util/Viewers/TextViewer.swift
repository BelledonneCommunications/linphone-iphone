//
//  TextViewer.swift
//  linphone
//
//  Created by Benoît Martins on 03/05/2023.
//

import Foundation

@objc class TextViewer:  BackNextNavigationView, UICompositeViewDelegate {

	static let compositeDescription = UICompositeViewDescription(TextViewer.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	@objc var textViewer = ""
	let textViewViewer = UITextView()
	
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
		
		super.backButton.isHidden = UIDevice.ipad()
		
		textViewViewer.isScrollEnabled = true
		textViewViewer.isUserInteractionEnabled = true
		textViewViewer.frame = CGRect(x: 0, y: top_bar_height, width: UIScreen.main.bounds.size.width, height: UIScreen.main.bounds.size.height-top_bar_height*2-32.0)

		self.view.addSubview(textViewViewer)
		
		UIDeviceBridge.displayModeSwitched.readCurrentAndObserve { _ in
			self.view.backgroundColor = VoipTheme.voipBackgroundBWColor.get()
			self.textViewViewer.frame = CGRect(x: 0, y: self.top_bar_height, width: UIScreen.main.bounds.size.width, height: UIScreen.main.bounds.size.height - (self.top_bar_height * 2) - 32.0)
		}
		 
	}
	
	override func viewDidAppear(_ animated: Bool) {
		textViewViewer.text = textViewer
	}
	
	@IBAction func shareTextButton(_ sender: UIButton) {
			
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
			
		}
}
