/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

import Foundation

class TextViewer: BackNextNavigationView, UICompositeViewDelegate  {
	
	static let compositeDescription = UICompositeViewDescription(TextViewer.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	@objc var textViewer = ""
	@objc var textNameViewer = ""
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

		shareButton.addTarget(self, action: #selector(shareMediaButton), for: .touchUpInside)
		
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
		titleLabel.text = textNameViewer
	}
	
	@objc func shareMediaButton(_ sender: UIButton) {
		let text = textViewer
		
		let textToShare = [ text ]
		let activityViewController = UIActivityViewController(activityItems: textToShare, applicationActivities: nil)
		activityViewController.popoverPresentationController?.sourceView = self.view // so that iPads won't crash
		
		activityViewController.excludedActivityTypes = [ UIActivity.ActivityType.airDrop, UIActivity.ActivityType.postToFacebook ]
		
		self.present(activityViewController, animated: true, completion: nil)
		
	}
}
