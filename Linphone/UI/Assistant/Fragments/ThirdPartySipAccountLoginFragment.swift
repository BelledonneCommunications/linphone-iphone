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

struct ThirdPartySipAccountLoginFragment: View {
	
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	@ObservedObject private var coreContext = CoreContext.shared
	@ObservedObject var accountLoginViewModel: AccountLoginViewModel
	
	@Environment(\.dismiss) var dismiss
	
	@State private var isSecured: Bool = true
	
	@FocusState var isNameFocused: Bool
	@FocusState var isPasswordFocused: Bool
	@FocusState var isDomainFocused: Bool
	@FocusState var isDisplayNameFocused: Bool
	
	var body: some View {
		GeometryReader { geometry in
			ScrollView(.vertical) {
				VStack {
					ZStack {
						Image("mountain")
							.resizable()
							.scaledToFill()
							.frame(width: geometry.size.width, height: 100)
							.clipped()
						
						VStack(alignment: .leading) {
							HStack {
								Image("caret-left")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
									.padding(.top, -75)
									.padding(.leading, -10)
									.onTapGesture {
										withAnimation {
											accountLoginViewModel.domain = "sip.linphone.org"
											accountLoginViewModel.transportType = "TLS"
											dismiss()
										}
									}
								
								Spacer()
							}
							.padding(.leading)
						}
						.frame(width: geometry.size.width)
						
						Text("Use a SIP account")
							.default_text_style_white_800(styleSize: 20)
							.padding(.top, 20)
					}
					.padding(.top, 35)
					.padding(.bottom, 10)
					
					VStack(alignment: .leading) {
						Text(String(localized: "username")+"*")
							.default_text_style_700(styleSize: 15)
							.padding(.bottom, -5)
						
						TextField("username", text: $accountLoginViewModel.username)
							.default_text_style(styleSize: 15)
							.disableAutocorrection(true)
							.autocapitalization(.none)
							.disabled(coreContext.loggedIn)
							.frame(height: 25)
							.padding(.horizontal, 20)
							.padding(.vertical, 15)
							.cornerRadius(60)
							.overlay(
								RoundedRectangle(cornerRadius: 60)
									.inset(by: 0.5)
									.stroke(isNameFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
							)
							.padding(.bottom)
							.focused($isNameFocused)
						
						Text(String(localized: "password")+"*")
							.default_text_style_700(styleSize: 15)
							.padding(.bottom, -5)
						
						ZStack(alignment: .trailing) {
							Group {
								if isSecured {
									SecureField("password", text: $accountLoginViewModel.passwd)
										.default_text_style(styleSize: 15)
										.frame(height: 25)
										.focused($isPasswordFocused)
								} else {
									TextField("password", text: $accountLoginViewModel.passwd)
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
						.disabled(coreContext.loggedIn)
						.padding(.horizontal, 20)
						.padding(.vertical, 15)
						.cornerRadius(60)
						.overlay(
							RoundedRectangle(cornerRadius: 60)
								.inset(by: 0.5)
								.stroke(isPasswordFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
						)
						.padding(.bottom)
						
						Text(String(localized: "Domain")+"*")
							.default_text_style_700(styleSize: 15)
							.padding(.bottom, -5)
						
						TextField("sip.linphone.org", text: $accountLoginViewModel.domain)
							.default_text_style(styleSize: 15)
							.disableAutocorrection(true)
							.autocapitalization(.none)
							.disabled(coreContext.loggedIn)
							.frame(height: 25)
							.padding(.horizontal, 20)
							.padding(.vertical, 15)
							.cornerRadius(60)
							.overlay(
								RoundedRectangle(cornerRadius: 60)
									.inset(by: 0.5)
									.stroke(isDomainFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
							)
							.padding(.bottom)
							.focused($isDomainFocused)
						
						Text(String(localized: "Display Name"))
							.default_text_style_700(styleSize: 15)
							.padding(.bottom, -5)
						
						TextField("Display Name", text: $accountLoginViewModel.displayName)
							.default_text_style(styleSize: 15)
							.disableAutocorrection(true)
							.autocapitalization(.none)
							.disabled(coreContext.loggedIn)
							.frame(height: 25)
							.padding(.horizontal, 20)
							.padding(.vertical, 15)
							.cornerRadius(60)
							.overlay(
								RoundedRectangle(cornerRadius: 60)
									.inset(by: 0.5)
									.stroke(isDisplayNameFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
							)
							.padding(.bottom)
							.focused($isDisplayNameFocused)
						
						Text(String(localized: "Transport"))
							.default_text_style_700(styleSize: 15)
							.padding(.bottom, -5)
						
						Menu {
							Button("TLS") {accountLoginViewModel.transportType = "TLS"}
							Button("TCP") {accountLoginViewModel.transportType = "TCP"}
							Button("UDP") {accountLoginViewModel.transportType = "UDP"}
						} label: {
							Text(accountLoginViewModel.transportType)
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
						.padding(.bottom)
						
						Spacer()
						
						Button(action: {
							self.accountLoginViewModel.login()
						}, label: {
							Text(coreContext.loggedIn ? "Log out" : "assistant_account_login")
								.default_text_style_white_600(styleSize: 20)
								.frame(height: 35)
								.frame(maxWidth: .infinity)
						})
						.padding(.horizontal, 20)
						.padding(.vertical, 10)
						.background(
							(accountLoginViewModel.username.isEmpty || accountLoginViewModel.passwd.isEmpty || accountLoginViewModel.domain.isEmpty)
							? Color.orangeMain100
							: Color.orangeMain500)
						.cornerRadius(60)
						.disabled(accountLoginViewModel.username.isEmpty || accountLoginViewModel.passwd.isEmpty || accountLoginViewModel.domain.isEmpty)
						.padding(.bottom, geometry.safeAreaInsets.bottom.isEqual(to: 0.0) ? 20 : 0)
					}
					.frame(maxWidth: sharedMainViewModel.maxWidth)
					.padding(.horizontal, 20)
				}
				.frame(minHeight: geometry.size.height)
			}
		}
		.navigationBarHidden(true)
	}
}

#Preview {
	ThirdPartySipAccountLoginFragment(accountLoginViewModel: AccountLoginViewModel())
}
