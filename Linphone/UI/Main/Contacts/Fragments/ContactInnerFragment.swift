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
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@EnvironmentObject var contactAvatarModel: ContactAvatarModel
	@EnvironmentObject var contactsListViewModel: ContactsListViewModel
	
	@State private var orientation = UIDevice.current.orientation
	
	@State var cnContact: CNContact?
	@State private var presentingEditContact = false
	
	@Binding var isShowDeletePopup: Bool
	@Binding var showingSheet: Bool
	@Binding var showShareSheet: Bool
	@Binding var isShowDismissPopup: Bool
	@Binding var isShowSipAddressesPopup: Bool
	@Binding var isShowSipAddressesPopupType: Int
	@Binding var isShowEditContactFragmentInContactDetails: Bool
	
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
										SharedMainViewModel.shared.displayedFriend = nil
									}
								}
						}
						
						Spacer()
						
						if !contactAvatarModel.isReadOnly {
							if !contactAvatarModel.editable {
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
									contactAvatarModel: contactAvatarModel,
									isShowEditContactFragment: $isShowEditContactFragmentInContactDetails,
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
											isShowEditContactFragmentInContactDetails = true
										}
									)
							}
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
									if SharedMainViewModel.shared.displayedFriend != nil {
										Avatar(contactAvatarModel: contactAvatarModel, avatarSize: 100)
										
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
										CoreContext.shared.doOnCoreQueue { core in
											if contactAvatarModel.addresses.count == 1 {
												do {
													let address = try Factory.Instance.createAddress(addr: contactAvatarModel.address)
													telecomManager.doCallOrJoinConf(address: address, isVideo: false)
												} catch {
													Log.error("[ContactInnerFragment] unable to create address for a new outgoing call : \(contactAvatarModel.address) \(error) ")
												}
											} else if contactAvatarModel.addresses.count < 1 && contactAvatarModel.phoneNumbersWithLabel.count == 1 {
												if let firstPhoneNumbersWithLabel = contactAvatarModel.phoneNumbersWithLabel.first, let address = core.interpretUrl(url: firstPhoneNumbersWithLabel.phoneNumber, applyInternationalPrefix: LinphoneUtils.applyInternationalPrefix(core: core)) {
													telecomManager.doCallOrJoinConf(address: address, isVideo: false)
												}
											} else {
												DispatchQueue.main.async {
													isShowSipAddressesPopupType = 0
											  		isShowSipAddressesPopup = true
												}
											}
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
											
											Text("contact_call_action")
												.default_text_style(styleSize: 14)
										}
									})
                                    
                                    if !CorePreferences.disableChatFeature {
                                        Spacer()
                                        
                                        Button(action: {
											CoreContext.shared.doOnCoreQueue { core in
												if contactAvatarModel.addresses.count == 1 {
													do {
														let address = try Factory.Instance.createAddress(addr: contactAvatarModel.address)
														contactsListViewModel.createOneToOneChatRoomWith(remote: address)
													} catch {
														Log.error("[ContactInnerFragment] unable to create address for a new outgoing call : \(contactAvatarModel.address) \(error) ")
													}
												} else if contactAvatarModel.addresses.count < 1 && contactAvatarModel.phoneNumbersWithLabel.count == 1 {
													if let firstPhoneNumbersWithLabel = contactAvatarModel.phoneNumbersWithLabel.first, let address = core.interpretUrl(url: firstPhoneNumbersWithLabel.phoneNumber, applyInternationalPrefix: LinphoneUtils.applyInternationalPrefix(core: core)) {
														contactsListViewModel.createOneToOneChatRoomWith(remote: address)
													}
												} else {
													DispatchQueue.main.async {
														isShowSipAddressesPopupType = 1
														isShowSipAddressesPopup = true
													}
												}
											}
                                        }, label: {
                                            VStack {
                                                HStack(alignment: .center) {
                                                    Image("chat-teardrop-text")
                                                        .renderingMode(.template)
                                                        .resizable()
                                                        .foregroundStyle(Color.grayMain2c600)
                                                        .frame(width: 25, height: 25)
                                                }
                                                .padding(16)
                                                .background(Color.grayMain2c200)
                                                .cornerRadius(40)
                                                
                                                Text("contact_message_action")
                                                    .default_text_style(styleSize: 14)
                                            }
                                        })
                                    }
                                    
									Spacer()
									
									Button(action: {
										CoreContext.shared.doOnCoreQueue { core in
											if contactAvatarModel.addresses.count == 1 {
												do {
													let address = try Factory.Instance.createAddress(addr: contactAvatarModel.address)
													telecomManager.doCallOrJoinConf(address: address, isVideo: true)
												} catch {
													Log.error("[ContactInnerFragment] unable to create address for a new outgoing call : \(contactAvatarModel.address) \(error) ")
												}
											} else if contactAvatarModel.addresses.count < 1 && contactAvatarModel.phoneNumbersWithLabel.count == 1 {
												if let firstPhoneNumbersWithLabel = contactAvatarModel.phoneNumbersWithLabel.first, let address = core.interpretUrl(url: firstPhoneNumbersWithLabel.phoneNumber, applyInternationalPrefix: LinphoneUtils.applyInternationalPrefix(core: core)) {
													telecomManager.doCallOrJoinConf(address: address, isVideo: true)
												}
											} else {
												DispatchQueue.main.async {
													isShowSipAddressesPopupType = 2
													isShowSipAddressesPopup = true
												}
											}
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
											
											Text("contact_video_call_action")
												.default_text_style(styleSize: 14)
										}
									})
									
									Spacer()
								}
								.padding(.top, 20)
								.frame(maxWidth: .infinity)
								.background(Color.gray100)
								
								ContactInnerActionsFragment(
									showingSheet: $showingSheet,
									showShareSheet: $showShareSheet,
									isShowDeletePopup: $isShowDeletePopup,
									isShowDismissPopup: $isShowDismissPopup,
									isShowEditContactFragmentInContactDetails: $isShowEditContactFragmentInContactDetails,
									actionEditButton: editNativeContact
								)
							}
							.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
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
							.navigationBarTitle("contact_edit_title")
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
}

#Preview {
	ContactInnerFragment(
		isShowDeletePopup: .constant(false),
		showingSheet: .constant(false),
		showShareSheet: .constant(false),
		isShowDismissPopup: .constant(false),
		isShowSipAddressesPopup: .constant(false),
		isShowSipAddressesPopupType: .constant(0),
		isShowEditContactFragmentInContactDetails: .constant(false)
	)
}
