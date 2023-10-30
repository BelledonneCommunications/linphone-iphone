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

struct ContactInnerFragment: View {
	
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel()
	
	@ObservedObject var contactViewModel: ContactViewModel
	@ObservedObject var editContactViewModel: EditContactViewModel
	@ObservedObject var magicSearch = MagicSearchSingleton.shared
	
	@State private var orientation = UIDevice.current.orientation
	
	@State private var informationIsOpen = true
	
	@Binding var isShowDeletePopup: Bool
	@Binding var showingSheet: Bool
	@Binding var isShowDismissPopup: Bool
	
	var body: some View {
		NavigationView {
			VStack(spacing: 1) {
				Rectangle()
					.foregroundColor(Color.orangeMain500)
					.edgesIgnoringSafeArea(.top)
					.frame(height: 0)
				
				HStack {
					if !(orientation == .landscapeLeft
						 || orientation == .landscapeRight
						 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
						Image("caret-left")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.orangeMain500)
							.frame(width: 25, height: 25, alignment: .leading)
							.padding(.top, 2)
							.onTapGesture {
								withAnimation {
									contactViewModel.indexDisplayedFriend = nil
								}
							}
					}
					
					Spacer()
					
					NavigationLink(destination: EditContactFragment(editContactViewModel: editContactViewModel, isShowEditContactFragment: .constant(false), isShowDismissPopup: $isShowDismissPopup)) {
						Image("pencil-simple")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.orangeMain500)
							.frame(width: 25, height: 25, alignment: .leading)
							.padding(.top, 2)
					}
					.simultaneousGesture(
						TapGesture().onEnded {
							editContactViewModel.selectedEditFriend = magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend
							editContactViewModel.resetValues()
						}
					)
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
								if contactViewModel.indexDisplayedFriend != nil  
									&& magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
									&& magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.photo != nil
									&& !magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.photo!.isEmpty {
									AsyncImage(url: ContactsManager.shared.getImagePath(friendPhotoPath: magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.photo!)) { image in
										switch image {
										case .empty:
											ProgressView()
												.frame(width: 100, height: 100)
										case .success(let image):
											image
												.resizable()
												.frame(width: 100, height: 100)
												.clipShape(Circle())
										case .failure:
											Image("profil-picture-default")
												.resizable()
												.frame(width: 100, height: 100)
												.clipShape(Circle())
										@unknown default:
											EmptyView()
										}
									}
								} else if contactViewModel.indexDisplayedFriend != nil && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil {
									Image("profil-picture-default")
										.resizable()
										.frame(width: 100, height: 100)
										.clipShape(Circle())
								}
								if contactViewModel.indexDisplayedFriend != nil
									&& magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
									&& magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend?.name != nil {
									Text((magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend?.name)!)
										.foregroundStyle(Color.grayMain2c700)
										.multilineTextAlignment(.center)
										.default_text_style(styleSize: 14)
										.frame(maxWidth: .infinity)
										.padding(.top, 10)
									
									Text("En ligne")
										.foregroundStyle(Color.greenSuccess500)
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
									
								}, label: {
									VStack {
										HStack(alignment: .center) {
											Image("phone")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(Color.grayMain2c600)
												.frame(width: 25, height: 25)
												.onTapGesture {
													withAnimation {
														
													}
												}
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
												.foregroundStyle(Color.grayMain2c600)
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
									
								}, label: {
									VStack {
										HStack(alignment: .center) {
											Image("video-camera")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(Color.grayMain2c600)
												.frame(width: 25, height: 25)
												.onTapGesture {
													withAnimation {
														
													}
												}
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
							
							HStack(alignment: .center) {
								Text("Information")
									.default_text_style_800(styleSize: 16)
								
								Spacer()
								
								Image(informationIsOpen ? "caret-up" : "caret-down")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 25, height: 25, alignment: .leading)
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
									if contactViewModel.indexDisplayedFriend != nil && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil {
										ForEach(0..<magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.addresses.count, id: \.self) { index in
											Button {
											} label: {
												HStack {
													VStack {
														Text("SIP address :")
															.default_text_style_700(styleSize: 14)
															.frame(maxWidth: .infinity, alignment: .leading)
														Text(magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.addresses[index].asStringUriOnly().dropFirst(4))
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
														.onTapGesture {
															withAnimation {
																
															}
														}
												}
												.padding(.vertical, 15)
												.padding(.horizontal, 20)
											}
											.simultaneousGesture(
												LongPressGesture()
													.onEnded { _ in
														contactViewModel.stringToCopy = magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.addresses[index].asStringUriOnly()
														showingSheet.toggle()
													}
											)
											.highPriorityGesture(
												TapGesture()
													.onEnded { _ in
														withAnimation {
															
														}
													}
											)
											
											if !magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbers.isEmpty 
												|| index < magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.addresses.count - 1 {
												VStack {
													Divider()
												}
												.padding(.horizontal)
											}
										}
										
										ForEach(0..<magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbers.count, id: \.self) { index in
											Button {
											} label: {
												HStack {
													VStack {
														if magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].label != nil 
															&& !magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].label!.isEmpty {
															Text("Phone (\(magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].label!)) :")
																.default_text_style_700(styleSize: 14)
																.frame(maxWidth: .infinity, alignment: .leading)
														} else {
															Text("Phone :")
															 .default_text_style_700(styleSize: 14)
															 .frame(maxWidth: .infinity, alignment: .leading)
														}
														Text(magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].phoneNumber)
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
														.onTapGesture {
															withAnimation {
																
															}
														}
												}
												.padding(.vertical, 15)
												.padding(.horizontal, 20)
											}
											.simultaneousGesture(
												LongPressGesture()
													.onEnded { _ in
														contactViewModel.stringToCopy = magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].phoneNumber
														showingSheet.toggle()
													}
											)
											.highPriorityGesture(
												TapGesture()
													.onEnded { _ in
														withAnimation {
															
														}
													}
											)
											
											if index < magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbers.count - 1 {
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
							
							if contactViewModel.indexDisplayedFriend != nil
								&& magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
								&& ((magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization != nil
									 && !magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization!.isEmpty) 
									|| (magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle != nil
																									  && !magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle!.isEmpty)) {
								VStack {
									if magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization != nil 
										&& !magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization!.isEmpty {
										Text("**Company :** \(magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization!)")
											.default_text_style(styleSize: 14)
											.padding(.vertical, 15)
											.padding(.horizontal, 20)
											.frame(maxWidth: .infinity, alignment: .leading)
									}
									
									if magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle != nil && !magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle!.isEmpty {
										Text("**Job :** \(magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle!)")
											.default_text_style(styleSize: 14)
											.padding(.vertical, 15)
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
								Text("Other actions")
									.default_text_style_800(styleSize: 16)
								
								Spacer()
							}
							.padding(.vertical, 10)
							.padding(.horizontal, 16)
							.background(Color.gray100)
							
							VStack(spacing: 0) {
								Button {
								} label: {
									HStack {
										Image("pencil-simple")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c600)
											.frame(width: 25, height: 25)
										
										Text("Edit")
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
									if magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil {
										contactViewModel.objectWillChange.send()
										magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.starred.toggle()
									}
								} label: {
									HStack {
										Image(contactViewModel.indexDisplayedFriend != nil && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
											  && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.starred == true ? "heart-fill" : "heart")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(contactViewModel.indexDisplayedFriend != nil
															 && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil 
															 && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.starred == true ? Color.redDanger500 : Color.grayMain2c500)
											.frame(width: 25, height: 25)
										Text(contactViewModel.indexDisplayedFriend != nil
											 && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
											 && magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.starred == true
											 ? "Remove from favourites"
											 : "Add to favourites")
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
								} label: {
									HStack {
										Image("share-network")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c600)
											.frame(width: 25, height: 25)
										
										Text("Share")
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
								
								/*
								 Button {
								 } label: {
								 HStack {
								 Image("bell-simple-slash")
								 .renderingMode(.template)
								 .resizable()
								 .foregroundStyle(Color.grayMain2c600)
								 .frame(width: 25, height: 25)
								 
								 Text("Mute")
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
								 } label: {
								 HStack {
								 Image("x-circle")
								 .renderingMode(.template)
								 .resizable()
								 .foregroundStyle(Color.grayMain2c600)
								 .frame(width: 25, height: 25)
								 
								 Text("Block")
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
								 */
								
								Button {
									if magicSearch.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil {
										isShowDeletePopup.toggle()
									}
								} label: {
									HStack {
										Image("trash-simple")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.redDanger500)
											.frame(width: 25, height: 25)
										
										Text("Delete this contact")
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
		}
		.navigationViewStyle(.stack)
	}
}

#Preview {
	ContactInnerFragment(
		contactViewModel: ContactViewModel(),
		editContactViewModel: EditContactViewModel(),
		isShowDeletePopup: .constant(false),
		showingSheet: .constant(false),
		isShowDismissPopup: .constant(false)
	)
}
