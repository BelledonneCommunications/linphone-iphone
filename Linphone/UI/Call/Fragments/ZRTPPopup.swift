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

struct ZRTPPopup: View {
	
	@ObservedObject private var telecomManager = TelecomManager.shared
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	
	@ObservedObject var callViewModel: CallViewModel
	
	var body: some View {
		GeometryReader { geometry in
			VStack(alignment: .leading) {
				Text("Validate the device")
					.default_text_style_600(styleSize: 20)
				
				Text("Say \(callViewModel.upperCaseAuthTokenToRead) and click on the letters given by your correspondent:")
					.default_text_style(styleSize: 15)
					.padding(.bottom, 20)
				
				HStack(spacing: 25) {
					Spacer()
					
					HStack(alignment: .center) {
						Text(callViewModel.letters1)
							.default_text_style(styleSize: 30)
							.frame(width: 60, height: 60)
					}
					.padding(10)
					.background(Color.grayMain2c200)
					.cornerRadius(40)
					.onTapGesture {
						callViewModel.updateZrtpSas(authTokenClicked: callViewModel.letters1)
						callViewModel.zrtpPopupDisplayed = false
					}
					
					HStack(alignment: .center) {
						Text(callViewModel.letters2)
							.default_text_style(styleSize: 30)
							.frame(width: 60, height: 60)
					}
					.padding(10)
					.background(Color.grayMain2c200)
					.cornerRadius(40)
					.onTapGesture {
						callViewModel.updateZrtpSas(authTokenClicked: callViewModel.letters2)
						callViewModel.zrtpPopupDisplayed = false
					}
					
					Spacer()
				}
				.padding(.bottom, 20)
				
				HStack(spacing: 25) {
					Spacer()
					
					HStack(alignment: .center) {
						Text(callViewModel.letters3)
							.default_text_style(styleSize: 30)
							.frame(width: 60, height: 60)
					}
					.padding(10)
					.background(Color.grayMain2c200)
					.cornerRadius(40)
					.onTapGesture {
						callViewModel.updateZrtpSas(authTokenClicked: callViewModel.letters3)
						callViewModel.zrtpPopupDisplayed = false
					}
					
					HStack(alignment: .center) {
						Text(callViewModel.letters4)
							.default_text_style(styleSize: 30)
							.frame(width: 60, height: 60)
					}
					.padding(10)
					.background(Color.grayMain2c200)
					.cornerRadius(40)
					.onTapGesture {
						callViewModel.updateZrtpSas(authTokenClicked: callViewModel.letters4)
						callViewModel.zrtpPopupDisplayed = false
					}
					
					Spacer()
				}
				.padding(.bottom, 20)
				
				HStack {
					Text("Skip")
						.underline()
						.tint(Color.grayMain2c600)
						.default_text_style_600(styleSize: 15)
						.foregroundStyle(Color.grayMain2c500)
				}
				.frame(maxWidth: .infinity)
				.padding(.bottom, 30)
				.onTapGesture {
					callViewModel.skipZrtpAuthentication()
					callViewModel.zrtpPopupDisplayed = false
				}
				
				Button(action: {
					callViewModel.updateZrtpSas(authTokenClicked: "")
					callViewModel.zrtpPopupDisplayed = false
				}, label: {
					Text("Letters don't match!")
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
				.padding(.bottom)
			}
			.padding(.horizontal, 20)
			.padding(.vertical, 20)
			.background(.white)
			.cornerRadius(20)
			.padding(.horizontal)
			.frame(maxHeight: .infinity)
			.shadow(color: Color.orangeMain500, radius: 0, x: 0, y: 2)
			.frame(maxWidth: sharedMainViewModel.maxWidth)
			.position(x: geometry.size.width / 2, y: geometry.size.height / 2)
			.onAppear {
				callViewModel.remoteAuthenticationTokens()
			}
		}
	}
}

#Preview {
	ZRTPPopup(callViewModel: CallViewModel())
}
