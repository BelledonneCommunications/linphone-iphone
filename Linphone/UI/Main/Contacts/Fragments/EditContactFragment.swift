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
struct EditContactFragment: View {
	
	@ObservedObject var editContactViewModel: EditContactViewModel
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	
	@Environment(\.dismiss) var dismiss
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	var contactViewModel: ContactViewModel
	
	@Binding var isShowEditContactFragment: Bool
	@Binding var isShowDismissPopup: Bool
	
	@State private var delayedColor = Color.white
	
	@FocusState var isFirstNameFocused: Bool
	@FocusState var isLastNameFocused: Bool
	@FocusState var isSIPAddressFocused: Int?
	@FocusState var isPhoneNumberFocused: Int?
	@FocusState var isCompanyFocused: Bool
	@FocusState var isJobTitleFocused: Bool
	
	@State private var showPhotoPicker = false
	@State private var selectedImage: UIImage?
	@State private var removedImage = false
	
	var body: some View {
		ZStack {
			VStack(spacing: 1) {
				if editContactViewModel.selectedEditFriend == nil {
					if #available(iOS 16.0, *) {
						Rectangle()
							.foregroundColor(delayedColor)
							.edgesIgnoringSafeArea(.top)
							.frame(height: 0)
							.task(delayColor)
					} else if idiom != .pad && !(orientation == .landscapeLeft || orientation == .landscapeRight
												 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
						Rectangle()
							.foregroundColor(delayedColor)
							.edgesIgnoringSafeArea(.top)
							.frame(height: 1)
							.task(delayColor)
					}
				} else {
					if #available(iOS 16.0, *) {
						Rectangle()
							.foregroundColor(Color.orangeMain500)
							.edgesIgnoringSafeArea(.top)
							.frame(height: 0)
					} else if idiom != .pad && !(orientation == .landscapeLeft || orientation == .landscapeRight
												 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
						Rectangle()
							.foregroundColor(Color.orangeMain500)
							.edgesIgnoringSafeArea(.top)
							.frame(height: 1)
					}
				}
				
				HStack {
					Image("caret-left")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.orangeMain500)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
						.padding(.top, 2)
						.padding(.leading, -10)
						.onTapGesture {
							if editContactViewModel.selectedEditFriend == nil
								&& editContactViewModel.firstName.isEmpty
								&& editContactViewModel.lastName.isEmpty
								&& editContactViewModel.sipAddresses.first!.isEmpty
								&& editContactViewModel.phoneNumbers.first!.isEmpty
								&& editContactViewModel.company.isEmpty
								&& editContactViewModel.jobTitle.isEmpty {
								delayColorDismiss()
								withAnimation {
									isShowEditContactFragment.toggle()
								}
							} else if editContactViewModel.selectedEditFriend == nil {
								isShowDismissPopup.toggle()
							} else {
								if editContactViewModel.firstName.isEmpty
									&& editContactViewModel.lastName.isEmpty
									&& editContactViewModel.sipAddresses.first!.isEmpty
									&& editContactViewModel.phoneNumbers.first!.isEmpty
									&& editContactViewModel.company.isEmpty
									&& editContactViewModel.jobTitle.isEmpty {
									withAnimation {
										dismiss()
									}
								} else {
									isShowDismissPopup.toggle()
								}
							}
						}
					
					Text(editContactViewModel.selectedEditFriend == nil ? "contact_new_title" : "contact_edit_title")
						.multilineTextAlignment(.leading)
						.default_text_style_orange_800(styleSize: 16)
					
					Spacer()
					
					Image("check")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(editContactViewModel.firstName.isEmpty ? Color.orangeMain100 : Color.orangeMain500)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
						.padding(.top, 2)
						.disabled(editContactViewModel.firstName.isEmpty)
						.onTapGesture {
							addOrEditFriend()
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
								if editContactViewModel.selectedEditFriend != nil
									&& editContactViewModel.selectedEditFriend!.photo != nil
									&& !editContactViewModel.selectedEditFriend!.photo!.isEmpty && selectedImage == nil && !removedImage {
									
									Avatar(contactAvatarModel: 
											ContactAvatarModel(
												friend: editContactViewModel.selectedEditFriend!,
												name: editContactViewModel.selectedEditFriend?.name ?? "",
												address: editContactViewModel.selectedEditFriend?.address?.asStringUriOnly() ?? "",
												withPresence: false
											), avatarSize: 100
									)
									
								} else if selectedImage == nil {
									Image("profil-picture-default")
										.resizable()
										.frame(width: 100, height: 100)
										.clipShape(Circle())
								} else {
									Image(uiImage: selectedImage!)
										.resizable()
										.aspectRatio(contentMode: .fill)
										.frame(width: 100, height: 100)
										.clipShape(Circle())
								}
								
								if editContactViewModel.selectedEditFriend != nil
									&& editContactViewModel.selectedEditFriend!.photo != nil
									&& !editContactViewModel.selectedEditFriend!.photo!.isEmpty
									&& (editContactViewModel.selectedEditFriend!.photo!.suffix(11) != "default.png" || selectedImage != nil) && !removedImage {
									HStack {
										Spacer()
										
										Button(action: {
											showPhotoPicker = true
										}, label: {
											HStack {
												Image("pencil-simple")
													.resizable()
													.frame(width: 20, height: 20)
												
												Text("manage_account_edit_picture")
													.foregroundStyle(Color.grayMain2c700)
													.multilineTextAlignment(.center)
													.default_text_style(styleSize: 14)
											}
										})
										.padding(.top, 10)
										.padding(.trailing, 10)
										.sheet(isPresented: $showPhotoPicker) {
											PhotoPicker(filter: .images, limit: 1) { results in
												PhotoPicker.convertToUIImageArray(fromResults: results) { imagesOrNil, errorOrNil in
													if let error = errorOrNil {
														print(error)
													}
													if let images = imagesOrNil {
														if let first = images.first {
															selectedImage = first
															removedImage = false
														}
													}
												}
											}
											.edgesIgnoringSafeArea(.all)
										}
										
										Button(action: {
											removedImage = true
											selectedImage = nil
										}, label: {
											HStack {
												Image("trash-simple")
													.resizable()
													.frame(width: 20, height: 20)
												
												Text("manage_account_remove_picture")
													.foregroundStyle(Color.grayMain2c700)
													.multilineTextAlignment(.center)
													.default_text_style(styleSize: 14)
											}
										})
										.padding(.top, 10)
										
										Spacer()
									}
								} else {
									Button(action: {
										showPhotoPicker = true
									}, label: {
										HStack {
											Image("camera")
												.resizable()
												.frame(width: 20, height: 20)
											
											Text("manage_account_add_picture")
												.foregroundStyle(Color.grayMain2c700)
												.multilineTextAlignment(.center)
												.default_text_style(styleSize: 14)
										}
									})
									.padding(.top, 10)
									.sheet(isPresented: $showPhotoPicker) {
										PhotoPicker(filter: .images, limit: 1) { results in
											PhotoPicker.convertToUIImageArray(fromResults: results) { imagesOrNil, errorOrNil in
												if let error = errorOrNil {
													print(error)
												}
												if let images = imagesOrNil {
													if let first = images.first {
														selectedImage = first
														removedImage = false
													}
												}
											}
										}
										.edgesIgnoringSafeArea(.all)
									}
								}
							}
							.frame(minHeight: 150)
							.frame(maxWidth: .infinity)
							.padding(.top, 10)
							.background(Color.gray100)
							
							VStack(alignment: .leading) {
								Text(String(localized: "contact_editor_first_name") + "*")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("contact_editor_first_name", text: $editContactViewModel.firstName)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.background(.white)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isFirstNameFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.padding(.bottom)
									.focused($isFirstNameFocused)
							}
							
							VStack(alignment: .leading) {
								Text("contact_editor_last_name")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("contact_editor_last_name", text: $editContactViewModel.lastName)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.background(.white)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isLastNameFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.padding(.bottom)
									.focused($isLastNameFocused)
							}
							
							VStack(alignment: .leading) {
								Text("sip_address")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								ForEach(0..<editContactViewModel.sipAddresses.count, id: \.self) { index in
									
									HStack(alignment: .center) {
										TextField("sip_address", text: $editContactViewModel.sipAddresses[index])
											.default_text_style(styleSize: 15)
											.disableAutocorrection(true)
									  		.autocapitalization(.none)
											.frame(height: 25)
											.padding(.horizontal, 20)
											.padding(.vertical, 15)
											.background(.white)
											.cornerRadius(60)
											.overlay(
												RoundedRectangle(cornerRadius: 60)
													.inset(by: 0.5)
													.stroke(isSIPAddressFocused == index ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
											)
											.focused($isSIPAddressFocused, equals: index)
											.onChange(of: editContactViewModel.sipAddresses[index]) { newValue in
												if !newValue.isEmpty && index + 1 == editContactViewModel.sipAddresses.count {
													editContactViewModel.sipAddresses.append("")
												}
											}
										
										Button(action: {
											editContactViewModel.sipAddresses.remove(at: index)
										}, label: {
											Image("x")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(
													editContactViewModel.sipAddresses[index].isEmpty && editContactViewModel.sipAddresses.count == index + 1
													? Color.gray100
													: Color.grayMain2c600
												)
												.frame(width: 25, height: 25)
												.padding(.all, 10)
										})
										.disabled(editContactViewModel.sipAddresses[index].isEmpty && editContactViewModel.sipAddresses.count == index + 1)
										.frame(maxHeight: .infinity)
									}
								}
							}
							.padding(.bottom)
							
							VStack(alignment: .leading) {
								Text("phone_number")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								ForEach(0..<editContactViewModel.phoneNumbers.count, id: \.self) { index in
									HStack(alignment: .center) {
										TextField("phone_number", text: $editContactViewModel.phoneNumbers[index])
											.default_text_style(styleSize: 15)
											.textContentType(.oneTimeCode)
		   									.keyboardType(.numberPad)
											.frame(height: 25)
											.padding(.horizontal, 20)
											.padding(.vertical, 15)
											.background(.white)
											.cornerRadius(60)
											.overlay(
												RoundedRectangle(cornerRadius: 60)
													.inset(by: 0.5)
													.stroke(isPhoneNumberFocused == index ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
											)
											.focused($isPhoneNumberFocused, equals: index)
											.onChange(of: editContactViewModel.phoneNumbers[index]) { newValue in
												if !newValue.isEmpty && index + 1 == editContactViewModel.phoneNumbers.count {
													withAnimation {
														editContactViewModel.phoneNumbers.append("")
													}
												}
											}
										
										Button(action: {
											editContactViewModel.phoneNumbers.remove(at: index)
										}, label: {
											Image("x")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(
													editContactViewModel.phoneNumbers[index].isEmpty && editContactViewModel.phoneNumbers.count == index + 1
													? Color.gray100
													: Color.grayMain2c600
												)
												.frame(width: 25, height: 25)
												.padding(.all, 10)
										})
										.disabled(editContactViewModel.phoneNumbers[index].isEmpty && editContactViewModel.phoneNumbers.count == index + 1)
										.frame(maxHeight: .infinity)
									}
									.zIndex(isPhoneNumberFocused == index ? 1 : 0)
									.transition(.move(edge: .top))
								}
							}
							.padding(.bottom)
							
							VStack(alignment: .leading) {
								Text("contact_editor_company")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("contact_editor_company", text: $editContactViewModel.company)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.background(.white)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isCompanyFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.padding(.bottom)
									.focused($isCompanyFocused)
							}
							
							VStack(alignment: .leading) {
								Text("contact_editor_job_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("contact_editor_job_title", text: $editContactViewModel.jobTitle)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.background(.white)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isJobTitleFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.padding(.bottom)
									.focused($isJobTitleFocused)
							}
						}
						.frame(maxWidth: sharedMainViewModel.maxWidth)
						.padding(.horizontal)
					}
					.frame(maxWidth: .infinity)
				}
				.background(Color.gray100)
			}
			.background(.white)
			
			if editContactViewModel.removePopup {
				ZStack {
					
				}.onAppear {
					if editContactViewModel.selectedEditFriend == nil {
						delayColorDismiss()
					} else {
						dismiss()
					}
					editContactViewModel.removePopup = false
				}
			}
		}
		.navigationBarHidden(true)
	}
	
	@Sendable private func delayColor() async {
		try? await Task.sleep(nanoseconds: 250_000_000)
		delayedColor = Color.orangeMain500
	}
	
	func delayColorDismiss() {
		if editContactViewModel.selectedEditFriend == nil {
			Task {
				try? await Task.sleep(nanoseconds: 80_000_000)
				delayedColor = .white
			}
		}
	}
	
	func addOrEditFriend() {
		let newContact = Contact(
			identifier: editContactViewModel.identifier,
			firstName: editContactViewModel.firstName,
			lastName: editContactViewModel.lastName,
			organizationName: editContactViewModel.company,
			jobTitle: editContactViewModel.jobTitle,
			displayName: "",
			sipAddresses: editContactViewModel.sipAddresses.map { $0 },
			phoneNumbers: editContactViewModel.phoneNumbers.map { PhoneNumber(numLabel: "", num: $0)},
			imageData: ""
		)
		
		if editContactViewModel.selectedEditFriend != nil && selectedImage == nil &&
			!removedImage && editContactViewModel.selectedEditFriend!.photo!.suffix(11) != "default.png" {
			ContactsManager.shared.saveFriend(
				result: String(editContactViewModel.selectedEditFriend!.photo!.dropFirst(6)),
				contact: newContact,
				existingFriend: editContactViewModel.selectedEditFriend, completion: {_ in }
			)
		} else {
			ContactsManager.shared.saveImage(
				image: selectedImage
				?? ContactsManager.shared.textToImage(
					firstName: editContactViewModel.firstName, lastName: editContactViewModel.lastName),
				name: editContactViewModel.firstName
				+ editContactViewModel.lastName,
				prefix: ((selectedImage == nil) ? "-default" : ""),
				contact: newContact, linphoneFriend: true, existingFriend: editContactViewModel.selectedEditFriend) {
					MagicSearchSingleton.shared.searchForContacts(sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
					
					DispatchQueue.main.async {
						if editContactViewModel.selectedEditFriend != nil {
							DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
								let result = ContactsManager.shared.lastSearch.firstIndex(where: {
									$0.friend!.name == newContact.firstName + " " + newContact.lastName
								})
								contactViewModel.indexDisplayedFriend = result
							}
						}
						
						delayColorDismiss()
						if editContactViewModel.selectedEditFriend == nil {
							withAnimation {
								isShowEditContactFragment.toggle()
							}
						} else {
							withAnimation {
								dismiss()
							}
						}
						editContactViewModel.resetValues()
					}
				}
		}
	}
}

#Preview {
	EditContactFragment(
		editContactViewModel: EditContactViewModel(),
		contactViewModel: ContactViewModel(),
		isShowEditContactFragment: .constant(false),
		isShowDismissPopup: .constant(false)
	)
}
// swiftlint:enable type_body_length
