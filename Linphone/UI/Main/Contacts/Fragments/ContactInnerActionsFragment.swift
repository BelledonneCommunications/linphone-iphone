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

// swiftlint:disable type_body_length
struct ContactInnerActionsFragment: View {
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@ObservedObject var contactViewModel: ContactViewModel
	@ObservedObject var editContactViewModel: EditContactViewModel
	@ObservedObject var contactAvatarModel: ContactAvatarModel
	
	@State private var informationIsOpen = true
	
	@Binding var showingSheet: Bool
	@Binding var showShareSheet: Bool
	@Binding var isShowDeletePopup: Bool
	@Binding var isShowDismissPopup: Bool
	
	var actionEditButton: () -> Void
	
    var body: some View {
		HStack(alignment: .center) {
			Text("contact_details_numbers_and_addresses_title")
				.default_text_style_800(styleSize: 15)
			
			Spacer()
			
			Image(informationIsOpen ? "caret-up" : "caret-down")
				.renderingMode(.template)
				.resizable()
				.foregroundStyle(Color.grayMain2c600)
				.frame(width: 25, height: 25, alignment: .leading)
				.padding(.all, 10)
		}
		.padding(.top, 30)
		.padding(.bottom, 10)
		.padding(.horizontal, 16)
		.background(Color.gray100)
		.onTapGesture {
			withAnimation {
				informationIsOpen.toggle()
			}
		}
		
		if informationIsOpen {
			VStack(spacing: 0) {
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
							
							Image("phone")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c600)
								.frame(width: 25, height: 25)
								.padding(.all, 10)
						}
						.padding(.vertical, 15)
						.padding(.horizontal, 20)
					}
					.background(.white)
					.onTapGesture {
						do {
							let address = try Factory.Instance.createAddress(addr: contactAvatarModel.addresses[index])
							withAnimation {
								telecomManager.doCallOrJoinConf(address: address)
							}
						} catch {
							Log.error("[ContactInnerActionsFragment] unable to create address for a new outgoing call : \(contactAvatarModel.addresses[index]) \(error) ")
						}
					}
					.onLongPressGesture(minimumDuration: 0.2) {
						contactViewModel.stringToCopy = contactAvatarModel.addresses[index]
						showingSheet.toggle()
					}
					
					if contactAvatarModel.friend != nil && !contactAvatarModel.friend!.phoneNumbers.isEmpty
						|| index < contactAvatarModel.addresses.count - 1 {
						VStack {
							Divider()
						}
						.padding(.horizontal)
					}
				}
				if contactAvatarModel.friend != nil {
					ForEach(0..<contactAvatarModel.friend!.phoneNumbers.count, id: \.self) { index in
					HStack {
						HStack {
							VStack {
								if contactAvatarModel.friend!.phoneNumbersWithLabel[index].label != nil
									&& !contactAvatarModel.friend!.phoneNumbersWithLabel[index].label!.isEmpty {
									Text("Phone (\(contactAvatarModel.friend!.phoneNumbersWithLabel[index].label!)) :")
										.default_text_style_700(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
								} else {
									Text("Phone :")
										.default_text_style_700(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
								}
								Text(contactAvatarModel.friend!.phoneNumbersWithLabel[index].phoneNumber)
									.default_text_style(styleSize: 14)
									.frame(maxWidth: .infinity, alignment: .leading)
									.lineLimit(1)
									.fixedSize(horizontal: false, vertical: true)
							}
							Spacer()
						}
						.padding(.vertical, 15)
						.padding(.horizontal, 20)
					}
					.background(.white)
					.onLongPressGesture(minimumDuration: 0.2) {
						contactViewModel.stringToCopy =
						contactAvatarModel.friend!.phoneNumbersWithLabel[index].phoneNumber
						showingSheet.toggle()
					}
					
					if index < contactAvatarModel.friend!.phoneNumbers.count - 1 {
						VStack {
							Divider()
						}
						.padding(.horizontal)
					}
				}
			}
		}
			.background(.white)
			.cornerRadius(15)
			.padding(.horizontal)
			.zIndex(-1)
			.transition(.move(edge: .top))
	}
		
		if contactAvatarModel.friend != nil && (contactAvatarModel.friend!.organization != nil
				 && !contactAvatarModel.friend!.organization!.isEmpty)
				|| (contactAvatarModel.friend!.jobTitle != nil
					&& !contactAvatarModel.friend!.jobTitle!.isEmpty) {
			VStack {
				if contactAvatarModel.friend!.organization != nil
					&& !contactAvatarModel.friend!.organization!.isEmpty {
					Text("**Company :** \(contactAvatarModel.friend!.organization!)")
						.default_text_style(styleSize: 14)
						.padding(.vertical, 15)
						.padding(.horizontal, 20)
						.frame(maxWidth: .infinity, alignment: .leading)
				}
				
				if contactAvatarModel.friend!.jobTitle != nil
					&& !contactAvatarModel.friend!.jobTitle!.isEmpty {
					Text("**Job :** \(contactAvatarModel.friend!.jobTitle!)")
						.default_text_style(styleSize: 14)
						.padding(.top, 
								 contactAvatarModel.friend!.organization != nil
								 && !contactAvatarModel.friend!.organization!.isEmpty
								 ? 0 : 15
						)
						.padding(.bottom, 15)
						.padding(.horizontal, 20)
						.frame(maxWidth: .infinity, alignment: .leading)
				}
			}
			.background(.white)
			.cornerRadius(15)
			.padding(.top)
			.padding(.horizontal)
			.zIndex(-1)
			.transition(.move(edge: .top))
		}
		
		// TODO Trust Fragment
		
		// TODO Medias Fragment
		
		HStack(alignment: .center) {
			Text("contact_details_actions_title")
				.default_text_style_800(styleSize: 16)
			
			Spacer()
		}
		.padding(.vertical, 10)
		.padding(.horizontal, 16)
		.background(Color.gray100)
		
		VStack(spacing: 0) {
			if !contactAvatarModel.nativeUri.isEmpty {
				Button {
					actionEditButton()
				} label: {
					HStack {
						Image("pencil-simple")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c600)
							.frame(width: 25, height: 25)
							.padding(.all, 10)
						
						Text("contact_details_edit")
							.default_text_style(styleSize: 14)
							.frame(maxWidth: .infinity, alignment: .leading)
							.lineLimit(1)
							.fixedSize(horizontal: false, vertical: true)
						Spacer()
					}
					.padding(.vertical, 15)
					.padding(.horizontal, 20)
				}
			} else {
				NavigationLink(destination: EditContactFragment(
						editContactViewModel: editContactViewModel,
						contactViewModel: contactViewModel,
						isShowEditContactFragment: .constant(false),
						isShowDismissPopup: $isShowDismissPopup)) {
						HStack {
							Image("pencil-simple")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c600)
								.frame(width: 25, height: 25)
								.padding(.all, 10)
							
							Text("contact_details_edit")
								.default_text_style(styleSize: 14)
								.frame(maxWidth: .infinity, alignment: .leading)
								.lineLimit(1)
								.fixedSize(horizontal: false, vertical: true)
							Spacer()
						}
						.padding(.vertical, 15)
						.padding(.horizontal, 20)
				}
				.simultaneousGesture(
					TapGesture().onEnded {
						editContactViewModel.selectedEditFriend = contactAvatarModel.friend!
						editContactViewModel.resetValues()
					}
				)
			}
			
			VStack {
				Divider()
			}
			.padding(.horizontal)
			
			Button {
				if contactAvatarModel.friend != nil {
					contactAvatarModel.friend!.edit()
					contactAvatarModel.friend!.starred.toggle()
					contactAvatarModel.friend!.done()
				}
			} label: {
				HStack {
					Image(contactAvatarModel.friend != nil && contactAvatarModel.friend!.starred == true ? "heart-fill" : "heart")
					.renderingMode(.template)
					.resizable()
					.foregroundStyle(contactAvatarModel.friend != nil && contactAvatarModel.friend!.starred == true ? Color.redDanger500 : Color.grayMain2c500)
					.frame(width: 25, height: 25)
					.padding(.all, 10)
					Text(contactAvatarModel.friend != nil && contactAvatarModel.friend!.starred == true
						 ? "contact_details_remove_from_favourites"
						 : "contact_details_add_to_favourites")
					.default_text_style(styleSize: 14)
					.frame(maxWidth: .infinity, alignment: .leading)
					.lineLimit(1)
					.fixedSize(horizontal: false, vertical: true)
					Spacer()
				}
				.padding(.vertical, 15)
				.padding(.horizontal, 20)
			}
			
			VStack {
				Divider()
			}
			.padding(.horizontal)
			
			Button {
				showShareSheet.toggle()
			} label: {
				HStack {
					Image("share-network")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.grayMain2c600)
						.frame(width: 25, height: 25)
						.padding(.all, 10)
					
					Text("contact_details_share")
						.default_text_style(styleSize: 14)
						.frame(maxWidth: .infinity, alignment: .leading)
						.lineLimit(1)
						.fixedSize(horizontal: false, vertical: true)
					Spacer()
				}
				.padding(.vertical, 15)
				.padding(.horizontal, 20)
			}
			
			VStack {
				Divider()
			}
			.padding(.horizontal)
			
			Button {
				isShowDeletePopup.toggle()
			} label: {
				HStack {
					Image("trash-simple")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.redDanger500)
						.frame(width: 25, height: 25)
						.padding(.all, 10)
					
					Text("contact_details_delete")
						.foregroundStyle(Color.redDanger500)
						.default_text_style(styleSize: 14)
						.frame(maxWidth: .infinity, alignment: .leading)
						.lineLimit(1)
						.fixedSize(horizontal: false, vertical: true)
					Spacer()
				}
				.padding(.vertical, 15)
				.padding(.horizontal, 20)
			}
		}
		.background(.white)
		.cornerRadius(15)
		.padding(.horizontal)
		.zIndex(-1)
		.transition(.move(edge: .top))
    }
}

#Preview {
	ContactInnerActionsFragment(
		contactViewModel: ContactViewModel(),
		editContactViewModel: EditContactViewModel(),
		contactAvatarModel: ContactAvatarModel(friend: nil, name: "", address: "", withPresence: false),
		showingSheet: .constant(false),
		showShareSheet: .constant(false),
		isShowDeletePopup: .constant(false),
		isShowDismissPopup: .constant(false),
		actionEditButton: {}
	)
}

// swiftlint:enable type_body_length
