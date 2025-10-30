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

// swiftlint:disable type_body_length
// swiftlint:disable line_length
import SwiftUI
import linphonesw

struct ContentView: View {
	
	@Environment(\.scenePhase) var scenePhase
	
	@EnvironmentObject var navigationManager: NavigationManager
	@EnvironmentObject var coreContext: CoreContext
	@EnvironmentObject var telecomManager: TelecomManager
	@EnvironmentObject var sharedMainViewModel: SharedMainViewModel
	
	@ObservedObject private var contactsManager = ContactsManager.shared
	@ObservedObject private var magicSearch = MagicSearchSingleton.shared
	
	@StateObject private var callViewModel = CallViewModel()
	@StateObject private var accountProfileViewModel = AccountProfileViewModel()
	
	@State private var contactsListViewModel: ContactsListViewModel?
	@State private var historyListViewModel: HistoryListViewModel?
	@State private var conversationsListViewModel: ConversationsListViewModel?
	@State private var meetingsListViewModel: MeetingsListViewModel?
	
	@State private var orientation = UIDevice.current.orientation
	@State var sideMenuIsOpen: Bool = false
	
	@State private var searchIsActive = false
	@State private var text = ""
	@FocusState private var focusedField: Bool
	@State private var showingDialer = false
	@State var isMenuOpen = false
	@State var isShowDeleteContactPopup = false
	@State var isShowDeleteAllHistoryPopup = false
	@State var isShowEditContactFragment = false
	@State var isShowEditContactFragmentInContactDetails = false
	@State var isShowEditContactFragmentAddress = ""
	@State var isShowStartCallFragment = false
	@State var isShowStartConversationFragment = false
	@State var isShowDismissPopup = false
	@State var isShowSendCancelMeetingNotificationPopup = false
	@State var isShowStartCallGroupPopup = false
	@State var isShowSipAddressesPopup = false
	@State var isShowSipAddressesPopupType = 0 // 0 to call, 1  to message, 2 to video call
	@State var isShowConversationFragment = false
	@State var isShowAccountProfileFragment = false
	@State var isShowSettingsFragment = false
	@State var isShowHelpFragment = false
	
	@State var fullscreenVideo = false
	
	@State var isShowScheduleMeetingFragment = false
	@State var isShowScheduleMeetingFragmentSubject = ""
	@State var isShowScheduleMeetingFragmentParticipants: [SelectedAddressModel] = []
	
	@State private var isShowLoginFragment: Bool = false
	
	private let avatarSize = 45.0
	@State private var imagePath: URL?
	@State private var imageTmp: Image?
	
	@State var isShowConversationInfoPopup: Bool = false
	@State var conversationInfoPopupText: String = ""
	
	@State var isShowUpdatePasswordPopup: Bool = false
	@State var passwordUpdateAddress: String = ""
	
	var body: some View {
		GeometryReader { geometry in
			VStack(spacing: 0) {
				if accountProfileViewModel.accountError && (!telecomManager.callInProgress || (telecomManager.callInProgress && !telecomManager.callDisplayed)) {
					HStack {
						if let index = accountProfileViewModel.defaultAccountModelIndex,
						   index < coreContext.accounts.count, coreContext.accounts[index].isDefaultAccount, coreContext.accounts[index].registrationStateAssociatedUIColor == .orangeWarning600 {
							Image("warning-circle")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(.white)
								.frame(width: 26, height: 26)
								.padding(.leading, 10)
							
							
							Text(String(localized: "default_account_disabled"))
								.default_text_style_white(styleSize: 16)
						} else {
							Image("bell-simple")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(.white)
								.frame(width: 26, height: 26)
								.padding(.leading, 10)
							
							
							Text(String(localized: "connection_error_for_non_default_account"))
								.default_text_style_white(styleSize: 16)
						}
						Spacer()
						
						Button(
							action: {
								withAnimation {
									accountProfileViewModel.accountError = false
								}
							}, label: {
								Image("x")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(.white)
									.frame(width: 26, height: 26)
									.padding(.trailing, 10)
							}
						)
						
					}
					.frame(maxWidth: .infinity)
					.frame(height: 40)
					.padding(.horizontal, 10)
					.background(Color.redDanger500)
				}
				
				if !sharedMainViewModel.fileUrlsToShare.isEmpty && (!telecomManager.callInProgress || (telecomManager.callInProgress && !telecomManager.callDisplayed)) {
					HStack {
						Image("share-network")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(.white)
							.frame(width: 26, height: 26)
							.padding(.leading, 10)
						
						if sharedMainViewModel.fileUrlsToShare.count > 1 {
							Text(String(format: String(localized: "conversations_files_waiting_to_be_shared_multiple"), sharedMainViewModel.fileUrlsToShare.count.description))
								.default_text_style_white(styleSize: 16)
						} else {
							Text(String(localized: "conversations_files_waiting_to_be_shared_single"))
								.default_text_style_white(styleSize: 16)
						}
						
						Spacer()
						
						Button(
							action: {
								withAnimation {
									sharedMainViewModel.fileUrlsToShare = []
								}
							}, label: {
								Image("x")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(.white)
									.frame(width: 26, height: 26)
									.padding(.trailing, 10)
							}
						)
						
					}
					.frame(maxWidth: .infinity)
					.frame(height: 40)
					.padding(.horizontal, 10)
					.background(Color.gray)
				}
				
				if (telecomManager.callInProgress && !fullscreenVideo && ((!telecomManager.callDisplayed && callViewModel.callsCounter == 1) || callViewModel.callsCounter > 1)) || isShowConversationFragment {
					HStack {
						Image("phone")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(.white)
							.frame(width: 26, height: 26)
							.padding(.leading, 10)
						
						if callViewModel.callsCounter > 1 {
							Text(String(format: String(localized: "calls_count_label"), callViewModel.callsCounter.description))
								.default_text_style_white(styleSize: 16)
						} else {
							Text("\(callViewModel.displayName)")
								.default_text_style_white(styleSize: 16)
						}
						
						Spacer()
						
						if callViewModel.callsCounter == 1 {
							Text(callViewModel.isPaused || telecomManager.isPausedByRemote ? String(localized: "call_state_paused") : String(localized: "call_state_connected"))
								.default_text_style_white(styleSize: 16)
								.padding(.trailing, 10)
						}
					}
					.frame(maxWidth: .infinity)
					.frame(height: 40)
					.padding(.horizontal, 10)
					.background(Color.greenSuccess500)
					.onTapGesture {
						withAnimation {
							telecomManager.callDisplayed = true
						}
					}
				}
				
				ZStack {
					VStack(spacing: 0) {
						HStack(spacing: 0) {
							if orientation == .landscapeLeft
								|| orientation == .landscapeRight
								|| UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height {
								VStack(spacing: 0) {
									Group {
										Spacer()
										
										Button(action: {
											sharedMainViewModel.changeIndexView(indexViewInt: 0)
											sharedMainViewModel.displayedCall = nil
											sharedMainViewModel.displayedConversation = nil
											sharedMainViewModel.displayedMeeting = nil
										}, label: {
											VStack {
												Image("address-book")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(sharedMainViewModel.indexView == 0 ? Color.orangeMain500 : Color.grayMain2c600)
													.frame(width: 25, height: 25)
												if sharedMainViewModel.indexView == 0 {
													Text("bottom_navigation_contacts_label")
														.default_text_style_700(styleSize: 10)
												} else {
													Text("bottom_navigation_contacts_label")
														.default_text_style(styleSize: 10)
												}
											}
										})
										.padding(.top)
										.frame(height: geometry.size.height/4)
										
										ZStack {
											if SharedMainViewModel.shared.missedCallsCount > 0 {
												VStack {
													HStack {
														Text(
                                                            SharedMainViewModel.shared.missedCallsCount < 99
															? String(SharedMainViewModel.shared.missedCallsCount)
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
												.padding(.bottom, 30)
												.padding(.leading, 30)
											}
											
											Button(action: {
												sharedMainViewModel.changeIndexView(indexViewInt: 1)
												sharedMainViewModel.displayedFriend = nil
												sharedMainViewModel.displayedConversation = nil
												sharedMainViewModel.displayedMeeting = nil
												if SharedMainViewModel.shared.missedCallsCount > 0 {
                                                    SharedMainViewModel.shared.resetMissedCallsCount()
												}
											}, label: {
												VStack {
													Image("phone")
														.renderingMode(.template)
														.resizable()
														.foregroundStyle(sharedMainViewModel.indexView == 1 ? Color.orangeMain500 : Color.grayMain2c600)
														.frame(width: 25, height: 25)
													if sharedMainViewModel.indexView == 1 {
														Text("bottom_navigation_calls_label")
															.default_text_style_700(styleSize: 10)
													} else {
														Text("bottom_navigation_calls_label")
															.default_text_style(styleSize: 10)
													}
												}
											})
											.padding(.top)
										}
										.frame(height: geometry.size.height/4)
										
                                        if !sharedMainViewModel.disableChatFeature {
                                            ZStack {
                                                if SharedMainViewModel.shared.unreadMessages > 0 {
                                                    VStack {
                                                        HStack {
                                                            Text(
                                                                SharedMainViewModel.shared.unreadMessages < 99
                                                                ? String(SharedMainViewModel.shared.unreadMessages)
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
                                                    .padding(.bottom, 30)
                                                    .padding(.leading, 30)
                                                }
                                                
                                                Button(action: {
                                                    sharedMainViewModel.changeIndexView(indexViewInt: 2)
                                                    sharedMainViewModel.displayedFriend = nil
                                                    sharedMainViewModel.displayedCall = nil
                                                    sharedMainViewModel.displayedMeeting = nil
                                                }, label: {
                                                    VStack {
                                                        Image("chat-teardrop-text")
                                                            .renderingMode(.template)
                                                            .resizable()
                                                            .foregroundStyle(sharedMainViewModel.indexView == 2 ? Color.orangeMain500 : Color.grayMain2c600)
                                                            .frame(width: 25, height: 25)
                                                        
                                                        if sharedMainViewModel.indexView == 2 {
                                                            Text("bottom_navigation_conversations_label")
                                                                .default_text_style_700(styleSize: 10)
                                                        } else {
                                                            Text("bottom_navigation_conversations_label")
                                                                .default_text_style(styleSize: 10)
                                                        }
                                                    }
                                                })
                                                .padding(.top)
                                            }
                                            .frame(height: geometry.size.height/4)
                                        }
										
										if !sharedMainViewModel.disableMeetingFeature {
											Button(action: {
												sharedMainViewModel.changeIndexView(indexViewInt: 3)
												sharedMainViewModel.displayedFriend = nil
												sharedMainViewModel.displayedCall = nil
												sharedMainViewModel.displayedConversation = nil
											}, label: {
												VStack {
													Image("video-conference")
														.renderingMode(.template)
														.resizable()
														.foregroundStyle(sharedMainViewModel.indexView == 3 ? Color.orangeMain500 : Color.grayMain2c600)
														.frame(width: 25, height: 25)
													if sharedMainViewModel.indexView == 0 {
														Text("bottom_navigation_meetings_label")
															.default_text_style_700(styleSize: 10)
													} else {
														Text("bottom_navigation_meetings_label")
															.default_text_style(styleSize: 10)
													}
												}
											})
											.padding(.top)
											.frame(height: geometry.size.height/4)
											
											Spacer()
										}
									}
								}
								.frame(width: 75, height: geometry.size.height)
								.padding(.leading,
										 orientation == .landscapeRight && geometry.safeAreaInsets.bottom > 0
										 ? -geometry.safeAreaInsets.leading
										 : 0)
							}
							
							VStack(spacing: 0) {
								Rectangle()
									.foregroundColor(Color.orangeMain500)
									.edgesIgnoringSafeArea(.top)
									.frame(height: 1)
								
								ZStack {
									VStack {
										Rectangle()
											.foregroundColor(
												(orientation == .landscapeLeft
												 || orientation == .landscapeRight
												 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)
												? Color.white
												: Color.orangeMain500
											)
											.frame(height: 100)
										
										Spacer()
									}
									
									VStack(spacing: 0) {
										if searchIsActive == false {
											HStack {
												Button {
													openMenu()
												} label: {
													Image("list")
														.renderingMode(.template)
														.resizable()
														.foregroundStyle(.white)
														.frame(width: 25, height: 25, alignment: .leading)
														.padding(.all, 5)
												}
												
                                                if let index = accountProfileViewModel.defaultAccountModelIndex,
                                                   index < coreContext.accounts.count {
                                                    
                                                    let account = coreContext.accounts[index]
                                                    let imagePath = account.getImagePath()
                                                    let finalUrl = imagePath.appendingQueryItem("v", value: UUID().uuidString)

                                                    AsyncImage(url: finalUrl)
                                                        { image in
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
                                                                    .onAppear {
                                                                        imageTmp = image
                                                                    }
                                                            case .failure:
                                                                if let avatar = account.avatarModel {
                                                                    let tmpImage = contactsManager.textToImage(firstName: avatar.name, lastName: "")
                                                                    Image(uiImage: tmpImage)
                                                                        .resizable()
                                                                        .frame(width: avatarSize, height: avatarSize)
                                                                        .clipShape(Circle())
                                                                } else if let cachedImage = imageTmp {
                                                                    cachedImage
                                                                        .resizable()
                                                                        .aspectRatio(contentMode: .fill)
                                                                        .frame(width: avatarSize, height: avatarSize)
                                                                        .clipShape(Circle())
                                                                } else {
                                                                    ProgressView()
                                                                        .frame(width: avatarSize, height: avatarSize)
                                                                }
                                                            @unknown default:
                                                                EmptyView()
                                                            }
                                                        }
                                                        .id(imagePath)
                                                        .onTapGesture {
                                                            openMenu()
                                                        }
                                                        .onReceive(NotificationCenter.default.publisher(for: NSNotification.Name("ImageChanged"))) { _ in
                                                            imageTmp = nil
                                                        }
                                                    
                                                } else if let cachedImage = imageTmp {
                                                    cachedImage
                                                        .resizable()
                                                        .aspectRatio(contentMode: .fill)
														.frame(width: avatarSize, height: avatarSize)
														.clipShape(Circle())
														.onTapGesture {
															openMenu()
														}
                                                } else {
                                                    ProgressView()
                                                        .frame(width: avatarSize, height: avatarSize)
                                                }

												
												Text(String(localized: sharedMainViewModel.indexView == 0 ? "bottom_navigation_contacts_label" : (sharedMainViewModel.indexView == 1 ? "bottom_navigation_calls_label" : (sharedMainViewModel.indexView == 2 ? "bottom_navigation_conversations_label" : "bottom_navigation_meetings_label"))))
													.default_text_style_white_800(styleSize: 20)
													.padding(.leading, 2)
												
												Spacer()
												
												Button {
													withAnimation {
														searchIsActive.toggle()
													}
												} label: {
													Image("magnifying-glass")
														.renderingMode(.template)
														.resizable()
														.foregroundStyle(.white)
														.frame(width: 25, height: 25, alignment: .leading)
														.padding(.all, 10)
												}
												.padding(.trailing, sharedMainViewModel.indexView == 2 ? 10 : 0)
												
												if sharedMainViewModel.indexView == 3 {
													Button {
														NotificationCenter.default.post(name: MeetingsListViewModel.ScrollToTodayNotification, object: nil)
													} label: {
														Image("calendar")
															.renderingMode(.template)
															.resizable()
															.foregroundStyle(.white)
															.frame(width: 25, height: 25, alignment: .leading)
															.padding(.all, 10)
													}
													.padding(.trailing, 10)
												} else if sharedMainViewModel.indexView != 2 {
													Menu {
														if sharedMainViewModel.indexView == 0 {
															Button {
																sharedMainViewModel.displayedFriend = nil
																isMenuOpen = false
																magicSearch.changeAllContact(allContactBool: true)
																magicSearch.searchForContacts()
															} label: {
																HStack {
																	Text("contacts_list_filter_popup_see_all")
																	Spacer()
																	if magicSearch.allContact {
																		Image("green-check")
																			.resizable()
																			.frame(width: 25, height: 25, alignment: .leading)
																			.padding(.all, 10)
																	}
																}
															}
															
															Button {
																sharedMainViewModel.displayedFriend = nil
																isMenuOpen = false
																magicSearch.changeAllContact(allContactBool: false)
																magicSearch.searchForContacts()
															} label: {
																HStack {
																	Text(String(format: String(localized: "contacts_list_filter_popup_see_linphone_only"), Bundle.main.displayName))
																	Spacer()
																	if !magicSearch.allContact {
																		Image("green-check")
																			.resizable()
																			.frame(width: 25, height: 25, alignment: .leading)
																			.padding(.all, 10)
																	}
																}
															}
														} else {
															Button(role: .destructive) {
																isMenuOpen = false
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
														}
													} label: {
														Image(sharedMainViewModel.indexView == 0 ? "funnel" : "dots-three-vertical")
															.renderingMode(.template)
															.resizable()
															.foregroundStyle(.white)
															.frame(width: 25, height: 25, alignment: .leading)
															.padding(.all, 10)
													}
													.padding(.trailing, 10)
													.onTapGesture {
														isMenuOpen = true
													}
												}
											}
											.frame(maxWidth: .infinity)
											.frame(height: 50)
											.padding(.leading)
											.padding(.top, 2.5)
											.padding(.bottom, 2.5)
											.background(Color.orangeMain500)
											.roundedCorner(10, corners: [.bottomRight, .bottomLeft])
										} else {
											HStack {
												Button {
													withAnimation {
														self.focusedField = false
														searchIsActive.toggle()
													}
													
													text = ""
													
													if sharedMainViewModel.indexView == 0 {
														magicSearch.currentFilter = ""
														magicSearch.searchForContacts()
													} else if let historyListVM = historyListViewModel, sharedMainViewModel.indexView == 1 {
														historyListVM.resetFilterCallLogs()
													} else if let conversationsListVM = conversationsListViewModel, sharedMainViewModel.indexView == 2 {
														conversationsListVM.resetFilterConversations()
													} else if let meetingsListVM = meetingsListViewModel, sharedMainViewModel.indexView == 3 {
														meetingsListVM.currentFilter = ""
														meetingsListVM.computeMeetingsList()
													}
												} label: {
													Image("caret-left")
														.renderingMode(.template)
														.resizable()
														.foregroundStyle(.white)
														.frame(width: 25, height: 25, alignment: .leading)
														.padding(.all, 10)
														.padding(.leading, -10)
												}
												
												if #available(iOS 16.0, *) {
													TextEditor(text: Binding(
														get: {
															return text
														},
														set: { value in
															var newValue = value
															if value.contains("\n") {
																newValue = value.replacingOccurrences(of: "\n", with: "")
															}
															text = newValue
														}
													))
													.default_text_style_white_700(styleSize: 15)
													.padding(.all, 6)
													.disableAutocorrection(true)
													.autocapitalization(.none)
													.accentColor(.white)
													.scrollContentBackground(.hidden)
													.focused($focusedField)
													.onAppear {
														self.focusedField = true
													}
													.onChange(of: text) { newValue in
														if sharedMainViewModel.indexView == 0 {
															magicSearch.currentFilter = newValue
															magicSearch.searchForContacts()
														} else if let historyListVM = historyListViewModel, sharedMainViewModel.indexView == 1 {
															if text.isEmpty {
																historyListVM.resetFilterCallLogs()
															} else {
																historyListVM.filterCallLogs(filter: text)
															}
														} else if let conversationsListVM = conversationsListViewModel, sharedMainViewModel.indexView == 2 {
															if text.isEmpty {
																conversationsListVM.resetFilterConversations()
															} else {
																conversationsListVM.filterConversations(filter: text)
															}
														} else if let meetingsListVM = meetingsListViewModel, sharedMainViewModel.indexView == 3 {
															meetingsListVM.currentFilter = text
															meetingsListVM.computeMeetingsList()
														}
													}
												} else {
													TextEditor(text: Binding(
														get: {
															return text
														},
														set: { value in
															var newValue = value
															if value.contains("\n") {
																newValue = value.replacingOccurrences(of: "\n", with: "")
															}
															text = newValue
														}
													))
													.default_text_style_700(styleSize: 15)
													.padding(.all, 6)
													.focused($focusedField)
													.disableAutocorrection(true)
													.autocapitalization(.none)
													.onAppear {
														self.focusedField = true
													}
													.onChange(of: text) { newValue in
														if sharedMainViewModel.indexView == 0 {
															magicSearch.currentFilter = newValue
															magicSearch.searchForContacts()
														} else if let historyListVM = historyListViewModel, sharedMainViewModel.indexView == 1 {
															historyListVM.filterCallLogs(filter: text)
														} else if let conversationsListVM = conversationsListViewModel, sharedMainViewModel.indexView == 2 {
															conversationsListVM.filterConversations(filter: text)
														} else if let meetingsListVM = meetingsListViewModel, sharedMainViewModel.indexView == 3 {
															meetingsListVM.currentFilter = text
															meetingsListVM.computeMeetingsList()
														}
													}
												}
												
												Button {
													text = ""
												} label: {
													Image("x")
														.renderingMode(.template)
														.resizable()
														.foregroundStyle(.white)
														.frame(width: 25, height: 25, alignment: .leading)
														.padding(.all, 10)
												}
												.padding(.leading)
											}
											.frame(maxWidth: .infinity)
											.frame(height: 50)
											.padding(.horizontal)
											.padding(.bottom, 5)
											.background(Color.orangeMain500)
											.roundedCorner(10, corners: [.bottomRight, .bottomLeft])
										}
										
										if sharedMainViewModel.indexView == 0 {
											ContactsContainer(
												contactsListViewModel: $contactsListViewModel,
												isShowEditContactFragment: $isShowEditContactFragment,
												isShowDeleteContactPopup: $isShowDeleteContactPopup,
												text: $text,
												orientation: orientation
											)
										} else if sharedMainViewModel.indexView == 1 {
											HistoryContainer(
												historyListViewModel: $historyListViewModel,
												isShowStartCallFragment: $isShowStartCallFragment,
												isShowEditContactFragment: $isShowEditContactFragment,
												text: $text,
												isShowEditContactFragmentAddress: $isShowEditContactFragmentAddress,
												orientation: orientation
											)
										} else if sharedMainViewModel.indexView == 2 {
											ConversationsContainer(
												conversationsListViewModel: $conversationsListViewModel,
												isShowStartConversationFragment: $isShowStartConversationFragment,
												text: $text,
												orientation: orientation
											)
										} else if sharedMainViewModel.indexView == 3 {
											MeetingsContainer(
												meetingsListViewModel: $meetingsListViewModel,
												isShowScheduleMeetingFragment: $isShowScheduleMeetingFragment,
												isShowSendCancelMeetingNotificationPopup: $isShowSendCancelMeetingNotificationPopup,
												text: $text,
												orientation: orientation
											)
										}
									}
								}
							}
							.frame(maxWidth:
									(orientation == .landscapeLeft
									 || orientation == .landscapeRight
									 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)
								   ? geometry.size.width/100*40
								   : .infinity
							)
							.background(
								Color.white
									.shadow(color: Color.gray200, radius: 4, x: 0, y: 0)
									.mask(Rectangle().padding(.horizontal, -8))
							)
							
							if orientation == .landscapeLeft
								|| orientation == .landscapeRight
								|| UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height {
								Spacer()
							}
						}
						
						if !(orientation == .landscapeLeft
							 || orientation == .landscapeRight
							 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) && !searchIsActive {
							HStack {
								Group {
									Spacer()
									Button(action: {
										sharedMainViewModel.changeIndexView(indexViewInt: 0)
										sharedMainViewModel.displayedCall = nil
										sharedMainViewModel.displayedConversation = nil
										sharedMainViewModel.displayedMeeting = nil
									}, label: {
										VStack {
											Image("address-book")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(sharedMainViewModel.indexView == 0 ? Color.orangeMain500 : Color.grayMain2c600)
												.frame(width: 25, height: 25)
											if sharedMainViewModel.indexView == 0 {
												Text("bottom_navigation_contacts_label")
													.default_text_style_700(styleSize: 10)
											} else {
												Text("bottom_navigation_contacts_label")
													.default_text_style(styleSize: 10)
											}
										}
									})
									.padding(.top)
									.frame(width: 66)
									
									Spacer()
									
									ZStack {
										if SharedMainViewModel.shared.missedCallsCount > 0 {
											VStack {
												HStack {
													Text(
                                                        SharedMainViewModel.shared.missedCallsCount < 99
														? String(SharedMainViewModel.shared.missedCallsCount)
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
											.padding(.bottom, 30)
											.padding(.leading, 30)
										}
										
										Button(action: {
											sharedMainViewModel.changeIndexView(indexViewInt: 1)
											sharedMainViewModel.displayedFriend = nil
											sharedMainViewModel.displayedConversation = nil
											sharedMainViewModel.displayedMeeting = nil
											if SharedMainViewModel.shared.missedCallsCount > 0 {
                                                SharedMainViewModel.shared.resetMissedCallsCount()
											}
										}, label: {
											VStack {
												Image("phone")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(sharedMainViewModel.indexView == 1 ? Color.orangeMain500 : Color.grayMain2c600)
													.frame(width: 25, height: 25)
												if sharedMainViewModel.indexView == 1 {
													Text("bottom_navigation_calls_label")
														.default_text_style_700(styleSize: 9)
												} else {
													Text("bottom_navigation_calls_label")
														.default_text_style(styleSize: 9)
												}
											}
										})
										.padding(.top)
										.frame(width: 66)
									}
                                    
                                    if !sharedMainViewModel.disableChatFeature {
                                        Spacer()
                                    
                                        ZStack {
                                            if SharedMainViewModel.shared.unreadMessages > 0 {
                                                VStack {
                                                    HStack {
                                                        Text(
                                                            SharedMainViewModel.shared.unreadMessages < 99
                                                            ? String(SharedMainViewModel.shared.unreadMessages)
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
                                                .padding(.bottom, 30)
                                                .padding(.leading, 30)
                                            }
                                            
                                            Button(action: {
                                                sharedMainViewModel.changeIndexView(indexViewInt: 2)
                                                sharedMainViewModel.displayedFriend = nil
                                                sharedMainViewModel.displayedCall = nil
                                                sharedMainViewModel.displayedMeeting = nil
                                            }, label: {
                                                VStack {
                                                    Image("chat-teardrop-text")
                                                        .renderingMode(.template)
                                                        .resizable()
                                                        .foregroundStyle(sharedMainViewModel.indexView == 2 ? Color.orangeMain500 : Color.grayMain2c600)
                                                        .frame(width: 25, height: 25)
                                                    
                                                    if sharedMainViewModel.indexView == 2 {
                                                        Text("bottom_navigation_conversations_label")
                                                            .default_text_style_700(styleSize: 9)
                                                    } else {
                                                        Text("bottom_navigation_conversations_label")
                                                            .default_text_style(styleSize: 9)
                                                    }
                                                }
                                            })
                                            .padding(.top)
                                            .frame(width: 66)
                                        }
                                    }
									
									if !sharedMainViewModel.disableMeetingFeature {
										Spacer()
										Button(action: {
											sharedMainViewModel.changeIndexView(indexViewInt: 3)
											sharedMainViewModel.displayedFriend = nil
											sharedMainViewModel.displayedCall = nil
											sharedMainViewModel.displayedConversation = nil
										}, label: {
											VStack {
												Image("video-conference")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(sharedMainViewModel.indexView == 3 ? Color.orangeMain500 : Color.grayMain2c600)
													.frame(width: 25, height: 25)
												if sharedMainViewModel.indexView == 3 {
													Text("bottom_navigation_meetings_label")
														.default_text_style_700(styleSize: 9)
												} else {
													Text("bottom_navigation_meetings_label")
														.default_text_style(styleSize: 9)
												}
											}
										})
										.padding(.top)
										.frame(width: 66)
									}
									
									Spacer()
								}
							}
							.padding(.bottom, geometry.safeAreaInsets.bottom > 0 ? 0 : 15)
							.background(
								Color.white
									.shadow(color: Color.gray200, radius: 4, x: 0, y: 0)
									.mask(Rectangle().padding(.top, -8))
							)
						}
					}
					
					if sharedMainViewModel.displayedFriend != nil || sharedMainViewModel.displayedCall != nil || sharedMainViewModel.displayedConversation != nil ||
						sharedMainViewModel.displayedMeeting != nil {
						HStack(spacing: 0) {
							Spacer()
								.frame(maxWidth:
										(orientation == .landscapeLeft
										 || orientation == .landscapeRight
										 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)
									   ? (geometry.size.width/100*40) + 75
									   : 0
								)
							if let contactsListVM = contactsListViewModel, let displayedFriend = sharedMainViewModel.displayedFriend, sharedMainViewModel.indexView == 0 {
								ContactFragment(
									isShowDeletePopup: $isShowDeleteContactPopup,
									isShowDismissPopup: $isShowDismissPopup,
									isShowSipAddressesPopup: $isShowSipAddressesPopup,
									isShowSipAddressesPopupType: $isShowSipAddressesPopupType,
									isShowEditContactFragmentInContactDetails: $isShowEditContactFragmentInContactDetails
								)
								.environmentObject(contactsListVM)
								.environmentObject(displayedFriend)
								.frame(maxWidth: .infinity)
								.background(Color.gray100)
								.ignoresSafeArea(.keyboard)
							} else if let historyListVM = historyListViewModel, let displayedCall = sharedMainViewModel.displayedCall, sharedMainViewModel.indexView == 1 {
								HistoryContactFragment(
									isShowDeleteAllHistoryPopup: $isShowDeleteAllHistoryPopup,
									isShowEditContactFragment: $isShowEditContactFragment,
									isShowEditContactFragmentAddress: $isShowEditContactFragmentAddress
								)
								.environmentObject(historyListVM)
								.environmentObject(displayedCall)
								.frame(maxWidth: .infinity)
								.background(Color.gray100)
								.ignoresSafeArea(.keyboard)
							} else if let conversationsListVM = conversationsListViewModel, let displayedConversation = sharedMainViewModel.displayedConversation, sharedMainViewModel.indexView == 2 {
								ConversationFragment(
									isShowConversationFragment: $isShowConversationFragment,
									isShowStartCallGroupPopup: $isShowStartCallGroupPopup,
									isShowEditContactFragment: $isShowEditContactFragment,
									isShowEditContactFragmentAddress: $isShowEditContactFragmentAddress,
									isShowScheduleMeetingFragment: $isShowScheduleMeetingFragment,
									isShowScheduleMeetingFragmentSubject: $isShowScheduleMeetingFragmentSubject,
									isShowScheduleMeetingFragmentParticipants: $isShowScheduleMeetingFragmentParticipants,
									isShowConversationInfoPopup: $isShowConversationInfoPopup,
									conversationInfoPopupText: $conversationInfoPopupText
								)
								.environmentObject(conversationsListVM)
								.environmentObject(accountProfileViewModel)
								.frame(maxWidth: .infinity)
								.background(Color.gray100)
								.ignoresSafeArea(.keyboard)
							} else if let meetingsListVM = meetingsListViewModel, let displayedMeeting = sharedMainViewModel.displayedMeeting, sharedMainViewModel.indexView == 3 {
								MeetingFragment(isShowScheduleMeetingFragment: $isShowScheduleMeetingFragment, isShowSendCancelMeetingNotificationPopup: $isShowSendCancelMeetingNotificationPopup)
									.environmentObject(meetingsListVM)
									.frame(maxWidth: .infinity)
									.background(Color.gray100)
									.ignoresSafeArea(.keyboard)
							}
							
						}
						.onAppear {
							if !(orientation == .landscapeLeft
								 || orientation == .landscapeRight
								 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)
								&& searchIsActive {
								self.focusedField = false
							}
						}
						.onDisappear {
							if !(orientation == .landscapeLeft
								 || orientation == .landscapeRight
								 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)
								&& searchIsActive {
								self.focusedField = true
							}
						}
						.padding(.leading,
								 orientation == .landscapeRight && geometry.safeAreaInsets.bottom > 0
								 ? -geometry.safeAreaInsets.leading
								 : 0)
						.transition(.move(edge: .trailing))
						.zIndex(1)
					}
					
					SideMenu(
						width: geometry.size.width / 5 * 4,
						isOpen: $sideMenuIsOpen,
						menuClose: self.openMenu,
						safeAreaInsets: geometry.safeAreaInsets,
						isShowLoginFragment: $isShowLoginFragment,
						isShowAccountProfileFragment: $isShowAccountProfileFragment,
						isShowSettingsFragment: $isShowSettingsFragment,
						isShowHelpFragment: $isShowHelpFragment
					)
					.environmentObject(accountProfileViewModel)
					.ignoresSafeArea(.all)
					.zIndex(2)
					
					if isShowLoginFragment {
						LoginFragment(
							isShowBack: true,
							onBackPressed: {
								withAnimation {
									isShowLoginFragment.toggle()
								}
							})
						.zIndex(3)
						.transition(.move(edge: .bottom))
						.onAppear {
						}
					}
					
					if isShowEditContactFragment {
						VStack {
							EditContactFragment(
								isShowEditContactFragment: $isShowEditContactFragment,
								isShowDismissPopup: $isShowDismissPopup,
								isShowEditContactFragmentAddress: isShowEditContactFragmentAddress
							)
							.frame(height: geometry.size.height)
							.onAppear {
								sharedMainViewModel.displayedFriend = nil
								isShowEditContactFragmentAddress = ""
							}
							
							Spacer()
						}
						.zIndex(3)
						.transition(.opacity.combined(with: .move(edge: .bottom)))
					}
					
					if isShowStartCallFragment {
						StartCallFragment(
							isShowStartCallFragment: $isShowStartCallFragment,
							showingDialer: $showingDialer,
							resetCallView: {callViewModel.resetCallView()}
						)
						.environmentObject(callViewModel)
						.zIndex(6)
						.transition(.opacity.combined(with: .move(edge: .bottom)))
					}
					
					if let conversationsListVM = conversationsListViewModel, isShowStartConversationFragment {
						StartConversationFragment(
							isShowStartConversationFragment: $isShowStartConversationFragment
						)
						.environmentObject(conversationsListVM)
						.zIndex(6)
						.transition(.opacity.combined(with: .move(edge: .bottom)))
					}
					
					if let contactsListVM = contactsListViewModel, isShowDeleteContactPopup {
						PopupView(
							isShowPopup: $isShowDeleteContactPopup,
							title: Text(
								String(
									format: String(localized: "contact_dialog_delete_title"),
									contactsListVM.selectedFriend?.name
									?? (SharedMainViewModel.shared.displayedFriend!.name ?? "Unknown Contact")
								)
							),
							content: Text("contact_dialog_delete_message"),
							titleFirstButton: Text("dialog_cancel"),
							actionFirstButton: {
								self.isShowDeleteContactPopup.toggle()},
							titleSecondButton: Text("dialog_ok"),
							actionSecondButton: {
								contactsListVM.deleteSelectedContact()
								self.isShowDeleteContactPopup.toggle()
						})
						.background(.black.opacity(0.65))
						.zIndex(3)
						.onTapGesture {
							self.isShowDeleteContactPopup.toggle()
						}
						.onAppear {
							contactsListVM.changeSelectedFriendToDelete()
						}
					}
					
					if isShowDeleteAllHistoryPopup {
						PopupView(isShowPopup: $isShowDeleteContactPopup,
								  title: Text("history_dialog_delete_all_call_logs_title"),
								  content: Text("history_dialog_delete_all_call_logs_message"),
								  titleFirstButton: Text("dialog_cancel"),
								  actionFirstButton: {
							self.isShowDeleteAllHistoryPopup.toggle()
							if let historyListVM = historyListViewModel {
								historyListVM.callLogsAddressToDelete = ""
							}
						},
								  titleSecondButton: Text("dialog_ok"),
								  actionSecondButton: {
							if let historyListVM = historyListViewModel {
								historyListVM.removeCallLogs()
							}
							self.isShowDeleteAllHistoryPopup.toggle()
							sharedMainViewModel.displayedCall = nil
							
							ToastViewModel.shared.toastMessage = "Success_remove_call_logs"
							ToastViewModel.shared.displayToast.toggle()
						})
						.background(.black.opacity(0.65))
						.zIndex(3)
						.onTapGesture {
							self.isShowDeleteAllHistoryPopup.toggle()
						}
					}
					
					if isShowDismissPopup {
						PopupView(isShowPopup: $isShowDismissPopup,
								  title: Text("contact_editor_dialog_abort_confirmation_title"),
								  content: Text("contact_editor_dialog_abort_confirmation_message"),
								  titleFirstButton: Text("dialog_cancel"),
								  actionFirstButton: {self.isShowDismissPopup.toggle()},
								  titleSecondButton: Text("dialog_ok"),
								  actionSecondButton: {
							self.isShowDismissPopup.toggle()
							if isShowEditContactFragment {
								isShowEditContactFragment = false
							} else {
								isShowEditContactFragmentInContactDetails = false
							}
						})
						.background(.black.opacity(0.65))
						.zIndex(3)
						.onTapGesture {
							self.isShowDismissPopup.toggle()
						}
					}
					
					if let contactsListVM = contactsListViewModel, let displayedFriend = sharedMainViewModel.displayedFriend, isShowSipAddressesPopup {
						SipAddressesPopup(
							isShowSipAddressesPopup: $isShowSipAddressesPopup,
							isShowSipAddressesPopupType: $isShowSipAddressesPopupType
						)
						.environmentObject(contactsListVM)
						.environmentObject(displayedFriend)
						.background(.black.opacity(0.65))
						.zIndex(3)
						.onTapGesture {
							isShowSipAddressesPopup.toggle()
						}
					}
					
					if sharedMainViewModel.operationInProgress {
						PopupLoadingView()
							.background(.black.opacity(0.65))
							.zIndex(3)
							.onDisappear {
								if let contactsListVM = contactsListViewModel, let displayedConversation = contactsListVM.displayedConversation {
                                    
                                    if !sharedMainViewModel.disableChatFeature {
                                        sharedMainViewModel.displayedFriend = nil
                                        sharedMainViewModel.displayedCall = nil
                                        sharedMainViewModel.changeIndexView(indexViewInt: 2)
                                        
                                        if let conversationsListVM = self.conversationsListViewModel {
                                            DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                                                withAnimation {
                                                    conversationsListVM.changeDisplayedChatRoom(conversationModel: displayedConversation)
                                                }
                                                contactsListVM.displayedConversation = nil
                                            }
                                        } else {
                                            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                                                if let conversationsListVM = self.conversationsListViewModel {
                                                    withAnimation {
                                                        conversationsListVM.changeDisplayedChatRoom(conversationModel: displayedConversation)
                                                    }
                                                }
                                                contactsListVM.displayedConversation = nil
                                            }
                                        }
                                    }
								} else if let historyListVM = historyListViewModel, let displayedConversation = historyListVM.displayedConversation {
                                    
                                    if !sharedMainViewModel.disableChatFeature {
                                        sharedMainViewModel.displayedFriend = nil
                                        sharedMainViewModel.displayedCall = nil
                                        sharedMainViewModel.changeIndexView(indexViewInt: 2)
                                        
                                        if let conversationsListVM = self.conversationsListViewModel {
                                            DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                                                withAnimation {
                                                    conversationsListVM.changeDisplayedChatRoom(conversationModel: displayedConversation)
                                                }
                                                historyListVM.displayedConversation = nil
                                            }
                                        } else {
                                            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                                                if let conversationsListVM = self.conversationsListViewModel {
                                                    withAnimation {
                                                        conversationsListVM.changeDisplayedChatRoom(conversationModel: displayedConversation)
                                                    }
                                                }
                                                historyListVM.displayedConversation = nil
                                            }
                                        }
                                    }
								}
							}
					}
					
					if let meetingsListVM = meetingsListViewModel, isShowScheduleMeetingFragment {
						ScheduleMeetingFragment(
							isShowScheduleMeetingFragmentSubject: isShowScheduleMeetingFragmentSubject,
							isShowScheduleMeetingFragmentParticipants: isShowScheduleMeetingFragmentParticipants,
							isShowScheduleMeetingFragment: $isShowScheduleMeetingFragment
						)
						.environmentObject(meetingsListVM)
						.zIndex(3)
						.transition(.move(edge: .bottom))
						.onAppear {
							isShowScheduleMeetingFragmentSubject = ""
							isShowScheduleMeetingFragmentParticipants = []
						}
					}
					
					if isShowAccountProfileFragment {
						AccountProfileFragment(
							isShowAccountProfileFragment: $isShowAccountProfileFragment
						)
						.environmentObject(accountProfileViewModel)
						.zIndex(3)
						.transition(.move(edge: .trailing))
					}
					
					if isShowSettingsFragment {
						SettingsFragment(
							isShowSettingsFragment: $isShowSettingsFragment
						)
						.zIndex(3)
						.transition(.move(edge: .trailing))
					}
					
					if isShowHelpFragment {
						HelpFragment(
							isShowHelpFragment: $isShowHelpFragment
						)
						.zIndex(3)
						.transition(.move(edge: .trailing))
					}
					
					if  let meetingsListVM = meetingsListViewModel, isShowSendCancelMeetingNotificationPopup {
						PopupView(isShowPopup: $isShowSendCancelMeetingNotificationPopup,
								  title: Text("meeting_schedule_cancel_dialog_title"),
                                  content: !sharedMainViewModel.disableChatFeature ? Text("meeting_schedule_cancel_dialog_message") : Text(""),
								  titleFirstButton: Text("dialog_cancel"),
								  actionFirstButton: {
							sharedMainViewModel.displayedMeeting = nil
							meetingsListVM.deleteSelectedMeeting()
							self.isShowSendCancelMeetingNotificationPopup.toggle(
							) },
								  titleSecondButton: Text("dialog_ok"),
								  actionSecondButton: {
							sharedMainViewModel.displayedMeeting = nil
							meetingsListVM.cancelMeetingWithNotifications()
							self.isShowSendCancelMeetingNotificationPopup.toggle()
						})
						.background(.black.opacity(0.65))
						.zIndex(3)
						.onTapGesture {
							self.isShowSendCancelMeetingNotificationPopup.toggle()
						}
					}
					
					if isShowStartCallGroupPopup {
						PopupView(
							isShowPopup: $isShowStartCallGroupPopup,
							title: Text("conversation_info_confirm_start_group_call_dialog_title"),
							content: Text("conversation_info_confirm_start_group_call_dialog_message"),
							titleFirstButton: Text("dialog_cancel"),
							actionFirstButton: {
								self.isShowStartCallGroupPopup.toggle()
							},
							titleSecondButton: Text("dialog_ok"),
							actionSecondButton: {
								if sharedMainViewModel.displayedConversation != nil {
									sharedMainViewModel.displayedConversation!.createGroupCall()
								}
								self.isShowStartCallGroupPopup.toggle()
							}
						)
						.background(.black.opacity(0.65))
						.zIndex(3)
						.onTapGesture {
							self.isShowStartCallGroupPopup.toggle()
						}
					}
					
					if isShowConversationInfoPopup {
						PopupViewWithTextField(
							isShowConversationInfoPopup: $isShowConversationInfoPopup,
							conversationInfoPopupText: $conversationInfoPopupText
						)
						.background(.black.opacity(0.65))
						.zIndex(3)
						.onTapGesture {
							self.isShowConversationInfoPopup.toggle()
						}
					}
					
					if isShowUpdatePasswordPopup {
						PopupUpdatePassword(
							isShowUpdatePasswordPopup: $isShowUpdatePasswordPopup,
							passwordUpdateAddress: $passwordUpdateAddress
						)
						.background(.black.opacity(0.65))
						.zIndex(3)
						.onTapGesture {
							self.isShowUpdatePasswordPopup.toggle()
						}
					}
					
					if telecomManager.meetingWaitingRoomDisplayed {
						MeetingWaitingRoomFragment()
							.zIndex(3)
							.transition(.opacity.combined(with: .move(edge: .bottom)))
					}
					
					if telecomManager.callDisplayed && ((telecomManager.callInProgress && telecomManager.outgoingCallStarted) || telecomManager.callConnected) && !telecomManager.meetingWaitingRoomDisplayed {
						CallView(
							fullscreenVideo: $fullscreenVideo,
							isShowStartCallFragment: $isShowStartCallFragment,
							isShowConversationFragment: $isShowConversationFragment,
							isShowStartCallGroupPopup: $isShowStartCallGroupPopup,
							isShowEditContactFragment: $isShowEditContactFragment,
							isShowScheduleMeetingFragment: $isShowScheduleMeetingFragment
						)
						.environmentObject(callViewModel)
						.zIndex(5)
						.transition(.scale.combined(with: .move(edge: .top)))
						.onAppear {
							UIApplication.shared.isIdleTimerDisabled = true
							callViewModel.resetCallView()
							if callViewModel.callsCounter >= 1 {
								DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
									callViewModel.resetCallView()
								}
							}
						}
						.onDisappear {
							UIApplication.shared.isIdleTimerDisabled = false
						}
					}
					
					ToastView()
						.zIndex(6)
				}
			}
			.onChange(of: navigationManager.selectedCallId) { newCallId in
				if newCallId != nil {
                    if !sharedMainViewModel.disableChatFeature {
                        sharedMainViewModel.changeIndexView(indexViewInt: 2)
                    }
				}
			}
			.onReceive(NotificationCenter.default.publisher(for: NSNotification.Name("ContactLoaded"))) { _ in
				if let conversationsListVM = conversationsListViewModel {
					conversationsListVM.updateChatRoomsList()
				}
				
				if let historyListVM = historyListViewModel {
					historyListVM.refreshHistoryAvatarModel()
				}
			}
			.onReceive(NotificationCenter.default.publisher(for: NSNotification.Name("ContactAdded")).compactMap { $0.userInfo?["address"] as? String }) { address in
				if let conversationsListVM = conversationsListViewModel {
					conversationsListVM.updateChatRoom(address: address)
				}
			}
			.onReceive(NotificationCenter.default.publisher(for: NSNotification.Name("CoreStarted"))) { _ in
				accountProfileViewModel.setAvatarModel()
			}
			.onReceive(NotificationCenter.default.publisher(for: NSNotification.Name("DefaultAccountChanged"))) { _ in
                accountProfileViewModel.defaultAccountModelIndex = CoreContext.shared.accounts.firstIndex(where: {$0.isDefaultAccount})
								
				accountProfileViewModel.accountError = CoreContext.shared.accounts.contains {
					($0.registrationState == .Cleared && $0.isDefaultAccount) ||
					$0.registrationState == .Failed
				}
				
                withAnimation {
                    if self.sideMenuIsOpen {
                        self.sideMenuIsOpen = false
                    }
                }
                
                if self.isShowLoginFragment {
                    self.isShowLoginFragment = false
                }
                
                if conversationsListViewModel != nil {
                    conversationsListViewModel = ConversationsListViewModel()
                }
                
				if historyListViewModel != nil {
                    historyListViewModel = HistoryListViewModel()
				}
				
				if meetingsListViewModel != nil {
                    meetingsListViewModel = MeetingsListViewModel()
				}
			}
			.onReceive(NotificationCenter.default.publisher(for: NSNotification.Name("PasswordUpdate")).compactMap { $0.userInfo?["address"] as? String }) { address in
				passwordUpdateAddress = address
				isShowUpdatePasswordPopup = true
			}
		}
		.overlay {
			if isMenuOpen {
				Color.white.opacity(0.001)
					.ignoresSafeArea()
					.frame(maxWidth: .infinity, maxHeight: .infinity)
					.onTapGesture {
						isMenuOpen = false
					}
			}
		}
		.onRotate { newOrientation in
			if (sharedMainViewModel.displayedFriend != nil || sharedMainViewModel.displayedCall != nil || sharedMainViewModel.displayedConversation != nil) && searchIsActive {
				self.focusedField = false
			} else if searchIsActive {
				self.focusedField = true
			}
			orientation = newOrientation
		}
		.onChange(of: scenePhase) { newPhase in
			orientation = UIDevice.current.orientation
			if newPhase == .active {
				if let conversationsListVM = conversationsListViewModel {
					conversationsListVM.computeChatRoomsList()
				}
			}
		}
	}
	
	func openMenu() {
		withAnimation {
			self.sideMenuIsOpen.toggle()
		}
	}
}

struct ContactsContainer: View {
	@Binding var contactsListViewModel: ContactsListViewModel?
	@Binding var isShowEditContactFragment: Bool
	@Binding var isShowDeleteContactPopup: Bool
	@Binding var text: String
	var orientation: UIDeviceOrientation

	var body: some View {
		Group {
			if let contactsListVM = contactsListViewModel {
				ContactsView(
					isShowEditContactFragment: $isShowEditContactFragment,
					isShowDeletePopup: $isShowDeleteContactPopup,
					text: $text
				)
				.environmentObject(contactsListVM)
				.roundedCorner(25, corners: [.topRight, .topLeft])
				.shadow(
					color: (orientation == .landscapeLeft
							|| orientation == .landscapeRight
							|| UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)
					? .white.opacity(0.0)
					: .black.opacity(0.2),
					radius: 25
				)
			} else {
				NavigationView {
					VStack {
						Spacer()
						
						ProgressView()
							.controlSize(.large)
						
						Spacer()
					}
					.onAppear {
						if contactsListViewModel == nil {
							contactsListViewModel = ContactsListViewModel()
						}
					}
				}
			}
		}
	}
}

struct HistoryContainer: View {
	@Binding var historyListViewModel: HistoryListViewModel?
	@Binding var isShowStartCallFragment: Bool
	@Binding var isShowEditContactFragment: Bool
	@Binding var text: String
	@Binding var isShowEditContactFragmentAddress: String
	var orientation: UIDeviceOrientation

	var body: some View {
		Group {
			if let historyListVM = historyListViewModel {
				HistoryView(
					isShowStartCallFragment: $isShowStartCallFragment,
					isShowEditContactFragment: $isShowEditContactFragment,
					text: $text,
					isShowEditContactFragmentAddress: $isShowEditContactFragmentAddress
				)
				.environmentObject(historyListVM)
				.roundedCorner(25, corners: [.topRight, .topLeft])
				.shadow(
					color: (orientation == .landscapeLeft
							|| orientation == .landscapeRight
							|| UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)
					? .white.opacity(0.0)
					: .black.opacity(0.2),
					radius: 25
				)
			} else {
				NavigationView {
					VStack {
						Spacer()
						
						ProgressView()
							.controlSize(.large)
						
						Spacer()
					}
					.onAppear {
						if historyListViewModel == nil {
							historyListViewModel = HistoryListViewModel()
						}
					}
				}
			}
		}
	}
}

struct ConversationsContainer: View {
	@Binding var conversationsListViewModel: ConversationsListViewModel?
	@Binding var isShowStartConversationFragment: Bool
	@Binding var text: String
	var orientation: UIDeviceOrientation

	var body: some View {
		Group {
			if let conversationsListVM = conversationsListViewModel {
				ConversationsView(
					text: $text,
					isShowStartConversationFragment: $isShowStartConversationFragment
				)
				.environmentObject(conversationsListVM)
				.roundedCorner(25, corners: [.topRight, .topLeft])
				.shadow(
					color: (orientation == .landscapeLeft
							|| orientation == .landscapeRight
							|| UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)
					? .white.opacity(0.0)
					: .black.opacity(0.2),
					radius: 25
				)
			} else {
				NavigationView {
					VStack {
						Spacer()
						
						ProgressView()
							.controlSize(.large)
						
						Spacer()
					}
					.onAppear {
						if conversationsListViewModel == nil {
							conversationsListViewModel = ConversationsListViewModel()
						}
					}
				}
			}
		}
	}
}

struct MeetingsContainer: View {
	@Binding var meetingsListViewModel: MeetingsListViewModel?
	@Binding var isShowScheduleMeetingFragment: Bool
	@Binding var isShowSendCancelMeetingNotificationPopup: Bool
	@Binding var text: String
	var orientation: UIDeviceOrientation

	var body: some View {
		Group {
			if let meetingsListVM = meetingsListViewModel {
				MeetingsView(
					isShowScheduleMeetingFragment: $isShowScheduleMeetingFragment,
					isShowSendCancelMeetingNotificationPopup: $isShowSendCancelMeetingNotificationPopup,
					text: $text
				)
				.environmentObject(meetingsListVM)
				.roundedCorner(25, corners: [.topRight, .topLeft])
				.shadow(
					color: (orientation == .landscapeLeft
							|| orientation == .landscapeRight
							|| UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)
					? .white.opacity(0.0)
					: .black.opacity(0.2),
					radius: 25
				)
			} else {
				NavigationView {
					VStack {
						Spacer()
						
						ProgressView()
							.controlSize(.large)
						
						Spacer()
					}
					.onAppear {
						if meetingsListViewModel == nil {
							meetingsListViewModel = MeetingsListViewModel()
						}
					}
				}
			}
		}
	}
}


class NavigationManager: ObservableObject {
	@Published var selectedCallId: String?
	@Published var peerAddr: String?
	@Published var localAddr: String?
	
	func openChatRoom(callId: String, peerAddr: String, localAddr: String) {
		self.selectedCallId = callId
		self.peerAddr = peerAddr
		self.localAddr = localAddr
	}
}

#Preview {
	ContentView()
}
// swiftlint:enable type_body_length
// swiftlint:enable line_length
