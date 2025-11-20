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
import linphonesw

// swiftlint:disable type_body_length
struct HistoryContactFragment: View {
	
	@State private var orientation = UIDevice.current.orientation
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@EnvironmentObject var historyModel: HistoryModel
	@EnvironmentObject var historyListViewModel: HistoryListViewModel
	
	@State var isMenuOpen = false
	
	@Binding var isShowDeleteAllHistoryPopup: Bool
	@Binding var isShowEditContactFragment: Bool
	@Binding var isShowEditContactFragmentAddress: String
	
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
							.padding(.all, 10)
							.padding(.top, 2)
							.padding(.leading, -10)
							.onTapGesture {
								withAnimation {
									SharedMainViewModel.shared.displayedCall = nil
								}
							}
					}
					
					Text("history_title")
						.default_text_style_orange_800(styleSize: 20)
					
					Spacer()
					
					Menu {
						if !historyModel.isConf {
							Button {
								isMenuOpen = false
								
								SharedMainViewModel.shared.displayedCall = nil
								SharedMainViewModel.shared.changeIndexView(indexViewInt: 0)
								
								if historyModel.isFriend {
									let friendIndex = contactsManager.avatarListModel.first(where: {$0.addresses.contains(where: {$0 == historyModel.address})})
									if friendIndex != nil {
										withAnimation {
											SharedMainViewModel.shared.displayedFriend = friendIndex
										}
									}
								} else {
									withAnimation {
										isShowEditContactFragment.toggle()
										isShowEditContactFragmentAddress = String(historyModel.address.dropFirst(4))
									}
								}
							} label: {
								HStack {
									Text(historyModel.isFriend ? "menu_see_existing_contact" : "menu_add_address_to_contacts")
									Spacer()
									Image(historyModel.isFriend ? "user-circle" : "plus-circle")
										.resizable()
										.frame(width: 25, height: 25, alignment: .leading)
										.padding(.all, 10)
								}
							}
						}
						
						Button {
							isMenuOpen = false
							
							if historyModel.isOutgoing {
								UIPasteboard.general.setValue(
									historyModel.address.dropFirst(4),
									forPasteboardType: UTType.plainText.identifier
								)
							} else {
								UIPasteboard.general.setValue(
									historyModel.address.dropFirst(4),
									forPasteboardType: UTType.plainText.identifier
								)
							}
							
							ToastViewModel.shared.toastMessage = "Success_address_copied_into_clipboard"
							ToastViewModel.shared.displayToast.toggle()
							
						} label: {
							HStack {
								Text("menu_copy_sip_address")
								Spacer()
								Image("copy")
									.resizable()
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
							}
						}
						
						Button(role: .destructive) {
							isMenuOpen = false
							historyListViewModel.callLogsAddressToDelete = historyModel.address
							isShowDeleteAllHistoryPopup.toggle()
							
						} label: {
							HStack {
								Text("menu_delete_history")
								Spacer()
								Image("trash-simple-red")
									.resizable()
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
							}
						}
					} label: {
						Image("dots-three-vertical")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.orangeMain500)
							.frame(width: 25, height: 25, alignment: .leading)
							.padding(.all, 10)
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
							if #unavailable(iOS 16.0) {
								Rectangle()
									.foregroundColor(Color.gray100)
									.frame(height: 7)
							}
							
							VStack(spacing: 0) {
								if !historyModel.isConf {
									if let avatarModel = historyModel.avatarModel {
										Avatar(contactAvatarModel: avatarModel, avatarSize: 100)
									}
									
									Text(historyModel.addressName)
										.foregroundStyle(Color.grayMain2c700)
										.multilineTextAlignment(.center)
										.default_text_style(styleSize: 14)
										.frame(maxWidth: .infinity)
										.padding(.top, 10)
									
									if !CorePreferences.hideSipAddresses {
										Text(historyModel.address)
											.foregroundStyle(Color.grayMain2c700)
											.multilineTextAlignment(.center)
											.default_text_style(styleSize: 14)
											.frame(maxWidth: .infinity)
											.padding(.top, 5)
									}
									
									if let avatar = historyModel.avatarModel {
										AvatarPresenceView(avatarModel: avatar)
									} else {
										Text("")
											.multilineTextAlignment(.center)
											.default_text_style_300(styleSize: 12)
											.frame(maxWidth: .infinity)
											.frame(height: 20)
									}
								} else {
									VStack {
										Image("video-conference")
											.renderingMode(.template)
											.resizable()
											.frame(width: 60, height: 60)
											.foregroundStyle(Color.grayMain2c600)
									}
									.frame(width: 100, height: 100)
									.background(Color.grayMain2c200)
									.clipShape(Circle())
									
									Text(historyModel.subject)
										.foregroundStyle(Color.grayMain2c700)
										.multilineTextAlignment(.center)
										.default_text_style(styleSize: 14)
										.frame(maxWidth: .infinity)
										.padding(.top, 10)
								}
							}
							.frame(minHeight: 150)
							.frame(maxWidth: .infinity)
							.padding(.top, 10)
							.padding(.bottom, 2)
							.background(Color.gray100)
							
							HStack {
								Spacer()
								
								if !historyModel.isConf {
									Button(action: {
										telecomManager.doCallOrJoinConf(address: historyModel.addressLinphone)
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
												.frame(minWidth: 80)
										}
									})
                                    
                                    if !CorePreferences.disableChatFeature {
                                        Spacer()
                                        
                                        Button(action: {
                                            historyListViewModel.createOneToOneChatRoomWith(remote: historyModel.addressLinphone)
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
                                                    .frame(minWidth: 80)
                                            }
                                        })
                                    }
                                    
									Spacer()
									
									Button(action: {
										telecomManager.doCallOrJoinConf(address: historyModel.addressLinphone, isVideo: true)
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
												.frame(minWidth: 80)
										}
									})
								} else {
									Button(action: {
										withAnimation {
											if historyModel.address.hasPrefix("sip:conference-focus@sip.linphone.org") {
												do {
													let meetingAddress = try Factory.Instance.createAddress(addr: historyModel.address)
													
													telecomManager.meetingWaitingRoomDisplayed = true
													telecomManager.meetingWaitingRoomSelected = meetingAddress
												} catch {}
											} else {
												telecomManager.doCallOrJoinConf(address: historyModel.addressLinphone)
											}
										}
									}, label: {
										VStack {
											HStack(alignment: .center) {
												Image("video-conference")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(Color.grayMain2c600)
													.frame(width: 25, height: 25)
											}
											.padding(16)
											.background(Color.grayMain2c200)
											.cornerRadius(40)
											
											Text("meeting_waiting_room_join")
												.default_text_style(styleSize: 14)
												.frame(minWidth: 80)
										}
									})
								}
								
								Spacer()
							}
							.padding(.top, 20)
							.padding(.bottom, 10)
							.frame(maxWidth: .infinity)
							.background(Color.gray100)
							
							VStack(spacing: 0) {
								let callLogsFilter = historyListViewModel.callLogs.filter({ $0.address == historyModel.address })
								
								ForEach(0..<callLogsFilter.count, id: \.self) { index in
									HStack {
										VStack {
											Image(historyListViewModel.getCallIconResId(callStatus: callLogsFilter[index].status, isOutgoing: callLogsFilter[index].isOutgoing))
												.resizable()
												.frame(
													width: historyListViewModel.getCallIconResId(
														callStatus: callLogsFilter[index].status,
														isOutgoing: callLogsFilter[index].isOutgoing
													).contains("rejected") ? 12 : 8,
													height: historyListViewModel.getCallIconResId(
														callStatus: callLogsFilter[index].status,
														isOutgoing: callLogsFilter[index].isOutgoing
													).contains("rejected") ? 6 : 8)
												.padding(.top, 6)
											
											Spacer()
										}
										
										VStack {
											Text(historyListViewModel.getCallText(
												callStatus: callLogsFilter[index].status,
												isOutgoing: callLogsFilter[index].isOutgoing)
											)
											.default_text_style(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
											
											Text(historyListViewModel.getCallTime(startDate: callLogsFilter[index].startDate))
												.foregroundStyle(
													callLogsFilter[index].status != .Success
													? Color.redDanger500
													: Color.grayMain2c600
												)
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
									.frame(maxHeight: 65)
								}
							}
							.background(.white)
							.cornerRadius(15)
							.padding(.all)
						}
						.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
					}
					.frame(maxWidth: .infinity)
					.padding(.top, 2)
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

struct AvatarPresenceView: View {
	@ObservedObject var avatarModel: ContactAvatarModel

	var body: some View {
		Text(avatarModel.lastPresenceInfo)
			.foregroundStyle(avatarModel.lastPresenceInfo == "Online" ? Color.greenSuccess500 : Color.orangeWarning600)
			.multilineTextAlignment(.center)
			.default_text_style_300(styleSize: 12)
			.frame(maxWidth: .infinity)
			.frame(height: 20)
			.padding(.top, 5)
	}
}


#Preview {
	HistoryContactFragment(
		isShowDeleteAllHistoryPopup: .constant(false),
		isShowEditContactFragment: .constant(false),
		isShowEditContactFragmentAddress: .constant("")
	)
}
// swiftlint:enable type_body_length
