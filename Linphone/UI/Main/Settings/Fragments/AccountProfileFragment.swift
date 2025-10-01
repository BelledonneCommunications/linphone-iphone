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
import UniformTypeIdentifiers

// swiftlint:disable type_body_length
struct AccountProfileFragment: View {
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject var sharedMainViewModel = SharedMainViewModel.shared
	
	@EnvironmentObject var accountProfileViewModel: AccountProfileViewModel
	
	@Binding var isShowAccountProfileFragment: Bool
	@State var detailIsOpen: Bool = true
	@State var deviceIsOpen: Bool = false
	
	@State private var showPhotoPicker = false
	@State private var selectedImage: UIImage?
	@State private var removedImage = false
	@State private var isShowPopup = false
	@State private var isShowLogoutPopup = false
	@State private var flag = true
	
	@FocusState var isDisplayNameFocused: Bool
	
	private let avatarSize = 100.0
	
	var body: some View {
		NavigationView {
			ZStack {
				VStack(spacing: 1) {
					Rectangle()
						.foregroundColor(Color.orangeMain500)
						.edgesIgnoringSafeArea(.top)
						.frame(height: 0)
					
					HStack {
						Image("caret-left")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.orangeMain500)
							.frame(width: 25, height: 25, alignment: .leading)
							.padding(.all, 10)
							.padding(.top, 4)
							.padding(.leading, -10)
							.onTapGesture {
								accountProfileViewModel.saveChangesWhenLeaving()
								withAnimation {
									if isShowAccountProfileFragment {
										isShowAccountProfileFragment = false
									}
								}
							}
						
						Text("manage_account_title")
							.default_text_style_orange_800(styleSize: 16)
							.frame(maxWidth: .infinity, alignment: .leading)
							.padding(.top, 4)
							.lineLimit(1)
						
						Spacer()
					}
					.frame(maxWidth: .infinity)
					.frame(height: 50)
					.padding(.horizontal)
					.padding(.bottom, 4)
					.background(.white)
					
					ScrollView {
						VStack(spacing: 0) {
							if accountProfileViewModel.accountModelIndex != nil && CoreContext.shared.accounts.count > accountProfileViewModel.accountModelIndex! {
								let accountModel = CoreContext.shared.accounts[accountProfileViewModel.accountModelIndex!]
								VStack(spacing: 0) {
									if #unavailable(iOS 16.0) {
										Rectangle()
											.foregroundColor(Color.gray100)
											.frame(height: 7)
									}
									
									VStack(spacing: 0) {
										if accountModel.avatarModel != nil
											&& accountModel.photoAvatarModel != nil
											&& !accountModel.photoAvatarModel!.isEmpty
											&& selectedImage == nil && !removedImage {
											
											AsyncImage(url: CoreContext.shared.accounts[accountProfileViewModel.accountModelIndex!].imagePathAvatar) { image in
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
														firstName: accountModel.avatarModel?.name ?? "",
														lastName: ""))
													.resizable()
													.frame(width: avatarSize, height: avatarSize)
													.clipShape(Circle())
												@unknown default:
													EmptyView()
												}
											}
										} else if selectedImage == nil {
											Image(uiImage: contactsManager.textToImage(
												firstName: accountModel.avatarModel?.name ?? "",
												lastName: ""))
											.resizable()
											.frame(width: avatarSize, height: avatarSize)
											.clipShape(Circle())
										} else {
											Image(uiImage: selectedImage!)
												.resizable()
												.aspectRatio(contentMode: .fill)
												.frame(width: avatarSize, height: avatarSize)
												.clipShape(Circle())
										}
										
										if accountModel.avatarModel != nil
											&& accountModel.photoAvatarModel != nil
											&& !accountModel.photoAvatarModel!.isEmpty
											&& (accountModel.photoAvatarModel!.suffix(11) != "default.png" || selectedImage != nil)
											&& !removedImage {
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
																	saveImage()
																}
															}
														}
													}
													.edgesIgnoringSafeArea(.all)
												}
												
												Button(action: {
													removedImage = true
													selectedImage = nil
													saveImage()
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
																showPhotoPicker = false
																saveImage()
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
									.padding(.bottom, 2)
									.background(Color.gray100)
									
									HStack(alignment: .center) {
										Text("manage_account_details_title")
											.default_text_style_800(styleSize: 18)
											.frame(maxWidth: .infinity, alignment: .leading)
										
										Spacer()
										
										Image(detailIsOpen ? "caret-up" : "caret-down")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c600)
											.frame(width: 25, height: 25, alignment: .leading)
											.padding(.all, 10)
									}
									.padding(.top, 30)
									.padding(.bottom, 10)
									.padding(.horizontal, 20)
									.background(Color.gray100)
									.onTapGesture {
										withAnimation {
											detailIsOpen.toggle()
										}
									}
									
									if detailIsOpen {
										if accountModel.avatarModel != nil {
											VStack(spacing: 0) {
												VStack(spacing: 30) {
													if !CorePreferences.hideSipAddresses {
														HStack {
															Text(String(localized: "sip_address") + ":")
																.default_text_style_700(styleSize: 15)
															
															Text(accountModel.avatarModel!.address)
																.foregroundStyle(Color.grayMain2c700)
																.default_text_style(styleSize: 15)
																.frame(maxWidth: .infinity, alignment: .leading)
																.lineLimit(1)
															
															Button(action: {
																UIPasteboard.general.setValue(
																	accountModel.avatarModel!.address,
																	forPasteboardType: UTType.plainText.identifier
																)
																
																ToastViewModel.shared.toastMessage = "Success_address_copied_into_clipboard"
																ToastViewModel.shared.displayToast.toggle()
															}, label: {
																Image("copy")
																	.resizable()
																	.frame(width: 20, height: 20)
															})
														}
													}
													
													VStack(alignment: .leading) {
														Text("sip_address_display_name")
															.default_text_style_700(styleSize: 15)
															.padding(.bottom, -5)
														
														TextField(accountModel.displayNameAvatar, text: Binding(
															get: { accountModel.displayNameAvatar },
															set: { newValue in
																accountModel.displayNameAvatar = newValue
															}
														))
														.default_text_style(styleSize: 15)
														.frame(height: 25)
														.padding(.horizontal, 20)
														.padding(.vertical, 15)
														.background(.white)
														.cornerRadius(60)
														.overlay(
															RoundedRectangle(cornerRadius: 60)
																.inset(by: 0.5)
																.stroke(isDisplayNameFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
														)
														.focused($isDisplayNameFocused)
													}
													
													VStack(alignment: .leading) {
														HStack {
															Text("manage_account_international_prefix")
																.default_text_style_700(styleSize: 15)
																.padding(.bottom, -5)
																.lineLimit(1)
															
															Button(action: {
																isShowPopup = true
															}, label: {
																Image("question")
																	.renderingMode(.template)
																	.resizable()
																	.foregroundStyle(Color.grayMain2c600)
																	.frame(width: 20, height: 20)
															})
															.padding(.bottom, -5)
														}
														Menu {
															Picker("", selection: $accountProfileViewModel.dialPlanValueSelected) {
																ForEach(sharedMainViewModel.dialPlansLabelList, id: \.self) { dialPlan in
																	Text(dialPlan).tag(dialPlan)
																}
															}
															.onChange(of: accountProfileViewModel.dialPlanValueSelected) { newValue in
																accountProfileViewModel.updateDialPlan(newDialPlan: newValue)
															}
														} label: {
															HStack {
																Text(accountProfileViewModel.dialPlanValueSelected)
																	.default_text_style(styleSize: 15)
																	.frame(maxWidth: .infinity, alignment: .leading)
																
																Image("caret-down")
																	.resizable()
																	.frame(width: 20, height: 20)
															}
															.frame(height: 25)
															.padding(.horizontal, 20)
															.padding(.vertical, 15)
															.background(.white)
															.cornerRadius(60)
															.overlay(
																RoundedRectangle(cornerRadius: 60)
																	.inset(by: 0.5)
																	.stroke(Color.gray200, lineWidth: 1)
															)
														}
													}
												}
												.padding(.vertical, 30)
												.padding(.horizontal, 20)
											}
											.background(.white)
											.cornerRadius(15)
											.padding(.horizontal)
											.zIndex(-1)
											.transition(.move(edge: .top))
										}
									}
									
									VStack(spacing: 0) {
										VStack(spacing: 15) {
											HStack(spacing: 20) {
												Toggle("", isOn: Binding(
													get: { accountModel.isRegistrered },
													set: { _ in
														accountProfileViewModel.toggleRegister()
													}
												))
												.labelsHidden()
												
												Text(accountModel.humanReadableRegistrationState)
													.default_text_style_700(styleSize: 15)
													.frame(maxWidth: .infinity, alignment: .leading)
													.padding(.bottom, -5)
													.lineLimit(1)
											}
											
											Text(accountModel.summary)
												.default_text_style(styleSize: 15)
												.frame(maxWidth: .infinity, alignment: .leading)
												.padding(.bottom, -5)
										}
										.padding(.vertical, 30)
										.padding(.horizontal, 20)
									}
									.background(.white)
									.cornerRadius(15)
									.padding(.all)
									.background(Color.gray100)
									
									HStack(alignment: .center) {
										Text("manage_account_devices_title")
											.default_text_style_800(styleSize: 18)
											.frame(maxWidth: .infinity, alignment: .leading)
										
										Spacer()
										
										Image(deviceIsOpen ? "caret-up" : "caret-down")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c600)
											.frame(width: 25, height: 25, alignment: .leading)
											.padding(.all, 10)
									}
									.padding(.vertical, 10)
									.padding(.horizontal, 20)
									.background(Color.gray100)
									.onTapGesture {
										withAnimation {
											deviceIsOpen.toggle()
										}
									}
									
									if deviceIsOpen {
										VStack(spacing: 0) {
											VStack(spacing: 15) {
												ForEach(accountModel.devices.indices, id: \.self) { index in
													VStack {
														HStack {
															Image(accountModel.devices[index].isMobileDevice ? "device-mobile-camera" : "desktop")
																.renderingMode(.template)
																.resizable()
																.foregroundStyle(Color.grayMain2c600)
																.frame(width: 25, height: 25, alignment: .leading)
															
															Text(accountModel.devices[index].deviceName)
																.default_text_style_700(styleSize: 15)
																.lineLimit(1)
																.frame(maxWidth: .infinity, alignment: .leading)
															
															Button(action: {
																deviceIsOpen = false
																accountModel.removeDevice(deviceIndex: index)
																deviceIsOpen = true
															}, label: {
																HStack {
																	Image("trash-simple")
																		.renderingMode(.template)
																		.resizable()
																		.foregroundStyle(Color.orangeMain500)
																		.frame(width: 20, height: 20)
																	
																	Text("manage_account_device_remove")
																		.default_text_style_orange_500(styleSize: 14)
																		.frame(height: 35)
																}
																
															})
															.padding(.horizontal, 10)
															.background(Color.orangeMain100)
															.cornerRadius(60)
														}
														.padding(.bottom, 10)
														
														Text("manage_account_device_last_connection")
															.default_text_style_700(styleSize: 15)
															.lineLimit(1)
															.frame(maxWidth: .infinity, alignment: .leading)
														
														HStack {
															Image("calendar-blank")
																.renderingMode(.template)
																.resizable()
																.foregroundStyle(Color.grayMain2c600)
																.frame(width: 25, height: 25, alignment: .leading)
															
															Text(accountModel.devices[index].lastDate)
																.default_text_style(styleSize: 15)
																.lineLimit(1)
																.frame(maxWidth: .infinity, alignment: .leading)
															
															Image("clock")
																.renderingMode(.template)
																.resizable()
																.foregroundStyle(Color.grayMain2c600)
																.frame(width: 25, height: 25, alignment: .leading)
															
															Text(accountModel.devices[index].lastTime)
																.default_text_style(styleSize: 15)
																.lineLimit(1)
																.frame(maxWidth: .infinity, alignment: .leading)
														}
													}
													.padding(.all, 20)
													.background(Color.gray100)
													.cornerRadius(15)
													
												}
											}
											.padding(.all, 20)
											.frame(maxWidth: .infinity)
											.overlay(
												VStack {
													if accountModel.devices.indices.isEmpty {
														Text("manage_account_no_device")
															.default_text_style_500(styleSize: 16)
													}
												}
													.padding(.all)
											)
										}
										.background(.white)
										.cornerRadius(15)
										.padding(.horizontal)
										.zIndex(-2)
										.transition(.move(edge: .top))
									}
									
									HStack(alignment: .center) {
										Text("contact_details_actions_title")
											.default_text_style_800(styleSize: 18)
											.frame(maxWidth: .infinity, alignment: .leading)
									}
									.padding(.top, 20)
									.padding(.bottom, 10)
									.padding(.horizontal, 20)
									.background(Color.gray100)
									
									VStack(spacing: 0) {
										VStack(spacing: 18) {
											if accountProfileViewModel.accountModelIndex != nil && CoreContext.shared.accounts.count > accountProfileViewModel.accountModelIndex! {
												NavigationLink(
													destination:
														AccountSettingsFragment(
															accountModel: CoreContext.shared.accounts[accountProfileViewModel.accountModelIndex!]
														),
													label: {
														HStack {
															Image("gear")
																.renderingMode(.template)
																.resizable()
																.foregroundStyle(Color.grayMain2c700)
																.frame(width: 25, height: 25)
															
															Text("manage_account_settings")
																.foregroundStyle(Color.grayMain2c700)
																.default_text_style(styleSize: 16)
																.frame(maxWidth: .infinity, alignment: .leading)
														}
													}
												)
											}
											
											Divider()
											
											Button(action: {
												isShowLogoutPopup = true
											}, label: {
												HStack {
													Image("sign-out")
														.renderingMode(.template)
														.resizable()
														.foregroundStyle(Color.redDanger500)
														.frame(width: 25, height: 25)
													
													Text("manage_account_delete")
														.foregroundStyle(Color.redDanger500)
														.default_text_style(styleSize: 16)
														.frame(maxWidth: .infinity, alignment: .leading)
												}
											})
										}
										.padding(.vertical, 20)
										.padding(.horizontal, 20)
									}
									.background(.white)
									.cornerRadius(15)
									.padding(.horizontal)
								}
								.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
								.onAppear {
									accountModel.requestDevicesList()
								}
							}
						}
						.frame(maxWidth: .infinity)
					}
					.background(Color.gray100)
				}
				.background(Color.gray100)
				
				if self.isShowPopup {
					PopupView(isShowPopup: $isShowPopup,
							  title: Text("manage_account_international_prefix"),
							  content: Text("manage_account_dialog_international_prefix_help_message"),
							  titleFirstButton: nil,
							  actionFirstButton: {},
							  titleSecondButton: Text("dialog_ok"),
							  actionSecondButton: {
						self.isShowPopup.toggle()
					}
					)
					.background(.black.opacity(0.65))
					.onTapGesture {
						self.isShowPopup.toggle()
					}
				}
				
				if self.isShowLogoutPopup {
					let localizedString = NSLocalizedString("manage_account_dialog_remove_account_message", comment: "")
					
					let components = localizedString.components(separatedBy: " ")
					let textPart = components.dropLast().joined(separator: " ")
					
					let contentPopup1 = Text(textPart + " ")
					let contentPopup2 = Text("[https://sip.linphone.org](https://sip.linphone.org)").underline()
					
					PopupView(
						isShowPopup: $isShowLogoutPopup,
						title: Text("manage_account_dialog_remove_account_title"),
						content: contentPopup1 + contentPopup2,
						titleFirstButton: Text("dialog_cancel"),
						actionFirstButton: {
							self.isShowLogoutPopup.toggle()
						},
						titleSecondButton: Text("manage_account_delete"),
						actionSecondButton: {
							if accountProfileViewModel.accountModelIndex != nil {
								CoreContext.shared.accounts[accountProfileViewModel.accountModelIndex!].logout()
								isShowAccountProfileFragment = false
								if let firstAccount = CoreContext.shared.accounts.first?.account {
									CoreContext.shared.doOnCoreQueue { core in
										core.defaultAccount = firstAccount
									}
								}
							}
						}
					)
					.background(.black.opacity(0.65))
					.onTapGesture {
						self.isShowLogoutPopup.toggle()
					}
				}
			}
			.navigationTitle("")
			.navigationBarHidden(true)
		}
		.navigationViewStyle(StackNavigationViewStyle())
	}
	
	func saveImage() {
		let accountModel = CoreContext.shared.accounts[accountProfileViewModel.accountModelIndex ?? 0]
		let usernameTmp = CoreContext.shared.accounts[accountProfileViewModel.accountModelIndex ?? 0].usernaneAvatar
		
		accountProfileViewModel.saveImage(
			image: selectedImage
			?? ContactsManager.shared.textToImage(
				firstName: accountModel.avatarModel!.name, lastName: ""),
			name: usernameTmp,
			prefix: ((selectedImage == nil) ? "-default" : ""))
	}
}
// swiftlint:enable type_body_length
