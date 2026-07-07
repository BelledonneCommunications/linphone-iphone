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

struct RecoverAccountFragment: View {
	@ObservedObject var sharedMainViewModel = SharedMainViewModel.shared
	
	@StateObject private var registerViewModel = RegisterViewModel()
	
	@StateObject private var keyboard = KeyboardResponder()
	
	@Environment(\.dismiss) var dismiss
	var body: some View {
		NavigationView {
			GeometryReader { geometry in
				ZStack {
					if #available(iOS 16.4, *) {
						ScrollView(.vertical) {
							innerScrollView(geometry: geometry)
						}
						.scrollBounceBehavior(.basedOnSize)
					} else {
						ScrollView(.vertical) {
							innerScrollView(geometry: geometry)
						}
					}
					
					if registerViewModel.isRecoveringPhoneNumberAccount {
						PopupLoadingView()
							.background(.black.opacity(0.65))
					}
				}
			}
			.navigationTitle("")
			.navigationBarHidden(true)
			.edgesIgnoringSafeArea(.bottom)
			.edgesIgnoringSafeArea(.horizontal)
		}
		.navigationViewStyle(StackNavigationViewStyle())
		.navigationTitle("")
		.navigationBarHidden(true)
	}
	
	func innerScrollView(geometry: GeometryProxy) -> some View {
		VStack {
			ZStack {
				HStack {
					Image("caret-left")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.grayMain2c500)
						.frame(width: 25, height: 25)
						.padding(.all, 10)
						.onTapGesture {
							withAnimation {
								dismiss()
							}
						}
					Spacer()
				}

				Text("assistant_forgotten_password_title")
					.default_text_style_800(styleSize: 20)
			}
			.frame(width: geometry.size.width)
			.padding(.top, 10)
			.padding(.bottom, 20)
			
			VStack(spacing: 20) {
				HStack(alignment: .center) {
					Image("password")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.grayMain2c600)
						.frame(width: 30, height: 30, alignment: .leading)
				}
				.padding(16)
				.background(Color.grayMain2c200)
				.cornerRadius(40)
				.padding(.bottom, 10)
				
				Text("assistant_forgotten_password_subtitle")
					.default_text_style_700(styleSize: 15)
					.foregroundStyle(Color.grayMain2c700)
					.padding(.horizontal, 10)
					.frame(maxWidth: .infinity, alignment: .center)
				
				Text("assistant_forgotten_password_message")
					.default_text_style(styleSize: 15)
					.foregroundStyle(Color.grayMain2c700)
					.padding(.horizontal, 10)
					.frame(maxWidth: .infinity, alignment: .center)
					.padding(.bottom, 10)
				
				Button(action: {
					self.registerViewModel.recoverEmailAccount()
				}, label: {
					Text("assistant_recover_email_account_label")
						.default_text_style_white_600(styleSize: 20)
						.frame(height: 35)
						.frame(maxWidth: .infinity, alignment: .center)
				})
				.padding(.horizontal, 20)
				.padding(.vertical, 10)
				.background(Color.orangeMain500)
				.cornerRadius(60)
				.padding(.horizontal, 10)
				
				Button(action: {
					self.registerViewModel.recoverPhoneNumberAccount()
				}, label: {
					Text("assistant_recover_phone_number_account_label")
						.default_text_style_white_600(styleSize: 20)
						.frame(height: 35)
						.frame(maxWidth: .infinity, alignment: .center)
				})
				.padding(.horizontal, 20)
				.padding(.vertical, 10)
				.background(Color.orangeMain500)
				.cornerRadius(60)
				.padding(.horizontal, 10)
			}
			.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
			.padding(.all, 20)
			
			Spacer()
			
			Image("mountain2")
				.resizable()
				.scaledToFill()
				.frame(width: geometry.size.width, height: 60)
				.clipped()
		}
		.frame(minHeight: geometry.size.height)
		.padding(.bottom, keyboard.currentHeight)
	}
}
