/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
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
import UniformTypeIdentifiers

struct SideMenuAccountRow: View {
	@ObservedObject var model: AccountModel
	var body: some View {
		HStack {
			
			Avatar(contactAvatarModel:
					ContactAvatarModel(friend: nil,
									   name: model.displayName,
									   address: model.address,
									   withPresence: true),
				   avatarSize: 45)
			.padding(.leading, 6)
			
			VStack {
				Text(model.displayName)
					.default_text_style_grey_400(styleSize: 14)
					.lineLimit(1)
					.frame(maxWidth: .infinity, alignment: .leading)
				
				VStack {
					Text(model.humanReadableRegistrationState)
						.default_text_style_uncolored(styleSize: 12)
						.foregroundStyle(model.registrationStateAssociatedUIColor)
				}
				.padding(EdgeInsets(top: 4, leading: 8, bottom: 4, trailing: 8))
				.background(Color.grayMain2c200)
				.cornerRadius(12)
				.frame(height: 20)
				.frame(maxWidth: .infinity, alignment: .leading)
				.onTapGesture {
					model.refreshRegiter()
				}
			}
			.padding(.leading, 4)
			
			Spacer()
			
			HStack {
				if model.notificationsCount > 0 {
					Text(String(model.notificationsCount))
						.foregroundStyle(.white)
						.default_text_style(styleSize: 12)
						.lineLimit(1)
						.frame(width: 20, height: 20)
						.background(Color.redDanger500)
						.cornerRadius(50)
						.frame(maxWidth: .infinity, alignment: .leading)
				}
				Image("dots-three-vertical")
					.renderingMode(.template)
					.resizable()
					.foregroundStyle(Color.grayMain2c600)
					.scaledToFit()
					.frame(height: 30)
			}
			.frame(width: 64, alignment: .trailing)
			.padding(.top, 12)
			.padding(.bottom, 12)
		}
		.frame(height: 61)
		.background(model.isDefaultAccount ? Color.grayMain2c100 : .clear)
	}
}
