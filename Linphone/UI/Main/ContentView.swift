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

struct ContentView: View {
	
	var contactManager = ContactsManager.shared
	var magicSearch = MagicSearchSingleton.shared
	
	@ObservedObject var contactViewModel: ContactViewModel
	@ObservedObject var historyViewModel: HistoryViewModel
	@ObservedObject private var coreContext = CoreContext.shared
	
	@State var index = 0
	@State private var orientation = UIDevice.current.orientation
	@State var sideMenuIsOpen: Bool = false
	
	@State private var searchIsActive = false
	@State private var text = ""
	@FocusState private var focusedField: Bool
	@State var isMenuOpen: Bool = false
	@State var isShowDeletePopup = false
	
	var body: some View {
		GeometryReader { geometry in
			ZStack {
				VStack(spacing: 0) {
					HStack(spacing: 0) {
						if orientation == .landscapeLeft
							|| orientation == .landscapeRight
							|| UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height {
							VStack {
								Group {
									Spacer()
									Button(action: {
										self.index = 0
									}, label: {
										VStack {
											Image("address-book")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(self.index == 0 ? Color.orangeMain500 : Color.grayMain2c600)
												.frame(width: 25, height: 25)
											if self.index == 0 {
												Text("Contacts")
													.default_text_style_700(styleSize: 10)
											} else {
												Text("Contacts")
													.default_text_style(styleSize: 10)
											}
										}
									})
									
									Spacer()
									
									Button(action: {
										self.index = 1
										contactViewModel.displayedFriend = nil
									}, label: {
										VStack {
											Image("phone")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(self.index == 1 ? Color.orangeMain500 : Color.grayMain2c600)
												.frame(width: 25, height: 25)
											if self.index == 1 {
												Text("Calls")
													.default_text_style_700(styleSize: 10)
											} else {
												Text("Calls")
													.default_text_style(styleSize: 10)
											}
										}
									})
									
									Spacer()
								}
							}
							.frame(width: 75)
							.padding(.leading,
									 orientation == .landscapeRight && geometry.safeAreaInsets.bottom > 0
									 ? -geometry.safeAreaInsets.leading
									 : 0)
						}
						
						VStack(spacing: 0) {
							if searchIsActive == false {
								HStack {
									Image("profile-image-example")
										.resizable()
										.frame(width: 45, height: 45)
										.clipShape(Circle())
										.onTapGesture {
											openMenu()
										}
									
									Text(index == 0 ? "Contacts" : "Calls")
										.default_text_style_white_800(styleSize: 20)
										.padding(.leading, 10)
									
									Spacer()
									
									Button {
										withAnimation {
											searchIsActive.toggle()
										}
									} label: {
										Image("search")
									}
									
									Menu {
										Button {
											isMenuOpen = false
											magicSearch.allContact = true
											magicSearch.searchForContacts(
												sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
										} label: {
											HStack {
												Text("See all")
												Spacer()
												if magicSearch.allContact {
													Image("green-check")
												}
											}
										}
										
										Button {
											isMenuOpen = false
											magicSearch.allContact = false
											magicSearch.searchForContacts(
												sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
										} label: {
											HStack {
												Text("See Linphone contact")
												Spacer()
												if !magicSearch.allContact {
													Image("green-check")
												}
											}
										}
									} label: {
										Image(index == 0 ? "filtres" : "more")
									}
									.padding(.leading)
									.onTapGesture {
										isMenuOpen = true
									}
								}
								.frame(maxWidth: .infinity)
								.frame(height: 50)
								.padding(.horizontal)
								.padding(.bottom, 5)
								.background(Color.orangeMain500)
							} else {
								HStack {
									Button {
										withAnimation {
											self.focusedField = false
											searchIsActive.toggle()
										}
										
										text = ""
										magicSearch.currentFilter = ""
										magicSearch.searchForContacts(
											sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
									} label: {
										Image("caret-left")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(.white)
											.frame(width: 25, height: 25, alignment: .leading)
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
										.accentColor(.white)
										.scrollContentBackground(.hidden)
										.focused($focusedField)
										.onAppear {
											self.focusedField = true
										}
										.onChange(of: text) { newValue in
											magicSearch.currentFilter = newValue
											magicSearch.searchForContacts(
												sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
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
										.default_text_style_white_700(styleSize: 15)
										.padding(.all, 6)
										.accentColor(.white)
										.focused($focusedField)
										.onAppear {
											self.focusedField = true
										}
										.onChange(of: text) { newValue in
											magicSearch.currentFilter = newValue
											magicSearch.searchForContacts(
												sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
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
									}
									.padding(.leading)
								}
								.frame(maxWidth: .infinity)
								.frame(height: 50)
								.padding(.horizontal)
								.padding(.bottom, 5)
								.background(Color.orangeMain500)
							}
							
							if self.index == 0 {
								ContactsView(contactViewModel: contactViewModel, historyViewModel: historyViewModel, isShowDeletePopup: $isShowDeletePopup)
							} else if self.index == 1 {
								HistoryView()
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
									self.index = 0
								}, label: {
									VStack {
										Image("address-book")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(self.index == 0 ? Color.orangeMain500 : Color.grayMain2c600)
											.frame(width: 25, height: 25)
										if self.index == 0 {
											Text("Contacts")
												.default_text_style_700(styleSize: 10)
										} else {
											Text("Contacts")
												.default_text_style(styleSize: 10)
										}
									}
								})
								.padding(.top)
								
								Spacer()
								
								Button(action: {
									self.index = 1
									contactViewModel.displayedFriend = nil
								}, label: {
									VStack {
										Image("phone")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(self.index == 1 ? Color.orangeMain500 : Color.grayMain2c600)
											.frame(width: 25, height: 25)
										if self.index == 1 {
											Text("Calls")
												.default_text_style_700(styleSize: 10)
										} else {
											Text("Calls")
												.default_text_style(styleSize: 10)
										}
									}
								})
								.padding(.top)
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
				
				if contactViewModel.displayedFriend != nil || !historyViewModel.historyTitle.isEmpty {
					HStack(spacing: 0) {
						Spacer()
							.frame(maxWidth:
									(orientation == .landscapeLeft
									 || orientation == .landscapeRight
									 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height)
								   ? (geometry.size.width/100*40) + 75
								   : 0
							)
						if self.index == 0 {
							ContactFragment(contactViewModel: contactViewModel, isShowDeletePopup: $isShowDeletePopup)
								.frame(maxWidth: .infinity)
								.background(Color.gray100)
								.ignoresSafeArea(.keyboard)
						} else if self.index == 1 {
							HistoryContactFragment()
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
					isOpen: self.sideMenuIsOpen,
					menuClose: self.openMenu,
					safeAreaInsets: geometry.safeAreaInsets
				)
				.ignoresSafeArea(.all)
				.zIndex(2)
				
				if isShowDeletePopup {
					PopupView(sharedMainViewModel: SharedMainViewModel(), isShowPopup: $isShowDeletePopup,
							  title: Text(
									contactViewModel.selectedFriend != nil 
									? "Delete \(contactViewModel.selectedFriend!.name!)?"
									: (contactViewModel.displayedFriend != nil 
									   ? "Delete \(contactViewModel.displayedFriend!.name!)?"
									   : "Error Name")),
							  content: Text("This contact will be deleted definitively."),
							  titleFirstButton: Text("Cancel"),
							  actionFirstButton: {self.isShowDeletePopup.toggle()},
							  titleSecondButton: Text("Ok"),
							  actionSecondButton: {
						if contactViewModel.selectedFriend != nil {
							contactViewModel.selectedFriend!.remove()
							if contactViewModel.displayedFriend != nil && contactViewModel.selectedFriend!.name == contactViewModel.displayedFriend!.name {
								withAnimation {
									contactViewModel.displayedFriend = nil
								}
							}
						} else if contactViewModel.displayedFriend != nil {
							contactViewModel.displayedFriend!.remove()
							withAnimation {
								contactViewModel.displayedFriend = nil
							}
						}
						magicSearch.searchForContacts(
							sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
						self.isShowDeletePopup.toggle()
					})
					.background(.black.opacity(0.65))
					.zIndex(3)
					.onTapGesture {
						self.isShowDeletePopup.toggle()
					}
				}
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
			if (contactViewModel.displayedFriend != nil || !historyViewModel.historyTitle.isEmpty) && searchIsActive {
				self.focusedField = false
			} else if searchIsActive {
				self.focusedField = true
			}
			orientation = newOrientation
		}
	}
	
	func openMenu() {
		withAnimation {
			self.sideMenuIsOpen.toggle()
		}
	}
}

#Preview {
	ContentView(contactViewModel: ContactViewModel(), historyViewModel: HistoryViewModel())
}
