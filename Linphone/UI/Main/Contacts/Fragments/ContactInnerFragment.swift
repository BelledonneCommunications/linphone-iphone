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
	
	@ObservedObject var contactViewModel: ContactViewModel
	
	@State private var orientation = UIDevice.current.orientation
	
	@State private var informationIsOpen = true
	
	@Binding var isShowDeletePopup: Bool
	
	@Binding var showingSheet: Bool
	
	var body: some View {
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
								contactViewModel.displayedFriend = nil
							}
						}
				}
				
				Spacer()
				
				Image("pencil-simple")
					.renderingMode(.template)
					.resizable()
					.foregroundStyle(Color.orangeMain500)
					.frame(width: 25, height: 25, alignment: .leading)
					.padding(.top, 2)
					.onTapGesture {
						withAnimation {
							
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
						if contactViewModel.displayedFriend != nil
							&& contactViewModel.displayedFriend!.photo != nil
							&& !contactViewModel.displayedFriend!.photo!.isEmpty {
							AsyncImage(url: URL(string: contactViewModel.displayedFriend!.photo!)) { image in
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
						} else if contactViewModel.displayedFriend != nil {
							Image("profil-picture-default")
								.resizable()
								.frame(width: 100, height: 100)
								.clipShape(Circle())
						}
						if contactViewModel.displayedFriend != nil && contactViewModel.displayedFriend?.name != nil {
							Text((contactViewModel.displayedFriend?.name)!)
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
							if contactViewModel.displayedFriend != nil {
								ForEach(0..<contactViewModel.displayedFriend!.addresses.count, id: \.self) { index in
									Button {
									} label: {
										HStack {
											VStack {
												Text("SIP address :")
													.default_text_style_700(styleSize: 14)
													.frame(maxWidth: .infinity, alignment: .leading)
												Text(contactViewModel.displayedFriend!.addresses[index].asStringUriOnly().dropFirst(4))
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
												contactViewModel.stringToCopy = contactViewModel.displayedFriend!.addresses[index].asStringUriOnly()
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
									
									if !contactViewModel.displayedFriend!.phoneNumbers.isEmpty || index < contactViewModel.displayedFriend!.addresses.count - 1 {
										VStack {
											Divider()
										}
										.padding(.horizontal)
									}
								}
								
								ForEach(0..<contactViewModel.displayedFriend!.phoneNumbers.count, id: \.self) { index in
									Button {
									} label: {
										HStack {
											VStack {
												Text("Phone (\(contactViewModel.displayedFriend!.phoneNumbersWithLabel[index].label!)) :")
													.default_text_style_700(styleSize: 14)
													.frame(maxWidth: .infinity, alignment: .leading)
												Text(contactViewModel.displayedFriend!.phoneNumbersWithLabel[index].phoneNumber)
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
												contactViewModel.stringToCopy = contactViewModel.displayedFriend!.phoneNumbersWithLabel[index].phoneNumber
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
									
									if index < contactViewModel.displayedFriend!.phoneNumbers.count - 1 {
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
					
					if contactViewModel.displayedFriend != nil
						&& contactViewModel.displayedFriend!.organization != nil
						&& !contactViewModel.displayedFriend!.organization!.isEmpty {
						VStack {
							Text("**Company :** \(contactViewModel.displayedFriend!.organization!)")
								.default_text_style(styleSize: 14)
								.padding(.vertical, 15)
								.padding(.horizontal, 20)
								.frame(maxWidth: .infinity, alignment: .leading)
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
							if contactViewModel.displayedFriend != nil {
								contactViewModel.displayedFriend!.starred.toggle()
							}
						} label: {
							HStack {
								Image("heart")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 25, height: 25)
								Text(contactViewModel.displayedFriend != nil && contactViewModel.displayedFriend!.starred == true
									 ? "Remove to favourites"
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
						
						Button {
						} label: {
							HStack {
								Image("bell-ringing_slash")
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
								Image("empty")
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
						
						Button {
							if contactViewModel.displayedFriend != nil {
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
			}
			.background(Color.gray100)
		}
		.background(.white)
		.navigationBarHidden(true)
		.onRotate { newOrientation in
			orientation = newOrientation
		}
		
	}
}

#Preview {
	ContactInnerFragment(contactViewModel: ContactViewModel(), isShowDeletePopup: .constant(false), showingSheet: .constant(false))
}
