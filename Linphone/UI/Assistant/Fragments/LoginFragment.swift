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

struct LoginFragment: View {
	
	@ObservedObject private var coreContext = CoreContext.shared
	@ObservedObject var accountLoginViewModel: AccountLoginViewModel
	@ObservedObject var sharedMainViewModel: SharedMainViewModel
	
	@State private var isSecured: Bool = true
	
	@FocusState var isNameFocused: Bool
	@FocusState var isPasswordFocused: Bool
	
	@State private var isShowPopup = false
	
	@State private var linkActive = ""
	
	@State private var isLinkQRActive = false
	@State private var isLinkSIPActive = false
	@State private var isLinkREGActive = false
	
	var body: some View {
		NavigationView {
			ZStack {
				GeometryReader { geometry in
					ScrollView(.vertical) {
						VStack {
							ZStack {
								Image("mountain")
									.resizable()
									.scaledToFill()
									.frame(width: geometry.size.width, height: 100)
									.clipped()
								Text("assistant_account_login")
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
								.background((accountLoginViewModel.username.isEmpty || accountLoginViewModel.passwd.isEmpty) ? Color.orangeMain100 : Color.orangeMain500)
								.cornerRadius(60)
								.disabled(accountLoginViewModel.username.isEmpty || accountLoginViewModel.passwd.isEmpty)
								.padding(.bottom)
								
								HStack {
									Text("[Forgotten password?](https://subscribe.linphone.org/)")
										.underline()
										.tint(Color.grayMain2c600)
										.default_text_style_600(styleSize: 15)
										.foregroundStyle(Color.grayMain2c500)
								}
								.frame(maxWidth: .infinity)
								.padding(.bottom, 30)
								
								HStack {
									VStack {
										Divider()
									}
									Text(" or ")
										.default_text_style(styleSize: 15)
										.foregroundStyle(Color.grayMain2c500)
									VStack {
										Divider()
									}
								}
								.padding(.bottom, 10)
								
								NavigationLink(isActive: $isLinkQRActive, destination: {
									QrCodeScannerFragment()
								}, label: {
									HStack {
										Image("qr-code")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.orangeMain500)
											.frame(width: 20, height: 20)
										
										Text("Scan QR code")
											.default_text_style_orange_600(styleSize: 20)
											.frame(height: 35)
									}
									.frame(maxWidth: .infinity)
									
								})
								.disabled(!sharedMainViewModel.generalTermsAccepted)
								.padding(.horizontal, 20)
								.padding(.vertical, 10)
								.cornerRadius(60)
								.overlay(
									RoundedRectangle(cornerRadius: 60)
										.inset(by: 0.5)
										.stroke(Color.orangeMain500, lineWidth: 1)
								)
								.padding(.bottom)
								.simultaneousGesture(
									TapGesture().onEnded {
										self.linkActive = "QR"
										if !sharedMainViewModel.generalTermsAccepted {
											withAnimation {
												self.isShowPopup.toggle()
											}
										} else {
											self.isLinkQRActive = true
										}
									}
								)
								
								NavigationLink(isActive: $isLinkSIPActive, destination: {
									ThirdPartySipAccountWarningFragment(sharedMainViewModel: sharedMainViewModel, accountLoginViewModel: accountLoginViewModel)
								}, label: {
									Text("Use SIP Account")
										.default_text_style_orange_600(styleSize: 20)
										.frame(height: 35)
										.frame(maxWidth: .infinity)
									
								})
								.disabled(!sharedMainViewModel.generalTermsAccepted)
								.padding(.horizontal, 20)
								.padding(.vertical, 10)
								.cornerRadius(60)
								.overlay(
									RoundedRectangle(cornerRadius: 60)
										.inset(by: 0.5)
										.stroke(Color.orangeMain500, lineWidth: 1)
								)
								.padding(.bottom)
								.simultaneousGesture(
									TapGesture().onEnded {
										self.linkActive = "SIP"
										if !sharedMainViewModel.generalTermsAccepted {
											withAnimation {
												self.isShowPopup.toggle()
											}
										} else {
											self.isLinkSIPActive = true
										}
									}
								)
								
								Spacer()
								
								HStack(alignment: .center) {
									
									Spacer()
									
									Text("Not account yet?")
										.default_text_style(styleSize: 15)
										.foregroundStyle(Color.grayMain2c700)
										.padding(.horizontal, 10)
									
									NavigationLink(destination: RegisterFragment(), isActive: $isLinkREGActive, label: {Text("Register")
											.default_text_style_orange_600(styleSize: 20)
											.frame(height: 35)
									})
									.disabled(!sharedMainViewModel.generalTermsAccepted)
									.padding(.horizontal, 20)
									.padding(.vertical, 10)
									.cornerRadius(60)
									.overlay(
										RoundedRectangle(cornerRadius: 60)
											.inset(by: 0.5)
											.stroke(Color.orangeMain500, lineWidth: 1)
									)
									.padding(.horizontal, 10)
									.simultaneousGesture(
										TapGesture().onEnded {
											self.linkActive = "REG"
											if !sharedMainViewModel.generalTermsAccepted {
												withAnimation {
													self.isShowPopup.toggle()
												}
											} else {
												self.isLinkREGActive = true
											}
										}
									)
									
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
						let contentPopup1 = Text("En continuant, vous acceptez ces conditions, ")
						let contentPopup2 = Text("[notre politique de confidentialité](https://linphone.org/privacy-policy)").underline()
						let contentPopup3 = Text(" et ")
						let contentPopup4 = Text("[nos conditions d’utilisation](https://linphone.org/general-terms)").underline()
						let contentPopup5 = Text(".")
						PopupView(sharedMainViewModel: sharedMainViewModel, isShowPopup: $isShowPopup,
								  title: Text("Conditions de service"),
								  content: contentPopup1 + contentPopup2 + contentPopup3 + contentPopup4 + contentPopup5,
								  titleFirstButton: Text("Deny all"),
								  actionFirstButton: {self.isShowPopup.toggle()},
								  titleSecondButton: Text("Accept all"),
								  actionSecondButton: {acceptGeneralTerms()})
						.background(.black.opacity(0.65))
						.onTapGesture {
							self.isShowPopup.toggle()
						}
					}
				}
				.onAppear {
					sharedMainViewModel.changeDisplayProfileMode()
				}
				
				if coreContext.loggingInProgress {
					PopupLoadingView(sharedMainViewModel: sharedMainViewModel)
						.background(.black.opacity(0.65))
				}
				
				if !coreContext.loggingInProgress && !coreContext.loggedIn {
					ZStack {
						
					}.onAppear {
						self.accountLoginViewModel.unregister()
						self.accountLoginViewModel.delete()
					}
				}
			}
		}
		.navigationViewStyle(StackNavigationViewStyle())
	}
	
	func acceptGeneralTerms() {
		sharedMainViewModel.changeGeneralTerms()
		self.isShowPopup.toggle()
		switch linkActive {
		case "QR":
			self.isLinkQRActive = true
		case "SIP":
			self.isLinkSIPActive = true
		case "REG":
			self.isLinkREGActive = true
		default:
			print("Link Not Active")
		}
	}
}

#Preview {
	LoginFragment(accountLoginViewModel: AccountLoginViewModel(), sharedMainViewModel: SharedMainViewModel())
}
