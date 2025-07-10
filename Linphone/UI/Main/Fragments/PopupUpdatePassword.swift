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

struct PopupUpdatePassword: View {
	
	@ObservedObject var sharedMainViewModel = SharedMainViewModel.shared
	
	@Binding var isShowUpdatePasswordPopup: Bool
	@Binding var passwordUpdateAddress: String
	
	@State private var passwordPopupText: String = ""
	@State private var isSecured: Bool = true
	
	@FocusState var isPasswordFocused: Bool
	
	var body: some View {
		GeometryReader { geometry in
			VStack(alignment: .leading) {
				Text("account_settings_dialog_invalid_password_title")
					.default_text_style_800(styleSize: 16)
					.frame(alignment: .leading)
					.padding(.bottom, 2)
				
				Text(String(format: String(localized: "account_settings_dialog_invalid_password_message"), passwordUpdateAddress))
					.default_text_style(styleSize: 15)
					.padding(.bottom, 20)
				
				ZStack(alignment: .trailing) {
					Group {
						if isSecured {
							SecureField("account_settings_dialog_invalid_password_hint", text: $passwordPopupText)
								.default_text_style(styleSize: 15)
								.frame(height: 25)
								.focused($isPasswordFocused)
						} else {
							TextField("account_settings_dialog_invalid_password_hint", text: $passwordPopupText)
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
				.onTapGesture {
					isPasswordFocused = true
				}
				
				Button(action: {
					isShowUpdatePasswordPopup = false
				}, label: {
					Text("dialog_cancel")
						.default_text_style_orange_600(styleSize: 20)
						.frame(height: 35)
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
				.padding(.bottom, 10)
				
				Button(action: {
					updateAuthInfo()
					isShowUpdatePasswordPopup = false
				}, label: {
					Text("dialog_ok")
						.default_text_style_white_600(styleSize: 20)
						.frame(height: 35)
						.frame(maxWidth: .infinity)
				})
				.padding(.horizontal, 20)
				.padding(.vertical, 10)
				.background(passwordPopupText.isEmpty ? Color.orangeMain100 : Color.orangeMain500)
				.cornerRadius(60)
				.disabled(passwordPopupText.isEmpty)
			}
			.padding(.horizontal, 20)
			.padding(.vertical, 20)
			.background(.white)
			.cornerRadius(20)
			.padding(.horizontal)
			.frame(maxHeight: .infinity)
			.shadow(color: Color.orangeMain500, radius: 0, x: 0, y: 2)
			.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
			.position(x: geometry.size.width / 2, y: geometry.size.height / 2)
			.onTapGesture {}
		}
	}
	
	/*
	func setNewPassword() {
		CoreContext.shared.doOnCoreQueue { core in
			Log.info("[SetNewPassword] ---- \(core.defaultAccount?.params?.identityAddress?.asStringUriOnly() ?? "No account found") \(passwordUpdateAddress)")
			
			if let account = core.accountList.first { $0.params?.identityAddress?.asStringUriOnly() == passwordUpdateAddress } {
				let authInfo = account.findAuthInfo()
				if (authInfo != nil) {
					Log.info(
						"[SetNewPassword] Updating password for username \(authInfo!.username) using auth info \(authInfo!)"
					)
					authInfo!.password = passwordPopupText
					core.addAuthInfo(info: authInfo!)
					core.refreshRegisters()
				} else {
					Log.warn(
						"[SetNewPassword] Failed to find auth info for account \(account.params?.identityAddress?.asStringUriOnly())"
					)
				}
			}
		}
	}
	*/
	
	func updateAuthInfo() {
		CoreContext.shared.doOnCoreQueue { core in
			Log.info("[SetNewPassword] ---- \(core.defaultAccount?.params?.identityAddress?.asStringUriOnly() ?? "No account found") \(passwordUpdateAddress)")
			
			if let account = core.accountList.first { $0.params?.identityAddress?.asStringUriOnly() == passwordUpdateAddress } {
				let authInfo = CoreContext.shared.digestAuthInfoPendingPasswordUpdate
				if (authInfo != nil) {
					Log.info(
						"[SetNewPassword] Updating password for username \(authInfo!.username) using auth info \(authInfo!)"
					)
					authInfo!.password = passwordPopupText
					core.addAuthInfo(info: authInfo!)
					CoreContext.shared.digestAuthInfoPendingPasswordUpdate = nil
					core.refreshRegisters()
				} else {
					Log.warn(
						"[SetNewPassword] Failed to find auth info for account \(account.params?.identityAddress?.asStringUriOnly())"
					)
				}
			}
		}
	}
}

#Preview {
	PopupUpdatePassword(isShowUpdatePasswordPopup: .constant(true), passwordUpdateAddress: .constant("example@sip.linphone.org"))
}
