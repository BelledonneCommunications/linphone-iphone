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

struct SipAddressesPopup: View {
	
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@EnvironmentObject var contactAvatarModel: ContactAvatarModel
	@EnvironmentObject var contactsListViewModel: ContactsListViewModel
	
	@Binding var isShowSipAddressesPopup: Bool
	@Binding var isShowSipAddressesPopupType: Int
	
    var body: some View {
		GeometryReader { geometry in
			VStack(alignment: .leading) {
				HStack {
					Text("contact_dialog_pick_phone_number_or_sip_address_title")
						.default_text_style_800(styleSize: 16)
						.padding(.bottom, 2)
					
					Spacer()
					
					Image("x")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.grayMain2c600)
						.frame(width: 25, height: 25)
						.padding(.all, 10)
				}
				.frame(maxWidth: .infinity)
				
				ForEach(0..<contactAvatarModel.addresses.count, id: \.self) { index in
					HStack {
						HStack {
							VStack {
								Text(String(localized: "sip_address") + ":")
									.default_text_style_700(styleSize: 14)
									.frame(maxWidth: .infinity, alignment: .leading)
								Text(contactAvatarModel.addresses[index].dropFirst(4))
									.default_text_style(styleSize: 14)
									.frame(maxWidth: .infinity, alignment: .leading)
									.lineLimit(1)
									.fixedSize(horizontal: false, vertical: true)
							}
							Spacer()
						}
						.padding(.vertical, 15)
						.padding(.horizontal, 10)
					}
					.background(.white)
					.onTapGesture {
						do {
							let address = try Factory.Instance.createAddress(addr: contactAvatarModel.addresses[index])
							if isShowSipAddressesPopupType != 1 {
								withAnimation {
									isShowSipAddressesPopup = false
									telecomManager.doCallOrJoinConf(address: address, isVideo: isShowSipAddressesPopupType == 2)
									isShowSipAddressesPopupType = 0
								}
							} else {
								withAnimation {
									isShowSipAddressesPopup = false
									contactsListViewModel.createOneToOneChatRoomWith(remote: address)
									isShowSipAddressesPopupType = 0
								}
							}
						} catch {
							Log.error("[ContactInnerActionsFragment] unable to create address for a new outgoing call : \(contactAvatarModel.addresses[index]) \(error) ")
						}
					}
				}
			}
			.padding(.horizontal, 20)
			.padding(.vertical, 20)
			.background(.white)
			.cornerRadius(20)
			.frame(maxHeight: .infinity)
			.shadow(color: Color.orangeMain500, radius: 0, x: 0, y: 2)
			.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
			.position(x: geometry.size.width / 2, y: geometry.size.height / 2)
		}
    }
}

#Preview {
	SipAddressesPopup(
		isShowSipAddressesPopup: .constant(true),
		isShowSipAddressesPopupType: .constant(0)
	)
}
