/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
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
import Foundation
import linphonesw

struct AddParticipantsFragment: View {
	
	@Environment(\.dismiss) var dismiss
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject var magicSearch = MagicSearchSingleton.shared
	@ObservedObject var addParticipantsViewModel: AddParticipantsViewModel
	var confirmAddParticipantsFunc: ([SelectedAddressModel]) -> Void
	
	@FocusState var isSearchFieldFocused: Bool
	
	var body: some View {
		ZStack(alignment: .bottomTrailing) {
			VStack(spacing: 16) {
				if #available(iOS 16.0, *) {
					Rectangle()
						.foregroundColor(Color.orangeMain500)
						.edgesIgnoringSafeArea(.top)
						.frame(height: 0)
				} else if idiom != .pad && !(orientation == .landscapeLeft || orientation == .landscapeRight
											 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
					Rectangle()
						.foregroundColor(Color.orangeMain500)
						.edgesIgnoringSafeArea(.top)
						.frame(height: 1)
				}
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
							addParticipantsViewModel.reset()
							dismiss()
						}
					
					VStack(alignment: .leading, spacing: 3) {
						Text("conversation_add_participants_title")
							.multilineTextAlignment(.leading)
							.default_text_style_orange_800(styleSize: 16)
							.padding(.top, 20)
						Text("\($addParticipantsViewModel.participantsToAdd.count) selected participants")
							.default_text_style_300(styleSize: 12)
					}
					Spacer()
				}
				.frame(maxWidth: .infinity)
				.frame(height: 50)
				.padding(.horizontal)
				.padding(.bottom, 4)
				.background(.white)
				
				ScrollView(.horizontal) {
					HStack {
						ForEach(0..<addParticipantsViewModel.participantsToAdd.count, id: \.self) { index in
							ZStack(alignment: .topTrailing) {
								VStack {
									Avatar(contactAvatarModel: addParticipantsViewModel.participantsToAdd[index].avatarModel, avatarSize: 50)
									
									Text(addParticipantsViewModel.participantsToAdd[index].avatarModel.name)
										.default_text_style(styleSize: 12)
										.frame(minWidth: 60, maxWidth: 80)
								}
								Image("x-circle")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 25, height: 25)
									.onTapGesture {
										addParticipantsViewModel.participantsToAdd.remove(at: index)
									}
							}
						}
					}
				}
				.padding(.leading, 16)
				
				ZStack(alignment: .trailing) {
					TextField("new_conversation_search_bar_filter_hint", text: $addParticipantsViewModel.searchField)
						.default_text_style(styleSize: 15)
						.frame(height: 25)
						.focused($isSearchFieldFocused)
						.padding(.horizontal, 30)
						.onChange(of: addParticipantsViewModel.searchField) { newValue in
							magicSearch.currentFilterSuggestions = newValue
							magicSearch.searchForSuggestions()
						}.onAppear {
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
						
						if !addParticipantsViewModel.searchField.isEmpty {
							Button(action: {
								addParticipantsViewModel.searchField = ""
								magicSearch.currentFilterSuggestions = ""
								magicSearch.searchForSuggestions()
								isSearchFieldFocused = false
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
					ForEach(0..<contactsManager.lastSearch.count, id: \.self) { index in
						HStack {
							HStack {
								if index == 0
									|| contactsManager.lastSearch[index].friend?.name!.lowercased().folding(
										options: .diacriticInsensitive,
										locale: .current
									).first
									!= contactsManager.lastSearch[index-1].friend?.name!.lowercased().folding(
										options: .diacriticInsensitive,
										locale: .current
									).first {
									Text(
										String(
											(contactsManager.lastSearch[index].friend?.name!.uppercased().folding(
												options: .diacriticInsensitive,
												locale: .current
											).first)!))
									.contact_text_style_500(styleSize: 20)
									.frame(width: 18)
									.padding(.leading, 5)
									.padding(.trailing, 5)
								} else {
									Text("")
										.contact_text_style_500(styleSize: 20)
										.frame(width: 18)
										.padding(.leading, 5)
										.padding(.trailing, 5)
								}
								
								if index < contactsManager.avatarListModel.count
									&& contactsManager.avatarListModel[index].friend!.photo != nil
									&& !contactsManager.avatarListModel[index].friend!.photo!.isEmpty {
									Avatar(contactAvatarModel: contactsManager.avatarListModel[index], avatarSize: 50)
								} else {
									Image("profil-picture-default")
										.resizable()
										.frame(width: 50, height: 50)
										.clipShape(Circle())
								}
								Text((contactsManager.lastSearch[index].friend?.name)!)
									.default_text_style(styleSize: 16)
									.frame(maxWidth: .infinity, alignment: .leading)
									.foregroundStyle(Color.orangeMain500)
								Spacer()
								
							}
						}
						.background(.white)
						.onTapGesture {
							if let addr = contactsManager.lastSearch[index].address {
								addParticipantsViewModel.selectParticipant(addr: addr)
							}
						}
						.buttonStyle(.borderless)
						.listRowSeparator(.hidden)
					}
					
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
			Button {
				withAnimation {
					confirmAddParticipantsFunc(addParticipantsViewModel.participantsToAdd)
					dismiss()
				}
			} label: {
				Image("check")
					.renderingMode(.template)
					.foregroundStyle(.white)
					.padding()
					.background(Color.orangeMain500)
					.clipShape(Circle())
					.shadow(color: .black.opacity(0.2), radius: 4)
				
			}
			.padding()
		}
		.navigationTitle("")
		.navigationBarHidden(true)
	}
	
	var suggestionsList: some View {
		ForEach(0..<contactsManager.lastSearchSuggestions.count, id: \.self) { index in
			Button {
				if let addr = contactsManager.lastSearchSuggestions[index].address {
					addParticipantsViewModel.selectParticipant(addr: addr)
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
}

#Preview {
	AddParticipantsFragment(addParticipantsViewModel: AddParticipantsViewModel(), confirmAddParticipantsFunc: { _ in })
}
