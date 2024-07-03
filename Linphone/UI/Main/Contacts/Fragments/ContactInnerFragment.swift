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
import Contacts
import ContactsUI
import linphonesw

struct ContactInnerFragment: View {
	
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@ObservedObject var contactAvatarModel: ContactAvatarModel
	@ObservedObject var contactViewModel: ContactViewModel
	@ObservedObject var editContactViewModel: EditContactViewModel
	
	@State private var orientation = UIDevice.current.orientation
	
	@State var cnContact: CNContact?
	@State private var presentingEditContact = false
	
	@Binding var isShowDeletePopup: Bool
	@Binding var showingSheet: Bool
	@Binding var showShareSheet: Bool
	@Binding var isShowDismissPopup: Bool
	@Binding var isShowSipAddressesPopup: Bool
	
	var body: some View {
		NavigationView {
			ZStack {
				VStack(spacing: 1) {
					Rectangle()
						.foregroundColor(Color.orangeMain500)
						.edgesIgnoringSafeArea(.top)
						.frame(height: 0)
					
					HStack {
						if !(orientation == .landscapeLeft || orientation == .landscapeRight
							 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
							Image("caret-left")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.orangeMain500)
								.frame(width: 25, height: 25, alignment: .leading)
								.padding(.all, 10)
								.padding(.top, 2)
								.padding(.leading, -10)
								.onTapGesture {
									withAnimation {
										contactViewModel.indexDisplayedFriend = nil
									}
								}
						}
						
						Spacer()
						if contactViewModel.indexDisplayedFriend != nil && contactViewModel.indexDisplayedFriend! < contactsManager.lastSearch.count
							&& !contactAvatarModel.nativeUri.isEmpty {
							Button(action: {
								editNativeContact()
							}, label: {
								Image("pencil-simple")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.orangeMain500)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
									.padding(.top, 2)
							})
						} else {
							NavigationLink(destination: EditContactFragment(
								editContactViewModel: editContactViewModel,
								contactViewModel: contactViewModel,
								isShowEditContactFragment: .constant(false),
								isShowDismissPopup: $isShowDismissPopup)) {
									Image("pencil-simple")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.orangeMain500)
										.frame(width: 25, height: 25, alignment: .leading)
										.padding(.all, 10)
										.padding(.top, 2)
								}
								.simultaneousGesture(
									TapGesture().onEnded {
										editContactViewModel.selectedEditFriend = contactAvatarModel.friend
										editContactViewModel.resetValues()
									}
								)
						}
					}
					.frame(maxWidth: .infinity)
					.frame(height: 50)
					.padding(.horizontal)
					.padding(.bottom, 4)
					.background(.white)
					
					ScrollView {
						VStack(spacing: 0) {
							VStack(spacing: 0) {
								VStack(spacing: 0) {
									if contactViewModel.indexDisplayedFriend != nil && contactViewModel.indexDisplayedFriend! < contactsManager.lastSearch.count {
										Avatar(contactAvatarModel: contactAvatarModel, avatarSize: 100)
									} else if contactViewModel.indexDisplayedFriend != nil
												&& contactViewModel.indexDisplayedFriend! < contactsManager.lastSearch.count {
										Image("profil-picture-default")
											.resizable()
											.frame(width: 100, height: 100)
											.clipShape(Circle())
									}
									if contactViewModel.indexDisplayedFriend != nil
										&& contactViewModel.indexDisplayedFriend! < contactsManager.lastSearch.count {
										Text(contactAvatarModel.name)
											.foregroundStyle(Color.grayMain2c700)
											.multilineTextAlignment(.center)
											.default_text_style(styleSize: 14)
											.frame(maxWidth: .infinity)
											.padding(.top, 10)
										
										Text(contactAvatarModel.lastPresenceInfo)
											.foregroundStyle(contactAvatarModel.lastPresenceInfo == "Online"
															 ? Color.greenSuccess500
															 : Color.orangeWarning600)
											.multilineTextAlignment(.center)
											.default_text_style_300(styleSize: 12)
											.frame(maxWidth: .infinity)
									}
									
								}
								.frame(minHeight: 150)
								.frame(maxWidth: .infinity)
								.padding(.top, 10)
								.background(Color.gray100)
								
								HStack {
									Spacer()
									
									Button(action: {
										if contactAvatarModel.addresses.count <= 1 {
											do {
												let address = try Factory.Instance.createAddress(addr: contactAvatarModel.address)
												telecomManager.doCallOrJoinConf(address: address, isVideo: false)
											} catch {
												Log.error("[ContactInnerFragment] unable to create address for a new outgoing call : \(contactAvatarModel.address) \(error) ")
											}
										} else {
											isShowSipAddressesPopup = true
										}
									}, label: {
										VStack {
											HStack(alignment: .center) {
												Image("phone")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(Color.grayMain2c600)
													.frame(width: 25, height: 25)
											}
											.padding(16)
											.background(Color.grayMain2c200)
											.cornerRadius(40)
											
											Text("Appel")
												.default_text_style(styleSize: 14)
										}
									})
									
									Spacer()
									
									Button(action: {
										
									}, label: {
										VStack {
											HStack(alignment: .center) {
												Image("chat-teardrop-text")
													.renderingMode(.template)
													.resizable()
												//.foregroundStyle(Color.grayMain2c600)
													.foregroundStyle(Color.grayMain2c300)
													.frame(width: 25, height: 25)
													.onTapGesture {
														withAnimation {
															
														}
													}
											}
											.padding(16)
											.background(Color.grayMain2c200)
											.cornerRadius(40)
											
											Text("Message")
												.default_text_style(styleSize: 14)
										}
									})
									
									Spacer()
									
									Button(action: {
										if contactAvatarModel.addresses.count <= 1 {
											do {
												let address = try Factory.Instance.createAddress(addr: contactAvatarModel.address)
												telecomManager.doCallOrJoinConf(address: address, isVideo: true)
											} catch {
												Log.error("[ContactInnerFragment] unable to create address for a new outgoing call : \(contactAvatarModel.address) \(error) ")
											}
										} else {
											isShowSipAddressesPopup = true
										}
									}, label: {
										VStack {
											HStack(alignment: .center) {
												Image("video-camera")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(Color.grayMain2c600)
													.frame(width: 25, height: 25)
											}
											.padding(16)
											.background(Color.grayMain2c200)
											.cornerRadius(40)
											
											Text("Video Call")
												.default_text_style(styleSize: 14)
										}
									})
									
									Spacer()
								}
								.padding(.top, 20)
								.frame(maxWidth: .infinity)
								.background(Color.gray100)
								
								ContactInnerActionsFragment(
									contactViewModel: contactViewModel,
									editContactViewModel: editContactViewModel,
									contactAvatarModel: contactAvatarModel, showingSheet: $showingSheet,
									showShareSheet: $showShareSheet,
									isShowDeletePopup: $isShowDeletePopup,
									isShowDismissPopup: $isShowDismissPopup,
									actionEditButton: editNativeContact
								)
							}
							.frame(maxWidth: sharedMainViewModel.maxWidth)
						}
						.frame(maxWidth: .infinity)
					}
					.background(Color.gray100)
				}
				.background(.white)
				.navigationBarHidden(true)
				.onRotate { newOrientation in
					orientation = newOrientation
				}
				.fullScreenCover(isPresented: $presentingEditContact) {
					NavigationView {
						EditContactView(contact: $cnContact)
							.navigationBarTitle("Edit Contact")
							.navigationBarTitleDisplayMode(.inline)
							.edgesIgnoringSafeArea(.vertical)
					}
				}
			}
		}
		.navigationViewStyle(.stack)
	}
	
	func editNativeContact() {
		do {
			let store = CNContactStore()
			let descriptor = CNContactViewController.descriptorForRequiredKeys()
			cnContact = try store.unifiedContact(
				withIdentifier: contactAvatarModel.nativeUri,
				keysToFetch: [descriptor]
			)
			
			if cnContact != nil {
				presentingEditContact.toggle()
			}
		} catch {
			print(error)
		}
	}
	
	var sipAddressesPopup: some View {
		GeometryReader { geometry in
			VStack(alignment: .leading) {
				HStack {
					Text("contact_dialog_pick_phone_number_or_sip_address_title")
						.default_text_style_800(styleSize: 16)
						.background(.red)
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
								Text("SIP address :")
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
							withAnimation {
								isShowSipAddressesPopup.toggle()
								telecomManager.doCallOrJoinConf(address: address)
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
			.frame(maxWidth: sharedMainViewModel.maxWidth)
			.position(x: geometry.size.width / 2, y: geometry.size.height / 2)
		}
	}
}

#Preview {
	ContactInnerFragment(
		contactAvatarModel: ContactAvatarModel(friend: nil, name: "", address: "", withPresence: true),
		contactViewModel: ContactViewModel(),
		editContactViewModel: EditContactViewModel(),
		isShowDeletePopup: .constant(false),
		showingSheet: .constant(false),
		showShareSheet: .constant(false),
		isShowDismissPopup: .constant(false),
		isShowSipAddressesPopup: .constant(false)
	)
}
