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

struct AccountProfileFragment: View {
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	
	@ObservedObject var accountProfileViewModel: AccountProfileViewModel
	@ObservedObject var registerViewModel: RegisterViewModel
	
	@Binding var isShowAccountProfileFragment: Bool
	@State var detailIsOpen: Bool = true
	
	@State private var showPhotoPicker = false
	@State private var selectedImage: UIImage?
	@State private var removedImage = false
	
	@FocusState var isDisplayNameFocused: Bool
	
	private let avatarSize = 100.0
	
	var body: some View {
		GeometryReader { geometry in
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
						VStack(spacing: 0) {
							if #unavailable(iOS 16.0) {
								Rectangle()
									.foregroundColor(Color.gray100)
									.frame(height: 7)
							}
							
							if accountProfileViewModel.avatarModel != nil {
								VStack(spacing: 0) {
									if accountProfileViewModel.avatarModel != nil
										&& accountProfileViewModel.photoAvatarModel != nil
										&& !accountProfileViewModel.photoAvatarModel!.isEmpty
										&& selectedImage == nil && !removedImage {
										
										AsyncImage(url: accountProfileViewModel.getImagePath()) { image in
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
													firstName: accountProfileViewModel.avatarModel?.name ?? "",
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
											firstName: accountProfileViewModel.avatarModel?.name ?? "",
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
									
									if accountProfileViewModel.avatarModel != nil
										&& accountProfileViewModel.photoAvatarModel != nil
										&& !accountProfileViewModel.photoAvatarModel!.isEmpty
										&& (accountProfileViewModel.photoAvatarModel!.suffix(11) != "default.png" || selectedImage != nil)
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
							}
							
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
								VStack(spacing: 0) {
									VStack(spacing: 30) {
										HStack {
											Text(String(localized: "sip_address") + ":")
												.default_text_style_600(styleSize: 14)
											
											Text(accountProfileViewModel.avatarModel!.address)
												.foregroundStyle(Color.grayMain2c700)
												.default_text_style(styleSize: 14)
												.frame(maxWidth: .infinity, alignment: .leading)
												.lineLimit(1)
											
											Button(action: {
												UIPasteboard.general.setValue(
													accountProfileViewModel.avatarModel!.address,
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
										
										VStack(alignment: .leading) {
											Text("sip_address_display_name")
												.default_text_style_700(styleSize: 15)
												.padding(.bottom, -5)
											TextField(accountProfileViewModel.displayName, text: $accountProfileViewModel.displayName)
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
											Text("sip_address_display_name")
												.default_text_style_700(styleSize: 15)
												.padding(.bottom, -5)
											
											Menu {
												Picker("", selection: $accountProfileViewModel.dialPlanValueSelected) {
													ForEach(registerViewModel.dialPlansLabelList, id: \.self) { dialPlan in
														Text(dialPlan).tag(dialPlan)
													}
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
						.frame(maxWidth: sharedMainViewModel.maxWidth)
					}
					.frame(maxWidth: .infinity)
					.padding(.top, 2)
				}
				.background(Color.gray100)
			}
			.background(Color.gray100)
		}
		.navigationTitle("")
		.navigationBarHidden(true)
	}
	
	func saveImage() {
		accountProfileViewModel.saveImage(
			image: selectedImage
			?? ContactsManager.shared.textToImage(
				firstName: accountProfileViewModel.avatarModel!.name, lastName: ""),
			name: accountProfileViewModel.avatarModel!.name,
			prefix: ((selectedImage == nil) ? "-default" : ""))
	}
}
