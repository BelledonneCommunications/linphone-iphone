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
import UniformTypeIdentifiers

struct HistoryContactFragment: View {
	
	@State private var orientation = UIDevice.current.orientation
	
	@ObservedObject var sharedMainViewModel = SharedMainViewModel()
	@ObservedObject var historyViewModel: HistoryViewModel
    @ObservedObject var historyListViewModel: HistoryListViewModel
    @ObservedObject var contactViewModel: ContactViewModel
    @ObservedObject var editContactViewModel: EditContactViewModel
	
	@State var isMenuOpen = false
	
	@Binding var isShowDeleteAllHistoryPopup: Bool
    @Binding var isShowEditContactFragment: Bool
    @Binding var indexPage: Int
	
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
                        let fromAddressFriend = historyViewModel.displayedCall != nil ? ContactsManager.shared.getFriendWithAddress(address: historyViewModel.displayedCall!.fromAddress!) : nil
                        let toAddressFriend = historyViewModel.displayedCall != nil ? ContactsManager.shared.getFriendWithAddress(address: historyViewModel.displayedCall!.toAddress!) : nil
                        let addressFriend = historyViewModel.displayedCall != nil ? (historyViewModel.displayedCall!.dir == .Incoming ? fromAddressFriend : toAddressFriend) : nil
                        
						Button {
							isMenuOpen = false
                            
                            indexPage = 0
                            
                            if ContactsManager.shared.getFriendWithAddress(
                                address: historyViewModel.displayedCall != nil && historyViewModel.displayedCall!.dir == .Outgoing
                                   ? historyViewModel.displayedCall!.toAddress!
                                   : historyViewModel.displayedCall!.fromAddress!
                            ) != nil {
                                let addressCall = historyViewModel.displayedCall != nil && historyViewModel.displayedCall!.dir == .Outgoing
                                ? historyViewModel.displayedCall!.toAddress!
                                : historyViewModel.displayedCall!.fromAddress!
                                
                                let friendIndex = MagicSearchSingleton.shared.lastSearch.firstIndex(
                                    where: {$0.friend!.addresses.contains(where: {$0.asStringUriOnly() == addressCall.asStringUriOnly()})})
                                if friendIndex != nil {
                                    
                                    withAnimation {
										historyViewModel.displayedCall = nil
                                        contactViewModel.indexDisplayedFriend = friendIndex
                                    }
                                }
                            } else {
                                let addressCall = historyViewModel.displayedCall != nil && historyViewModel.displayedCall!.dir == .Outgoing
                                ? historyViewModel.displayedCall!.toAddress!
                                : historyViewModel.displayedCall!.fromAddress!
                                
                                withAnimation {
									historyViewModel.displayedCall = nil
                                    isShowEditContactFragment.toggle()
                                    editContactViewModel.sipAddresses.removeAll()
                                    editContactViewModel.sipAddresses.append(String(addressCall.asStringUriOnly().dropFirst(4)))
                                    editContactViewModel.sipAddresses.append("")
                                }
                            }
                            
						} label: {
							HStack {
                                Text(addressFriend != nil ? "See contact" : "Add to contacts")
								Spacer()
                                Image(addressFriend != nil ? "user-circle" : "plus-circle")
									.resizable()
									.frame(width: 25, height: 25, alignment: .leading)
							}
						}
						
						Button {
							isMenuOpen = false
                            
                            if historyViewModel.displayedCall != nil && historyViewModel.displayedCall!.dir == .Outgoing {
                                UIPasteboard.general.setValue(
                                    historyViewModel.displayedCall!.toAddress!.asStringUriOnly().dropFirst(4),
                                    forPasteboardType: UTType.plainText.identifier
                                )
                            } else {
                                UIPasteboard.general.setValue(
                                    historyViewModel.displayedCall!.fromAddress!.asStringUriOnly().dropFirst(4),
                                    forPasteboardType: UTType.plainText.identifier
                                )
                            }
						} label: {
							HStack {
								Text("Copy SIP address")
								Spacer()
								Image("copy")
									.resizable()
									.frame(width: 25, height: 25, alignment: .leading)
							}
						}
						
						Button(role: .destructive) {
							isMenuOpen = false
							
							if historyViewModel.displayedCall != nil && historyViewModel.displayedCall!.dir == .Outgoing {
								historyListViewModel.callLogsAddressToDelete = historyViewModel.displayedCall!.toAddress!.asStringUriOnly()
							} else {
								historyListViewModel.callLogsAddressToDelete = historyViewModel.displayedCall!.fromAddress!.asStringUriOnly()
							}
							
							isShowDeleteAllHistoryPopup.toggle()
							
						} label: {
							HStack {
								Text("Delete history")
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
                                            
                                            Text(historyViewModel.displayedCall!.toAddress!.asStringUriOnly())
                                                .foregroundStyle(Color.grayMain2c700)
                                                .multilineTextAlignment(.center)
                                                .default_text_style(styleSize: 14)
                                                .frame(maxWidth: .infinity)
                                                .padding(.top, 5)
											
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
                                            
                                            Text(historyViewModel.displayedCall!.toAddress!.asStringUriOnly())
                                                .foregroundStyle(Color.grayMain2c700)
                                                .multilineTextAlignment(.center)
                                                .default_text_style(styleSize: 14)
                                                .frame(maxWidth: .infinity)
                                                .padding(.top, 5)
											
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
                                            
                                            Text(historyViewModel.displayedCall!.fromAddress!.asStringUriOnly())
                                                .foregroundStyle(Color.grayMain2c700)
                                                .multilineTextAlignment(.center)
                                                .default_text_style(styleSize: 14)
                                                .frame(maxWidth: .infinity)
                                                .padding(.top, 5)
                                            
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
                                            
                                            Text(historyViewModel.displayedCall!.fromAddress!.asStringUriOnly())
                                                .foregroundStyle(Color.grayMain2c700)
                                                .multilineTextAlignment(.center)
                                                .default_text_style(styleSize: 14)
                                                .frame(maxWidth: .infinity)
                                                .padding(.top, 5)
                                            
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
                                    
                                    if historyViewModel.displayedCall!.dir == .Outgoing && historyViewModel.displayedCall!.toAddress != nil {
                                        Text(historyViewModel.displayedCall!.toAddress!.asStringUriOnly())
                                            .foregroundStyle(Color.grayMain2c700)
                                            .multilineTextAlignment(.center)
                                            .default_text_style(styleSize: 14)
                                            .frame(maxWidth: .infinity)
                                            .padding(.top, 5)
                                    } else if historyViewModel.displayedCall!.fromAddress != nil {
                                        Text(historyViewModel.displayedCall!.fromAddress!.asStringUriOnly())
                                            .foregroundStyle(Color.grayMain2c700)
                                            .multilineTextAlignment(.center)
                                            .default_text_style(styleSize: 14)
                                            .frame(maxWidth: .infinity)
                                            .padding(.top, 5)
                                    }
									
									Text("En ligne")
										.foregroundStyle(Color.greenSuccess500)
										.multilineTextAlignment(.center)
										.default_text_style_300(styleSize: 12)
										.frame(maxWidth: .infinity)
										.frame(height: 20)
                                        .padding(.top, 5)
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
                                
                                let addressFriend = historyViewModel.displayedCall != nil 
                                ? (historyViewModel.displayedCall!.dir == .Incoming ? historyViewModel.displayedCall!.fromAddress!.asStringUriOnly()
                                   : historyViewModel.displayedCall!.toAddress!.asStringUriOnly()) : nil
                                
                                let callLogsFilter = historyListViewModel.callLogs.filter({ $0.dir == .Incoming
                                    ? $0.fromAddress!.asStringUriOnly() == addressFriend
                                    : $0.toAddress!.asStringUriOnly() == addressFriend })
                                
                                ForEach(0..<callLogsFilter.count, id: \.self) { index in
                                        HStack {
                                            VStack {
                                                Image(historyListViewModel.getCallIconResId(callStatus: callLogsFilter[index].status, callDir: callLogsFilter[index].dir))
                                                 .resizable()
                                                 .frame(
                                                    width: historyListViewModel.getCallIconResId(
                                                        callStatus: callLogsFilter[index].status,
                                                        callDir: callLogsFilter[index].dir
                                                    ).contains("rejected") ? 12 : 8,
                                                    height: historyListViewModel.getCallIconResId(
                                                        callStatus: callLogsFilter[index].status,
                                                        callDir: callLogsFilter[index].dir
                                                    ).contains("rejected") ? 6 : 8)
                                                 .padding(.top, 5)
                                                Spacer()
                                            }
                                            
                                            VStack {
                                                Text(historyListViewModel.getCallText(
                                                    callStatus: callLogsFilter[index].status,
                                                    callDir: callLogsFilter[index].dir)
                                                )
                                                .default_text_style(styleSize: 14)
                                                .frame(maxWidth: .infinity, alignment: .leading)
                                                
                                                Text(historyListViewModel.getCallTime(startDate: callLogsFilter[index].startDate))
                                                    .foregroundStyle(callLogsFilter[index].status != .Success ? Color.redDanger500 : Color.grayMain2c600)
                                                    .default_text_style_300(styleSize: 12)
                                                    .frame(maxWidth: .infinity, alignment: .leading)
                                            }
                                            
                                            VStack {
                                                Spacer()
                                                Text(callLogsFilter[index].duration.convertDurationToString())
                                                    .default_text_style_300(styleSize: 12)
                                                Spacer()
                                            }
                                        }
                                        .padding(.vertical, 15)
                                        .padding(.horizontal, 20)
                                }
							}
							.background(.white)
							.cornerRadius(15)
							.padding(.all)
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
    HistoryContactFragment(
        historyViewModel: HistoryViewModel(),
        historyListViewModel: HistoryListViewModel(),
        contactViewModel: ContactViewModel(),
        editContactViewModel: EditContactViewModel(),
        isShowDeleteAllHistoryPopup: .constant(false),
        isShowEditContactFragment: .constant(false),
        indexPage: .constant(1)
    )
}
