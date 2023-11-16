/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
 *
 * This file is part of Linphone
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

struct HistoryContactFragment: View {
	
	@State private var orientation = UIDevice.current.orientation
	
	@ObservedObject var sharedMainViewModel = SharedMainViewModel()
	@ObservedObject var historyViewModel: HistoryViewModel
	
	@State var isMenuOpen = false
	
	@Binding var isShowDeleteAllHistoryPopup: Bool
	
	var body: some View {
		NavigationView {
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
							.padding(.top, 2)
							.onTapGesture {
								withAnimation {
									historyViewModel.displayedCall = nil
								}
							}
					}
					
					Text("Call history")
						.default_text_style_orange_800(styleSize: 20)
					
					Spacer()
					
					Menu {
						Button {
							isMenuOpen = false
						} label: {
							HStack {
								Text("See all")
								Spacer()
								Image("green-check")
									.resizable()
									.frame(width: 25, height: 25, alignment: .leading)
							}
						}
						
						Button {
							isMenuOpen = false
						} label: {
							HStack {
								Text("See Linphone contact")
								Spacer()
								Image("green-check")
									.resizable()
									.frame(width: 25, height: 25, alignment: .leading)
							}
						}
						
						Button(role: .destructive) {
							isMenuOpen = false
						} label: {
							HStack {
								Text("Delete all history")
								Spacer()
								Image("trash-simple-red")
									.resizable()
									.frame(width: 25, height: 25, alignment: .leading)
							}
						}
					} label: {
						Image("dots-three-vertical")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.orangeMain500)
							.frame(width: 25, height: 25, alignment: .leading)
					}
					.padding(.leading)
					.onTapGesture {
						isMenuOpen = true
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
								
								let fromAddressFriend = historyViewModel.displayedCall != nil ? ContactsManager.shared.getFriendWithAddress(address: historyViewModel.displayedCall!.fromAddress!) : nil
								let toAddressFriend = historyViewModel.displayedCall != nil ? ContactsManager.shared.getFriendWithAddress(address: historyViewModel.displayedCall!.toAddress!) : nil
								let addressFriend = historyViewModel.displayedCall != nil ? (historyViewModel.displayedCall!.dir == .Incoming ? fromAddressFriend : toAddressFriend) : nil
								
								if historyViewModel.displayedCall != nil
									&& addressFriend != nil
									&& addressFriend!.photo != nil
									&& !addressFriend!.photo!.isEmpty {
									AsyncImage(
										url: ContactsManager.shared.getImagePath(
											friendPhotoPath: addressFriend!.photo!)) { image in
												switch image {
												case .empty:
													ProgressView()
														.frame(width: 100, height: 100)
												case .success(let image):
													image
														.resizable()
														.aspectRatio(contentMode: .fill)
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
								} else if historyViewModel.displayedCall != nil {
									if historyViewModel.displayedCall!.dir == .Outgoing && historyViewModel.displayedCall!.toAddress != nil {
										if historyViewModel.displayedCall!.toAddress!.displayName != nil {
											Image(uiImage: ContactsManager.shared.textToImage(
												firstName: historyViewModel.displayedCall!.toAddress!.displayName!,
												lastName: historyViewModel.displayedCall!.toAddress!.displayName!.components(separatedBy: " ").count > 1
												? historyViewModel.displayedCall!.toAddress!.displayName!.components(separatedBy: " ")[1]
												: ""))
											.resizable()
											.frame(width: 100, height: 100)
											.clipShape(Circle())
											
											Text(historyViewModel.displayedCall!.toAddress!.displayName!)
												.foregroundStyle(Color.grayMain2c700)
												.multilineTextAlignment(.center)
												.default_text_style(styleSize: 14)
												.frame(maxWidth: .infinity)
												.padding(.top, 10)
											
											Text("")
												.multilineTextAlignment(.center)
												.default_text_style_300(styleSize: 12)
												.frame(maxWidth: .infinity)
												.frame(height: 20)
										} else {
											Image(uiImage: ContactsManager.shared.textToImage(
												firstName: historyViewModel.displayedCall!.toAddress!.username ?? "Username Error",
												lastName: historyViewModel.displayedCall!.toAddress!.username!.components(separatedBy: " ").count > 1
												? historyViewModel.displayedCall!.toAddress!.username!.components(separatedBy: " ")[1]
												: ""))
											.resizable()
											.frame(width: 100, height: 100)
											.clipShape(Circle())
											
											Text(historyViewModel.displayedCall!.toAddress!.username ?? "Username Error")
												.foregroundStyle(Color.grayMain2c700)
												.multilineTextAlignment(.center)
												.default_text_style(styleSize: 14)
												.frame(maxWidth: .infinity)
												.padding(.top, 10)
											
											Text("")
												.multilineTextAlignment(.center)
												.default_text_style_300(styleSize: 12)
												.frame(maxWidth: .infinity)
												.frame(height: 20)
										}
										
									} else if historyViewModel.displayedCall!.fromAddress != nil {
										if historyViewModel.displayedCall!.fromAddress!.displayName != nil {
											Image(uiImage: ContactsManager.shared.textToImage(
												firstName: historyViewModel.displayedCall!.fromAddress!.displayName!,
												lastName: historyViewModel.displayedCall!.fromAddress!.displayName!.components(separatedBy: " ").count > 1
												? historyViewModel.displayedCall!.fromAddress!.displayName!.components(separatedBy: " ")[1]
												: ""))
											.resizable()
											.frame(width: 100, height: 100)
											.clipShape(Circle())
											
											Text(historyViewModel.displayedCall!.fromAddress!.displayName!)
												.foregroundStyle(Color.grayMain2c700)
												.multilineTextAlignment(.center)
												.default_text_style(styleSize: 14)
												.frame(maxWidth: .infinity)
												.padding(.top, 10)
											
											Text("")
												.multilineTextAlignment(.center)
												.default_text_style_300(styleSize: 12)
												.frame(maxWidth: .infinity)
												.frame(height: 20)
										} else {
											Image(uiImage: ContactsManager.shared.textToImage(
												firstName: historyViewModel.displayedCall!.fromAddress!.username ?? "Username Error",
												lastName: historyViewModel.displayedCall!.fromAddress!.username!.components(separatedBy: " ").count > 1
												? historyViewModel.displayedCall!.fromAddress!.username!.components(separatedBy: " ")[1]
												: ""))
											.resizable()
											.frame(width: 100, height: 100)
											.clipShape(Circle())
											
											Text(historyViewModel.displayedCall!.fromAddress!.username ?? "Username Error")
												.foregroundStyle(Color.grayMain2c700)
												.multilineTextAlignment(.center)
												.default_text_style(styleSize: 14)
												.frame(maxWidth: .infinity)
												.padding(.top, 10)
											
											Text("")
												.multilineTextAlignment(.center)
												.default_text_style_300(styleSize: 12)
												.frame(maxWidth: .infinity)
												.frame(height: 20)
										}
									}
								}
								if historyViewModel.displayedCall != nil
									&& addressFriend != nil
									&& addressFriend!.name != nil {
									Text((addressFriend!.name)!)
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
										.frame(height: 20)
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
							
							VStack(spacing: 0) {
								
								let fromAddressFriend = historyViewModel.displayedCall != nil ? ContactsManager.shared.getFriendWithAddress(address: historyViewModel.displayedCall!.fromAddress!) : nil
								let toAddressFriend = historyViewModel.displayedCall != nil ? ContactsManager.shared.getFriendWithAddress(address: historyViewModel.displayedCall!.toAddress!) : nil
								let addressFriend = historyViewModel.displayedCall != nil ? (historyViewModel.displayedCall!.dir == .Incoming ? fromAddressFriend : toAddressFriend) : nil
								
								if historyViewModel.displayedCall != nil && addressFriend != nil && addressFriend != nil {
									ForEach(0..<addressFriend!.addresses.count, id: \.self) { index in
										Button {
										} label: {
											HStack {
												VStack {
													Text("SIP address :")
														.default_text_style_700(styleSize: 14)
														.frame(maxWidth: .infinity, alignment: .leading)
													Text(addressFriend!.addresses[index].asStringUriOnly().dropFirst(4))
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
										
										if !addressFriend!.phoneNumbers.isEmpty
											|| index < addressFriend!.addresses.count - 1 {
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
	HistoryContactFragment(historyViewModel: HistoryViewModel(), isShowDeleteAllHistoryPopup: .constant(false))
}
