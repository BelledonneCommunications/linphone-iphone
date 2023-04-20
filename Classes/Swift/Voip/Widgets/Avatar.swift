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
import linphonesw
import SnapKit

class Avatar : UIView {
	
	static let diameter_for_call_views = 191
	static let diameter_for_call_views_land = 130
	static let groupAvatar = UIImage(named:"voip_multiple_contacts_avatar")
	static let singleAvatar = UIImage(named:"avatar")
	
	var friend: [Friend] = []
	var friendDelegate: [FriendDelegate] = []
	
	required init?(coder: NSCoder) {
		initialsLabel =  StyledLabel(VoipTheme.call_generated_avatar_large)
		super.init(coder: coder)
	}
	
	let initialsLabel: StyledLabel
	let iconImageView = UIImageView()

	init (color:LightDarkColor,textStyle:TextStyle) {
		initialsLabel =  StyledLabel(textStyle)
		super.init(frame: .zero)
		clipsToBounds = true
		self.backgroundColor = color.get()
		addSubview(initialsLabel)
		addSubview(iconImageView)
		iconImageView.backgroundColor = .white
		initialsLabel.matchParentSideBorders().matchParentHeight().done()
		iconImageView.matchParentDimmensions().done()
		UIDeviceBridge.displayModeSwitched.observe { _ in
			self.initialsLabel.applyStyle(textStyle)
			self.backgroundColor = color.get()
		}
	}
	
	
	func fillFromAddress(address:Address, isGroup:Bool = false) {
		if (isGroup) {
			iconImageView.image = Avatar.groupAvatar
			iconImageView.isHidden = false
			initialsLabel.isHidden = true
		} else if let image = address.contact()?.avatar() {
			iconImageView.image = image
			initialsLabel.isHidden = true
			iconImageView.isHidden = false
		} else {
			if (Core.get().defaultAccount?.isPhoneNumber(username: address.username) == true) {
				iconImageView.image = Avatar.singleAvatar
				initialsLabel.isHidden = true
				iconImageView.isHidden = false
			} else {
				initialsLabel.text = address.initials()
				initialsLabel.isHidden = false
				iconImageView.isHidden = true
			}
		}
	}
	
	func showAsAvatarIcon() {
		iconImageView.image = Avatar.singleAvatar
		initialsLabel.isHidden = true
		iconImageView.isHidden = false
	}
		
	override func layoutSubviews() {
		super.layoutSubviews()
		layer.cornerRadius = self.frame.width / 2.0
	}
	
	func addDelegate(contactAddress: Contact){
		var delegatePresence = false
		
		friend.forEach { friendForEach in
			if friendForEach.address?.asStringUriOnly() == Friend.getSwiftObject(cObject: (contactAddress.friend)!).address?.asStringUriOnly() {
				delegatePresence = true
			}
		}
		if delegatePresence == false {
			friend.append(Friend.getSwiftObject(cObject: (contactAddress.friend)!))
			let newFriendDelegate = FriendDelegateStub(
				onPresenceReceived: { (linphoneFriend: Friend) -> Void in
					
					let presenceModel = linphoneFriend.getPresenceModelForUriOrTel(uriOrTel: (linphoneFriend.address?.asStringUriOnly())!)
					NotificationCenter.default.post(name: Notification.Name("LinphoneFriendPresenceUpdate"), object: nil, userInfo: ["friend": linphoneFriend.address?.asStringUriOnly() ?? "", "isOnline": presenceModel!.consolidatedPresence.rawValue == LinphoneConsolidatedPresenceOnline.rawValue])
				}
			)
			friendDelegate.append(newFriendDelegate)
			friend.last?.addDelegate(delegate: friendDelegate.last!)
		}
	}
}


@objc class AvatarBridge : NSObject { // Ugly work around to tap into the swift Avatars, until rest of the app is reworked in Swift.
	static var shared : Avatar? = nil
	static let size = 50.0
	
	@objc static func prepareIt() {
		if (shared != nil) {
			shared?.removeFromSuperview()
		}
		shared = Avatar(color:VoipTheme.primaryTextColor, textStyle: VoipTheme.call_generated_avatar_small)
		PhoneMainView.instance().mainViewController.view.addSubview(shared!)
		PhoneMainView.instance().mainViewController.view.sendSubviewToBack(shared!)
		shared?.bounds.size = CGSize(width: size, height: size)
	}
	
	@objc static func imageForAddress(address:OpaquePointer) -> UIImage? {
		if (shared == nil) {
			prepareIt()
		}
		let sAddr = Address.getSwiftObject(cObject: address)
		shared?.fillFromAddress(address: sAddr)
        
        let avatarWithPresence = UIView(frame: CGRect(x: 0, y: 0, width: size, height: size))
        let avatarImageWihtoutPresence = UIImageView(image: shared?.toImage())
        let contactAddress = Address.getSwiftObject(cObject: address).contact()
        var iconPresenceView = UIImageView()
        if (contactAddress != nil) {
            
			
			shared?.addDelegate(contactAddress: contactAddress!)
			
			iconPresenceView = updatePresenceImage(contact: contactAddress!)
			
			avatarWithPresence.addSubview(avatarImageWihtoutPresence)
			avatarWithPresence.addSubview(iconPresenceView)
			iconPresenceView.frame = CGRect(x: 35, y: 35, width: 16, height: 16)
        }else{
            avatarWithPresence.addSubview(avatarImageWihtoutPresence)
        }
        return avatarWithPresence.toImage()
	}
	
    @objc static func imageForInitials(contact:Contact, displayName:String) -> UIImage? {
		if (shared == nil) {
			prepareIt()
		}
		if (displayName.replacingOccurrences(of: " ", with: "").count == 0) {
			return Avatar.singleAvatar
		}
		shared?.initialsLabel.text = Address.initials(displayName: displayName)
		shared?.initialsLabel.isHidden = false
		shared?.iconImageView.isHidden = true
        
        let avatarWithPresence = UIView(frame: CGRect(x: 0, y: 0, width: size, height: size))
        let avatarImageWihtoutPresence = UIImageView(image: shared?.toImage())
		
		avatarWithPresence.addSubview(avatarImageWihtoutPresence)
		
        return avatarWithPresence.toImage()
	}
	
    @objc static func updatePresenceImage(contact:Contact) -> UIImageView {

        let friend = Friend.getSwiftObject(cObject: contact.friend)
        
        var presenceModel : PresenceModel?
        var hasPresence : Bool? = false
        
        var imageName = "";
        
        if friend.address?.asStringUriOnly() != nil {
            presenceModel = friend.getPresenceModelForUriOrTel(uriOrTel: (friend.address?.asStringUriOnly())!)
            hasPresence = presenceModel != nil && presenceModel!.basicStatus == PresenceBasicStatus.Open
        }
		
        if (hasPresence! && presenceModel?.consolidatedPresence == ConsolidatedPresence.Online) {
            imageName = "led_connected";
        } else  if (hasPresence! && presenceModel?.consolidatedPresence == ConsolidatedPresence.Busy){
            imageName = "led_inprogress";
		} else {
			imageName = "";
		}

        return UIImageView(image: UIImage(named:imageName))
    }
	
	@objc static func clearFriends(){
		if shared != nil {
			shared!.friend = []
			shared!.friendDelegate = []
		}
	}
}
