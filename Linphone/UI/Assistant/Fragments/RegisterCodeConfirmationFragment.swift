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

// swiftlint:disable line_length
struct RegisterCodeConfirmationFragment: View {
	
	@ObservedObject var registerViewModel: RegisterViewModel
	
	@Environment(\.dismiss) var dismiss
	
	@FocusState var isFocused: Bool
	
	let textLimit = 4
	let textBoxWidth = UIScreen.main.bounds.width / 5
	let textBoxHeight = (UIScreen.main.bounds.width / 5) + 20
	let spaceBetweenBoxes: CGFloat = 10
	let paddingOfBox: CGFloat = 1
	var textFieldOriginalWidth: CGFloat {
		(textBoxWidth*4)+(spaceBetweenBoxes*3)+((paddingOfBox*2)*3)
	}
	
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
					
					if registerViewModel.createInProgress {
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
				
				Text("assistant_account_register")
					.default_text_style_800(styleSize: 20)
			}
			.frame(width: geometry.size.width)
			.padding(.top, 10)
			.padding(.bottom, 20)
			
			ZStack {
				VStack {
					Spacer()
					HStack {
						Spacer()
						Image("confirm_sms_code_illu")
							.padding(.bottom, -geometry.safeAreaInsets.bottom)
					}
				}
				VStack(alignment: .center) {
					Spacer()
					
					Text(String(format: NSLocalizedString("assistant_account_creation_sms_confirmation_explanation", comment: ""), registerViewModel.phoneNumber))
						.default_text_style(styleSize: 15)
						.foregroundStyle(Color.grayMain2c700)
						.padding(.horizontal, 10)
						.frame(maxWidth: .infinity, alignment: .center)
						.multilineTextAlignment(.center)
					
					VStack {
						ZStack {
							
							HStack(spacing: spaceBetweenBoxes) {
								otpText(text: registerViewModel.otp1, focused: registerViewModel.otpField.isEmpty)
								otpText(text: registerViewModel.otp2, focused: registerViewModel.otpField.count == 1)
								otpText(text: registerViewModel.otp3, focused: registerViewModel.otpField.count == 2)
								otpText(text: registerViewModel.otp4, focused: registerViewModel.otpField.count == 3)
							}
							
							TextField("", text: $registerViewModel.otpField)
								.default_text_style_600(styleSize: 80)
								.frame(width: isFocused ? 0 : textFieldOriginalWidth, height: textBoxHeight)
								.textContentType(.oneTimeCode)
								.foregroundColor(.clear)
								.accentColor(.clear)
								.background(.clear)
								.keyboardType(.numberPad)
								.focused($isFocused)
								.onChange(of: registerViewModel.otpField) { _ in
									limitText(textLimit)
									if registerViewModel.otpField.count > 3 {
										registerViewModel.validateCode()
									}
								}
						}
					}
					.frame(maxWidth: .infinity, alignment: .center)
					.padding(.vertical, 20)
					
					Button(action: {
						dismiss()
					}, label: {
						Text("assistant_account_creation_wrong_phone_number")
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
					Spacer()
				}
				.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
				.padding(.horizontal, 20)
			}
			
			Spacer()
			
			Image("mountain2")
				.resizable()
				.scaledToFill()
				.frame(width: geometry.size.width, height: 60)
				.clipped()
		}
		.frame(minHeight: geometry.size.height)
		.onAppear {
			registerViewModel.otpField = ""
		}
	}
	
	private func otpText(text: String, focused: Bool) -> some View {
		
		return Text(text)
			.foregroundStyle(isFocused && focused ? Color.orangeMain500 : Color.grayMain2c600)
			.default_text_style_600(styleSize: 40)
			.frame(width: textBoxWidth, height: textBoxHeight)
			.overlay(
				RoundedRectangle(cornerRadius: 20)
					.inset(by: 0.5)
					.stroke(isFocused && focused ? Color.orangeMain500 : Color.grayMain2c600, lineWidth: 1)
			)
			.padding(paddingOfBox)
	}
	
	func limitText(_ upper: Int) {
		if registerViewModel.otpField.count > upper {
			registerViewModel.otpField = String(registerViewModel.otpField.prefix(upper))
		}
	}
}

#Preview {
	RegisterCodeConfirmationFragment(registerViewModel: RegisterViewModel())
}
// swiftlint:enable line_length
