/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
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

import SwiftUI
import linphonesw

struct Avatar: View {
    var friend: Friend
    let avatarSize: CGFloat
    
    @State private var friendDelegate: FriendDelegate?
    @State private var presenceImage = ""
    
    var body: some View {
        AsyncImage(url: ContactsManager.shared.getImagePath(friendPhotoPath: friend.photo!)) { image in
            switch image {
            case .empty:
                ProgressView()
                    .frame(width: avatarSize, height: avatarSize)
            case .success(let image):
                ZStack {
                    image
                        .resizable()
                        .aspectRatio(contentMode: .fill)
                        .frame(width: avatarSize, height: avatarSize)
                        .clipShape(Circle())
                    HStack {
                        Spacer()
                        VStack {
                            Spacer()
							if !friend.addresses.isEmpty {
								if presenceImage.isEmpty && (friend.consolidatedPresence == ConsolidatedPresence.Online || friend.consolidatedPresence == ConsolidatedPresence.Busy) {
									Image(friend.consolidatedPresence == ConsolidatedPresence.Online ? "presence-online" : "presence-busy")
										.resizable()
										.frame(width: avatarSize/4, height: avatarSize/4)
										.padding(.trailing, avatarSize == 45 ? 1 : 3)
										.padding(.bottom, avatarSize == 45 ? 1 : 3)
								} else if !presenceImage.isEmpty && (friend.consolidatedPresence != ConsolidatedPresence.DoNotDisturb || friend.consolidatedPresence != ConsolidatedPresence.Offline) {
									Image(presenceImage)
										.resizable()
										.frame(width: avatarSize/4, height: avatarSize/4)
										.padding(.trailing, avatarSize == 45 ? 1 : 3)
										.padding(.bottom, avatarSize == 45 ? 1 : 3)
								}
                            }
                        }
                    }
                    .frame(width: avatarSize, height: avatarSize)
                }
            case .failure:
                Image("profil-picture-default")
                    .resizable()
                    .frame(width: avatarSize, height: avatarSize)
                    .clipShape(Circle())
            @unknown default:
                EmptyView()
            }
        }
		.onAppear {
			addDelegate()
		}
		.onDisappear {
			removeAllDelegate()
		}
    }
    
    func addDelegate() {
        if friend.address != nil {
            print("Presence received for first \(friend.name!) \(friend.consolidatedPresence)")
			//self.presenceImage = friend.consolidatedPresence == ConsolidatedPresence.Online ? "presence-online" : "presence-busy"
        }
        
        let newFriendDelegate = FriendDelegateStub(
            onPresenceReceived: { (linphoneFriend: Friend) -> Void in
				print("Presence received for second \(linphoneFriend.name) \(linphoneFriend.consolidatedPresence)")
				
				/*
                if linphoneFriend.address != nil && friend.address != nil
                    && linphoneFriend.address!.asStringUriOnly() == friend.address!.asStringUriOnly() {
                    let presenceModel = linphoneFriend.getPresenceModelForUriOrTel(uriOrTel: (linphoneFriend.address!.asStringUriOnly()))
                    if presenceModel != nil {
                        presenceImage = presenceModel!.consolidatedPresence == ConsolidatedPresence.Online ? "presence-online" : "presence-busy"
                    }
                }
				 */
            }
        )
        
        friendDelegate = newFriendDelegate
        if friendDelegate != nil {
            friend.addDelegate(delegate: friendDelegate!)
        }
    }
    
	func removeAllDelegate(){
		if friendDelegate != nil {
			presenceImage = ""
			friend.removeDelegate(delegate: friendDelegate!)
			friendDelegate = nil
		}
	}
}
