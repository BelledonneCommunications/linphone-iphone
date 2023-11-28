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
	
	@ObservedObject var contactAvatarModel: ContactAvatarModel
	let avatarSize: CGFloat
	
	var body: some View {
        AsyncImage(url: ContactsManager.shared.getImagePath(friendPhotoPath: contactAvatarModel.friend!.photo!)) { image in
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
							if contactAvatarModel.presenceStatus == .Online ||	contactAvatarModel.presenceStatus == .Busy {
								Image(contactAvatarModel.presenceStatus == .Online ? "presence-online" : "presence-busy")
									.resizable()
									.frame(width: avatarSize/4, height: avatarSize/4)
									.padding(.trailing, avatarSize == 45 ? 1 : 3)
									.padding(.bottom, avatarSize == 45 ? 1 : 3)
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
	}
}
