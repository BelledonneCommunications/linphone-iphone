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

struct AccountProfileFragment: View {
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	
	@ObservedObject var accountProfileViewModel: AccountProfileViewModel
	
	@Binding var isShowAccountProfileFragment: Bool
	@State var detailIsOpen: Bool = true
	
	@State private var showPhotoPicker = false
	@State private var selectedImage: UIImage?
	@State private var removedImage = false
	
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
				
				Spacer()
				/*
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
										&& !accountProfileViewModel.photoAvatarModel!.isEmpty && selectedImage == nil && !removedImage {
										Avatar(
											contactAvatarModel: ContactAvatarModel(accountProfileViewModel.avatarModel!),
											avatarSize: 100
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
								.padding(.bottom, 2)
								.background(Color.gray100)
							}
							
							HStack(alignment: .center) {
								Text("conversation_info_participants_list_title")
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
				 */
			}
			.background(Color.gray100)
		}
		.navigationTitle("")
		.navigationBarHidden(true)
	}
}
