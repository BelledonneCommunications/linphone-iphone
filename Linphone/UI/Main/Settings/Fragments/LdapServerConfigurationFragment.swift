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

struct LdapServerConfigurationFragment: View {
	@StateObject private var ldapViewModel: LdapViewModel
	
	@EnvironmentObject var settingsViewModel: SettingsViewModel
	
	@Environment(\.dismiss) var dismiss
	
	@State var isSecured: Bool = true
	
	@FocusState var isServerUrlFocused: Bool
	@FocusState var isBindDnFocused: Bool
	@FocusState var isPasswordFocused: Bool
	@FocusState var isSearchBaseFocused: Bool
	@FocusState var isSearchFilterFocused: Bool
	@FocusState var isMaxResultsFocused: Bool
	@FocusState var isRequestTimeoutFocused: Bool
	@FocusState var isRequestDelayFocused: Bool
	@FocusState var isMinCharactersFocused: Bool
	@FocusState var isNameAttributesFocused: Bool
	@FocusState var isSipAttributesFocused: Bool
	@FocusState var isSipDomainFocused: Bool
	
	init(url: String? = "") {
		_ldapViewModel = StateObject(wrappedValue: LdapViewModel(url: url))
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
					
					Text(ldapViewModel.isEdit ? "settings_contacts_edit_ldap_server_title" : "settings_contacts_add_ldap_server_title")
						.default_text_style_orange_800(styleSize: 16)
						.frame(maxWidth: .infinity, alignment: .leading)
						.padding(.top, 4)
						.lineLimit(1)
					
					Spacer()
					
					if ldapViewModel.isEdit {
						Button {
							ldapViewModel.delete()
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
							Toggle("settings_contacts_ldap_enabled_title", isOn: $ldapViewModel.isEnabled)
								.default_text_style_700(styleSize: 15)
							
							VStack(alignment: .leading) {
								Text("settings_contacts_ldap_server_url_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_ldap_server_url_title", text: $ldapViewModel.serverUrl)
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
								Text("settings_contacts_ldap_bind_dn_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_ldap_bind_dn_title", text: $ldapViewModel.bindDn)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isBindDnFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isBindDnFocused)
							}
							
							VStack(alignment: .leading) {
								Text("settings_contacts_ldap_password_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								ZStack(alignment: .trailing) {
									Group {
										if isSecured {
											SecureField("settings_contacts_ldap_password_title", text: $ldapViewModel.password)
												.default_text_style(styleSize: 15)
												.frame(height: 25)
												.focused($isPasswordFocused)
										} else {
											TextField("settings_contacts_ldap_password_title", text: $ldapViewModel.password)
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
							
							Toggle("settings_contacts_ldap_use_tls_title", isOn: $ldapViewModel.useTls)
								.default_text_style_700(styleSize: 15)
							
							VStack(alignment: .leading) {
								Text("settings_contacts_ldap_search_base_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_ldap_search_base_title", text: $ldapViewModel.searchBase)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isSearchBaseFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isSearchBaseFocused)
							}
							
							VStack(alignment: .leading) {
								Text("settings_contacts_ldap_search_filter_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_ldap_search_filter_title", text: $ldapViewModel.searchFilter)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isSearchFilterFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isSearchFilterFocused)
							}
							
							VStack(alignment: .leading) {
								Text("settings_contacts_ldap_max_results_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_ldap_max_results_title", text: $ldapViewModel.maxResults)
									.default_text_style(styleSize: 15)
									.keyboardType(.numberPad)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isMaxResultsFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isMaxResultsFocused)
							}
							
							VStack(alignment: .leading) {
								Text("settings_contacts_ldap_request_timeout_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_ldap_request_timeout_title", text: $ldapViewModel.requestTimeout)
									.default_text_style(styleSize: 15)
									.keyboardType(.numberPad)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isRequestTimeoutFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isRequestTimeoutFocused)
							}
							
							VStack(alignment: .leading) {
								Text("settings_contacts_ldap_request_delay_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_ldap_request_delay_title", text: $ldapViewModel.requestDelay)
									.default_text_style(styleSize: 15)
									.keyboardType(.numberPad)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isRequestDelayFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isRequestDelayFocused)
							}
							
							VStack(alignment: .leading) {
								Text("settings_contacts_ldap_min_characters_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_ldap_min_characters_title", text: $ldapViewModel.minCharacters)
									.default_text_style(styleSize: 15)
									.keyboardType(.numberPad)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isMinCharactersFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isMinCharactersFocused)
							}
							
							VStack(alignment: .leading) {
								Text("settings_contacts_ldap_name_attributes_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_ldap_name_attributes_title", text: $ldapViewModel.nameAttributes)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isNameAttributesFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isNameAttributesFocused)
							}
							
							VStack(alignment: .leading) {
								Text("settings_contacts_ldap_sip_attributes_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_ldap_sip_attributes_title", text: $ldapViewModel.sipAttributes)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isSipAttributesFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isSipAttributesFocused)
							}
							
							VStack(alignment: .leading) {
								Text("settings_contacts_ldap_sip_domain_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_contacts_ldap_sip_domain_title", text: $ldapViewModel.sipDomain)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isSipDomainFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isSipDomainFocused)
							}
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
						ldapViewModel.addServer()
					} label: {
						Image("plus-circle")
							.renderingMode(.template)
							.resizable()
							.frame(width: 28, height: 28)
							.foregroundStyle(.white)
							.padding()
							.background(ldapViewModel.isFormComplete ? Color.orangeMain500 : Color.gray300)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
						
					}
					.padding()
					.disabled(!ldapViewModel.isFormComplete)
				}
			}
		}
		.onChange(of: ldapViewModel.ldapServerOperationSuccessful) { event in
			if event {
				dismiss()
			  	settingsViewModel.reloadLdapServers()
			}
		}
		.navigationTitle("")
		.navigationBarHidden(true)
	}
}
