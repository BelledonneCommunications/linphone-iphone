/*
* Copyright (c) 2010-2023 Belledonne Communications SARL.
*
* This file is part of Linphone
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

struct ToastView: ViewModifier {
	
	@ObservedObject var sharedMainViewModel : SharedMainViewModel
	
	@Binding var isShowing: String
	
	func body(content: Content) -> some View {
		ZStack {
			content
			toastView
		}
	}
	
	private var toastView: some View {
		VStack {
			if !isShowing.isEmpty {
				HStack {
					Image(isShowing == "Successful" ? "success" : "danger")
						.resizable()
						.frame(width: 25, height: 25, alignment: .leading)
					
					switch isShowing {
					case "Successful":
						Text("QR code validated!")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.green_success_500)
							.default_text_style(styleSize: 15)
							.padding(8)

					case "Failed":
						Text("Invalid QR code!")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.red_danger_500)
							.default_text_style(styleSize: 15)
							.padding(8)

					case "Invalide URI":
						Text("Invalide URI")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.red_danger_500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Registration failed":
						Text("The user name or password is incorrects")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.red_danger_500)
							.default_text_style(styleSize: 15)
							.padding(8)

					default:
						Text("Error")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.red_danger_500)
							.default_text_style(styleSize: 15)
							.padding(8)
					}
				}
				.frame(maxWidth: .infinity)
				.background(.white)
				.cornerRadius(50)
				.overlay(
					RoundedRectangle(cornerRadius: 50)
						.inset(by: 0.5)
						.stroke(isShowing == "Successful" ? Color.green_success_500 : Color.red_danger_500, lineWidth: 1)
				)
				.onTapGesture {
				  isShowing = ""
				}
				.onAppear {
					DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
						isShowing = ""
					}
				}
			}
			Spacer()
		}
		.frame(maxWidth: sharedMainViewModel.maxWidth)
		.padding(.horizontal, 16)
		.padding(.bottom, 18)
		.animation(.linear(duration: 0.3), value: isShowing)
		.transition(.opacity)
	}
}

extension View {	
	func toast(isShowing: Binding<String>) -> some View {
		self.modifier(ToastView(sharedMainViewModel: SharedMainViewModel(), isShowing: isShowing))
	}
}
