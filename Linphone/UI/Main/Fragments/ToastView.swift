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

struct ToastView: View {
	
	@ObservedObject private var toastViewModel = ToastViewModel.shared
	
	var body: some View {
		VStack {
			if toastViewModel.displayToast {
				HStack {
					Image(toastViewModel.toastMessage.contains("Success") ? "check" : "warning-circle")
						.resizable()
						.renderingMode(.template)
						.frame(width: 25, height: 25, alignment: .leading)
						.foregroundStyle(toastViewModel.toastMessage.contains("Success") ? Color.greenSuccess500 : Color.redDanger500)
					
					switch toastViewModel.toastMessage {
					case "Successful":
						Text("QR code validated!")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_remove_call_logs":
						Text("History has been deleted")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Success_copied_into_clipboard":
						Text("SIP address copied into clipboard")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.greenSuccess500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Failed":
						Text("Invalid QR code!")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Invalide URI":
						Text("Invalide URI")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					case "Registration failed":
						Text("The user name or password is incorrects")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 15)
							.padding(8)
						
					default:
						Text("Error")
							.multilineTextAlignment(.center)
							.foregroundStyle(Color.redDanger500)
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
						.stroke(toastViewModel.toastMessage.contains("Success") ? Color.greenSuccess500 : Color.redDanger500, lineWidth: 1)
				)
				.onTapGesture {
					withAnimation {
						toastViewModel.toastMessage = ""
						toastViewModel.displayToast = false
					}
				}
				.onAppear {
					DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
						withAnimation {
							toastViewModel.toastMessage = ""
							toastViewModel.displayToast = false
						}
					}
				}
				
				Spacer()
			}
		}
		.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
		.padding(.horizontal, 16)
		.padding(.bottom, 18)
		.transition(.move(edge: .top))
		.padding(.top, 60)
	}
}
