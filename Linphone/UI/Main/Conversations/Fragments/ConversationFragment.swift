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
import ElegantEmojiPicker

// swiftlint:disable line_length
// swiftlint:disable type_body_length
struct ConversationFragment: View {
	
	@Environment(\.scenePhase) var scenePhase
	@State private var orientation = UIDevice.current.orientation
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@EnvironmentObject var navigationManager: NavigationManager
	
	@ObservedObject var contactsManager = ContactsManager.shared
    
    @EnvironmentObject var conversationsListViewModel: ConversationsListViewModel
	@EnvironmentObject var accountProfileViewModel: AccountProfileViewModel
    
    @StateObject private var conversationViewModel = ConversationViewModel()
	
	@State var isMenuOpen = false
	@State private var isMuted: Bool = false
	
	@FocusState var isMessageTextFocused: Bool
	
	@State var offset: CGPoint = .zero
	
	private let ids: [String] = []
	
	@State private var displayFloatingButton = false
	
	@State private var areFilePickersOpen = false
	
	@State private var isShowPhotoLibrary = false
	@State private var isShowCamera = false
	@State private var isShowFilePicker = false
	
	@State private var mediasIsLoading = false
	@State private var voiceRecordingInProgress = false
	
	@State private var isShowConversationForwardMessageFragment = false
	@State private var isShowEphemeralFragment = false
	@State private var isShowInfoConversationFragment = false
	
	@Binding var isShowConversationFragment: Bool
	@Binding var isShowStartCallGroupPopup: Bool
	
	@State private var selectedCategoryIndex = 0
	
	@Binding var isShowEditContactFragment: Bool
	@Binding var isShowEditContactFragmentAddress: String
	
	@Binding var isShowScheduleMeetingFragment: Bool
    
    @State private var cachedConversation: ConversationModel?
	
	@Binding var isShowScheduleMeetingFragmentSubject: String
	@Binding var isShowScheduleMeetingFragmentParticipants: [SelectedAddressModel]
	
	@Binding var isShowConversationInfoPopup: Bool
	@Binding var conversationInfoPopupText: String
	
	@State var messageText: String = ""
	
	@State private var chosen: String?
 	@State private var showPicker = false
	@State private var isSheetVisible = false
	
	@State private var isImdnOrReactionsSheetVisible = false
	
	var body: some View {
		NavigationView {
			GeometryReader { geometry in
				if #available(iOS 16.0, *), idiom != .pad {
					innerView(geometry: geometry)
						.background(.white)
						.navigationBarHidden(true)
						.onRotate { newOrientation in
							orientation = newOrientation
						}
						.onAppear {
							displayedChatroomPeerAddr = SharedMainViewModel.shared.displayedConversation?.remoteSipUri
						}
						.onDisappear {
							displayedChatroomPeerAddr = nil
							conversationViewModel.removeConversationDelegate()
						}
						.sheet(isPresented: $conversationViewModel.isShowSelectedMessageToDisplayDetails, onDismiss: {
							conversationViewModel.isShowSelectedMessageToDisplayDetails = false
						}, content: {
							ImdnOrReactionsSheet(selectedCategoryIndex: $selectedCategoryIndex, isImdnOrReactionsSheetVisible: $isImdnOrReactionsSheetVisible)
								.environmentObject(conversationViewModel)
								.presentationDetents([.medium])
				 				.presentationDragIndicator(.visible)
						})
						.sheet(isPresented: $isShowPhotoLibrary, onDismiss: {
							isShowPhotoLibrary = false
						}, content: {
							PhotoPicker(filter: nil, limit: conversationViewModel.maxMediaCount - conversationViewModel.mediasToSend.count) { results in
								if results.isEmpty && conversationViewModel.mediasToSend.isEmpty {
									self.mediasIsLoading = false
								} else {
									PhotoPicker.convertToAttachmentArray(fromResults: results) { mediasOrNil, errorOrNil in
										if let error = errorOrNil {
											print(error)
										}
										
										if let medias = mediasOrNil {
											conversationViewModel.mediasToSend.append(contentsOf: medias)
										}
										
										self.mediasIsLoading = false
									}
								}
							}
							.edgesIgnoringSafeArea(.all)
						})
						.sheet(isPresented: $isShowFilePicker, onDismiss: {
							isShowFilePicker = false
						}, content: {
							FilePicker(onDocumentsPicked: { urlList in
								if urlList.isEmpty && conversationViewModel.mediasToSend.isEmpty {
									self.mediasIsLoading = false
								} else {
									FilePicker.convertToAttachmentArray(fromResults: urlList) { mediasOrNil, errorOrNil in
										if let error = errorOrNil {
											print(error)
										}
										
										if let medias = mediasOrNil {
											conversationViewModel.mediasToSend.append(contentsOf: medias)
										}
										self.mediasIsLoading = false
									}
								}
							})
							.edgesIgnoringSafeArea(.all)
						})
						.sheet(isPresented: $showPicker) {
							EmojiPickerView(selected: $chosen, isSheetVisible: $isSheetVisible)
								.presentationDetents([.medium])
								.edgesIgnoringSafeArea(.all)
						}
						.onChange(of: chosen ?? "") { newValue in
							if !newValue.isEmpty {
								conversationViewModel.sendReaction(emoji: newValue)
								chosen = nil
							}
						}
						.fullScreenCover(isPresented: $isShowCamera) {
							ImagePicker(selectedMedia: self.$conversationViewModel.mediasToSend)
								.environmentObject(conversationViewModel)
								.edgesIgnoringSafeArea(.all)
						}
						.background(Color.gray100.ignoresSafeArea(.keyboard))
				} else {
					innerView(geometry: geometry)
						.background(.white)
						.navigationBarHidden(true)
						.onRotate { newOrientation in
							orientation = newOrientation
						}
						.onAppear {
							displayedChatroomPeerAddr = SharedMainViewModel.shared.displayedConversation?.remoteSipUri
						}
						.onDisappear {
							displayedChatroomPeerAddr = nil
							conversationViewModel.removeConversationDelegate()
						}
						.halfSheet(showSheet: $conversationViewModel.isShowSelectedMessageToDisplayDetails) {
							ImdnOrReactionsSheet(selectedCategoryIndex: $selectedCategoryIndex, isImdnOrReactionsSheetVisible: $isImdnOrReactionsSheetVisible)
								.environmentObject(conversationViewModel)
						} onDismiss: {
							conversationViewModel.isShowSelectedMessageToDisplayDetails = false
						}
						.sheet(isPresented: $isShowPhotoLibrary, onDismiss: {
							isShowPhotoLibrary = false
						}, content: {
							PhotoPicker(filter: nil, limit: conversationViewModel.maxMediaCount - conversationViewModel.mediasToSend.count) { results in
								PhotoPicker.convertToAttachmentArray(fromResults: results) { mediasOrNil, errorOrNil in
									if let error = errorOrNil {
										print(error)
									}
									
									if let medias = mediasOrNil {
										conversationViewModel.mediasToSend.append(contentsOf: medias)
									}
									
									self.mediasIsLoading = false
								}
							}
							.edgesIgnoringSafeArea(.all)
						})
						.sheet(isPresented: $showPicker) {
							EmojiPickerView(selected: $chosen, isSheetVisible: $isSheetVisible)
								.edgesIgnoringSafeArea(.all)
						}
						.onChange(of: chosen ?? "") { newValue in
							if !newValue.isEmpty {
								conversationViewModel.sendReaction(emoji: newValue)
								chosen = nil
							}
						}
						.fullScreenCover(isPresented: $isShowCamera) {
							ImagePicker(selectedMedia: self.$conversationViewModel.mediasToSend)
								.environmentObject(conversationViewModel)
						}
						.background(Color.gray100.ignoresSafeArea(.keyboard))
				}
			}
			.onChange(of: scenePhase) { newPhase in
				if newPhase == .active {
					if SharedMainViewModel.shared.displayedConversation != nil && (navigationManager.peerAddr == nil || navigationManager.peerAddr!.contains(SharedMainViewModel.shared.displayedConversation!.remoteSipUri)) {
						conversationViewModel.resetDisplayedChatRoom()
					}
				}
			}
		}
		.navigationViewStyle(.stack)
		.onAppear {
			if let conv = SharedMainViewModel.shared.displayedConversation {
				cachedConversation = conv
			}
			
			if !SharedMainViewModel.shared.fileUrlsToShare.isEmpty {
				var urlList: [URL] = []
				SharedMainViewModel.shared.fileUrlsToShare.forEach { urlFile in
					urlList.append(URL(fileURLWithPath: urlFile))
				}
				
				FilePicker.convertToAttachmentArray(fromResults: urlList) { mediasOrNil, errorOrNil in
					if let error = errorOrNil {
						print(error)
					}
					
					if let medias = mediasOrNil {
						conversationViewModel.mediasToSend.append(contentsOf: medias)
					}
				}
				
				SharedMainViewModel.shared.fileUrlsToShare.removeAll()
			}
		}
	}
	
	// swiftlint:disable cyclomatic_complexity
	// swiftlint:disable function_body_length
	@ViewBuilder
	func innerView(geometry: GeometryProxy) -> some View {
		ZStack {
			VStack(spacing: 1) {
				if SharedMainViewModel.shared.displayedConversation != nil || cachedConversation != nil {
					Rectangle()
						.foregroundColor(Color.orangeMain500)
						.edgesIgnoringSafeArea(.top)
						.frame(height: 0)
					
					HStack {
						if (!(orientation == .landscapeLeft || orientation == .landscapeRight
							  || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)) || isShowConversationFragment {
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
										if isShowConversationFragment {
											isShowConversationFragment = false
										}
										SharedMainViewModel.shared.displayedConversation = nil
									}
								}
						}
						
                        Avatar(contactAvatarModel: SharedMainViewModel.shared.displayedConversation?.avatarModel ?? cachedConversation!.avatarModel, avatarSize: 50)
							.padding(.top, 4)
						
						VStack(spacing: 1) {
                            Text(SharedMainViewModel.shared.displayedConversation?.subject ?? cachedConversation!.subject)
								.default_text_style(styleSize: 16)
								.frame(maxWidth: .infinity, alignment: .leading)
								.padding(.top, 4)
								.lineLimit(1)
							
							if isMuted || conversationViewModel.ephemeralTime != NSLocalizedString("conversation_ephemeral_messages_duration_disabled", comment: "") {
								HStack {
									if isMuted {
										Image("bell-slash")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.orangeMain500)
											.frame(width: 16, height: 16, alignment: .trailing)
									}
									
									if conversationViewModel.ephemeralTime != NSLocalizedString("conversation_ephemeral_messages_duration_disabled", comment: "") {
										Image("clock-countdown")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.orangeMain500)
											.frame(width: 16, height: 16, alignment: .trailing)
										
										Text(conversationViewModel.ephemeralTime)
											.default_text_style(styleSize: 12)
											.padding(.leading, -2)
											.frame(maxWidth: .infinity, alignment: .leading)
											.lineLimit(1)
									}
									
									Spacer()
								}
							}
						}
						.background(.white)
						.onTapGesture {
							withAnimation {
								isShowInfoConversationFragment = true
							}
						}
						.padding(.vertical, 10)
						
						Spacer()
						
                        if !(SharedMainViewModel.shared.displayedConversation?.isReadOnly ?? cachedConversation!.isReadOnly) {
							Button {
								if SharedMainViewModel.shared.displayedConversation!.isGroup {
									isShowStartCallGroupPopup.toggle()
								} else {
									SharedMainViewModel.shared.displayedConversation!.call()
								}
							} label: {
								Image("phone")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
									.padding(.top, 4)
							}
						}
						
						Menu {
							Button {
								isMenuOpen = false
								withAnimation {
									isShowInfoConversationFragment = true
								}
							} label: {
								HStack {
									Text("conversation_menu_go_to_info")
									Spacer()
									Image("info")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.grayMain2c500)
										.frame(width: 25, height: 25, alignment: .leading)
										.padding(.all, 10)
								}
							}
							
                            if !(SharedMainViewModel.shared.displayedConversation?.isReadOnly ?? cachedConversation!.isReadOnly) {
								Button {
									isMenuOpen = false
									SharedMainViewModel.shared.displayedConversation!.toggleMute()
									isMuted = !isMuted
								} label: {
									HStack {
										Text(isMuted ? "conversation_action_unmute" : "conversation_action_mute")
										Spacer()
										Image(isMuted ? "bell-simple" : "bell-simple-slash")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c500)
											.frame(width: 25, height: 25, alignment: .leading)
											.padding(.all, 10)
									}
								}
								
								Button {
									isMenuOpen = false
									withAnimation {
										isShowEphemeralFragment = true
									}
								} label: {
									HStack {
										Text("conversation_menu_configure_ephemeral_messages")
										Spacer()
										Image("clock-countdown")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c500)
											.frame(width: 25, height: 25, alignment: .leading)
											.padding(.all, 10)
									}
								}
							}
						} label: {
							Image("dots-three-vertical")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c500)
								.frame(width: 25, height: 25, alignment: .leading)
								.padding(.all, 10)
								.padding(.top, 4)
								.onChange(of: isMuted) { _ in }
								.onAppear {
									isMuted = SharedMainViewModel.shared.displayedConversation!.isMuted
								}
						}
						.onTapGesture {
							isMenuOpen = true
						}
					}
					.frame(maxWidth: .infinity)
					.frame(height: 50)
					.padding(.horizontal)
					.padding(.bottom, 4)
					.background(.white)
					
					if #available(iOS 16.0, *) {
						ZStack(alignment: .bottomTrailing) {
							UIList(
								geometryProxy: geometry,
                                sections: conversationViewModel.conversationMessagesSection,
                                isMessageTextFocused: Binding(get: {
                                    isMessageTextFocused
                                }, set: { newValue in
                                    isMessageTextFocused = newValue
                                })
                            )
							.environmentObject(conversationViewModel)
							.environmentObject(conversationsListViewModel)
						}
						.onAppear {
							if conversationViewModel.displayedConversationUnreadMessagesCount > 0 {
								conversationViewModel.markAsRead()
							}
					  	}
						.onDisappear {
							conversationViewModel.resetMessage()
						}
					} else {
						ScrollViewReader { proxy in
							ZStack(alignment: .bottomTrailing) {
								List {
									if conversationViewModel.conversationMessagesSection.first != nil {
										let counter = conversationViewModel.conversationMessagesSection.first!.rows.count
										ForEach(0..<counter, id: \.self) { index in
											ChatBubbleView(eventLogMessage: conversationViewModel.conversationMessagesSection.first!.rows[index], geometryProxy: geometry)
												.environmentObject(conversationViewModel)
												.id(conversationViewModel.conversationMessagesSection.first!.rows[index].message.id)
												.listRowInsets(EdgeInsets(top: 2, leading: 10, bottom: 2, trailing: 10))
												.listRowSeparator(.hidden)
												.scaleEffect(x: 1, y: -1, anchor: .center)
												.onAppear {
													if index == counter - 1
														&& conversationViewModel.displayedConversationHistorySize > conversationViewModel.conversationMessagesSection.first!.rows.count {
														DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
															conversationViewModel.getOldMessages()
														}
													}
													
													if index == 0 {
														displayFloatingButton = false
														conversationViewModel.markAsRead()
													}
												}
												.onDisappear {
													if index == 0 {
														displayFloatingButton = true
													}
												}
										}
									}
								}
								.scaleEffect(x: 1, y: -1, anchor: .center)
								.listStyle(.plain)
								.onAppear {
									conversationViewModel.markAsRead()
								}
								
								if displayFloatingButton {
									Button {
										if conversationViewModel.conversationMessagesSection.first != nil && conversationViewModel.conversationMessagesSection.first!.rows.first != nil {
											withAnimation {
												proxy.scrollTo(conversationViewModel.conversationMessagesSection.first!.rows.first!.message.id)
											}
										}
									} label: {
										ZStack {
											
											Image("caret-double-down")
												.renderingMode(.template)
												.foregroundStyle(.white)
												.padding()
												.background(Color.orangeMain500)
												.clipShape(Circle())
												.shadow(color: .black.opacity(0.2), radius: 4)
											
											if conversationViewModel.displayedConversationUnreadMessagesCount > 0 {
												VStack {
													HStack {
														Spacer()
														
														HStack {
															Text(
																conversationViewModel.displayedConversationUnreadMessagesCount < 99
																? String(conversationViewModel.displayedConversationUnreadMessagesCount)
																: "99+"
															)
															.foregroundStyle(.white)
															.default_text_style(styleSize: 10)
															.lineLimit(1)
															
														}
														.frame(width: 18, height: 18)
														.background(Color.redDanger500)
														.cornerRadius(50)
													}
													
													Spacer()
												}
											}
										}
										
									}
									.frame(width: 50, height: 50)
									.padding()
								}
							}
							.onAppear {
								conversationViewModel.getMessages()
							}
							.onDisappear {
								conversationViewModel.resetMessage()
							}
						}
					}
					
					if !conversationViewModel.composingLabel.isEmpty {
						HStack {
							Text(conversationViewModel.composingLabel)
								.lineLimit(1)
								.default_text_style_300(styleSize: 15)
								.frame(maxWidth: .infinity, alignment: .leading)
								.padding(.horizontal, 10)
						}
						.onDisappear {
							conversationViewModel.composingLabel = ""
						}
						.transition(.move(edge: .bottom))
					}
					
                    if !(SharedMainViewModel.shared.displayedConversation?.isReadOnly ?? cachedConversation!.isReadOnly) {
						if conversationViewModel.messageToReply != nil {
							ZStack(alignment: .top) {
								HStack {
									VStack {
										(
											Text("conversation_reply_to_message_title")
											+ Text("**\(conversationViewModel.participantConversationModel.first(where: {$0.address == conversationViewModel.messageToReply!.message.address})?.name ?? "")**"))
										.default_text_style_300(styleSize: 15)
										.frame(maxWidth: .infinity, alignment: .leading)
										.padding(.bottom, 1)
										.lineLimit(1)
										
										if conversationViewModel.messageToReply!.message.text.isEmpty {
											Text(conversationViewModel.messageToReply!.message.attachmentsNames)
												.default_text_style_300(styleSize: 15)
												.frame(maxWidth: .infinity, alignment: .leading)
												.lineLimit(1)
										} else {
											Text("\(conversationViewModel.messageToReply!.message.text)")
												.default_text_style_300(styleSize: 15)
												.frame(maxWidth: .infinity, alignment: .leading)
												.lineLimit(1)
										}
									}
								}
								.frame(maxWidth: .infinity)
								.padding(.all, 20)
								.background(Color.gray100)
								
								HStack {
									Spacer()
									
									Button(action: {
										withAnimation {
											conversationViewModel.messageToReply = nil
										}
									}, label: {
										Image("x")
											.resizable()
											.frame(width: 30, height: 30, alignment: .leading)
											.padding(.all, 10)
									})
								}
							}
							.transition(.move(edge: .bottom))
						}
						
						if !conversationViewModel.mediasToSend.isEmpty || mediasIsLoading {
							ZStack(alignment: .top) {
								HStack {
									if mediasIsLoading {
										HStack {
											Spacer()
											
											ProgressView()
											
											Spacer()
										}
										.frame(height: 120)
									}
									
									if !mediasIsLoading {
										LazyVGrid(columns: [
											GridItem(.adaptive(minimum: 100), spacing: 1)
										], spacing: 3) {
											ForEach(conversationViewModel.mediasToSend, id: \.id) { attachment in
												ZStack {
													Rectangle()
														.fill(Color(.white))
														.frame(width: 100, height: 100)
													
													VStack {
														ZStack {
															if attachment.type == .image || attachment.type == .gif || attachment.type == .video {
																AsyncImage(url: attachment.thumbnail) { image in
																	ZStack {
																		image
																			.resizable()
																			.interpolation(.medium)
																			.aspectRatio(contentMode: .fill)
																		
																		if attachment.type == .video {
																			Image("play-fill")
																				.renderingMode(.template)
																				.resizable()
																				.foregroundStyle(.white)
																				.frame(width: 40, height: 40, alignment: .leading)
																		}
																	}
																} placeholder: {
																	ProgressView()
																}
															} else {
																VStack {
																	Spacer()
																	Text(attachment.name)
																		.foregroundStyle(Color.grayMain2c700)
																		.default_text_style_800(styleSize: 14)
																		.truncationMode(.middle)
																		.frame(maxWidth: .infinity, alignment: .center)
																		.multilineTextAlignment(.center)
																		.lineLimit(2)
																	Spacer()
																}
																.background(Color.grayMain2c200)
															}
															
															VStack {
																HStack {
																	Spacer()
																	
																	Image("x")
																		.renderingMode(.template)
																		.resizable()
																		.foregroundStyle(Color.orangeMain500)
																		.padding(4)
																		.background(.white)
																		.cornerRadius(12.5)
																		.frame(width: 20, height: 20)
																		.padding(4)
																}
																
																Spacer()
															}
															.frame(width: 100, height: 100)
														}
													}
													.layoutPriority(-1)
													.onTapGesture {
														if conversationViewModel.mediasToSend.count == 1 {
															withAnimation {
																conversationViewModel.mediasToSend.removeAll()
															}
														} else {
															guard let index = self.conversationViewModel.mediasToSend.firstIndex(of: attachment) else { return }
															self.conversationViewModel.mediasToSend.remove(at: index)
														}
													}
												}
												.clipShape(RoundedRectangle(cornerRadius: 4))
												.contentShape(Rectangle())
											}
										}
										.frame(
											width: geometry.size.width > 0 && CGFloat(102 * conversationViewModel.mediasToSend.count) > geometry.size.width - 20
											? 102 * floor(CGFloat(geometry.size.width - 20) / 102)
											: CGFloat(102 * conversationViewModel.mediasToSend.count)
										)
									}
								}
								.frame(maxWidth: .infinity)
								.padding(.all, conversationViewModel.mediasToSend.isEmpty ? 0 : 10)
								.background(Color.gray100)
								
								if !mediasIsLoading {
									HStack {
										Spacer()
										
										Button(action: {
											withAnimation {
												conversationViewModel.mediasToSend.removeAll()
											}
										}, label: {
											Image("x")
												.resizable()
												.frame(width: 30, height: 30, alignment: .leading)
												.padding(.all, 10)
										})
									}
								}
							}
							.transition(.move(edge: .bottom))
						}
						
						if areFilePickersOpen {
							ZStack(alignment: .top) {
								HStack {
									Button {
										self.areFilePickersOpen.toggle()
										self.isShowCamera = true
									} label: {
										VStack {
											Image("camera")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading ? Color.grayMain2c300 : Color.grayMain2c500)
												.frame(width: 25, height: 25, alignment: .leading)
											
											Text("conversation_take_picture_label")
												.foregroundStyle(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading ? Color.grayMain2c300 : Color.grayMain2c500)
												.default_text_style_300(styleSize: 15)
												.frame(maxWidth: .infinity, alignment: .center)
												.lineLimit(1)
										}
									}
									.disabled(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading)
									
									Button {
										self.areFilePickersOpen.toggle()
										self.isShowPhotoLibrary = true
										self.mediasIsLoading = true
									} label: {
										VStack {
											Image("image")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading ? Color.grayMain2c300 : Color.grayMain2c500)
												.frame(width: 25, height: 25, alignment: .leading)
											
											Text("conversation_pick_file_from_gallery_label")
												.foregroundStyle(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading ? Color.grayMain2c300 : Color.grayMain2c500)
												.default_text_style_300(styleSize: 15)
												.frame(maxWidth: .infinity, alignment: .center)
												.lineLimit(1)
										}
									}
									.disabled(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading)
									
									Button {
										self.areFilePickersOpen.toggle()
										self.isShowFilePicker = true
										self.mediasIsLoading = true
									} label: {
										VStack {
											Image("file")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading ? Color.grayMain2c300 : Color.grayMain2c500)
												.frame(width: 25, height: 25, alignment: .leading)
											
											Text("conversation_pick_any_file_label")
												.foregroundStyle(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading ? Color.grayMain2c300 : Color.grayMain2c500)
												.default_text_style_300(styleSize: 15)
												.frame(maxWidth: .infinity, alignment: .center)
												.lineLimit(1)
										}
									}
									.disabled(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading)
								}
								.frame(maxWidth: .infinity)
								.padding(.all, 20)
								.background(Color.gray100)
							}
							.transition(.move(edge: .bottom))
						}
						
						HStack(spacing: 0) {
							if !voiceRecordingInProgress {
								Button {
									withAnimation {
										areFilePickersOpen.toggle()
									}
								} label: {
									Image(areFilePickersOpen ? "x" : "paperclip")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading ? Color.grayMain2c300 : Color.grayMain2c500)
										.frame(width: 28, height: 28, alignment: .leading)
										.padding(.all, 6)
										.padding(.top, 4)
										.disabled(conversationViewModel.maxMediaCount <= conversationViewModel.mediasToSend.count || mediasIsLoading)
										.animation(.none, value: areFilePickersOpen)
								}
								.padding(.horizontal, isMessageTextFocused ? 0 : 2)
								
								HStack {
									if #available(iOS 16.0, *) {
										TextField("conversation_text_field_hint", text: $messageText, axis: .vertical)
											.default_text_style(styleSize: 15)
											.focused($isMessageTextFocused)
											.padding(.vertical, 5)
											.onChange(of: messageText) { text in
												if !text.isEmpty {
													conversationViewModel.compose()
												}
											}
									} else {
										ZStack(alignment: .leading) {
											TextEditor(text: $messageText)
												.multilineTextAlignment(.leading)
												.frame(maxHeight: 160)
												.fixedSize(horizontal: false, vertical: true)
												.default_text_style(styleSize: 15)
												.focused($isMessageTextFocused)
												.onChange(of: messageText) { text in
													if !text.isEmpty {
														conversationViewModel.compose()
													}
												}
											
											if messageText.isEmpty {
												Text("conversation_text_field_hint")
													.padding(.leading, 4)
													.lineLimit(1)
													.opacity(messageText.isEmpty ? 1 : 0)
													.foregroundStyle(Color.gray300)
													.default_text_style(styleSize: 15)
											}
										}
										.onTapGesture {
											isMessageTextFocused = true
										}
									}
									
									if messageText.isEmpty && conversationViewModel.mediasToSend.isEmpty {
										Button {
											voiceRecordingInProgress = true
										} label: {
											Image("microphone")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(Color.grayMain2c500)
												.frame(width: 28, height: 28, alignment: .leading)
												.padding(.all, 6)
												.padding(.top, 4)
										}
									} else {
										Button {
											if conversationViewModel.displayedConversationHistorySize > 1 {
												NotificationCenter.default.post(name: .onScrollToBottom, object: nil)
											}
											
											let messageTextTmp = self.messageText
                                            messageText = " "
                                            DispatchQueue.main.asyncAfter(deadline: .now() + 0.01) {
                                                messageText = ""
                                                isMessageTextFocused = true
                                                
                                                conversationViewModel.sendMessage(messageText: messageTextTmp)
                                            }
										} label: {
											Image("paper-plane-tilt")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(Color.orangeMain500)
												.frame(width: 28, height: 28, alignment: .leading)
												.padding(.all, 6)
												.padding(.top, 4)
												.rotationEffect(.degrees(45))
										}
										.padding(.trailing, 4)
									}
								}
								.padding(.leading, 15)
								.padding(.trailing, 5)
								.padding(.vertical, 6)
								.frame(maxWidth: .infinity, minHeight: 55)
								.background(.white)
								.cornerRadius(30)
								.overlay(
									RoundedRectangle(cornerRadius: 30)
										.inset(by: 0.5)
										.stroke(Color.gray200, lineWidth: 1.5)
								)
								.padding(.horizontal, 4)
							} else {
								VoiceRecorderPlayer(voiceRecordingInProgress: $voiceRecordingInProgress)
									.environmentObject(conversationViewModel)
									.frame(maxHeight: 60)
							}
						}
						.frame(maxWidth: .infinity, minHeight: 60)
						.padding(.top, 12)
						.padding(.bottom, geometry.safeAreaInsets.bottom > 0 ? (isMessageTextFocused ? 12 : 0) : 12)
						.padding(.horizontal, 10)
						.background(Color.gray100)
					}
				}
			}
			.blur(radius: conversationViewModel.selectedMessage != nil ? 8 : 0)
			
			if conversationViewModel.selectedMessage != nil && SharedMainViewModel.shared.displayedConversation != nil {
				let iconSize = ((geometry.size.width - (SharedMainViewModel.shared.displayedConversation!.isGroup ? 43 : 10) - 10) / 6) - 30
				
				ScrollView {
					VStack {
						Spacer()
						
						VStack {
							if !isSheetVisible && !isImdnOrReactionsSheetVisible {
								HStack {
									if conversationViewModel.selectedMessage!.message.isOutgoing {
										Spacer()
									}
									
									HStack {
										Button {
											conversationViewModel.sendReaction(emoji: "👍")
										} label: {
											Text("👍")
												.default_text_style(styleSize: iconSize > 50 ? 50 : iconSize)
										}
										.padding(.horizontal, 8)
										.background(conversationViewModel.selectedMessage?.message.ownReaction == "👍" ? Color.gray200 : .white)
										.cornerRadius(10)
										
										Button {
											conversationViewModel.sendReaction(emoji: "❤️")
										} label: {
											Text("❤️")
												.default_text_style(styleSize: iconSize > 50 ? 50 : iconSize)
										}
										.padding(.horizontal, 8)
										.background(conversationViewModel.selectedMessage?.message.ownReaction == "❤️" ? Color.gray200 : .white)
										.cornerRadius(10)
										
										Button {
											conversationViewModel.sendReaction(emoji: "😂")
										} label: {
											Text("😂")
												.default_text_style(styleSize: iconSize > 50 ? 50 : iconSize)
										}
										.padding(.horizontal, 8)
										.background(conversationViewModel.selectedMessage?.message.ownReaction == "😂" ? Color.gray200 : .white)
										.cornerRadius(10)
										
										Button {
											conversationViewModel.sendReaction(emoji: "😮")
										} label: {
											Text("😮")
												.default_text_style(styleSize: iconSize > 50 ? 50 : iconSize)
										}
										.padding(.horizontal, 8)
										.background(conversationViewModel.selectedMessage?.message.ownReaction == "😮" ? Color.gray200 : .white)
										.cornerRadius(10)
										
										Button {
											conversationViewModel.sendReaction(emoji: "😢")
										} label: {
											Text("😢")
												.default_text_style(styleSize: iconSize > 50 ? 50 : iconSize)
										}
										.padding(.horizontal, 8)
										.background(conversationViewModel.selectedMessage?.message.ownReaction == "😢" ? Color.gray200 : .white)
										.cornerRadius(10)
										
										Button {
											showPicker = true
											isSheetVisible = true
										} label: {
											Image("plus-circle")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(Color.grayMain2c500)
												.frame(width: iconSize > 50 ? 55 : iconSize + 5, height: iconSize > 50 ? 55 : iconSize + 5, alignment: .leading)
										}
										.padding(.top, 3)
										.padding(.leading, 2)
										.padding(.trailing, 6)
										.cornerRadius(10)
									}
									.padding(.vertical, 5)
									.padding(.horizontal, 10)
									.background(.white)
									.cornerRadius(20)
									
									if !conversationViewModel.selectedMessage!.message.isOutgoing {
										Spacer()
									}
								}
								.frame(maxWidth: .infinity)
								.padding(.horizontal, 10)
								.padding(.leading, SharedMainViewModel.shared.displayedConversation!.isGroup ? 43 : 0)
								.shadow(color: .black.opacity(0.1), radius: 10)
							}
							
							ChatBubbleView(eventLogMessage: conversationViewModel.selectedMessage!, geometryProxy: geometry)
								.environmentObject(conversationViewModel)
								.padding(.horizontal, 10)
								.padding(.vertical, 1)
								.shadow(color: .black.opacity(0.1), radius: 10)
								.offset(y: isSheetVisible || isImdnOrReactionsSheetVisible ? -(UIScreen.main.bounds.height * 0.5) - 10 : 0)
							
							if !isSheetVisible && !isImdnOrReactionsSheetVisible {
								HStack {
									if conversationViewModel.selectedMessage!.message.isOutgoing {
										Spacer()
									}
									
									VStack {
										if conversationViewModel.selectedMessage!.message.status == .error {
											Button {
												conversationViewModel.resendMessage(chatMessage: conversationViewModel.selectedMessage!)
												conversationViewModel.selectedMessage = nil
											} label: {
												HStack {
													Text("menu_resend_chat_message")
														.default_text_style(styleSize: 15)
													Spacer()
													
													Image("paper-plane-tilt")
														.resizable()
														.frame(width: 20, height: 20, alignment: .leading)
														.rotationEffect(.degrees(45))
												}
												.padding(.vertical, 5)
												.padding(.horizontal, 20)
											}
											
											Divider()
										}
										
										if !(CoreContext.shared.imdnToEverybodyThreshold && !conversationViewModel.selectedMessage!.message.isOutgoing) {
											Button {
												conversationViewModel.selectedMessageToDisplayDetails = conversationViewModel.selectedMessage
												conversationViewModel.prepareBottomSheetForDeliveryStatus()
											} label: {
												HStack {
													Text("menu_show_imdn")
														.default_text_style(styleSize: 15)
													Spacer()
													Image("info")
														.resizable()
														.frame(width: 20, height: 20, alignment: .leading)
												}
												.padding(.vertical, 5)
												.padding(.horizontal, 20)
											}
											
											Divider()
										}
									
										Button {
											let indexMessage = conversationViewModel.conversationMessagesSection[0].rows.firstIndex(where: {$0.message.id == conversationViewModel.selectedMessage!.message.id})
											conversationViewModel.selectedMessage = nil
											conversationViewModel.replyToMessage(index: indexMessage ?? 0, isMessageTextFocused: Binding(
                                                get: { isMessageTextFocused },
                                                set: { isMessageTextFocused = $0 }
                                            ))
										} label: {
											HStack {
												Text("menu_reply_to_chat_message")
													.default_text_style(styleSize: 15)
												Spacer()
												Image("reply")
													.resizable()
													.frame(width: 20, height: 20, alignment: .leading)
											}
											.padding(.vertical, 5)
											.padding(.horizontal, 20)
										}
										
										Divider()
										
										if !conversationViewModel.selectedMessage!.message.text.isEmpty {
											Button {
												UIPasteboard.general.setValue(
													conversationViewModel.selectedMessage?.message.text ?? "Error_message_not_available",
													forPasteboardType: UTType.plainText.identifier
												)
												
												ToastViewModel.shared.toastMessage = "Success_message_copied_into_clipboard"
												ToastViewModel.shared.displayToast = true
												
												conversationViewModel.selectedMessage = nil
											} label: {
												HStack {
													Text("menu_copy_chat_message")
														.default_text_style(styleSize: 15)
													Spacer()
													Image("copy")
														.resizable()
														.frame(width: 20, height: 20, alignment: .leading)
												}
												.padding(.vertical, 5)
												.padding(.horizontal, 20)
											}
											
											Divider()
										}
										
										Button {
											withAnimation {
												isShowConversationForwardMessageFragment = true
											}
										} label: {
											HStack {
												Text("menu_forward_chat_message")
													.default_text_style(styleSize: 15)
												Spacer()
												Image("forward")
													.resizable()
													.frame(width: 20, height: 20, alignment: .leading)
											}
											.padding(.vertical, 5)
											.padding(.horizontal, 20)
										}
										
										Divider()
										
										Button {
											conversationViewModel.deleteMessage()
										} label: {
											HStack {
												Text("menu_delete_selected_item")
													.foregroundStyle(.red)
													.default_text_style(styleSize: 15)
												Spacer()
												Image("trash-simple-red")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(.red)
													.frame(width: 20, height: 20, alignment: .leading)
											}
											.padding(.vertical, 5)
											.padding(.horizontal, 20)
										}
									}
									.frame(maxWidth: geometry.size.width / 1.5)
									.padding(.vertical, 8)
									.background(.white)
									.cornerRadius(20)
									
									if !conversationViewModel.selectedMessage!.message.isOutgoing {
										Spacer()
									}
								}
								.frame(maxWidth: .infinity)
								.padding(.horizontal, 10)
								.padding(.bottom, 20)
								.padding(.leading, SharedMainViewModel.shared.displayedConversation!.isGroup ? 43 : 0)
								.shadow(color: .black.opacity(0.1), radius: 10)
							}
						}
					}
					.frame(maxWidth: .infinity)
					.frame(minHeight: geometry.size.height)
				}
				.background(.gray.opacity(0.1))
				.onTapGesture {
					withAnimation {
						conversationViewModel.selectedMessage = nil
					}
				}
				.onAppear {
					touchFeedback()
				}
				.onDisappear {
					if conversationViewModel.selectedMessage != nil {
						conversationViewModel.selectedMessage = nil
					}
				}
			}
			
			if isShowConversationForwardMessageFragment {
				ConversationForwardMessageFragment(
					conversationsList: conversationsListViewModel.conversationsList,
					selectedMessage: conversationViewModel.selectedMessage,
					isShowConversationForwardMessageFragment: $isShowConversationForwardMessageFragment
				)
				.environmentObject(conversationViewModel)
				.environmentObject(conversationsListViewModel)
				.zIndex(5)
				.transition(.move(edge: .trailing))
				.onAppear {
					conversationViewModel.selectedMessage = nil
				}
			}
			
			if isShowInfoConversationFragment {
				ConversationInfoFragment(
					isMuted: $isMuted,
					isShowEphemeralFragment: $isShowEphemeralFragment,
					isShowStartCallGroupPopup: $isShowStartCallGroupPopup,
					isShowInfoConversationFragment: $isShowInfoConversationFragment,
					isShowEditContactFragment: $isShowEditContactFragment,
					isShowEditContactFragmentAddress: $isShowEditContactFragmentAddress,
					isShowScheduleMeetingFragment: $isShowScheduleMeetingFragment,
					isShowScheduleMeetingFragmentSubject: $isShowScheduleMeetingFragmentSubject,
					isShowScheduleMeetingFragmentParticipants: $isShowScheduleMeetingFragmentParticipants,
					isShowConversationInfoPopup: $isShowConversationInfoPopup,
					conversationInfoPopupText: $conversationInfoPopupText
				)
				.environmentObject(conversationViewModel)
				.zIndex(5)
				.transition(.move(edge: .trailing))
			}
            
			if isShowEphemeralFragment {
				EphemeralFragment(
					isShowEphemeralFragment: $isShowEphemeralFragment
				)
				.environmentObject(conversationViewModel)
				.zIndex(5)
				.transition(.move(edge: .trailing))
			}
		}
	}
	// swiftlint:enable cyclomatic_complexity
	// swiftlint:enable function_body_length
}

struct ImdnOrReactionsSheet: View {
	@EnvironmentObject var conversationViewModel: ConversationViewModel
	
	@Binding var selectedCategoryIndex: Int
	@Binding var isImdnOrReactionsSheetVisible: Bool
	
	var body: some View {
		VStack {
			Picker("picker_categories", selection: $selectedCategoryIndex) {
				ForEach(0..<conversationViewModel.sheetCategories.count, id: \.self) { index in
					Text(conversationViewModel.sheetCategories[index].name)
				}
			}
			.pickerStyle(SegmentedPickerStyle())
			.padding()
			
			ScrollView {
				LazyVStack {
					if selectedCategoryIndex < conversationViewModel.sheetCategories.count && !conversationViewModel.sheetCategories[selectedCategoryIndex].innerCategory.isEmpty {
						ForEach(conversationViewModel.sheetCategories[selectedCategoryIndex].innerCategory, id: \.id) { participant in
							Button(
								action: {
									if participant.isMe {
										conversationViewModel.removeReaction()
									}
								},
								label: {
									Avatar(contactAvatarModel: participant.contact, avatarSize: 50)
									
									VStack {
										
										Text(participant.contact.name)
											.default_text_style(styleSize: 16)
											.frame(maxWidth: .infinity, alignment: .leading)
											.lineLimit(1)
										
										if participant.isMe {
											Text("message_reaction_click_to_remove_label")
												.foregroundStyle(Color.grayMain2c400)
												.default_text_style_300(styleSize: 14)
												.frame(maxWidth: .infinity, alignment: .leading)
												.lineLimit(1)
										}
									}
									
									Spacer()
									
									Text(participant.detail)
										.default_text_style(styleSize: 16)
										.lineLimit(1)
								}
							)
							.disabled(!participant.isMe)
							.padding(.horizontal)
							.buttonStyle(.borderless)
							.background(.white)
						}
					}
				}
			}
			.listStyle(.plain)
			
			WillDisappearNotifierView {
				isImdnOrReactionsSheetVisible = false
			}
			.frame(width: 0, height: 0)
		}
		.onAppear {
			selectedCategoryIndex = 0
			isImdnOrReactionsSheetVisible = true
		}
		.padding(.top)
		.background(.white)
	}
}

struct ImagePicker: UIViewControllerRepresentable {
	@EnvironmentObject var conversationViewModel: ConversationViewModel
	@Binding var selectedMedia: [Attachment]
	@Environment(\.presentationMode) private var presentationMode
 
	final class Coordinator: NSObject, UIImagePickerControllerDelegate, UINavigationControllerDelegate {
	 
		var parent: ImagePicker
	 
		init(_ parent: ImagePicker) {
			self.parent = parent
		}
	 
		func imagePickerController(_ picker: UIImagePickerController, didFinishPickingMediaWithInfo info: [UIImagePickerController.InfoKey: Any]) {
			let mediaType = info[UIImagePickerController.InfoKey.mediaType] as? String
			switch mediaType {
			case "public.image":
				let image = info[UIImagePickerController.InfoKey.originalImage] as? UIImage
				
				let date = Date()
				let dformater = DateFormatter()
				dformater.dateFormat = "yyyy-MM-dd-HHmmss"
				let dateString = dformater.string(from: date)
				
				let path = FileManager.default.temporaryDirectory.appendingPathComponent((dateString.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? "") + ".jpeg")
				
				if image != nil {
					let data  = image!.jpegData(compressionQuality: 1)
					if data != nil {
						do {
							_ = try data!.write(to: path)
							let attachment = Attachment(id: UUID().uuidString, name: (dateString.addingPercentEncoding(withAllowedCharacters: .urlHostAllowed) ?? "") + ".jpeg", url: path, type: .image)
							parent.selectedMedia.append(attachment)
						} catch {
						}
					}
				}
			case "public.movie":
				let videoUrl = info[UIImagePickerController.InfoKey.mediaURL] as? URL
				if videoUrl != nil {
					let name = videoUrl!.lastPathComponent
					let path = videoUrl!.deletingLastPathComponent()
					let pathThumbnail = URL(string: parent.conversationViewModel.generateThumbnail(name: name, pathThumbnail: path))
					
					if pathThumbnail != nil {
						let attachment =
						Attachment(
							id: UUID().uuidString,
							name: name,
							thumbnail: pathThumbnail!,
							full: videoUrl!,
							type: .video
						)
						parent.selectedMedia.append(attachment)
					}
				}
			default:
				Log.info("Mismatched type: \(mediaType ?? "mediaType is nil")")
			}
	 
			parent.presentationMode.wrappedValue.dismiss()
		}
	}
	
	func makeUIViewController(context: UIViewControllerRepresentableContext<ImagePicker>) -> UIImagePickerController {
		let imagePicker = UIImagePickerController()
		imagePicker.sourceType = .camera
		imagePicker.mediaTypes = ["public.image", "public.movie"]
		imagePicker.delegate = context.coordinator
 
		return imagePicker
	}
 
	func updateUIViewController(_ uiViewController: UIImagePickerController, context: UIViewControllerRepresentableContext<ImagePicker>) {
 
	}
	
	func makeCoordinator() -> Coordinator {
		Coordinator(self)
	}
}

struct VoiceRecorderPlayer: View {
	@EnvironmentObject var conversationViewModel: ConversationViewModel
	
	@Binding var voiceRecordingInProgress: Bool
	
	@StateObject var audioRecorder = AudioRecorder()
	
	@State private var value: Double = 0.0
	@State private var isPlaying: Bool = false
	@State private var isRecording: Bool = true
	@State private var timer: Timer?
	
	var minTrackColor: Color = .white.opacity(0.5)
	var maxTrackGradient: Gradient = Gradient(colors: [Color.orangeMain300, Color.orangeMain500])
	
	var body: some View {
		GeometryReader { geometry in
			let radius = geometry.size.height * 0.5
			HStack {
				Button(
					action: {
						self.audioRecorder.stopVoiceRecorder()
						voiceRecordingInProgress = false
					},
					label: {
						Image("x")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.orangeMain500)
							.frame(width: 25, height: 25)
					}
				)
				.padding(10)
				.background(.white)
				.clipShape(RoundedRectangle(cornerRadius: 25))
				
				ZStack(alignment: .leading) {
					LinearGradient(
						gradient: maxTrackGradient,
						startPoint: .leading,
						endPoint: .trailing
					)
					.frame(width: geometry.size.width - 110, height: 50)
					HStack {
						if !isRecording {
							Rectangle()
								.foregroundColor(minTrackColor)
								.frame(width: self.value * (geometry.size.width - 110) / 100, height: 50)
						} else {
							Rectangle()
								.foregroundColor(minTrackColor)
								.frame(width: CGFloat(audioRecorder.soundPower) * (geometry.size.width - 110) / 100, height: 50)
						}
					}
					
					HStack {
						Button(
							action: {
								if isRecording {
									self.audioRecorder.stopVoiceRecorder()
									isRecording = false
								} else if isPlaying {
									conversationViewModel.pauseVoiceRecordPlayer()
									pauseProgress()
								} else {
									if audioRecorder.audioFilename != nil {
										conversationViewModel.startVoiceRecordPlayer(voiceRecordPath: audioRecorder.audioFilename!)
										playProgress()
									}
								}
							},
							label: {
								Image(isRecording ? "stop-fill" : (isPlaying ? "pause-fill" : "play-fill"))
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.orangeMain500)
									.frame(width: 20, height: 20)
							}
						)
						.padding(8)
						.background(.white)
						.clipShape(RoundedRectangle(cornerRadius: 25))
						
						Spacer()
						
						HStack {
							if isRecording {
								Image("record-fill")
								 .renderingMode(.template)
								 .resizable()
								 .foregroundStyle(isRecording ? Color.redDanger500 : Color.orangeMain500)
								 .frame(width: 18, height: 18)
							}
							
							Text(Int(audioRecorder.recordingTime).convertDurationToString())
								.default_text_style(styleSize: 16)
								.padding(.horizontal, 5)
						}
						.padding(8)
						.background(.white)
						.clipShape(RoundedRectangle(cornerRadius: 25))
					}
					.padding(.horizontal, 10)
				}
				.clipShape(RoundedRectangle(cornerRadius: radius))
				
				Button {
					if conversationViewModel.displayedConversationHistorySize > 0 {
						NotificationCenter.default.post(name: .onScrollToBottom, object: nil)
					}
					conversationViewModel.sendMessage(messageText: "", audioRecorder: self.audioRecorder)
					voiceRecordingInProgress = false
				} label: {
					Image("paper-plane-tilt")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.orangeMain500)
						.frame(width: 28, height: 28, alignment: .leading)
						.padding(.all, 6)
						.padding(.top, 4)
						.rotationEffect(.degrees(45))
				}
				.padding(.trailing, 4)
			}
			.padding(.horizontal, 4)
			.padding(.vertical, 5)
			.onAppear {
				self.audioRecorder.startRecording()
			}
			.onDisappear {
				self.audioRecorder.stopVoiceRecorder()
				resetProgress()
			}
		}
	}
	
	private func playProgress() {
		isPlaying = true
		if audioRecorder.audioFilename != nil {
			self.value = conversationViewModel.getPositionVoiceRecordPlayer(voiceRecordPath: audioRecorder.audioFilename!)
			timer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { _ in
				if self.value < 100.0 {
					let valueTmp = conversationViewModel.getPositionVoiceRecordPlayer(voiceRecordPath: audioRecorder.audioFilename!)
					if self.value > 90 && self.value == valueTmp {
						self.value = 100
					} else {
						if valueTmp == 0 && !conversationViewModel.isPlayingVoiceRecordPlayer(voiceRecordPath: audioRecorder.audioFilename!) {
							stopProgress()
							value = 0.0
							isPlaying = false
						} else {
							self.value = valueTmp
						}
					}
				} else {
					resetProgress()
				}
			}
		}
	}
	
	// Pause the progress
	private func pauseProgress() {
		isPlaying = false
		stopProgress()
	}
	
	// Reset the progress
	private func resetProgress() {
		conversationViewModel.stopVoiceRecordPlayer()
		stopProgress()
		value = 0.0
		isPlaying = false
	}
	
	// Stop the progress and invalidate the timer
	private func stopProgress() {
		timer?.invalidate()
		timer = nil
	}
}

struct WillDisappearNotifierView: UIViewControllerRepresentable {
	let onWillDisappear: () -> Void
	
	func makeUIViewController(context: Context) -> UIViewController {
		Controller(onWillDisappear: onWillDisappear)
	}
	
	func updateUIViewController(_ uiViewController: UIViewController, context: Context) {}
	
	class Controller: UIViewController {
		let onWillDisappearCallback: () -> Void
		
		init(onWillDisappear: @escaping () -> Void) {
			self.onWillDisappearCallback = onWillDisappear
			super.init(nibName: nil, bundle: nil)
		}
		
		required init?(coder: NSCoder) {
			fatalError("init(coder:) has not been implemented")
		}
		
		override func viewWillDisappear(_ animated: Bool) {
			super.viewWillDisappear(animated)
			onWillDisappearCallback()
		}
	}
}

// swiftlint:enable type_body_length
// swiftlint:enable line_length
