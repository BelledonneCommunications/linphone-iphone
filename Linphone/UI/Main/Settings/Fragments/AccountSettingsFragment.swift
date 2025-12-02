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

// swiftlint:disable type_body_length
struct AccountSettingsFragment: View {
	
	@StateObject private var accountSettingsViewModel: AccountSettingsViewModel
	
	@Environment(\.dismiss) var dismiss
	
	@State var natPolicySettingsIsOpen: Bool = false
	@State var advancedSettingsIsOpen: Bool = false
	@State var isSecured: Bool = true
	
	@FocusState var isVoicemailUriFocused: Bool
	@FocusState var isMwiUriFocused: Bool
	@FocusState var isStunServerUriFocused: Bool
	@FocusState var isTurnUsernameFocused: Bool
	@FocusState var isTurnPasswordFocused: Bool
	@FocusState var isSipProxyUrlFocused: Bool
	@FocusState var isSettingsExpireFocused: Bool
	@FocusState var isConferenceFactoryUriFocused: Bool
	@FocusState var isAudioVideoConferenceFactoryUriFocused: Bool
	@FocusState var isCcmpServerUrlFocused: Bool
	@FocusState var isLimeServerUrlFocused: Bool
	
	init(accountModel: AccountModel) {
		_accountSettingsViewModel = StateObject(wrappedValue: AccountSettingsViewModel(accountModel: accountModel))
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
							accountSettingsViewModel.saveChanges()
							dismiss()
						}
					
					Text("account_settings_title")
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
						VStack(spacing: 0) {
							VStack(spacing: 0) {
								VStack(spacing: 30) {
									Toggle("account_settings_push_notification_title", isOn: $accountSettingsViewModel.pushNotification)
										.default_text_style_700(styleSize: 15)
									
									Toggle("account_settings_im_encryption_mandatory_title", isOn: $accountSettingsViewModel.imEncryptionMandatory)
										.default_text_style_700(styleSize: 15)
									
									VStack(alignment: .leading) {
										Text("account_settings_voicemail_uri_title")
											.default_text_style_700(styleSize: 15)
											.padding(.bottom, -5)
										
										TextField("account_settings_voicemail_uri_title", text: $accountSettingsViewModel.voicemailUri)
											.default_text_style(styleSize: 15)
											.frame(height: 25)
											.padding(.horizontal, 20)
											.padding(.vertical, 15)
											.background(.white)
											.cornerRadius(60)
											.overlay(
												RoundedRectangle(cornerRadius: 60)
													.inset(by: 0.5)
													.stroke(isVoicemailUriFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
											)
											.focused($isVoicemailUriFocused)
									}
									
									VStack(alignment: .leading) {
										Text("account_settings_mwi_uri_title")
											.default_text_style_700(styleSize: 15)
											.padding(.bottom, -5)
										
										TextField("account_settings_mwi_uri_title", text: $accountSettingsViewModel.mwiUri)
											.default_text_style(styleSize: 15)
											.frame(height: 25)
											.padding(.horizontal, 20)
											.padding(.vertical, 15)
											.background(.white)
											.cornerRadius(60)
											.overlay(
												RoundedRectangle(cornerRadius: 60)
													.inset(by: 0.5)
													.stroke(isMwiUriFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
											)
											.focused($isMwiUriFocused)
									}
									
									Toggle("account_settings_apply_international_prefix_title", isOn: $accountSettingsViewModel.applyInternationalPrefix)
										.default_text_style_700(styleSize: 15)
									
									Toggle("account_settings_replace_plus_by_00_title", isOn: $accountSettingsViewModel.replacePlusBy00)
										.default_text_style_700(styleSize: 15)
								}
								.padding(.vertical, 30)
								.padding(.horizontal, 20)
							}
							.background(.white)
							.cornerRadius(15)
							.padding(.horizontal)
							.padding(.top, 10)
							.background(Color.gray100)
							
							HStack(alignment: .center) {
								Text("account_settings_nat_policy_title")
									.default_text_style_800(styleSize: 18)
									.frame(maxWidth: .infinity, alignment: .leading)
								
								Spacer()
								
								Image(natPolicySettingsIsOpen ? "caret-up" : "caret-down")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
							}
							.padding(.top, 30)
							.padding(.bottom, 10)
							.padding(.horizontal, 20)
							.background(Color.gray100)
							.onTapGesture {
								withAnimation {
									natPolicySettingsIsOpen.toggle()
								}
							}
							
							if natPolicySettingsIsOpen {
								if accountSettingsViewModel.accountModel.avatarModel != nil {
									VStack(spacing: 0) {
										VStack(spacing: 30) {
											VStack(alignment: .leading) {
												Text("account_settings_stun_server_url_title")
													.default_text_style_700(styleSize: 15)
													.padding(.bottom, -5)
												
												TextField("account_settings_stun_server_url_title", text: $accountSettingsViewModel.stunServerUrl)
													.default_text_style(styleSize: 15)
													.frame(height: 25)
													.padding(.horizontal, 20)
													.padding(.vertical, 15)
													.background(.white)
													.cornerRadius(60)
													.overlay(
														RoundedRectangle(cornerRadius: 60)
															.inset(by: 0.5)
															.stroke(isStunServerUriFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
													)
													.focused($isStunServerUriFocused)
											}
											
											Toggle("account_settings_enable_ice_title", isOn: $accountSettingsViewModel.enableIce)
												.default_text_style_700(styleSize: 15)
											
											Toggle("account_settings_enable_turn_title", isOn: $accountSettingsViewModel.enableTurn)
												.default_text_style_700(styleSize: 15)
											
											VStack(alignment: .leading) {
												Text("account_settings_turn_username_title")
													.default_text_style_700(styleSize: 15)
													.padding(.bottom, -5)
												
												TextField("account_settings_turn_username_title", text: $accountSettingsViewModel.turnUsername)
													.default_text_style(styleSize: 15)
													.frame(height: 25)
													.padding(.horizontal, 20)
													.padding(.vertical, 15)
													.background(.white)
													.cornerRadius(60)
													.overlay(
														RoundedRectangle(cornerRadius: 60)
															.inset(by: 0.5)
															.stroke(isTurnUsernameFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
													)
													.focused($isTurnUsernameFocused)
											}
											
											VStack(alignment: .leading) {
												Text("account_settings_turn_password_title")
													.default_text_style_700(styleSize: 15)
													.padding(.bottom, -5)
												
												ZStack(alignment: .trailing) {
													Group {
														if isSecured {
															SecureField("account_settings_turn_password_title", text: $accountSettingsViewModel.turnPassword)
																.default_text_style(styleSize: 15)
																.frame(height: 25)
																.focused($isTurnPasswordFocused)
														} else {
															TextField("account_settings_turn_password_title", text: $accountSettingsViewModel.turnPassword)
																.default_text_style(styleSize: 15)
																.disableAutocorrection(true)
																.autocapitalization(.none)
																.frame(height: 25)
																.focused($isTurnPasswordFocused)
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
														.stroke(isTurnPasswordFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
												)
												.padding(.bottom)
											}
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
							}
							
							HStack(alignment: .center) {
								Text("settings_advanced_title")
									.default_text_style_800(styleSize: 18)
									.frame(maxWidth: .infinity, alignment: .leading)
								
								Spacer()
								
								Image(advancedSettingsIsOpen ? "caret-up" : "caret-down")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
							}
							.padding(.top, 30)
							.padding(.bottom, 10)
							.padding(.horizontal, 20)
							.background(Color.gray100)
							.onTapGesture {
								withAnimation {
									advancedSettingsIsOpen.toggle()
								}
							}
							
							if advancedSettingsIsOpen {
								if accountSettingsViewModel.accountModel.avatarModel != nil {
									VStack(spacing: 0) {
										VStack(spacing: 30) {
											VStack(alignment: .leading) {
												Text("assistant_sip_account_transport_protocol")
													.default_text_style_700(styleSize: 15)
													.padding(.bottom, -5)
												
												Menu {
													Button("TLS") { accountSettingsViewModel.transport = "TLS" }
													Button("TCP") { accountSettingsViewModel.transport = "TCP" }
													Button("UDP") { accountSettingsViewModel.transport = "UDP" }
												} label: {
													Text(accountSettingsViewModel.transport)
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
											
											VStack(alignment: .leading) {
												Text("account_settings_sip_proxy_url_title")
													.default_text_style_700(styleSize: 15)
													.padding(.bottom, -5)
												
												TextField("account_settings_sip_proxy_url_title", text: $accountSettingsViewModel.sipProxyUrl)
													.default_text_style(styleSize: 15)
													.frame(height: 25)
													.padding(.horizontal, 20)
													.padding(.vertical, 15)
													.background(.white)
													.cornerRadius(60)
													.overlay(
														RoundedRectangle(cornerRadius: 60)
															.inset(by: 0.5)
															.stroke(isSipProxyUrlFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
													)
													.focused($isSipProxyUrlFocused)
											}
											
											Toggle("account_settings_outbound_proxy_title", isOn: $accountSettingsViewModel.outboundProxy)
												.default_text_style_700(styleSize: 15)
											
											Toggle("account_settings_avpf_title", isOn: $accountSettingsViewModel.avpf)
												.default_text_style_700(styleSize: 15)
											
											Toggle("account_settings_bundle_mode_title", isOn: $accountSettingsViewModel.bundleMode)
												.default_text_style_700(styleSize: 15)
											
											Toggle("account_settings_cpim_in_basic_conversations_title", isOn: $accountSettingsViewModel.cpimInBasicConversations)
												.default_text_style_700(styleSize: 15)
											
											VStack(alignment: .leading) {
												Text("account_settings_expire_title")
													.default_text_style_700(styleSize: 15)
													.padding(.bottom, -5)
												
												TextField("account_settings_expire_title", text: $accountSettingsViewModel.expire)
													.default_text_style(styleSize: 15)
													.frame(height: 25)
													.padding(.horizontal, 20)
													.padding(.vertical, 15)
													.background(.white)
													.cornerRadius(60)
													.overlay(
														RoundedRectangle(cornerRadius: 60)
															.inset(by: 0.5)
															.stroke(isSettingsExpireFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
													)
													.focused($isSettingsExpireFocused)
											}
											
											VStack(alignment: .leading) {
												Text("account_settings_conference_factory_uri_title")
													.default_text_style_700(styleSize: 15)
													.padding(.bottom, -5)
												
												TextField("account_settings_conference_factory_uri_title", text: $accountSettingsViewModel.conferenceFactoryUri)
													.default_text_style(styleSize: 15)
													.frame(height: 25)
													.padding(.horizontal, 20)
													.padding(.vertical, 15)
													.background(.white)
													.cornerRadius(60)
													.overlay(
														RoundedRectangle(cornerRadius: 60)
															.inset(by: 0.5)
															.stroke(isConferenceFactoryUriFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
													)
													.focused($isConferenceFactoryUriFocused)
											}
											
											VStack(alignment: .leading) {
												Text("account_settings_audio_video_conference_factory_uri_title")
													.default_text_style_700(styleSize: 15)
													.padding(.bottom, -5)
												
												TextField("account_settings_audio_video_conference_factory_uri_title", text: $accountSettingsViewModel.audioVideoConferenceFactoryUri)
													.default_text_style(styleSize: 15)
													.frame(height: 25)
													.padding(.horizontal, 20)
													.padding(.vertical, 15)
													.background(.white)
													.cornerRadius(60)
													.overlay(
														RoundedRectangle(cornerRadius: 60)
															.inset(by: 0.5)
															.stroke(isAudioVideoConferenceFactoryUriFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
													)
													.focused($isAudioVideoConferenceFactoryUriFocused)
											}
											
											VStack(alignment: .leading) {
												Text("account_settings_ccmp_server_url_title")
													.default_text_style_700(styleSize: 15)
													.padding(.bottom, -5)
												
												TextField("account_settings_ccmp_server_url_title", text: $accountSettingsViewModel.ccmpServerUrl)
													.default_text_style(styleSize: 15)
													.frame(height: 25)
													.padding(.horizontal, 20)
													.padding(.vertical, 15)
													.background(.white)
													.cornerRadius(60)
													.overlay(
														RoundedRectangle(cornerRadius: 60)
															.inset(by: 0.5)
															.stroke(isCcmpServerUrlFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
													)
													.focused($isCcmpServerUrlFocused)
											}
											
											VStack(alignment: .leading) {
												Text("account_settings_lime_server_url_title")
													.default_text_style_700(styleSize: 15)
													.padding(.bottom, -5)
												
												TextField("account_settings_lime_server_url_title", text: $accountSettingsViewModel.limeServerUrl)
													.default_text_style(styleSize: 15)
													.frame(height: 25)
													.padding(.horizontal, 20)
													.padding(.vertical, 15)
													.background(.white)
													.cornerRadius(60)
													.overlay(
														RoundedRectangle(cornerRadius: 60)
															.inset(by: 0.5)
															.stroke(isLimeServerUrlFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
													)
													.focused($isLimeServerUrlFocused)
											}
											/*
											Button {
												// TODO Update password
											} label: {
												Text("account_settings_update_password_title")
													.default_text_style_700(styleSize: 15)
													.frame(maxWidth: .infinity, alignment: .leading)
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
							}
						}
						.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
					}
					.frame(maxWidth: .infinity)
				}
				.background(Color.gray100)
			}
			.background(Color.gray100)
		}
		.navigationTitle("")
		.navigationBarHidden(true)
	}
}
// swiftlint:enable type_body_length
