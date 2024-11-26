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

// swiftlint:disable line_length

import SwiftUI

struct RegisterFragment: View {
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	@ObservedObject var registerViewModel: RegisterViewModel
	
	@Environment(\.dismiss) var dismiss
	
	@State private var isSecured: Bool = true
	
	@FocusState var isNameFocused: Bool
	@FocusState var isPhoneNumberFocused: Bool
	@FocusState var isPasswordFocused: Bool
	
	@State private var isShowPopup = false
	
	var body: some View {
		NavigationView {
			GeometryReader { geometry in
				ZStack {
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
													dismiss()
												}
											}
										
										Spacer()
									}
									.padding(.leading)
								}
								.frame(width: geometry.size.width)
								
								Text("assistant_account_register")
									.default_text_style_white_800(styleSize: 20)
									.padding(.top, 20)
							}
							.padding(.top, 35)
							.padding(.bottom, 10)
							
							VStack(alignment: .leading) {
								Text(String(localized: "username")+"*")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								TextField("username", text: $registerViewModel.username)
									.default_text_style(styleSize: 15)
									.disableAutocorrection(true)
									.autocapitalization(.none)
									.frame(height: 25)
									.padding(.horizontal, 20)
									.padding(.vertical, 15)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(isNameFocused ? Color.orangeMain500 : (!registerViewModel.usernameError.isEmpty ? Color.redDanger500 : Color.gray200), lineWidth: 1)
									)
									.focused($isNameFocused)
									.onChange(of: registerViewModel.username) { _ in
										if !registerViewModel.usernameError.isEmpty {
											registerViewModel.usernameError = ""
										}
									}
								
								Text(registerViewModel.usernameError)
									.foregroundStyle(Color.redDanger500)
									.default_text_style_600(styleSize: 15)
									.padding(.bottom)
								
								Text(String(localized: "Phone number")+"*")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								HStack {
									Menu {
										Picker("", selection: $registerViewModel.dialPlanValueSelected) {
											ForEach(Array(registerViewModel.dialPlansLabelList.enumerated()), id: \.offset) { index, dialPlan in
												Text(dialPlan).tag(registerViewModel.dialPlansShortLabelList[index])
											}
										}
									} label: {
										HStack {
											Text(registerViewModel.dialPlanValueSelected)
											
											Image("caret-down")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(Color.blue)
												.frame(width: 15, height: 15)
										}
									}
									.padding(.trailing, 5)
									
									Divider()
									
									TextField("Phone number", text: $registerViewModel.phoneNumber)
										.default_text_style(styleSize: 15)
										.disableAutocorrection(true)
										.autocapitalization(.none)
										.padding(.leading, 5)
										.keyboardType(.numberPad)
										.onChange(of: registerViewModel.phoneNumber) { _ in
											if !registerViewModel.phoneNumberError.isEmpty {
												registerViewModel.phoneNumberError = ""
											}
										}
								}
								.frame(height: 25)
								.padding(.horizontal, 20)
								.padding(.vertical, 15)
								.cornerRadius(60)
								.overlay(
									RoundedRectangle(cornerRadius: 60)
										.inset(by: 0.5)
										.stroke(isPhoneNumberFocused ? Color.orangeMain500 : (!registerViewModel.phoneNumberError.isEmpty ? Color.redDanger500 : Color.gray200), lineWidth: 1)
								)
								.focused($isPhoneNumberFocused)
								
								Text(registerViewModel.phoneNumberError)
									.foregroundStyle(Color.redDanger500)
									.default_text_style_600(styleSize: 15)
									.padding(.bottom)
								
								Text(String(localized: "password")+"*")
									.default_text_style_700(styleSize: 15)
									.padding(.bottom, -5)
								
								ZStack(alignment: .trailing) {
									Group {
										if isSecured {
											SecureField("password", text: $registerViewModel.passwd)
												.default_text_style(styleSize: 15)
												.frame(height: 25)
												.focused($isPasswordFocused)
												.onChange(of: registerViewModel.passwd) { _ in
													if !registerViewModel.passwordError.isEmpty {
														registerViewModel.passwordError = ""
													}
												}
										} else {
											TextField("password", text: $registerViewModel.passwd)
												.default_text_style(styleSize: 15)
												.disableAutocorrection(true)
												.autocapitalization(.none)
												.frame(height: 25)
												.focused($isPasswordFocused)
												.onChange(of: registerViewModel.passwd) { _ in
													if !registerViewModel.passwordError.isEmpty {
														registerViewModel.passwordError = ""
													}
												}
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
										.stroke(isPasswordFocused ? Color.orangeMain500 : (!registerViewModel.passwordError.isEmpty ? Color.redDanger500 : Color.gray200), lineWidth: 1)
								)
								
								Text(registerViewModel.passwordError)
									.foregroundStyle(Color.redDanger500)
									.default_text_style_600(styleSize: 15)
									.padding(.bottom)
								
								NavigationLink(isActive: $registerViewModel.isLinkActive, destination: {
									RegisterCodeConfirmationFragment(registerViewModel: registerViewModel)
								}, label: {
									Text("assistant_account_create")
										.default_text_style_white_600(styleSize: 20)
										.frame(height: 35)
										.frame(maxWidth: .infinity)
									
								})
								.padding(.horizontal, 20)
								.padding(.vertical, 10)
								.background((registerViewModel.username.isEmpty || registerViewModel.phoneNumber.isEmpty || registerViewModel.passwd.isEmpty) ? Color.orangeMain100 : Color.orangeMain500)
								.cornerRadius(60)
								.disabled(!registerViewModel.isLinkActive)
								.padding(.bottom)
								.simultaneousGesture(
									TapGesture().onEnded {
										if !(registerViewModel.username.isEmpty || registerViewModel.phoneNumber.isEmpty || registerViewModel.passwd.isEmpty) {
											withAnimation {
												self.isShowPopup = true
											}
										}
									}
								)
								
								Spacer()
								
								Text("assistant_create_account_using_email_on_our_web_platform")
									.default_text_style(styleSize: 15)
									.foregroundStyle(Color.grayMain2c700)
									.padding(.horizontal, 10)
									.frame(maxWidth: .infinity, alignment: .center)
								
								Button(action: {
									UIApplication.shared.open(URL(string: "https://subscribe.linphone.org/register/email")!)
								}, label: {
									Text("assistant_web_platform_link")
										.default_text_style_orange_600(styleSize: 15)
										.frame(height: 35)
								})
								.padding(.horizontal, 15)
								.padding(.vertical, 5)
								.cornerRadius(60)
								.overlay(
									RoundedRectangle(cornerRadius: 60)
										.inset(by: 0.5)
										.stroke(Color.orangeMain500, lineWidth: 1)
								)
								.padding(.bottom)
								.frame(maxWidth: .infinity)
								
								Spacer()
								
								HStack(alignment: .center) {
									
									Spacer()
									
									Text("assistant_already_have_an_account")
										.default_text_style(styleSize: 15)
										.foregroundStyle(Color.grayMain2c700)
										.padding(.horizontal, 10)
									
									Button(action: {
										dismiss()
									}, label: {
										Text("assistant_account_login")
											.default_text_style_white_600(styleSize: 20)
											.frame(height: 35)
									})
									.padding(.horizontal, 20)
									.padding(.vertical, 10)
									.background(Color.orangeMain500)
									.cornerRadius(60)
									.padding(.horizontal, 10)
									
									Spacer()
								}
								.padding(.bottom)
							}
							.frame(maxWidth: sharedMainViewModel.maxWidth)
							.padding(.horizontal, 20)
						}
						.frame(minHeight: geometry.size.height)
					}
					
					if self.isShowPopup {
						let titlePopup = Text("assistant_dialog_confirm_phone_number_title")
						let contentPopup = Text(String(format: NSLocalizedString("assistant_dialog_confirm_phone_number_message", comment: ""), registerViewModel.phoneNumber))
						
						PopupView(
							isShowPopup: $isShowPopup,
							title: titlePopup,
							content: contentPopup,
							titleFirstButton: Text("Cancel"),
							actionFirstButton: {
								self.isShowPopup = false
							},
							titleSecondButton: Text("Continue"),
							actionSecondButton: {
								self.isShowPopup = false
								registerViewModel.createInProgress = true
								registerViewModel.startAccountCreation()
								registerViewModel.phoneNumberConfirmedByUser()
							}
						)
						.background(.black.opacity(0.65))
						.onTapGesture {
							self.isShowPopup = false
						}
					}
					
					if registerViewModel.createInProgress {
						PopupLoadingView()
							.background(.black.opacity(0.65))
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

#Preview {
	RegisterFragment(registerViewModel: RegisterViewModel())
}

// swiftlint:enable line_length
