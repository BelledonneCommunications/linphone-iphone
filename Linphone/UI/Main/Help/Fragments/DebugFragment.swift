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

struct DebugFragment: View {
	@ObservedObject var helpViewModel: HelpViewModel
	
	@Environment(\.dismiss) var dismiss
	
	@State private var showShareSheet: Bool = false
	
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
					
					Text("help_troubleshooting_title")
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
							/*
							Toggle("help_troubleshooting_print_logs_in_logcat", isOn: $helpViewModel.logcat)
								.default_text_style_700(styleSize: 15)
							*/
							
							HStack {
								Spacer()
								
								Button(
									action: {
										helpViewModel.cleanLogs()
									}, label: {
										Text("help_troubleshooting_clean_logs")
											.default_text_style_orange_500(styleSize: 14)
											.lineLimit(1)
									}
								)
								.padding(.horizontal, 15)
								.padding(.vertical, 10)
								.background(Color.orangeMain100)
								.cornerRadius(60)
								
								Spacer()
								
								Button(
									action: {
										helpViewModel.shareLogs()
									}, label: {
										Text("help_troubleshooting_share_logs")
											.default_text_style_orange_500(styleSize: 14)
											.lineLimit(1)
									}
								)
								.padding(.horizontal, 15)
								.padding(.vertical, 10)
								.background(Color.orangeMain100)
								.cornerRadius(60)
								
								Spacer()
							}
							
							Button {
								UIPasteboard.general.setValue(
									helpViewModel.version,
									forPasteboardType: UTType.plainText.identifier
								)
								ToastViewModel.shared.toastMessage = "Success_text_copied_into_clipboard"
							   	ToastViewModel.shared.displayToast = true
							} label: {
								HStack {
									Image("app-store-logo")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.orangeMain500)
										.frame(width: 30, height: 30)
									
									VStack {
										Text("help_troubleshooting_app_version_title")
											.default_text_style_700(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
											.multilineTextAlignment(.leading)
										
										Text(helpViewModel.version)
											.default_text_style(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
											.multilineTextAlignment(.leading)
									}
									.padding(.horizontal, 5)
									
									Image("copy")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.grayMain2c600)
										.frame(width: 25, height: 25)
								}
							}
							Button {
								UIPasteboard.general.setValue(
									helpViewModel.sdkVersion,
									forPasteboardType: UTType.plainText.identifier
								)
								ToastViewModel.shared.toastMessage = "Success_text_copied_into_clipboard"
								ToastViewModel.shared.displayToast = true
							} label: {
								HStack {
									Image("package")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.orangeMain500)
										.frame(width: 30, height: 30)
									
									VStack {
										Text("help_troubleshooting_sdk_version_title")
											.default_text_style_700(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
											.multilineTextAlignment(.leading)
										
										Text(helpViewModel.sdkVersion)
											.default_text_style(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
											.multilineTextAlignment(.leading)
									}
									.padding(.horizontal, 5)
									
									Image("copy")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.grayMain2c600)
										.frame(width: 25, height: 25)
								}
							}
							Button {
							} label: {
								HStack {
									Image("fire")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.orangeMain500)
										.frame(width: 30, height: 30)
									
									VStack {
										Text("help_troubleshooting_firebase_project_title")
											.default_text_style_700(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
											.multilineTextAlignment(.leading)
										
										Text(helpViewModel.firebaseProjectId)
											.default_text_style(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
											.multilineTextAlignment(.leading)
									}
									.padding(.horizontal, 5)
								}
							}
						}
						.padding(.vertical, 20)
						.padding(.horizontal, 20)
						.background(Color.gray100)
					}
				}
				.background(Color.gray100)
			}
			.background(Color.gray100)
			
			if helpViewModel.logsUploadInProgress {
				PopupLoadingView()
					.background(.black.opacity(0.65))
			}
		}
		.navigationTitle("")
		.navigationBarHidden(true)
		.onChange(of: helpViewModel.logText) { _ in
			showShareSheet = true
		}
		.sheet(isPresented: $showShareSheet) {
			ShareAnySheet(items: [helpViewModel.logText])
				.edgesIgnoringSafeArea(.bottom)
		}
	}
}

struct ShareAnySheet: UIViewControllerRepresentable {
	var items: [Any] // The content to share
	
	func makeUIViewController(context: Context) -> UIActivityViewController {
		UIActivityViewController(activityItems: items, applicationActivities: nil)
	}
	
	func updateUIViewController(_ uiViewController: UIActivityViewController, context: Context) {
		// No updates needed
	}
}
