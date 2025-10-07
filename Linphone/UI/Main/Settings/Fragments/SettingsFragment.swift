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

struct SettingsFragment: View {
	
	@StateObject private var settingsViewModel = SettingsViewModel()
	
	@Binding var isShowSettingsFragment: Bool
	
	@State var securityIsOpen: Bool = false
	@State var callsIsOpen: Bool = false
	@State var conversationsIsOpen: Bool = false
	@State var contactsIsOpen: Bool = false
	@State var meetingsIsOpen: Bool = false
	@State var networkIsOpen: Bool = false
	@State var userInterfaceIsOpen: Bool = false
	
	var body: some View {
		NavigationView {
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
								settingsViewModel.saveChangesWhenLeaving()
								withAnimation {
									if isShowSettingsFragment {
										isShowSettingsFragment = false
									}
								}
							}
						
						Text("settings_title")
							.default_text_style_orange_800(styleSize: 16)
							.frame(maxWidth: .infinity, alignment: .leading)
							.padding(.top, 4)
							.lineLimit(1)
						
						Spacer()
					}
					.frame(maxWidth: .infinity)
					.frame(height: 50)
					.padding(.horizontal)
					.padding(.bottom, 4)
					.background(.white)
					
					ScrollView {
						VStack(spacing: 0) {
							// TODO: Wait for VFS fix
							/*
							HStack(alignment: .center) {
								Text("settings_security_title")
									.default_text_style_800(styleSize: 18)
									.frame(maxWidth: .infinity, alignment: .leading)
								
								Spacer()
								
								Image(securityIsOpen ? "caret-up" : "caret-down")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
							}
							.padding(.vertical, 10)
							.padding(.horizontal, 20)
							.background(Color.gray100)
							.onTapGesture {
								withAnimation {
									securityIsOpen.toggle()
								}
							}
							
							if securityIsOpen {
								VStack(spacing: 0) {
									VStack(spacing: 30) {
										HStack {
											VStack(alignment: .leading, spacing: 2) {
												Text("settings_security_enable_vfs_title")
													.default_text_style_700(styleSize: 15)
												Text("settings_security_enable_vfs_subtitle")
													.foregroundColor(Color.grayMain2c500)
													.default_text_style(styleSize: 14)
											}
											.layoutPriority(1)
											
											Toggle(isOn: $settingsViewModel.enableVfs) {
												EmptyView()
											}
											.toggleStyle(SwitchToggleStyle())
											.disabled(settingsViewModel.enableVfs)
										}
										
										/*
										Toggle("settings_security_prevent_screenshots_title", isOn: $isOn)
											.default_text_style_700(styleSize: 15)
										*/
									}
									.padding(.vertical, 30)
									.padding(.horizontal, 20)
								}
								.background(.white)
								.cornerRadius(15)
								.padding(.horizontal)
								.zIndex(-1)
								.transition(.move(edge: .top))
							}
							*/
							
							HStack(alignment: .center) {
								Text("settings_calls_title")
									.default_text_style_800(styleSize: 18)
									.frame(maxWidth: .infinity, alignment: .leading)
								
								Spacer()
								
								Image(callsIsOpen ? "caret-up" : "caret-down")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
							}
							.padding(.vertical, 10)
							.padding(.horizontal, 20)
							.background(Color.gray100)
							.onTapGesture {
								withAnimation {
									callsIsOpen.toggle()
								}
							}
							
							if callsIsOpen {
								VStack(spacing: 0) {
									VStack(spacing: 30) {
										Toggle("settings_calls_adaptive_rate_control_title", isOn: $settingsViewModel.adaptiveRateControl)
											.default_text_style_700(styleSize: 15)
										
										Toggle("settings_calls_enable_video_title", isOn: $settingsViewModel.enableVideo)
											.default_text_style_700(styleSize: 15)
										
										/*
										Toggle("settings_calls_vibrate_while_ringing_title", isOn: $isOn)
											.default_text_style_700(styleSize: 15)
										*/
										
										Toggle("settings_calls_auto_record_title", isOn: $settingsViewModel.autoRecord)
											.default_text_style_700(styleSize: 15)
										
										/*
										Button {
										} label: {
											HStack {
												Text("settings_calls_change_ringtone_title")
													.default_text_style_700(styleSize: 15)
													.frame(maxWidth: .infinity, alignment: .leading)
												
												Image("arrow-square-out")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(Color.grayMain2c600)
													.frame(width: 25, height: 25, alignment: .leading)
													.padding(.trailing, 10)
											}
										}
										*/
									}
									.padding(.vertical, 30)
									.padding(.horizontal, 20)
								}
								.background(.white)
								.cornerRadius(15)
								.padding(.horizontal)
								.zIndex(-2)
								.transition(.move(edge: .top))
							}
							
							HStack(alignment: .center) {
								Text("settings_conversations_title")
									.default_text_style_800(styleSize: 18)
									.frame(maxWidth: .infinity, alignment: .leading)
								
								Spacer()
								
								Image(conversationsIsOpen ? "caret-up" : "caret-down")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
							}
							.padding(.vertical, 10)
							.padding(.horizontal, 20)
							.background(Color.gray100)
							.onTapGesture {
								withAnimation {
									conversationsIsOpen.toggle()
								}
							}
							
							if conversationsIsOpen {
								VStack(spacing: 0) {
									VStack(spacing: 30) {
										Toggle("settings_conversations_auto_download_title", isOn: $settingsViewModel.autoDownload)
											.default_text_style_700(styleSize: 15)
									}
									.padding(.vertical, 30)
									.padding(.horizontal, 20)
								}
								.background(.white)
								.cornerRadius(15)
								.padding(.horizontal)
								.zIndex(-3)
								.transition(.move(edge: .top))
							}
							
							HStack(alignment: .center) {
								Text("settings_contacts_title")
									.default_text_style_800(styleSize: 18)
									.frame(maxWidth: .infinity, alignment: .leading)
								
								Spacer()
								
								Image(contactsIsOpen ? "caret-up" : "caret-down")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
							}
							.padding(.vertical, 10)
							.padding(.horizontal, 20)
							.background(Color.gray100)
							.onTapGesture {
								withAnimation {
									contactsIsOpen.toggle()
								}
							}
							
							if contactsIsOpen {
								VStack(spacing: 0) {
									VStack(spacing: 20) {
										NavigationLink(destination: {
											LdapServerConfigurationFragment()
												.environmentObject(settingsViewModel)
										}, label: {
											HStack(alignment: .center) {
												Text("settings_contacts_add_ldap_server_title")
													.default_text_style_700(styleSize: 15)
													.frame(maxWidth: .infinity, alignment: .leading)
												
												Spacer()
												
												Image("caret-right")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(Color.grayMain2c600)
													.frame(width: 20, height: 20, alignment: .leading)
													.padding(.all, 10)
											}
											.frame(maxWidth: .infinity)
										})
										
										if !settingsViewModel.ldapServers.isEmpty {
											ForEach(settingsViewModel.ldapServers, id: \.self) { ldap in
												NavigationLink(destination: {
													LdapServerConfigurationFragment(url: ldap)
														.environmentObject(settingsViewModel)
												}, label: {
													HStack(alignment: .center) {
														Text(ldap)
															.default_text_style_700(styleSize: 15)
															.frame(maxWidth: .infinity, alignment: .leading)
														
														Spacer()
														
														Image("pencil-simple")
															.renderingMode(.template)
															.resizable()
															.foregroundStyle(Color.grayMain2c600)
															.frame(width: 20, height: 20, alignment: .leading)
															.padding(.all, 10)
													}
													.padding(.horizontal, 10)
													.frame(maxWidth: .infinity)
												})
											}
										}
										
										NavigationLink(destination: {
											CardDavAddressBookConfigurationFragment()
												.environmentObject(settingsViewModel)
										}, label: {
											HStack(alignment: .center) {
												Text("settings_contacts_add_carddav_server_title")
													.default_text_style_700(styleSize: 15)
													.frame(maxWidth: .infinity, alignment: .leading)
												
												Spacer()
												
												Image("caret-right")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(Color.grayMain2c600)
													.frame(width: 20, height: 20, alignment: .leading)
													.padding(.all, 10)
											}
											.frame(maxWidth: .infinity)
										})
										
										if !settingsViewModel.cardDavFriendsLists.isEmpty {
											ForEach(settingsViewModel.cardDavFriendsLists, id: \.self) { cardDavName in
												NavigationLink(destination: {
													CardDavAddressBookConfigurationFragment(name: cardDavName)
												  .environmentObject(settingsViewModel)
												}, label: {
													HStack(alignment: .center) {
														Text(cardDavName)
															.default_text_style_700(styleSize: 15)
															.frame(maxWidth: .infinity, alignment: .leading)
														
														Spacer()
														
														Image("pencil-simple")
															.renderingMode(.template)
															.resizable()
															.foregroundStyle(Color.grayMain2c600)
															.frame(width: 20, height: 20, alignment: .leading)
															.padding(.all, 10)
													}
													.padding(.horizontal, 10)
													.frame(maxWidth: .infinity)
												})
											}
										}
									}
									.padding(.vertical, 30)
									.padding(.horizontal, 20)
								}
								.background(.white)
								.cornerRadius(15)
								.padding(.horizontal)
								.zIndex(-4)
								.transition(.move(edge: .top))
							}
							
							HStack(alignment: .center) {
								Text("settings_meetings_title")
									.default_text_style_800(styleSize: 18)
									.frame(maxWidth: .infinity, alignment: .leading)
								
								Spacer()
								
								Image(meetingsIsOpen ? "caret-up" : "caret-down")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
							}
							.padding(.vertical, 10)
							.padding(.horizontal, 20)
							.background(Color.gray100)
							.onTapGesture {
								withAnimation {
									meetingsIsOpen.toggle()
								}
							}
							
							if meetingsIsOpen {
								VStack(spacing: 0) {
									VStack(spacing: 30) {
										VStack(alignment: .leading) {
											Text("settings_meetings_default_layout_title")
												.default_text_style_700(styleSize: 15)
												.padding(.bottom, -5)
											
											Menu {
												Button("settings_meetings_layout_active_speaker_label") { settingsViewModel.defaultLayout = String(localized: "settings_meetings_layout_active_speaker_label") }
												Button("settings_meetings_layout_mosaic_label") { settingsViewModel.defaultLayout = String(localized: "settings_meetings_layout_mosaic_label") }
											} label: {
												Text(settingsViewModel.defaultLayout)
													.default_text_style(styleSize: 15)
													.frame(maxWidth: .infinity, alignment: .leading)
												Image("caret-down")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(Color.grayMain2c500)
													.frame(width: 20, height: 20)
											}
											.frame(height: 25)
											.padding(.horizontal, 20)
											.padding(.vertical, 15)
											.cornerRadius(60)
											.overlay(
												RoundedRectangle(cornerRadius: 60)
													.inset(by: 0.5)
													.stroke(Color.gray200, lineWidth: 1)
											)
										}
									}
									.padding(.vertical, 30)
									.padding(.horizontal, 20)
								}
								.background(.white)
								.cornerRadius(15)
								.padding(.horizontal)
								.zIndex(-5)
								.transition(.move(edge: .top))
							}
							
							HStack(alignment: .center) {
								Text("settings_network_title")
									.default_text_style_800(styleSize: 18)
									.frame(maxWidth: .infinity, alignment: .leading)
								
								Spacer()
								
								Image(networkIsOpen ? "caret-up" : "caret-down")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
							}
							.padding(.vertical, 10)
							.padding(.horizontal, 20)
							.background(Color.gray100)
							.onTapGesture {
								withAnimation {
									networkIsOpen.toggle()
								}
							}
							
							if networkIsOpen {
								VStack(spacing: 0) {
									VStack(spacing: 30) {
										Toggle("settings_network_use_wifi_only", isOn: $settingsViewModel.useWifiOnly)
											.default_text_style_700(styleSize: 15)
										
										Toggle("settings_network_allow_ipv6", isOn: $settingsViewModel.allowIpv6)
											.default_text_style_700(styleSize: 15)
									}
									.padding(.vertical, 30)
									.padding(.horizontal, 20)
								}
								.background(.white)
								.cornerRadius(15)
								.padding(.horizontal)
								.zIndex(-6)
								.transition(.move(edge: .top))
							}
							
							/*
							// Hide User interface (Dark mode)
							
							HStack(alignment: .center) {
								Text("manage_account_details_title")
									.default_text_style_800(styleSize: 18)
									.frame(maxWidth: .infinity, alignment: .leading)
								
								Spacer()
								
								Image(userInterfaceIsOpen ? "caret-up" : "caret-down")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
							}
							.padding(.vertical, 10)
							.padding(.horizontal, 20)
							.background(Color.gray100)
							.onTapGesture {
								withAnimation {
									userInterfaceIsOpen.toggle()
								}
							}
							
							if userInterfaceIsOpen {
								VStack(spacing: 0) {
									VStack(spacing: 30) {
										Toggle("account_settings_avpf_title", isOn: $isOn)
											.default_text_style_700(styleSize: 15)
										
										Toggle("account_settings_avpf_title", isOn: $isOn)
											.default_text_style_700(styleSize: 15)
									}
									.padding(.vertical, 30)
									.padding(.horizontal, 20)
								}
								.background(.white)
								.cornerRadius(15)
								.padding(.horizontal)
								.zIndex(-7)
								.transition(.move(edge: .top))
							}
							*/
							NavigationLink(destination: {
								SettingsAdvancedFragment(settingsViewModel: settingsViewModel)
							}, label: {
								HStack(alignment: .center) {
									Text("settings_advanced_title")
										.default_text_style_800(styleSize: 18)
										.frame(maxWidth: .infinity, alignment: .leading)
									
									Spacer()
									
									Image("caret-right")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.grayMain2c600)
										.frame(width: 25, height: 25, alignment: .leading)
										.padding(.all, 10)
								}
								.frame(maxWidth: .infinity)
								
							})
							.padding(.vertical, 10)
							.padding(.horizontal, 20)
							.background(Color.gray100)
						}
					}
					.background(Color.gray100)
				}
				.background(Color.gray100)
			}
			.navigationTitle("")
			.navigationBarHidden(true)
		}
		.navigationViewStyle(StackNavigationViewStyle())
	}
}
