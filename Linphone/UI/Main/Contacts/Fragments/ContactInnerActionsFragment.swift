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
	
	@EnvironmentObject var contactAvatarModel: ContactAvatarModel
	@EnvironmentObject var contactsListViewModel: ContactsListViewModel
	
	@State private var informationIsOpen = true
	
	@Binding var showingSheet: Bool
	@Binding var showShareSheet: Bool
	@Binding var isShowDeletePopup: Bool
	@Binding var isShowDismissPopup: Bool
	@Binding var isShowEditContactFragmentInContactDetails: Bool
	
	var actionEditButton: () -> Void
	
	var body: some View {
		if !CorePreferences.hideSipAddresses || (CorePreferences.hideSipAddresses && !contactAvatarModel.phoneNumbersWithLabel.isEmpty) {
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
					if !CorePreferences.hideSipAddresses {
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
								CoreContext.shared.doOnCoreQueue { core in
									do {
										let address = try Factory.Instance.createAddress(addr: contactAvatarModel.addresses[index])
										telecomManager.doCallOrJoinConf(address: address)
									} catch {
										Log.error("[ContactInnerActionsFragment] unable to create address for a new outgoing call : \(contactAvatarModel.addresses[index]) \(error) ")
									}
								}
							}
							.onLongPressGesture(minimumDuration: 0.2) {
								contactsListViewModel.stringToCopy = contactAvatarModel.addresses[index]
								showingSheet.toggle()
							}
							
							if !contactAvatarModel.phoneNumbersWithLabel.isEmpty
								|| index < contactAvatarModel.addresses.count - 1 {
								VStack {
									Divider()
								}
								.padding(.horizontal)
							}
						}
					}
					
					ForEach(contactAvatarModel.phoneNumbersWithLabel.indices, id: \.self) { index in
						let entry = contactAvatarModel.phoneNumbersWithLabel[index]
						HStack {
							HStack {
								VStack {
									if !entry.label.isEmpty {
										Text(String(localized: "phone_number") + " (\(entry.label)):")
											.default_text_style_700(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
									} else {
										Text(String(localized: "phone_number") + ":")
											.default_text_style_700(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
									}
									Text(entry.phoneNumber)
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
							CoreContext.shared.doOnCoreQueue { core in
								let address = core.interpretUrl(url: contactAvatarModel.phoneNumbersWithLabel[index].phoneNumber, applyInternationalPrefix: LinphoneUtils.applyInternationalPrefix(core: core))
								if address != nil {
									TelecomManager.shared.doCallOrJoinConf(address: address!)
								}
							}
						}
						.onLongPressGesture(minimumDuration: 0.2) {
							contactsListViewModel.stringToCopy = entry.phoneNumber
							showingSheet.toggle()
						}
						
						if index < contactAvatarModel.phoneNumbersWithLabel.count - 1 {
							VStack {
								Divider()
							}
							.padding(.horizontal)
						}
					}
				}
				.background(.white)
				.cornerRadius(15)
				.padding(.horizontal)
				.zIndex(-1)
				.transition(.move(edge: .top))
			}
		} else {
			HStack {}
				.frame(height: 20)
		}
		
		if !contactAvatarModel.organization.isEmpty || !contactAvatarModel.jobTitle.isEmpty {
			VStack {
				if !contactAvatarModel.organization.isEmpty {
					Text(.init(String(format:"**%@ :** %@", String(localized: "contact_editor_company"), contactAvatarModel.organization)))
						.default_text_style(styleSize: 14)
						.padding(.vertical, 15)
						.padding(.horizontal, 20)
						.frame(maxWidth: .infinity, alignment: .leading)
				}
				
				if !contactAvatarModel.jobTitle.isEmpty {
					Text(.init(String(format:"**%@ :** %@", String(localized: "contact_editor_job_title"), contactAvatarModel.jobTitle)))
						.default_text_style(styleSize: 14)
						.padding(.top, !contactAvatarModel.organization.isEmpty ? 0 : 15)
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
			if !contactAvatarModel.isReadOnly {
				if !contactAvatarModel.editable {
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
						contactAvatarModel: contactAvatarModel,
						isShowEditContactFragment: $isShowEditContactFragmentInContactDetails,
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
								isShowEditContactFragmentInContactDetails = true
							}
						)
				}
				
				VStack {
					Divider()
				}
				.padding(.horizontal)
				
				Button {
					contactsListViewModel.toggleStarredSelectedFriend()
				} label: {
					HStack {
						Image(contactAvatarModel.starred == true ? "heart-fill" : "heart")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(contactAvatarModel.starred == true ? Color.redDanger500 : Color.grayMain2c500)
							.frame(width: 25, height: 25)
							.padding(.all, 10)
						Text(contactAvatarModel.starred == true
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
			}
			
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
			
			if !contactAvatarModel.isReadOnly {
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
		showingSheet: .constant(false),
		showShareSheet: .constant(false),
		isShowDeletePopup: .constant(false),
		isShowDismissPopup: .constant(false),
		isShowEditContactFragmentInContactDetails: .constant(false),
		actionEditButton: {}
	)
}

// swiftlint:enable type_body_length
