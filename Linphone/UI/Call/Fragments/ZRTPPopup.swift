/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
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
import Foundation

// swiftlint:disable:next type_body_length
struct ZRTPPopup: View {
	
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@ObservedObject var callViewModel: CallViewModel
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	var resizeView: CGFloat
	
	var body: some View {
		if callViewModel.isNotVerified {
			alertZRTP
		} else {
			popupZRTP
		}
	}
	
	var popupZRTP: some View {
		GeometryReader { geometry in
			VStack(alignment: .leading) {
				ZStack(alignment: .top, content: {
					HStack {
						Spacer()
						
						VStack {
							Image("security")
								.resizable()
								.frame(width: 20, height: 20, alignment: .leading)
							
							Text("call_dialog_zrtp_validate_trust_title")
								.default_text_style_white_700(styleSize: 16 / resizeView)
						}
						.frame(maxWidth: .infinity)
						
						Spacer()
					}
					.padding(.top, 15)
					.padding(.bottom, 2)
					
					HStack {
						Spacer()
						HStack {
							Text("call_zrtp_sas_validation_skip")
								.underline()
								.tint(.white)
								.default_text_style_white_600(styleSize: 16 / resizeView)
								.foregroundStyle(.white)
						}
						.onTapGesture {
							callViewModel.skipZrtpAuthentication()
							callViewModel.zrtpPopupDisplayed = false
						}
					}
					.padding(.top, 10 / resizeView)
					.padding(.trailing, 15 / resizeView)
				})
				
				VStack(alignment: .center) {
					VStack {
						if idiom != .pad && (orientation == .landscapeLeft
											 || orientation == .landscapeRight
											 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
							HStack {
								Text("call_dialog_zrtp_validate_trust_message")
									.default_text_style(styleSize: 16 / resizeView)
									.multilineTextAlignment(.center)
									.padding(.bottom, 10 / resizeView)
								
								VStack {
									Text("call_dialog_zrtp_validate_trust_local_code_label")
										.default_text_style(styleSize: 16 / resizeView)
										.multilineTextAlignment(.center)
									
									Text(!callViewModel.upperCaseAuthTokenToRead.isEmpty ? callViewModel.upperCaseAuthTokenToRead : "ZZ")
										.default_text_style_700(styleSize: 22 / resizeView)
										.padding(.bottom, 20 / resizeView)
								}
							}
						} else {
							Text(callViewModel.cacheMismatch ? "call_dialog_zrtp_validate_trust_warning_message" : "call_dialog_zrtp_validate_trust_message")
								.default_text_style(styleSize: 16 / resizeView)
								.multilineTextAlignment(.center)
								.padding(.bottom, 10 / resizeView)
							
							Text("call_dialog_zrtp_validate_trust_local_code_label")
								.default_text_style(styleSize: 16 / resizeView)
								.multilineTextAlignment(.center)
							
							Text(!callViewModel.upperCaseAuthTokenToRead.isEmpty ? callViewModel.upperCaseAuthTokenToRead : "ZZ")
								.default_text_style_800(styleSize: 22 / resizeView)
						}
					}
					.padding(.bottom, 5)
					
					VStack {
						Text("call_dialog_zrtp_validate_trust_remote_code_label")
							.default_text_style(styleSize: 16 / resizeView)
							.multilineTextAlignment(.center)
							.padding(.top, 15 / resizeView)
							.padding(.bottom, 10 / resizeView)
						
						if idiom != .pad && (orientation == .landscapeLeft
											 || orientation == .landscapeRight
											 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
							HStack(spacing: 30) {
								HStack(alignment: .center) {
									Text(callViewModel.letters1)
										.default_text_style(styleSize: 24 / resizeView)
										.frame(width: 45 / resizeView, height: 45 / resizeView)
								}
								.padding(10 / resizeView)
								.background(.white)
								.clipShape(Circle())
								.shadow(color: .gray.opacity(0.4), radius: 4)
								.onTapGesture {
									callViewModel.updateZrtpSas(authTokenClicked: callViewModel.letters1)
									callViewModel.zrtpPopupDisplayed = false
								}
								
								HStack(alignment: .center) {
									Text(callViewModel.letters2)
										.default_text_style(styleSize: 24 / resizeView)
										.frame(width: 45 / resizeView, height: 45 / resizeView)
								}
								.padding(10 / resizeView)
								.background(.white)
								.clipShape(Circle())
								.shadow(color: .gray.opacity(0.4), radius: 4)
								.onTapGesture {
									callViewModel.updateZrtpSas(authTokenClicked: callViewModel.letters2)
									callViewModel.zrtpPopupDisplayed = false
								}
								
								HStack(alignment: .center) {
									Text(callViewModel.letters3)
										.default_text_style(styleSize: 24 / resizeView)
										.frame(width: 45 / resizeView, height: 45 / resizeView)
								}
								.padding(10 / resizeView)
								.background(.white)
								.clipShape(Circle())
								.shadow(color: .gray.opacity(0.4), radius: 4)
								.onTapGesture {
									callViewModel.updateZrtpSas(authTokenClicked: callViewModel.letters3)
									callViewModel.zrtpPopupDisplayed = false
								}
								
								HStack(alignment: .center) {
									Text(callViewModel.letters4)
										.default_text_style(styleSize: 24 / resizeView)
										.frame(width: 45 / resizeView, height: 45 / resizeView)
								}
								.padding(10 / resizeView)
								.background(.white)
								.clipShape(Circle())
								.shadow(color: .gray.opacity(0.4), radius: 4)
								.onTapGesture {
									callViewModel.updateZrtpSas(authTokenClicked: callViewModel.letters4)
									callViewModel.zrtpPopupDisplayed = false
								}
							}
							.padding(.horizontal, 40 / resizeView)
							.padding(.bottom, 20 / resizeView)
						} else {
							HStack(spacing: 30) {
								HStack(alignment: .center) {
									Text(callViewModel.letters1)
										.default_text_style(styleSize: 34 / resizeView)
										.frame(width: 60 / resizeView, height: 60 / resizeView)
								}
								.padding(10 / resizeView)
								.background(.white)
								.clipShape(Circle())
								.shadow(color: .gray.opacity(0.4), radius: 4)
								.onTapGesture {
									callViewModel.updateZrtpSas(authTokenClicked: callViewModel.letters1)
									callViewModel.zrtpPopupDisplayed = false
								}
								
								HStack(alignment: .center) {
									Text(callViewModel.letters2)
										.default_text_style(styleSize: 34 / resizeView)
										.frame(width: 60 / resizeView, height: 60 / resizeView)
								}
								.padding(10 / resizeView)
								.background(.white)
								.clipShape(Circle())
								.shadow(color: .gray.opacity(0.4), radius: 4)
								.onTapGesture {
									callViewModel.updateZrtpSas(authTokenClicked: callViewModel.letters2)
									callViewModel.zrtpPopupDisplayed = false
								}
							}
							.padding(.horizontal, 40 / resizeView)
							.padding(.bottom, 20 / resizeView)
							
							HStack(spacing: 30) {
								HStack(alignment: .center) {
									Text(callViewModel.letters3)
										.default_text_style(styleSize: 34 / resizeView)
										.frame(width: 60 / resizeView, height: 60 / resizeView)
								}
								.padding(10 / resizeView)
								.background(.white)
								.clipShape(Circle())
								.shadow(color: .gray.opacity(0.4), radius: 4)
								.onTapGesture {
									callViewModel.updateZrtpSas(authTokenClicked: callViewModel.letters3)
									callViewModel.zrtpPopupDisplayed = false
								}
								
								HStack(alignment: .center) {
									Text(callViewModel.letters4)
										.default_text_style(styleSize: 34 / resizeView)
										.frame(width: 60 / resizeView, height: 60 / resizeView)
								}
								.padding(10 / resizeView)
								.background(.white)
								.clipShape(Circle())
								.shadow(color: .gray.opacity(0.4), radius: 4)
								.onTapGesture {
									callViewModel.updateZrtpSas(authTokenClicked: callViewModel.letters4)
									callViewModel.zrtpPopupDisplayed = false
								}
							}
							.padding(.horizontal, 40 / resizeView)
							.padding(.bottom, 20 / resizeView)
						}
					}
					.padding(.horizontal, 10 / resizeView)
					.padding(.bottom, 10 / resizeView)
					.cornerRadius(20)
					.overlay(
						RoundedRectangle(cornerRadius: 20)
							.inset(by: 0.5)
							.stroke(Color.grayMain2c200, lineWidth: 1)
					)
					.padding(.bottom, 10 / resizeView)
					
					Button(action: {
						callViewModel.updateZrtpSas(authTokenClicked: "")
						callViewModel.zrtpPopupDisplayed = false
					}, label: {
						Text("call_dialog_zrtp_validate_trust_letters_do_not_match")
							.foregroundStyle(Color.redDanger500)
							.default_text_style_orange_600(styleSize: 20 / resizeView)
							.frame(height: 35 / resizeView)
							.frame(maxWidth: .infinity)
					})
					.padding(.horizontal, 20 / resizeView)
					.padding(.vertical, 10 / resizeView)
					.cornerRadius(60)
					.overlay(
						RoundedRectangle(cornerRadius: 60)
							.inset(by: 0.5)
							.stroke(Color.redDanger500, lineWidth: 1)
					)
					.padding(.bottom)
				}
				.padding(.top, 20 / resizeView)
				.padding(.horizontal, 20 / resizeView)
				.background(.white)
				.cornerRadius(20)
			}
			.background(callViewModel.cacheMismatch ? Color.orangeWarning600 : Color.blueInfo500)
			.cornerRadius(20)
			.padding(.horizontal, 2)
			.frame(maxHeight: .infinity)
			.shadow(color: callViewModel.cacheMismatch ? Color.orangeWarning600 : Color.blueInfo500, radius: 0, x: 0, y: 2)
			.frame(maxWidth: SharedMainViewModel.shared.maxWidth * 1.2)
			.position(x: geometry.size.width / 2, y: geometry.size.height / 2)
			.onAppear {
				callViewModel.remoteAuthenticationTokens()
			}
		}
	}
	
	var alertZRTP: some View {
		GeometryReader { geometry in
			VStack(alignment: .leading) {
				ZStack(alignment: .top, content: {
					HStack {
						Spacer()
						
						VStack {
							Image("shield-warning")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(.white)
								.frame(width: 25, height: 25, alignment: .leading)
							
							Text("call_dialog_zrtp_security_alert_title")
								.default_text_style_white_700(styleSize: 16 / resizeView)
						}
						.frame(maxWidth: .infinity)
						
						Spacer()
					}
					.padding(.top, 15)
					.padding(.bottom, 2)
				})
				
				VStack(alignment: .center) {
					VStack {
						Text("call_dialog_zrtp_security_alert_message")
							.default_text_style(styleSize: 16 / resizeView)
							.multilineTextAlignment(.center)
							.padding(.bottom, 10 / resizeView)
					}
					.padding(.bottom, 5)
					
					Button(action: {
						callViewModel.terminateCall()
					}, label: {
						HStack {
							Image("phone-disconnect")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(.white)
								.frame(width: 20, height: 20)
							
							Text("call_action_hang_up")
								.default_text_style_white_600(styleSize: 20)
								.frame(height: 35)
						}
						.frame(maxWidth: .infinity)
					})
					.padding(.horizontal, 20 / resizeView)
					.padding(.vertical, 10 / resizeView)
					.background(Color.redDanger500)
					.cornerRadius(60)
					.padding(.bottom)
				}
				.padding(.top, 20 / resizeView)
				.padding(.horizontal, 20 / resizeView)
				.background(.white)
				.cornerRadius(20)
			}
			.background(Color.redDanger500)
			.cornerRadius(20)
			.padding(.horizontal, 2)
			.frame(maxHeight: .infinity)
			.shadow(color: Color.redDanger500, radius: 0, x: 0, y: 2)
			.frame(maxWidth: SharedMainViewModel.shared.maxWidth * 1.2)
			.position(x: geometry.size.width / 2, y: geometry.size.height / 2)
			.onAppear {
				callViewModel.remoteAuthenticationTokens()
			}
		}
	}
}

#Preview {
	ZRTPPopup(callViewModel: CallViewModel(), resizeView: 1)
}
