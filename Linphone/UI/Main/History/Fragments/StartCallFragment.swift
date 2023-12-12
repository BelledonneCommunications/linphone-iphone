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

struct StartCallFragment: View {
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject var magicSearch = MagicSearchSingleton.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@ObservedObject var startCallViewModel: StartCallViewModel
	
	@Binding var isShowStartCallFragment: Bool
	@Binding var showingDialer: Bool
	
	@FocusState var isSearchFieldFocused: Bool
	@State private var hasTimeElapsed = false
	@State private var delayedColor = Color.white
	
	var body: some View {
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
						.padding(.top, 2)
						.onTapGesture {
							DispatchQueue.global().asyncAfter(deadline: .now() + 0.2) {
								magicSearch.searchForContacts(
									sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
							}
							
							startCallViewModel.searchField = ""
							magicSearch.currentFilterSuggestions = ""
							delayColorDismiss()
							withAnimation {
								isShowStartCallFragment.toggle()
							}
						}
					
					Text("New call")
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
						TextField("Search contact or history call", text: $startCallViewModel.searchField)
							.default_text_style(styleSize: 15)
							.frame(height: 25)
							.focused($isSearchFieldFocused)
							.padding(.horizontal, 30)
							.onChange(of: startCallViewModel.searchField) { newValue in
								magicSearch.currentFilterSuggestions = newValue
								magicSearch.searchForSuggestions()
							}
						
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
									isSearchFieldFocused = false
									
									DispatchQueue.global().asyncAfter(deadline: .now() + 0.2) {
										showingDialer.toggle()
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
					
					ScrollView {
						if !ContactsManager.shared.lastSearch.isEmpty {
							HStack(alignment: .center) {
								Text("All contacts")
									.default_text_style_800(styleSize: 16)
								
								Spacer()
							}
							.padding(.vertical, 10)
							.padding(.horizontal, 16)
						}
						
                        ContactsListFragment(contactViewModel: ContactViewModel(), contactsListViewModel: ContactsListViewModel(), showingSheet: .constant(false), startCallFunc: { addr in
							DispatchQueue.global().asyncAfter(deadline: .now() + 0.2) {
								magicSearch.searchForContacts(
									sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
							}
							
							startCallViewModel.searchField = ""
							magicSearch.currentFilterSuggestions = ""
							delayColorDismiss()
							
							withAnimation {
								isShowStartCallFragment.toggle()
                                telecomManager.doCallWithCore(addr: addr)
							}
						})
							.padding(.horizontal, 16)
						
						HStack(alignment: .center) {
							Text("Suggestions")
								.default_text_style_800(styleSize: 16)
							
							Spacer()
						}
						.padding(.vertical, 10)
						.padding(.horizontal, 16)
						
						suggestionsList
					}
				}
				.frame(maxWidth: .infinity)
			}
			.background(.white)
		}
		.navigationBarHidden(true)
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
				.onTapGesture {
					DispatchQueue.global().asyncAfter(deadline: .now() + 0.2) {
						magicSearch.searchForContacts(
							sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
					}
					
					startCallViewModel.searchField = ""
					magicSearch.currentFilterSuggestions = ""
					delayColorDismiss()
					
					withAnimation {
						isShowStartCallFragment.toggle()
                        if contactsManager.lastSearchSuggestions[index].address != nil {
                            telecomManager.doCallWithCore(
                                addr: contactsManager.lastSearchSuggestions[index].address!
                            )
                        }
					}
				}
				.padding(.horizontal)
			}
			.buttonStyle(.borderless)
			.listRowSeparator(.hidden)
		}
	}
}

#Preview {
	StartCallFragment(startCallViewModel: StartCallViewModel(), isShowStartCallFragment: .constant(true), showingDialer: .constant(false))
}
