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
	
	@State private var letters1: String = "AA"
	@State private var letters2: String = "BB"
	@State private var letters3: String = "CC"
	@State private var letters4: String = "DD"
	
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
						Text(letters1)
							.default_text_style(styleSize: 30)
							.frame(width: 60, height: 60)
					}
					.padding(10)
					.background(Color.grayMain2c200)
					.cornerRadius(40)
					.onTapGesture {
						callViewModel.lettersClicked(letters: letters1)
						callViewModel.zrtpPopupDisplayed = false
					}
					
					HStack(alignment: .center) {
						Text(letters2)
							.default_text_style(styleSize: 30)
							.frame(width: 60, height: 60)
					}
					.padding(10)
					.background(Color.grayMain2c200)
					.cornerRadius(40)
					.onTapGesture {
						callViewModel.lettersClicked(letters: letters2)
						callViewModel.zrtpPopupDisplayed = false
					}
					
					Spacer()
				}
				.padding(.bottom, 20)
				
				HStack(spacing: 25) {
					Spacer()
					
					HStack(alignment: .center) {
						Text(letters3)
							.default_text_style(styleSize: 30)
							.frame(width: 60, height: 60)
					}
					.padding(10)
					.background(Color.grayMain2c200)
					.cornerRadius(40)
					.onTapGesture {
						callViewModel.lettersClicked(letters: letters3)
						callViewModel.zrtpPopupDisplayed = false
					}
					
					HStack(alignment: .center) {
						Text(letters4)
							.default_text_style(styleSize: 30)
							.frame(width: 60, height: 60)
					}
					.padding(10)
					.background(Color.grayMain2c200)
					.cornerRadius(40)
					.onTapGesture {
						callViewModel.lettersClicked(letters: letters4)
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
					callViewModel.zrtpPopupDisplayed = false
				}
				
				Button(action: {
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
				
				var random = SystemRandomNumberGenerator()
				let correctLetters = Int(random.next(upperBound: UInt32(4)))
				
				letters1 = (correctLetters == 0) ? callViewModel.upperCaseAuthTokenToListen : self.randomAlphanumericString(2)
				letters2 = (correctLetters == 1) ? callViewModel.upperCaseAuthTokenToListen : self.randomAlphanumericString(2)
				letters3 = (correctLetters == 2) ? callViewModel.upperCaseAuthTokenToListen : self.randomAlphanumericString(2)
				letters4 = (correctLetters == 3) ? callViewModel.upperCaseAuthTokenToListen : self.randomAlphanumericString(2)
			}
		}
	}
	
	func randomAlphanumericString(_ length: Int) -> String {
		let letters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
		let len = UInt32(letters.count)
		var random = SystemRandomNumberGenerator()
		var randomString = ""
		for _ in 0..<length {
			let randomIndex = Int(random.next(upperBound: len))
			let randomCharacter = letters[letters.index(letters.startIndex, offsetBy: randomIndex)]
			randomString.append(randomCharacter)
		}
		return randomString
	}
}

#Preview {
	ZRTPPopup(callViewModel: CallViewModel())
}
