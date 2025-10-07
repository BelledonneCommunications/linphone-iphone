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
	
	private var contactsManager = ContactsManager.shared
	
	@ObservedObject var contactAvatarModel: ContactAvatarModel
	
	let avatarSize: CGFloat
    let hidePresence: Bool
    
    init(contactAvatarModel: ContactAvatarModel, avatarSize: CGFloat, hidePresence: Bool = false) {
        self.contactAvatarModel = contactAvatarModel
        self.avatarSize = avatarSize
        self.hidePresence = hidePresence
    }
	
	var body: some View {
		if !contactAvatarModel.photo.isEmpty {
			let uniqueUrl = ContactsManager.shared.getImagePath(friendPhotoPath: contactAvatarModel.photo)
			//let finalUrl = uniqueUrl.appendingQueryItem("v", value: UUID().uuidString)

			AsyncImage(url: uniqueUrl) { image in
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
								if !hidePresence && (contactAvatarModel.presenceStatus == .Online || contactAvatarModel.presenceStatus == .Busy) {
									Image(contactAvatarModel.presenceStatus == .Online ? "presence-online" : "presence-busy")
										.resizable()
										.frame(width: avatarSize/4, height: avatarSize/4)
										.padding(.trailing, avatarSize == 50 || avatarSize == 35 ? 1 : 3)
										.padding(.bottom, avatarSize == 50 || avatarSize == 35 ? 1 : 3)
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
		} else if !contactAvatarModel.name.isEmpty {
			ZStack {
				Image(uiImage: contactsManager.textToImage(
					firstName: contactAvatarModel.name,
					lastName: contactAvatarModel.name.components(separatedBy: " ").count > 1
					? contactAvatarModel.name.components(separatedBy: " ")[1]
					: ""))
				.resizable()
				.frame(width: avatarSize, height: avatarSize)
				.clipShape(Circle())
				
				HStack {
					Spacer()
					VStack {
						Spacer()
						if !hidePresence && (contactAvatarModel.presenceStatus == .Online || contactAvatarModel.presenceStatus == .Busy) {
							Image(contactAvatarModel.presenceStatus == .Online ? "presence-online" : "presence-busy")
								.resizable()
								.frame(width: avatarSize/4, height: avatarSize/4)
								.padding(.trailing, avatarSize == 50 || avatarSize == 35 ? 1 : 3)
								.padding(.bottom, avatarSize == 50 || avatarSize == 35 ? 1 : 3)
						}
					}
				}
				.frame(width: avatarSize, height: avatarSize)
			}
		} else {
			Image("profil-picture-default")
				.resizable()
				.frame(width: avatarSize, height: avatarSize)
				.clipShape(Circle())
		}
	}
}
