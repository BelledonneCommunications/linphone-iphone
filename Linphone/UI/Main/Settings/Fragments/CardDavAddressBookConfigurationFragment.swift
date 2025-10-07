/*
 * Copyright (c) 2010-2025 Belledonne Communications SARL.
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

struct CardDavAddressBookConfigurationFragment: View {
	@StateObject private var cardDavViewModel: CardDavViewModel
	
	@EnvironmentObject var settingsViewModel: SettingsViewModel
	
	@Environment(\.dismiss) var dismiss
	
	@State var isSecured: Bool = true
	
	@FocusState var isDisplayNameFocused: Bool
	@FocusState var isServerUrlFocused: Bool
	@FocusState var isUsernameFocused: Bool
	@FocusState var isPasswordFocused: Bool
	@FocusState var isRealmFocused: Bool
	
	init(name: String? = "") {
		_cardDavViewModel = StateObject(wrappedValue: CardDavViewModel(name: name))
	}
	
	var body: some View {
		ZStack {
			VStack(spacing: 1) {
				Rectangle()
					.foregroundColor(Color.orangeMain500)
					.edgesIgnoringSafeArea(.top)
					.frame(height: 0)
				
				HStack {
					Image("caret-left")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.orangeMain500)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
						.padding(.top, 4)
						.padding(.leading, -10)
						.onTapGesture {
							dismiss()
						}
					
					Text(cardDavViewModel.isEdit ? "settings_contacts_edit_carddav_server_title" : "settings_contacts_add_carddav_server_title")
						.default_text_style_orange_800(styleSize: 16)
						.frame(maxWidth: .infinity, alignment: .leading)
						.padding(.top, 4)
						.lineLimit(1)
					
					Spacer()
					
					if cardDavViewModel.isEdit {
						Button {
							cardDavViewModel.delete()
						} label: {
							Image("trash-simple")
								.renderingMode(.template)
								.resizable()
								.frame(width: 28, height: 28)
								.foregroundStyle(.red)
								.padding(.horizontal, 5)
								.padding(.top, 4)
						}
					}
				}
				.frame(maxWidth: .infinity)
				.frame(height: 50)
				.padding(.horizontal)
				.padding(.bottom, 4)
				.background(.white)
				
				ScrollView {
					VStack(spacing: 0) {
						VStack(spacing: 30) {
							VStack(alignment: .leading) {
								Text("settings_contacts_carddav_name_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_carddav_name_title", text: $cardDavViewModel.displayName)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isDisplayNameFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isDisplayNameFocused)
							}
							
							VStack(alignment: .leading) {
								Text("settings_contacts_carddav_server_url_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_carddav_server_url_title", text: $cardDavViewModel.serverUrl)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isServerUrlFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isServerUrlFocused)
							}
							
							VStack(alignment: .leading) {
								Text("settings_contacts_carddav_username_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_carddav_username_title", text: $cardDavViewModel.username)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isUsernameFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isUsernameFocused)
							}
							
							VStack(alignment: .leading) {
								Text("settings_contacts_carddav_password_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								ZStack(alignment: .trailing) {
									Group {
										if isSecured {
											SecureField("settings_contacts_carddav_password_title", text: $cardDavViewModel.password)
												.default_text_style(styleSize: 15)
												.frame(height: 25)
												.focused($isPasswordFocused)
										} else {
											TextField("settings_contacts_carddav_password_title", text: $cardDavViewModel.password)
												.default_text_style(styleSize: 15)
												.disableAutocorrection(true)
												.autocapitalization(.none)
												.frame(height: 25)
												.focused($isPasswordFocused)
										}
									}
									
									Button(action: {
										isSecured.toggle()
									}, label: {
										Image(self.isSecured ? "eye-slash" : "eye")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c500)
											.frame(width: 20, height: 20)
									})
								}
								.padding(.horizontal, 20)
								.padding(.vertical, 15)
								.cornerRadius(60)
								.overlay(
									RoundedRectangle(cornerRadius: 60)
										.inset(by: 0.5)
										.stroke(isPasswordFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
								)
								.padding(.bottom)
							}
							
							VStack(alignment: .leading) {
								Text("settings_contacts_carddav_realm_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_carddav_realm_title", text: $cardDavViewModel.realm)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isRealmFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isRealmFocused)
							}
							
							Toggle("settings_contacts_carddav_use_as_default_title", isOn: $cardDavViewModel.storeNewContactsInIt)
								.default_text_style_700(styleSize: 15)
							
						}
						.padding(.vertical, 30)
						.padding(.horizontal, 20)
						.background(Color.gray100)
					}
				}
				.background(Color.gray100)
			}
			.background(Color.gray100)
			
			VStack {
				Spacer()
				HStack {
					Spacer()
					
					Button {
						cardDavViewModel.addAddressBook()
					} label: {
						Image(cardDavViewModel.isEdit ? "check" : "plus-circle")
							.renderingMode(.template)
							.resizable()
							.frame(width: 28, height: 28)
							.foregroundStyle(.white)
							.padding()
							.background(cardDavViewModel.isFormComplete ? Color.orangeMain500 : Color.gray300)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
						
					}
					.padding()
					.disabled(!cardDavViewModel.isFormComplete)
				}
			}
			
			if cardDavViewModel.cardDavServerOperationInProgress {
				PopupLoadingView()
					.background(.black.opacity(0.65))
			}
		}
		.onChange(of: cardDavViewModel.cardDavServerOperationSuccessful) { event in
			if event {
				dismiss()
				settingsViewModel.reloadConfiguredCardDavServers()
			}
		}
		.navigationTitle("")
		.navigationBarHidden(true)
	}
}
