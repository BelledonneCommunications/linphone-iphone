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
	@ObservedObject var contactsManager = ContactsManager.shared
	
	@ObservedObject var model: AccountModel
	@EnvironmentObject var accountProfileViewModel: AccountProfileViewModel
	
	@State private var navigateToOption = false
	@Binding var isOpen: Bool
	@Binding var isShowAccountProfileFragment: Bool
	
	private let avatarSize = 45.0
	
	var body: some View {
		HStack {
			AsyncImage(url: model.imagePathAvatar) { image in
				switch image {
				case .empty:
					ProgressView()
						.frame(width: avatarSize, height: avatarSize)
				case .success(let image):
					image
						.resizable()
						.aspectRatio(contentMode: .fill)
						.frame(width: avatarSize, height: avatarSize)
						.clipShape(Circle())
				case .failure:
					Image(uiImage: contactsManager.textToImage(
						firstName: model.avatarModel?.name ?? "",
						lastName: ""))
					.resizable()
					.frame(width: avatarSize, height: avatarSize)
					.clipShape(Circle())
				@unknown default:
					EmptyView()
				}
			}
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
						.onChange(of: model.registrationStateAssociatedUIColor) { _ in
							accountProfileViewModel.accountError = CoreContext.shared.accounts.contains {
								($0.registrationState == .Cleared && $0.isDefaultAccount) ||
								$0.registrationState == .Failed
							}
						}
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
				if model.notificationsCount > 0 && !CorePreferences.disableChatFeature {
					Text(String(model.notificationsCount))
						.foregroundStyle(.white)
						.default_text_style(styleSize: 12)
						.lineLimit(1)
						.frame(width: 20, height: 20)
						.background(Color.redDanger500)
						.cornerRadius(50)
						.frame(maxWidth: .infinity, alignment: .leading)
				}
				
				Menu {
					Button {
                        accountProfileViewModel.accountModelIndex = CoreContext.shared.accounts.firstIndex(where: {$0.displayName == model.displayName})
                        withAnimation {
                            isOpen = false
                            isShowAccountProfileFragment = true
                        }
					} label: {
						Label("drawer_menu_manage_account", systemImage: "arrow.right.circle")
					}
				} label: {
					Image("dots-three-vertical")
						.renderingMode(.template)
						.resizable()
						.foregroundColor(Color.gray)
						.scaledToFit()
						.frame(height: 30)
				}
			}
			.frame(width: 64, alignment: .trailing)
			.padding(.top, 12)
			.padding(.bottom, 12)
		}
		.frame(height: 61)
		.padding(.horizontal, 16)
		.background(model.isDefaultAccount ? Color.grayMain2c100 : .white)
		.onTapGesture {
			model.setAsDefault()
		}
	}
}
