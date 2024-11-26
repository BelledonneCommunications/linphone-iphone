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
import linphonesw

// swiftlint:disable type_body_length
struct StartCallFragment: View {
	
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject var magicSearch = MagicSearchSingleton.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@ObservedObject var callViewModel: CallViewModel
	@ObservedObject var startCallViewModel: StartCallViewModel
	
	@Binding var isShowStartCallFragment: Bool
	@Binding var showingDialer: Bool
	
	@FocusState var isSearchFieldFocused: Bool
	@State private var delayedColor = Color.white
	
	@FocusState var isMessageTextFocused: Bool
	
	var resetCallView: () -> Void
	
	var body: some View {
		NavigationView {
			ZStack {
				VStack(spacing: 1) {
					
					Rectangle()
						.foregroundColor(delayedColor)
						.edgesIgnoringSafeArea(.top)
						.frame(height: 0)
						.task(delayColor)
					
					HStack {
						Image("caret-left")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.orangeMain500)
							.frame(width: 25, height: 25, alignment: .leading)
							.padding(.all, 10)
							.padding(.top, 2)
							.padding(.leading, -10)
							.onTapGesture {
								DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
									magicSearch.searchForContacts(
										sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
									
									if callViewModel.isTransferInsteadCall == true {
										callViewModel.isTransferInsteadCall = false
									}
									
									resetCallView()
								}
								
								startCallViewModel.searchField = ""
								magicSearch.currentFilterSuggestions = ""
								delayColorDismiss()
								withAnimation {
									isShowStartCallFragment.toggle()
								}
							}
						
						Text(!callViewModel.isTransferInsteadCall ? "history_call_start_title" : "Transfer call to")
							.multilineTextAlignment(.leading)
							.default_text_style_orange_800(styleSize: 16)
						
						Spacer()
						
					}
					.frame(maxWidth: .infinity)
					.frame(height: 50)
					.padding(.horizontal)
					.padding(.bottom, 4)
					.background(.white)
					
					VStack(spacing: 0) {
						ZStack(alignment: .trailing) {
							TextField("history_call_start_search_bar_filter_hint", text: $startCallViewModel.searchField)
								.default_text_style(styleSize: 15)
								.frame(height: 25)
								.focused($isSearchFieldFocused)
								.padding(.horizontal, 30)
								.onChange(of: startCallViewModel.searchField) { newValue in
									magicSearch.currentFilterSuggestions = newValue
									magicSearch.searchForSuggestions()
								}
								.simultaneousGesture(TapGesture().onEnded {
									showingDialer = false
								})
							
							HStack {
								Button(action: {
								}, label: {
									Image("magnifying-glass")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.grayMain2c500)
										.frame(width: 25, height: 25)
								})
								
								Spacer()
								
								if startCallViewModel.searchField.isEmpty {
									Button(action: {
										if !showingDialer {
											isSearchFieldFocused = false
											
											DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
												showingDialer = true
											}
										} else {
											showingDialer = false
											
											DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
												isSearchFieldFocused = true
											}
										}
									}, label: {
										Image(!showingDialer ? "dialer" : "keyboard")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c500)
											.frame(width: 25, height: 25)
									})
								} else {
									Button(action: {
										startCallViewModel.searchField = ""
										magicSearch.currentFilterSuggestions = ""
										magicSearch.searchForSuggestions()
									}, label: {
										Image("x")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c500)
											.frame(width: 25, height: 25)
									})
								}
							}
						}
						.padding(.horizontal, 15)
						.padding(.vertical, 10)
						.cornerRadius(60)
						.overlay(
							RoundedRectangle(cornerRadius: 60)
								.inset(by: 0.5)
								.stroke(isSearchFieldFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
						)
						.padding(.vertical)
						.padding(.horizontal)
						
						NavigationLink(destination: {
							StartGroupCallFragment(startCallViewModel: startCallViewModel)
						}, label: {
							HStack {
								HStack(alignment: .center) {
									Image("meetings")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(.white)
										.frame(width: 20, height: 20, alignment: .leading)
								}
								.padding(16)
								.background(Color.orangeMain500)
								.cornerRadius(40)
								
								Text("history_call_start_create_group_call")
									.foregroundStyle(.black)
									.default_text_style_800(styleSize: 16)
								
								Spacer()
								
								Image("caret-right")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 25, height: 25, alignment: .leading)
							}
						})
						.padding(.vertical, 10)
						.padding(.horizontal, 20)
						.background(
							LinearGradient(gradient: Gradient(colors: [.grayMain2c100, .white]), startPoint: .leading, endPoint: .trailing)
								.padding(.vertical, 10)
								.padding(.horizontal, 40)
						)
						
						ScrollView {
							if !ContactsManager.shared.lastSearch.isEmpty {
								HStack(alignment: .center) {
									Text("contacts_list_all_contacts_title")
										.default_text_style_800(styleSize: 16)
									
									Spacer()
								}
								.padding(.vertical, 10)
								.padding(.horizontal, 16)
							}
							
							ContactsListFragment(contactViewModel: ContactViewModel(), contactsListViewModel: ContactsListViewModel(), showingSheet: .constant(false)
												 , startCallFunc: { addr in
								if callViewModel.isTransferInsteadCall {
									showingDialer = false
									
									DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
										magicSearch.searchForContacts(
											sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
										
										if callViewModel.isTransferInsteadCall == true {
											callViewModel.isTransferInsteadCall = false
										}
										
										resetCallView()
									}
									
									startCallViewModel.searchField = ""
									magicSearch.currentFilterSuggestions = ""
									delayColorDismiss()
									
									withAnimation {
										isShowStartCallFragment.toggle()
										callViewModel.blindTransferCallTo(toAddress: addr)
									}
								} else {
									showingDialer = false
									
									DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
										magicSearch.searchForContacts(
											sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
										
										if callViewModel.isTransferInsteadCall == true {
											callViewModel.isTransferInsteadCall = false
										}
										
										resetCallView()
									}
									
									startCallViewModel.searchField = ""
									magicSearch.currentFilterSuggestions = ""
									delayColorDismiss()
									
									withAnimation {
										isShowStartCallFragment.toggle()
										telecomManager.doCallOrJoinConf(address: addr)
									}
								}
							})
							.padding(.horizontal, 16)
							
							if !contactsManager.lastSearchSuggestions.isEmpty {
								HStack(alignment: .center) {
									Text("generic_address_picker_suggestions_list_title")
										.default_text_style_800(styleSize: 16)
									
									Spacer()
								}
								.padding(.vertical, 10)
								.padding(.horizontal, 16)
								
								suggestionsList
							}
						}
					}
					.frame(maxWidth: .infinity)
				}
				.background(.white)
				
				if !startCallViewModel.participants.isEmpty {
					startCallPopup
						.background(.black.opacity(0.65))
						.onAppear {
							DispatchQueue.main.asyncAfter(deadline: .now() + 0.6) {
								isMessageTextFocused = true
							}
						}
				}
				
				if startCallViewModel.operationInProgress {
					PopupLoadingView()
						.background(.black.opacity(0.65))
						.onDisappear {
							isShowStartCallFragment.toggle()
						}
				}
			}
			.navigationBarHidden(true)
		}
	}
	
	@Sendable private func delayColor() async {
		try? await Task.sleep(nanoseconds: 250_000_000)
		delayedColor = Color.orangeMain500
	}
	
	func delayColorDismiss() {
		Task {
			try? await Task.sleep(nanoseconds: 80_000_000)
			delayedColor = .white
		}
	}
	
	var suggestionsList: some View {
		ForEach(0..<contactsManager.lastSearchSuggestions.count, id: \.self) { index in
			Button {
				if callViewModel.isTransferInsteadCall {
					showingDialer = false
					
					DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
						magicSearch.searchForContacts(
							sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
						
						if callViewModel.isTransferInsteadCall == true {
							callViewModel.isTransferInsteadCall = false
						}
						
						resetCallView()
					}
					
					startCallViewModel.searchField = ""
					magicSearch.currentFilterSuggestions = ""
					delayColorDismiss()
					
					withAnimation {
						isShowStartCallFragment.toggle()
						if contactsManager.lastSearchSuggestions[index].address != nil {
							callViewModel.blindTransferCallTo(toAddress: contactsManager.lastSearchSuggestions[index].address!)
						}
					}
				} else {
					showingDialer = false
					
					DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
						magicSearch.searchForContacts(
							sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
						
						if callViewModel.isTransferInsteadCall == true {
							callViewModel.isTransferInsteadCall = false
						}
						
						resetCallView()
					}
					
					startCallViewModel.searchField = ""
					magicSearch.currentFilterSuggestions = ""
					delayColorDismiss()
					
					withAnimation {
						isShowStartCallFragment.toggle()
						if contactsManager.lastSearchSuggestions[index].address != nil {
							telecomManager.doCallOrJoinConf(address: contactsManager.lastSearchSuggestions[index].address!)
						}
					}
				}
			} label: {
				HStack {
					if index < contactsManager.lastSearchSuggestions.count
						&& contactsManager.lastSearchSuggestions[index].address != nil
						&& contactsManager.lastSearchSuggestions[index].address!.username != nil {
						
						Image(uiImage: contactsManager.textToImage(
							firstName: contactsManager.lastSearchSuggestions[index].address!.username!,
							lastName: ""))
						.resizable()
						.frame(width: 45, height: 45)
						.clipShape(Circle())
						
						Text(contactsManager.lastSearchSuggestions[index].address?.username ?? "")
							.default_text_style(styleSize: 16)
							.frame(maxWidth: .infinity, alignment: .leading)
							.foregroundStyle(Color.orangeMain500)
					} else {
						Image("profil-picture-default")
							.resizable()
							.frame(width: 45, height: 45)
							.clipShape(Circle())
						
						Text("Username error")
							.default_text_style(styleSize: 16)
							.frame(maxWidth: .infinity, alignment: .leading)
							.foregroundStyle(Color.orangeMain500)
					}
				}
				.padding(.horizontal)
			}
			.buttonStyle(.borderless)
			.listRowSeparator(.hidden)
		}
	}
	
	var startCallPopup: some View {
		GeometryReader { geometry in
			VStack(alignment: .leading) {
				Text("history_group_call_start_dialog_set_subject")
					.default_text_style_800(styleSize: 16)
					.frame(alignment: .leading)
					.padding(.bottom, 2)
				
				TextField("history_group_call_start_dialog_subject_hint", text: $startCallViewModel.messageText)
					.default_text_style(styleSize: 15)
					.frame(height: 25)
					.padding(.horizontal, 20)
					.padding(.vertical, 15)
					.cornerRadius(60)
					.overlay(
						RoundedRectangle(cornerRadius: 60)
							.inset(by: 0.5)
							.stroke(isMessageTextFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
					)
					.padding(.bottom)
					.focused($isMessageTextFocused)
				
				Button(action: {
					startCallViewModel.participants.removeAll()
				}, label: {
					Text("dialog_cancel")
						.default_text_style_orange_600(styleSize: 20)
						.frame(height: 35)
						.frame(maxWidth: .infinity)
				})
				.padding(.horizontal, 20)
				.padding(.vertical, 10)
				.cornerRadius(60)
				.overlay(
					RoundedRectangle(cornerRadius: 60)
						.inset(by: 0.5)
						.stroke(Color.orangeMain500, lineWidth: 1)
				)
				.padding(.bottom, 10)
				
				Button(action: {
					startCallViewModel.createGroupCall()
				}, label: {
					Text("dialog_call")
						.default_text_style_white_600(styleSize: 20)
						.frame(height: 35)
						.frame(maxWidth: .infinity)
				})
				.padding(.horizontal, 20)
				.padding(.vertical, 10)
				.background(startCallViewModel.messageText.isEmpty ? Color.orangeMain100 : Color.orangeMain500)
				.cornerRadius(60)
				.disabled(startCallViewModel.messageText.isEmpty)
			}
			.padding(.horizontal, 20)
			.padding(.vertical, 20)
			.background(.white)
			.cornerRadius(20)
			.padding(.horizontal)
			.frame(maxHeight: .infinity)
			.shadow(color: Color.orangeMain500, radius: 0, x: 0, y: 2)
			.frame(maxWidth: sharedMainViewModel.maxWidth)
			.position(x: geometry.size.width / 2, y: geometry.size.height / 2)
		}
	}
}

#Preview {
	StartCallFragment(
		callViewModel: CallViewModel(),
		startCallViewModel: StartCallViewModel(),
		isShowStartCallFragment: .constant(true),
		showingDialer: .constant(false),
		resetCallView: {}
	)
}

// swiftlint:enable type_body_length
