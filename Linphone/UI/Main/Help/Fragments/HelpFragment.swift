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

struct HelpFragment: View {
	
	@StateObject private var helpViewModel = HelpViewModel()
	
	@Binding var isShowHelpFragment: Bool
	
	@State var advancedSettingsIsOpen: Bool = false
	
	@FocusState var isVoicemailUriFocused: Bool
	
	var showAssistant: Bool {
		(CoreContext.shared.coreIsStarted && CoreContext.shared.accounts.isEmpty)
		|| SharedMainViewModel.shared.displayProfileMode
	}
	
	var body: some View {
		NavigationView {
			ZStack {
				VStack(spacing: 1) {
					if !showAssistant {
						Rectangle()
							.foregroundColor(Color.orangeMain500)
							.edgesIgnoringSafeArea(.top)
							.frame(height: 0)
					}
					
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
								withAnimation {
									isShowHelpFragment = false
								}
							}
						
						Text("help_title")
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
							VStack(spacing: 20) {
								if let urlString = CorePreferences.themeAboutPictureUrl,
								   let url = URL(string: urlString) {
									AsyncImage(url: url) { phase in
										switch phase {
										case .empty:
											ProgressView()
												.frame(maxWidth: .infinity, minHeight: 100, maxHeight: 100)
										case .success(let image):
											image
												.resizable()
												.scaledToFit()
												.frame(maxWidth: .infinity, maxHeight: 100, alignment: .center)
										case .failure:
											EmptyView()
										@unknown default:
											EmptyView()
										}
									}
								} else {
									EmptyView()
								}
								Text("help_about_title")
									.default_text_style_800(styleSize: 16)
									.frame(maxWidth: .infinity, alignment: .leading)
									.padding(.bottom, 5)
								Button {
									if let url = URL(string: NSLocalizedString("website_user_guide_url", comment: "")) {
										UIApplication.shared.open(url)
									}
								} label: {
									HStack {
										Image("book-open-text")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.orangeMain500)
											.frame(width: 30, height: 30)
										
										VStack {
											Text("help_about_user_guide_title")
												.default_text_style_700(styleSize: 14)
												.frame(maxWidth: .infinity, alignment: .leading)
												.multilineTextAlignment(.leading)
											
											Text("help_about_user_guide_subtitle")
												.default_text_style(styleSize: 14)
												.frame(maxWidth: .infinity, alignment: .leading)
												.multilineTextAlignment(.leading)
										}
										.padding(.horizontal, 5)
										
										Image("arrow-square-out")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c600)
											.frame(width: 25, height: 25)
									}
								}
								
								Button {
									if let url = URL(string: NSLocalizedString("website_privacy_policy_url", comment: "")) {
										UIApplication.shared.open(url)
									}
								} label: {
									HStack {
										Image("detective")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.orangeMain500)
											.frame(width: 30, height: 30)
										
										VStack {
											Text("help_about_privacy_policy_title")
												.default_text_style_700(styleSize: 14)
												.frame(maxWidth: .infinity, alignment: .leading)
												.multilineTextAlignment(.leading)
											
											Text("help_about_privacy_policy_subtitle")
												.default_text_style(styleSize: 14)
												.frame(maxWidth: .infinity, alignment: .leading)
												.multilineTextAlignment(.leading)
										}
										.padding(.horizontal, 5)
										
										Image("arrow-square-out")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c600)
											.frame(width: 25, height: 25)
									}
								}
								
								HStack {
									Image("info")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.orangeMain500)
										.frame(width: 30, height: 30)
									
									VStack {
										Text("help_about_version_title")
											.default_text_style_700(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
											.multilineTextAlignment(.leading)
										
										Text(helpViewModel.version)
											.default_text_style(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
											.multilineTextAlignment(.leading)
									}
									.padding(.horizontal, 5)
									
									Button(
										action: {
											helpViewModel.checkForUpdate()
										}, label: {
											Text("help_about_check_for_update")
												.default_text_style_orange_500(styleSize: 14)
												.lineLimit(1)
										}
									)
									.padding(.horizontal, 15)
									.padding(.vertical, 10)
									.background(Color.orangeMain100)
									.cornerRadius(60)
								}
								
								Button {
									if let url = URL(string: NSLocalizedString("website_open_source_licences_usage_url", comment: "")) {
										UIApplication.shared.open(url)
									}
								} label: {
									HStack {
										Image("check-square-offset")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.orangeMain500)
											.frame(width: 30, height: 30)
										
										VStack {
											Text("help_about_open_source_licenses_title")
												.default_text_style_700(styleSize: 14)
												.frame(maxWidth: .infinity, alignment: .leading)
												.multilineTextAlignment(.leading)
											
											Text("help_about_open_source_licenses_subtitle")
												.default_text_style(styleSize: 14)
												.frame(maxWidth: .infinity, alignment: .leading)
												.multilineTextAlignment(.leading)
										}
										.padding(.horizontal, 5)
										
										Image("arrow-square-out")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c600)
											.frame(width: 25, height: 25)
									}
								}
								
								Button {
									if let url = URL(string: NSLocalizedString("website_translate_weblate_url", comment: "")) {
										UIApplication.shared.open(url)
									}
								} label: {
									HStack {
										Image("earth")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.orangeMain500)
											.frame(width: 30, height: 30)
										
										Text("help_about_contribute_translations_title")
											.default_text_style_700(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
											.multilineTextAlignment(.leading)
											.padding(.horizontal, 5)
										
										Image("arrow-square-out")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c600)
											.frame(width: 25, height: 25)
									}
								}
								
								Text("help_about_advanced_title")
									.default_text_style_800(styleSize: 16)
									.frame(maxWidth: .infinity, alignment: .leading)
									.padding(.top, 20)
									.padding(.bottom, 5)
								
								NavigationLink(destination: {
									DebugFragment(helpViewModel: helpViewModel)
								}, label: {
									HStack {
										Image("wrench")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.orangeMain500)
											.frame(width: 30, height: 30)
										
										Text("help_troubleshooting_title")
											.default_text_style_700(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
											.multilineTextAlignment(.leading)
											.padding(.horizontal, 5)
										
										Image("caret-right")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c600)
											.frame(width: 25, height: 25)
									}
								})
							}
							.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
							.padding(.all, 20)
						}
						.frame(maxWidth: .infinity)
					}
					.background(Color.gray100)
				}
				.background(Color.gray100)
				
				if helpViewModel.checkUpdateAvailable {
					PopupView(
						isShowPopup: $helpViewModel.checkUpdateAvailable,
						title: Text("help_dialog_update_available_title"),
						content: Text(String(format: String(localized: "help_dialog_update_available_message"), helpViewModel.versionAvailable)),
						titleFirstButton: Text("dialog_cancel"),
						actionFirstButton: {
							helpViewModel.checkUpdateAvailable = false
						},
						titleSecondButton: Text("dialog_install"),
						actionSecondButton: {
							helpViewModel.checkUpdateAvailable = false
							if let url = URL(string: helpViewModel.urlVersionAvailable) {
								UIApplication.shared.open(url)
							}
						}
					)
					.background(.black.opacity(0.65))
					.zIndex(3)
					.onTapGesture {
						helpViewModel.checkUpdateAvailable = false
					}
				}
			}
			.navigationTitle("")
			.navigationBarHidden(true)
		}
		.navigationViewStyle(StackNavigationViewStyle())
		.navigationTitle("")
		.navigationBarHidden(true)
	}
}
