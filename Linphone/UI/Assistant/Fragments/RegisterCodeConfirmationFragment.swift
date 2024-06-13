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

struct RegisterCodeConfirmationFragment: View {
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	@ObservedObject var registerViewModel: RegisterViewModel
	
	@Environment(\.dismiss) var dismiss
	
	@StateObject var viewModel = ViewModel()
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
											otpText(text: viewModel.otp1, focused: viewModel.otpField.isEmpty)
											otpText(text: viewModel.otp2, focused: viewModel.otpField.count == 1)
											otpText(text: viewModel.otp3, focused: viewModel.otpField.count == 2)
											otpText(text: viewModel.otp4, focused: viewModel.otpField.count == 3)
										}
										
										TextField("", text: $viewModel.otpField)
											.default_text_style_600(styleSize: 80)
											.frame(width: isFocused ? 0 : textFieldOriginalWidth, height: textBoxHeight)
											.textContentType(.oneTimeCode)
											.foregroundColor(.clear)
											.accentColor(.clear)
											.background(.clear)
											.keyboardType(.numberPad)
											.focused($isFocused)
											.onChange(of: viewModel.otpField) { _ in
												limitText(textLimit)
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
							.frame(maxWidth: sharedMainViewModel.maxWidth)
							.padding(.horizontal, 20)
						}
					}
					.frame(minHeight: geometry.size.height)
				}
			}
			.navigationTitle("")
			.navigationBarHidden(true)
		}
		.navigationViewStyle(StackNavigationViewStyle())
		.navigationTitle("")
		.navigationBarHidden(true)
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
		if viewModel.otpField.count > upper {
			viewModel.otpField = String(viewModel.otpField.prefix(upper))
		}
	}
}

class ViewModel: ObservableObject {
	
	@Published var otpField = "" {
		didSet {
			guard otpField.count <= 5,
				  otpField.last?.isNumber ?? true else {
				otpField = oldValue
				return
			}
		}
	}
	var otp1: String {
		guard otpField.count >= 1 else {
			return ""
		}
		return String(Array(otpField)[0])
	}
	var otp2: String {
		guard otpField.count >= 2 else {
			return ""
		}
		return String(Array(otpField)[1])
	}
	var otp3: String {
		guard otpField.count >= 3 else {
			return ""
		}
		return String(Array(otpField)[2])
	}
	var otp4: String {
		guard otpField.count >= 4 else {
			return ""
		}
		return String(Array(otpField)[3])
	}
	
	@Published var borderColor: Color = .black
	var successCompletionHandler: (() -> Void)?
	@Published var showResendText = false
	
}

#Preview {
	RegisterCodeConfirmationFragment(registerViewModel: RegisterViewModel())
}
