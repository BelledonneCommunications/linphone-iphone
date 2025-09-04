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
	
	@StateObject private var accountLoginViewModel = AccountLoginViewModel()
	
	@State private var isSecured: Bool = true
	
	@FocusState var isNameFocused: Bool
	@FocusState var isPasswordFocused: Bool
	
	@State private var isShowPopup = false
	
	@State private var linkActive = ""
	
	@State private var isLinkSIPActive = false
	@State private var isLinkREGActive = false
	
	@State var isShowHelpFragment = false
	
	var isShowBack = false
	
	var onBackPressed: (() -> Void)?

	var body: some View {
		NavigationView {
			ZStack {
				GeometryReader { geometry in
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
					
					if self.isShowPopup {
						let generalTerms = String(format: "[%@](%@)", String(localized: "assistant_dialog_general_terms_label"), "https://www.linphone.org/en/terms-of-use/")
						let privacyPolicy = String(format: "[%@](%@)", String(localized: "assistant_dialog_privacy_policy_label"), "https://linphone.org/en/privacy-policy")
						let splitMsg = String(localized: "assistant_dialog_general_terms_and_privacy_policy_message").components(separatedBy: "%@")
						if splitMsg.count == 3 { // We expect form of  STRING %A STRING %@ STRING
							let contentPopup1 = Text(.init(splitMsg[0]))
							let contentPopup2 = Text(.init(generalTerms)).underline()
							let contentPopup3 = Text(.init(splitMsg[1]))
							let contentPopup4 = Text(.init(privacyPolicy)).underline()
							let contentPopup5 = Text(.init(splitMsg[2]))
							PopupView(isShowPopup: $isShowPopup,
									  title: Text("assistant_dialog_general_terms_and_privacy_policy_title"),
									  content: contentPopup1 + contentPopup2 + contentPopup3 + contentPopup4 + contentPopup5,
									  titleFirstButton: Text("dialog_deny"),
									  actionFirstButton: {self.isShowPopup.toggle()},
									  titleSecondButton: Text("dialog_accept"),
									  actionSecondButton: {acceptGeneralTerms()})
							.background(.black.opacity(0.65))
							.onTapGesture {
								self.isShowPopup.toggle()
							}
						} else {  // backup just in case
							PopupView(isShowPopup: $isShowPopup,
									  title: Text("assistant_dialog_general_terms_and_privacy_policy_title"),
									  content: Text(.init(String(format: String(localized: "assistant_dialog_general_terms_and_privacy_policy_message"), generalTerms, privacyPolicy))),
									  titleFirstButton: Text("dialog_deny"),
									  actionFirstButton: {self.isShowPopup.toggle()},
									  titleSecondButton: Text("dialog_accept"),
									  actionSecondButton: {acceptGeneralTerms()})
							.background(.black.opacity(0.65))
							.onTapGesture {
								self.isShowPopup.toggle()
							}
						}
					}
				}
				
				if isShowHelpFragment {
					HelpFragment(
						isShowHelpFragment: $isShowHelpFragment
					)
					.transition(.move(edge: .trailing))
					.zIndex(3)
				}
				
				if coreContext.loggingInProgress {
					PopupLoadingView()
						.background(.black.opacity(0.65))
				}
			}
			.navigationTitle("")
			.navigationBarHidden(true)
			.edgesIgnoringSafeArea(.bottom)
			.edgesIgnoringSafeArea(.horizontal)
		}
		.navigationViewStyle(StackNavigationViewStyle())
	}
	
	func innerScrollView(geometry: GeometryProxy) -> some View {
		VStack {
			ZStack {
				HStack {
					if isShowBack {
						Image("caret-left")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c500)
							.frame(width: 25, height: 25)
							.padding(.all, 10)
							.onTapGesture {
								withAnimation {
									onBackPressed?()
								}
							}
					} else {
						Color.clear
							.frame(width: 25, height: 25)
							.padding(.all, 10)
					}

					Spacer()
					
					Button {
						withAnimation {
							isShowHelpFragment = true
						}
					} label: {
						HStack {
							Image("question")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c500)
								.frame(width: 20, height: 20)
							
							Text("help_title")
								.foregroundStyle(Color.grayMain2c500)
								.default_text_style_orange_600(styleSize: 15)
								.frame(height: 35)
						}
						.padding(.horizontal, 20)
					}
				}

				Text("assistant_account_login")
					.default_text_style_800(styleSize: 20)
			}
			.frame(width: geometry.size.width)
			.padding(.top, 10)
			.padding(.bottom, 20)
			
			VStack(alignment: .leading) {
				Text(String(localized: "username")+"*")
					.default_text_style_700(styleSize: 15)
					.padding(.bottom, -5)
				
				TextField("username", text: $accountLoginViewModel.username)
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
					SharedMainViewModel.shared.changeDisplayProfileMode()
					self.accountLoginViewModel.login()
					coreContext.loggingInProgress = true
				}, label: {
					Text("assistant_account_login")
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
					Text(.init(String(format: ("[%@](%@)"), String(localized: "assistant_forgotten_password"), "https://subscribe.linphone.org/")))
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
					Text("or")
						.default_text_style(styleSize: 15)
						.foregroundStyle(Color.grayMain2c500)
					VStack {
						Divider()
					}
				}
				.padding(.bottom, 10)
				
				NavigationLink(destination: {
					QrCodeScannerFragment()
				}, label: {
					HStack {
						Image("qr-code")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.orangeMain500)
							.frame(width: 20, height: 20)
						
						Text("assistant_scan_qr_code")
							.default_text_style_orange_600(styleSize: 20)
							.frame(height: 35)
					}
					.frame(maxWidth: .infinity)
					
				})
				.padding(.horizontal, 20)
				.padding(.vertical, 10)
				.cornerRadius(60)
				.overlay(
					RoundedRectangle(cornerRadius: 60)
						.inset(by: 0.5)
						.stroke(Color.orangeMain500, lineWidth: 1)
				)
				.padding(.bottom)
				
				NavigationLink(isActive: $isLinkSIPActive, destination: {
					ThirdPartySipAccountWarningFragment(accountLoginViewModel: accountLoginViewModel)
				}, label: {
					Text("assistant_login_third_party_sip_account")
						.default_text_style_orange_600(styleSize: 20)
						.frame(height: 35)
						.frame(maxWidth: .infinity)
					
				})
				.disabled(!SharedMainViewModel.shared.generalTermsAccepted)
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
						if !SharedMainViewModel.shared.generalTermsAccepted {
							withAnimation {
								self.isShowPopup.toggle()
							}
						} else {
							self.isLinkSIPActive = true
						}
					}
				)
			}
			.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
			.padding(.horizontal, 20)
			
			Spacer()
			
			HStack(alignment: .center) {
				
				Spacer()
				
				Text("assistant_no_account_yet")
					.default_text_style(styleSize: 15)
					.foregroundStyle(Color.grayMain2c700)
					.padding(.horizontal, 10)
				
				NavigationLink(destination: RegisterFragment(registerViewModel: RegisterViewModel()), isActive: $isLinkREGActive, label: { Text("assistant_account_register")
						.default_text_style_white_600(styleSize: 20)
						.frame(height: 35)
				})
				.disabled(!SharedMainViewModel.shared.generalTermsAccepted)
				.padding(.horizontal, 20)
				.padding(.vertical, 10)
				.background(Color.orangeMain500)
				.cornerRadius(60)
				.padding(.horizontal, 10)
				.simultaneousGesture(
					TapGesture().onEnded {
						self.linkActive = "REG"
						if !SharedMainViewModel.shared.generalTermsAccepted {
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
			
			Image("mountain2")
				.resizable()
				.scaledToFill()
				.frame(width: geometry.size.width, height: 60)
				.clipped()
		}
		.frame(minHeight: geometry.size.height)
	}
	
	func acceptGeneralTerms() {
		SharedMainViewModel.shared.changeGeneralTerms()
		self.isShowPopup.toggle()
		switch linkActive {
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
	LoginFragment()
}
