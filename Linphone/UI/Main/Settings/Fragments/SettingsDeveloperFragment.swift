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

struct SettingsDeveloperFragment: View {
	@ObservedObject var settingsViewModel: SettingsViewModel
	
	@Environment(\.dismiss) var dismiss
	
	@FocusState var isUploadServerUrlFocused: Bool
	@FocusState var isLogsUploadServerUrlFocused: Bool
	@FocusState var isPushCompatibleDomainsListFocused: Bool
	
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
					
					Text("settings_developer_title")
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
						VStack(spacing: 30) {
							Toggle("settings_developer_show_title", isOn: $settingsViewModel.showDeveloperSettings)
								.default_text_style_700(styleSize: 15)
							
							Toggle("help_troubleshooting_print_logs_in_logcat", isOn: $settingsViewModel.printLogsInLogcat)
								.default_text_style_700(styleSize: 15)
							
							VStack(alignment: .leading) {
								Text("settings_advanced_upload_server_url")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_advanced_upload_server_url", text: $settingsViewModel.uploadServerUrl)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isUploadServerUrlFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isUploadServerUrlFocused)
							}
							
							VStack(alignment: .leading) {
								Text("settings_advanced_logs_upload_server_url")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_advanced_logs_upload_server_url", text: $settingsViewModel.logsUploadServerUrl)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isLogsUploadServerUrlFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isLogsUploadServerUrlFocused)
							}
							
							// TODO: Add these settings
							/*
							Toggle("settings_advanced_create_e2e_encrypted_conferences_title", isOn: $settingsViewModel.???)
								.default_text_style_700(styleSize: 15)
							
							Toggle("settings_developer_enable_vu_meters_title", isOn: $settingsViewModel.???)
								.default_text_style_700(styleSize: 15)
							
							Toggle("settings_developer_enable_advanced_call_stats_title", isOn: $settingsViewModel.???)
								.default_text_style_700(styleSize: 15)
							
							VStack(alignment: .leading) {
								Text("settings_developer_push_compatible_domains_list_title")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("settings_developer_push_compatible_domains_list_title", text: $settingsViewModel.???)
									.default_text_style(styleSize: 15)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isPushCompatibleDomainsListFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
									)
									.focused($isPushCompatibleDomainsListFocused)
							}
							*/
							
							VStack(alignment: .leading) {
								Button(
									action: {
										settingsViewModel.clearNativeFriendsDatabase()
									}, label: {
										Text("settings_developer_clear_native_friends_in_database_title")
											.default_text_style_white_600(styleSize: 15)
											.frame(maxWidth: .infinity, alignment: .center)
									}
								)
								.padding(.horizontal, 20)
								.padding(.vertical, 10)
								.background(Color.redDanger500)
								.cornerRadius(60)
								
								Text("settings_developer_clear_native_friends_in_database_subtitle")
									.default_text_style(styleSize: 14)
									.frame(maxWidth: .infinity, alignment: .leading)
							}
							
							Button(
								action: {
									settingsViewModel.clearOrphanAuthInfo()
								}, label: {
									Text("settings_developer_clear_orphan_auth_info_title")
										.default_text_style_white_600(styleSize: 15)
										.frame(maxWidth: .infinity, alignment: .center)
								}
							)
							.padding(.horizontal, 20)
							.padding(.vertical, 10)
							.background(Color.redDanger500)
							.cornerRadius(60)
							
						}
						.padding(.vertical, 20)
						.padding(.horizontal, 20)
						.background(.white)
						.cornerRadius(15)
						.background(Color.gray100)
					}
					.padding(.vertical, 20)
					.padding(.horizontal, 20)
					.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
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
