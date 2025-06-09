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
	
	@ObservedObject private var coreContext = CoreContext.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	@ObservedObject private var contactsManager = ContactsManager.shared
	@ObservedObject private var magicSearch = MagicSearchSingleton.shared
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	
	@StateObject private var callViewModel = CallViewModel()
	@StateObject private var accountProfileViewModel = AccountProfileViewModel()
	
	@State private var contactsListViewModel: ContactsListViewModel?
	@State private var historyListViewModel: HistoryListViewModel?
	
	//@ObservedObject var startConversationViewModel: StartConversationViewModel
	
	//@ObservedObject var meetingWaitingRoomViewModel: MeetingWaitingRoomViewModel
	
	//@ObservedObject var conversationsListViewModel: ConversationsListViewModel
	//@ObservedObject var conversationViewModel: ConversationViewModel
	
	//@ObservedObject var meetingsListViewModel: MeetingsListViewModel
	//@ObservedObject var meetingViewModel: MeetingViewModel
	
	//@ObservedObject var conversationForwardMessageViewModel: ConversationForwardMessageViewModel
	
	
	//@Binding var index: Int
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
	@State private var isShowLoginFragment: Bool = false
	
	private let avatarSize = 45.0
	@State private var imagePath: URL?
	@State private var imageTmp: Image?
	
	var body: some View {
		let contactLoaded = NotificationCenter.default
			.publisher(for: NSNotification.Name("ContactLoaded"))
		let contactAdded = NotificationCenter.default
				.publisher(for: NSNotification.Name("ContactAdded"))
				.compactMap { $0.userInfo?["address"] as? String }
		let imageChanged = NotificationCenter.default
			.publisher(for: NSNotification.Name("ImageChanged"))
		let coreStarted = NotificationCenter.default
			.publisher(for: NSNotification.Name("CoreStarted"))
		GeometryReader { geometry in
			VStack(spacing: 0) {
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
					.frame(height: 30)
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
											if historyListViewModel != nil && historyListViewModel!.missedCallsCount > 0 {
												VStack {
													HStack {
														Text(
															historyListViewModel!.missedCallsCount < 99
															? String(historyListViewModel!.missedCallsCount)
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
												if historyListViewModel != nil && historyListViewModel!.missedCallsCount > 0 {
													historyListViewModel!.resetMissedCallsCount()
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
										
										ZStack {
											/*
											if conversationsListViewModel.unreadMessages > 0 {
												VStack {
													HStack {
														Text(
															conversationsListViewModel.unreadMessages < 99
															? String(conversationsListViewModel.unreadMessages)
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
											*/
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
												if let accountModelIndex = accountProfileViewModel.accountModelIndex,
												   accountModelIndex < coreContext.accounts.count {
													AsyncImage(url: imagePath) { image in
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
															if coreContext.accounts[accountModelIndex].avatarModel != nil {
																let tmpImage = contactsManager.textToImage(
																	firstName: coreContext.accounts[accountModelIndex].avatarModel!.name,
																	lastName: "")
																Image(uiImage: tmpImage)
																	.resizable()
																	.frame(width: avatarSize, height: avatarSize)
																	.clipShape(Circle())
																	.onAppear {
																		accountProfileViewModel.saveImage(image: tmpImage, name: coreContext.accounts[accountModelIndex].avatarModel!.name, prefix: "-default")
																	}
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
													.onTapGesture {
														openMenu()
													}
													.onAppear {
														let imagePathTmp = coreContext.accounts[accountModelIndex].getImagePath()
														if !(imagePathTmp.lastPathComponent.isEmpty || imagePathTmp.lastPathComponent == "Error" || imagePathTmp.lastPathComponent == "ImageError.png") {
															imagePath = imagePathTmp
														}
													}
													.onChange(of: coreContext.accounts[accountModelIndex].usernaneAvatar) { username in
														if !username.isEmpty {
															let imagePathTmp = coreContext.accounts[accountModelIndex].getImagePath()
															if !(imagePathTmp.lastPathComponent.isEmpty || imagePathTmp.lastPathComponent == "Error" || imagePathTmp.lastPathComponent == "ImageError.png") {
																sharedMainViewModel.changeDefaultAvatar(defaultAvatarURL: imagePathTmp)
																imagePath = imagePathTmp
															}
														}
													}
													.onReceive(imageChanged) { _ in
														if !coreContext.accounts[accountModelIndex].usernaneAvatar.isEmpty {
															let imagePathTmp = coreContext.accounts[accountModelIndex].getImagePath()
															sharedMainViewModel.changeDefaultAvatar(defaultAvatarURL: imagePathTmp)
															imagePath = imagePathTmp
														}
													}
												}
												
												Text(String(localized: sharedMainViewModel.indexView == 0 ? "bottom_navigation_contacts_label" : (sharedMainViewModel.indexView == 1 ? "bottom_navigation_calls_label" : (sharedMainViewModel.indexView == 2 ? "bottom_navigation_conversations_label" : "bottom_navigation_meetings_label"))))
													.default_text_style_white_800(styleSize: 20)
													.padding(.leading, 10)
												
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
																magicSearch.allContact = true
																magicSearch.searchForContacts(
																	sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
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
																magicSearch.allContact = false
																magicSearch.searchForContacts(
																	sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
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
														magicSearch.searchForContacts(
															sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
													} else if sharedMainViewModel.indexView == 1 && historyListViewModel != nil {
														historyListViewModel!.resetFilterCallLogs()
													} else if sharedMainViewModel.indexView == 2 {
														//conversationsListViewModel.resetFilterConversations()
													} else if sharedMainViewModel.indexView == 3 {
														//meetingsListViewModel.currentFilter = ""
														//meetingsListViewModel.computeMeetingsList()
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
															magicSearch.searchForContacts(
																sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
														} else if sharedMainViewModel.indexView == 1 {
															if text.isEmpty && historyListViewModel != nil {
																historyListViewModel!.resetFilterCallLogs()
															} else if historyListViewModel != nil {
																historyListViewModel!.filterCallLogs(filter: text)
															}
														} else if sharedMainViewModel.indexView == 2 {
															if text.isEmpty {
																//conversationsListViewModel.resetFilterConversations()
															} else {
																//conversationsListViewModel.filterConversations(filter: text)
															}
														} else if sharedMainViewModel.indexView == 3 {
															//meetingsListViewModel.currentFilter = text
															//meetingsListViewModel.computeMeetingsList()
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
															magicSearch.searchForContacts(
																sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
														} else if sharedMainViewModel.indexView == 1 && historyListViewModel != nil {
															historyListViewModel!.filterCallLogs(filter: text)
														} else if sharedMainViewModel.indexView == 2 {
															//conversationsListViewModel.filterConversations(filter: text)
														} else if sharedMainViewModel.indexView == 3 {
															//meetingsListViewModel.currentFilter = text
															//meetingsListViewModel.computeMeetingsList()
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
														contactsListViewModel = ContactsListViewModel()
													}
												}
											}
										} else if sharedMainViewModel.indexView == 1 {
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
														historyListViewModel = HistoryListViewModel()
													}
												}
											}
										} else if sharedMainViewModel.indexView == 2 {
											//TODO a changer
											NavigationView {
												ZStack(alignment: .bottomTrailing) {
												}
											}
											.navigationViewStyle(.stack)
											/*
											ConversationsView(
												conversationViewModel: conversationViewModel,
												conversationsListViewModel: conversationsListViewModel,
												text: $text,
												isShowStartConversationFragment: $isShowStartConversationFragment
											)
											.roundedCorner(25, corners: [.topRight, .topLeft])
											.shadow(
												color: (orientation == .landscapeLeft
														|| orientation == .landscapeRight
														|| UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)
												? .white.opacity(0.0)
												: .black.opacity(0.2),
												radius: 25
											)
											*/
										} else if sharedMainViewModel.indexView == 3 {
											//TODO a changer
											NavigationView {
												ZStack(alignment: .bottomTrailing) {
												}
											}
											.navigationViewStyle(.stack)
											/*
											MeetingsView(
												meetingsListViewModel: meetingsListViewModel,
												meetingViewModel: meetingViewModel,
												isShowScheduleMeetingFragment: $isShowScheduleMeetingFragment,
												isShowSendCancelMeetingNotificationPopup: $isShowSendCancelMeetingNotificationPopup,
												text: $text
											)
											.roundedCorner(25, corners: [.topRight, .topLeft])
											.shadow(
												color: (orientation == .landscapeLeft
														|| orientation == .landscapeRight
														|| UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)
												? .white.opacity(0.0)
												: .black.opacity(0.2),
												radius: 25
											)
											*/
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
										if historyListViewModel != nil && historyListViewModel!.missedCallsCount > 0 {
											VStack {
												HStack {
													Text(
														historyListViewModel!.missedCallsCount < 99
														? String(historyListViewModel!.missedCallsCount)
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
											if historyListViewModel != nil && historyListViewModel!.missedCallsCount > 0 {
												historyListViewModel!.resetMissedCallsCount()
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
									
									Spacer()
									
									ZStack {
										/*
										if conversationsListViewModel.unreadMessages > 0 {
											VStack {
												HStack {
													Text(
														conversationsListViewModel.unreadMessages < 99
														? String(conversationsListViewModel.unreadMessages)
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
										*/
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
							if sharedMainViewModel.indexView == 0 && sharedMainViewModel.displayedFriend != nil && contactsListViewModel != nil {
								ContactFragment(
									isShowDeletePopup: $isShowDeleteContactPopup,
									isShowDismissPopup: $isShowDismissPopup,
									isShowSipAddressesPopup: $isShowSipAddressesPopup,
									isShowSipAddressesPopupType: $isShowSipAddressesPopupType,
									isShowEditContactFragmentInContactDetails: $isShowEditContactFragmentInContactDetails
								)
								.environmentObject(contactsListViewModel!)
								.environmentObject(sharedMainViewModel.displayedFriend!)
								.frame(maxWidth: .infinity)
								.background(Color.gray100)
								.ignoresSafeArea(.keyboard)
							} else if sharedMainViewModel.indexView == 1 && sharedMainViewModel.displayedCall != nil && historyListViewModel != nil {
								HistoryContactFragment(
									isShowDeleteAllHistoryPopup: $isShowDeleteAllHistoryPopup,
									isShowEditContactFragment: $isShowEditContactFragment,
									isShowEditContactFragmentAddress: $isShowEditContactFragmentAddress
								)
								.environmentObject(historyListViewModel!)
								.environmentObject(sharedMainViewModel.displayedCall!)
								.frame(maxWidth: .infinity)
								.background(Color.gray100)
								.ignoresSafeArea(.keyboard)
							} else if sharedMainViewModel.indexView == 2 {
								/*
								ConversationFragment(
									conversationViewModel: conversationViewModel,
									conversationsListViewModel: conversationsListViewModel,
									conversationForwardMessageViewModel: conversationForwardMessageViewModel,
									contactsListViewModel: contactsListViewModel,
									editContactViewModel: editContactViewModel,
									meetingViewModel: meetingViewModel,
									accountProfileViewModel: accountProfileViewModel,
									isShowConversationFragment: $isShowConversationFragment,
									isShowStartCallGroupPopup: $isShowStartCallGroupPopup,
									isShowEditContactFragment: $isShowEditContactFragment,
									indexPage: $index,
									isShowScheduleMeetingFragment: $isShowScheduleMeetingFragment
								)
								.frame(maxWidth: .infinity)
								.background(Color.gray100)
								.ignoresSafeArea(.keyboard)
								 */
							} else if sharedMainViewModel.indexView == 3 {
								/*
								MeetingFragment(meetingViewModel: meetingViewModel, meetingsListViewModel: meetingsListViewModel, isShowScheduleMeetingFragment: $isShowScheduleMeetingFragment, isShowSendCancelMeetingNotificationPopup: $isShowSendCancelMeetingNotificationPopup)
									.frame(maxWidth: .infinity)
									.background(Color.gray100)
									.ignoresSafeArea(.keyboard)
								 */
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
					
					/*
					if isShowLoginFragment {
						LoginFragment(
							accountLoginViewModel: AccountLoginViewModel(),
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
					*/
					
					if isShowEditContactFragment {
						EditContactFragment(
							isShowEditContactFragment: $isShowEditContactFragment,
							isShowDismissPopup: $isShowDismissPopup,
							isShowEditContactFragmentAddress: isShowEditContactFragmentAddress
						)
						.zIndex(3)
						.transition(.opacity.combined(with: .move(edge: .bottom)))
						.onAppear {
							sharedMainViewModel.displayedFriend = nil
							isShowEditContactFragmentAddress = ""
						}
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
					
					/*
					if isShowStartConversationFragment {
						StartConversationFragment(
							startConversationViewModel: startConversationViewModel,
							conversationViewModel: conversationViewModel,
							isShowStartConversationFragment: $isShowStartConversationFragment
						)
						.zIndex(6)
						.transition(.opacity.combined(with: .move(edge: .bottom)))
					}
					*/
					
					if isShowDeleteContactPopup {
						PopupView(
							isShowPopup: $isShowDeleteContactPopup,
							title: Text(
								String(
									format: String(localized: "contact_dialog_delete_title"),
									contactsListViewModel!.selectedFriend?.name
									?? (SharedMainViewModel.shared.displayedFriend!.name ?? "Unknown Contact")
								)
							),
							content: Text("contact_dialog_delete_message"),
							titleFirstButton: Text("dialog_cancel"),
							actionFirstButton: {
								self.isShowDeleteContactPopup.toggle()},
							titleSecondButton: Text("dialog_ok"),
							actionSecondButton: {
								self.contactsListViewModel!.deleteSelectedContact()
								self.isShowDeleteContactPopup.toggle()
						})
						.background(.black.opacity(0.65))
						.zIndex(3)
						.onTapGesture {
							self.isShowDeleteContactPopup.toggle()
						}
						.onAppear {
							self.contactsListViewModel!.changeSelectedFriendToDelete()
						}
					}
					
					if isShowDeleteAllHistoryPopup {
						PopupView(isShowPopup: $isShowDeleteContactPopup,
								  title: Text("history_dialog_delete_all_call_logs_title"),
								  content: Text("history_dialog_delete_all_call_logs_message"),
								  titleFirstButton: Text("dialog_cancel"),
								  actionFirstButton: {
							self.isShowDeleteAllHistoryPopup.toggle()
							if historyListViewModel != nil {
								historyListViewModel!.callLogsAddressToDelete = ""
							}
						},
								  titleSecondButton: Text("dialog_ok"),
								  actionSecondButton: {
							if historyListViewModel != nil {
								historyListViewModel!.removeCallLogs()
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
					
					if isShowSipAddressesPopup && sharedMainViewModel.displayedFriend != nil {
						SipAddressesPopup(
							isShowSipAddressesPopup: $isShowSipAddressesPopup,
							isShowSipAddressesPopupType: $isShowSipAddressesPopupType
						)
						.environmentObject(contactsListViewModel!)
						.environmentObject(sharedMainViewModel.displayedFriend!)
						.background(.black.opacity(0.65))
						.zIndex(3)
						.onTapGesture {
							isShowSipAddressesPopup.toggle()
						}
					}
					
					/*
					if contactsListViewModel.operationInProgress {
						PopupLoadingView()
							.background(.black.opacity(0.65))
							.zIndex(3)
							.onDisappear {
								if contactsListViewModel.displayedConversation != nil {
									sharedMainViewModel.displayedFriend = nil
									sharedMainViewModel.displayedCall = nil
									sharedMainViewModel.changeIndexView(indexViewInt: 2)
									DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
										withAnimation {
											self.conversationViewModel.changeDisplayedChatRoom(conversationModel: contactsListViewModel.displayedConversation!)
										}
										contactsListViewModel.displayedConversation = nil
									}
								}
							}
					}
					
					if isShowScheduleMeetingFragment {
						ScheduleMeetingFragment(
							meetingViewModel: meetingViewModel,
							meetingsListViewModel: meetingsListViewModel,
							isShowScheduleMeetingFragment: $isShowScheduleMeetingFragment
						)
						.zIndex(3)
						.transition(.move(edge: .bottom))
						.onAppear {
						}
					}
					*/
					
					if isShowAccountProfileFragment {
						AccountProfileFragment(
							isShowAccountProfileFragment: $isShowAccountProfileFragment
						)
						.environmentObject(accountProfileViewModel)
						.zIndex(3)
						.transition(.move(edge: .trailing))
					}
					
					/*
					if isShowSettingsFragment {
						SettingsFragment(
							settingsViewModel: SettingsViewModel(),
							isShowSettingsFragment: $isShowSettingsFragment
						)
						.zIndex(3)
						.transition(.move(edge: .trailing))
					}
					
					if isShowHelpFragment {
						HelpFragment(
							helpViewModel: HelpViewModel(),
							isShowHelpFragment: $isShowHelpFragment
						)
						.zIndex(3)
						.transition(.move(edge: .trailing))
					}
					
					if isShowSendCancelMeetingNotificationPopup {
						PopupView(isShowPopup: $isShowSendCancelMeetingNotificationPopup,
								  title: Text("meeting_schedule_cancel_dialog_title"),
								  content: Text("meeting_schedule_cancel_dialog_message"),
								  titleFirstButton: Text("dialog_cancel"),
								  actionFirstButton: {
							sharedMainViewModel.displayedMeeting = nil
							meetingsListViewModel.deleteSelectedMeeting()
							self.isShowSendCancelMeetingNotificationPopup.toggle(
							) },
								  titleSecondButton: Text("dialog_ok"),
								  actionSecondButton: {
							sharedMainViewModel.displayedMeeting = nil
							if let meetingToDelete = self.meetingsListViewModel.selectedMeetingToDelete {
								self.meetingViewModel.cancelMeetingWithNotifications(meeting: meetingToDelete)
								meetingsListViewModel.deleteSelectedMeeting()
								self.isShowSendCancelMeetingNotificationPopup.toggle()
							}
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
					
					if conversationViewModel.isShowConversationInfoPopup {
						PopupViewWithTextField(conversationViewModel: conversationViewModel)
							.background(.black.opacity(0.65))
							.zIndex(3)
							.onTapGesture {
								conversationViewModel.isShowConversationInfoPopup = false
							}
					}
					
					if telecomManager.meetingWaitingRoomDisplayed {
						MeetingWaitingRoomFragment(meetingWaitingRoomViewModel: meetingWaitingRoomViewModel)
							.zIndex(3)
							.transition(.opacity.combined(with: .move(edge: .bottom)))
							.onAppear {
								meetingWaitingRoomViewModel.resetMeetingRoomView()
							}
					}
					*/
					/*
					if telecomManager.callDisplayed && ((telecomManager.callInProgress && telecomManager.outgoingCallStarted) || telecomManager.callConnected) && !telecomManager.meetingWaitingRoomDisplayed {
						CallView(
							callViewModel: callViewModel,
							conversationViewModel: conversationViewModel,
							conversationsListViewModel: conversationsListViewModel,
							conversationForwardMessageViewModel: conversationForwardMessageViewModel,
							contactsListViewModel: contactsListViewModel,
							editContactViewModel: editContactViewModel,
							meetingViewModel: meetingViewModel,
							accountProfileViewModel: accountProfileViewModel,
							fullscreenVideo: $fullscreenVideo,
							isShowStartCallFragment: $isShowStartCallFragment,
							isShowConversationFragment: $isShowConversationFragment,
							isShowStartCallGroupPopup: $isShowStartCallGroupPopup,
							isShowEditContactFragment: $isShowEditContactFragment,
							indexPage: $index,
							isShowScheduleMeetingFragment: $isShowScheduleMeetingFragment
						)
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
					*/
					ToastView()
						.zIndex(6)
				}
			}
			.onChange(of: navigationManager.selectedCallId) { newCallId in
				if newCallId != nil {
					sharedMainViewModel.changeIndexView(indexViewInt: 2)
				}
			}
			.onReceive(contactLoaded) { _ in
				//conversationsListViewModel.updateChatRoomsList()
				if historyListViewModel != nil {
					historyListViewModel!.refreshHistoryAvatarModel()
				}
			}
			.onReceive(contactAdded) { address in
				//conversationsListViewModel.updateChatRoom(address: address)
			}
			.onReceive(coreStarted) { _ in
				accountProfileViewModel.setAvatarModel()
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
				//conversationsListViewModel.computeChatRoomsList()
			}
		}
	}
	
	func openMenu() {
		withAnimation {
			self.sideMenuIsOpen.toggle()
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
	ContentView(
		//meetingWaitingRoomViewModel: MeetingWaitingRoomViewModel(),
		//conversationsListViewModel: ConversationsListViewModel(),
		//conversationViewModel: ConversationViewModel(),
		//meetingsListViewModel: MeetingsListViewModel(),
		//meetingViewModel: MeetingViewModel(),
		//conversationForwardMessageViewModel: ConversationForwardMessageViewModel(),
		//accountProfileViewModel: AccountProfileViewModel(),
		//index: .constant(0)
	)
}
// swiftlint:enable type_body_length
// swiftlint:enable line_length
