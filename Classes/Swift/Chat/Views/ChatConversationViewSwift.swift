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


import UIKit
import Foundation
import linphonesw
import DropDown

@objc class ChatConversationViewSwift: BackActionsNavigationView, UICompositeViewDelegate { // Replaces ChatConversationView
	
	let controlsView = ControlsView(showVideo: true, controlsViewModel: ChatConversationViewModel.sharedModel)
    
    static let compositeDescription = UICompositeViewDescription(ChatConversationViewSwift.self, statusBar: StatusBarView.self, tabBar: nil, sideMenu: SideMenuView.self, fullscreen: false, isLeftFragment: false,fragmentWith: nil)
	
    static func compositeViewDescription() -> UICompositeViewDescription! { return compositeDescription }
	
    func compositeViewDescription() -> UICompositeViewDescription! { return type(of: self).compositeDescription }
	
	let menu: DropDown = {
		let menu = DropDown()
		menu.dataSource = [
			"Item 1",
			"Item 2",
			"Item 3",
			"Item 4",
			"Item 5"
		]
		menu.cellNib = UINib(nibName: "DropDownCell", bundle: nil)
		menu.customCellConfiguration = { index, title, cell in
			guard let cell = cell as? MyCell else {
				return
			}
			cell.myImageView.image = UIImage(named: "security_2_indicator.png")
		}
		return menu
	}()
	
    override func viewDidLoad() {
        super.viewDidLoad(
            backAction: {
                self.goBackChatListView()
            },
            action1: {
				
            },
            action2: {
				self.tapChooseMenuItem(self.action2Button)
            },
            title:"benoit.martins.test1"
			//title:"Coin à champis de François"
        )
        //view.backgroundColor = VoipTheme.backgroundColor3.get()
    }
    
    func goBackChatListView() {
        PhoneMainView.instance().pop(toView: ChatsListView.compositeViewDescription())
    }
	
	func tapChooseMenuItem(_ sender: UIButton) {
		menu.anchorView = sender
		menu.bottomOffset = CGPoint(x: -UIScreen.main.bounds.width * 0.6, y: sender.frame.size.height)
		menu.show()
		
		menu.selectionAction = { index, title in
			print("index \(index) and \(title)")
		}
  	}
}
