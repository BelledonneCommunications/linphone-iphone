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

struct ProfileModeFragment: View {
	
	
	@State var options: Int = 1
	@State private var isShowPopup = false
	@State private var isShowPopupForDefault = true
	
	var body: some View {
		GeometryReader { geometry in
			ScrollView(.vertical) {
				VStack {
					ZStack {
						Image("mountain")
							.resizable()
							.scaledToFill()
							.frame(width: geometry.size.width, height: 100)
							.clipped()
						Text("Personnalize your profil mode")
							.default_text_style_white_800(styleSize: 20)
							.padding(.top, -10)
						Text("You will change this mode later")
							.default_text_style_white(styleSize: 15)
							.padding(.top, 40)
					}
					.padding(.top, 35)
					.padding(.bottom, 10)
					
					VStack(spacing: 10) {						
						Button(action: {
							options = 1
						}, label: {
							HStack {
								Image(options == 1 ? "radio-button-fill" : "radio-button")
								Text("Default")
									.profile_mode_text_style_gray_800(styleSize: 16)
								Image("info")
									.resizable()
									.frame(width: 25, height: 25)
									.padding(.all, 10)
									.onTapGesture {
										withAnimation {
											self.isShowPopupForDefault = true
											self.isShowPopup.toggle()
										}
									}
								Spacer()
							}
						})
						
						HStack {
							Text("Chiffrement de bout en bout de tous vos échanges, grâce au mode default vos communications sont à l’abri des regards.")
								.profile_mode_text_style_gray(styleSize: 15)
						}
						.frame(maxWidth: .infinity, alignment: .leading)
						.padding(.horizontal, 16)
						.padding(.vertical, 20)
						.background(Color.gray100)
						.cornerRadius(15)
						.padding(.bottom, 5)
						
						Image("profile-mode")
							.resizable()
							.frame(width: 150, height: 60)
							.padding()
						
						Button(action: {
							options = 2
						}, label: {
							HStack {
								Image(options == 2 ? "radio-button-fill" : "radio-button")
								Text("Interoperable")
									.profile_mode_text_style_gray_800(styleSize: 16)
								Image("info")
									.resizable()
									.frame(width: 25, height: 25)
									.padding(.all, 10)
									.onTapGesture {
										withAnimation {
											self.isShowPopupForDefault = false
											self.isShowPopup.toggle()
										}
									}
								Spacer()
							}
						})
						
						HStack {
							Text("Ce mode vous permet d’être interopérable avec d’autres services SIP.\nVos communications seront chiffrées de point à point. ")
								.profile_mode_text_style_gray(styleSize: 15)
						}
						.frame(maxWidth: .infinity, alignment: .leading)
						.padding(.horizontal, 16)
						.padding(.vertical, 20)
						.background(Color.gray100)
						.cornerRadius(15)
					}
					.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
					.padding()
					
					Spacer()
					
					Button(action: {
						SharedMainViewModel.shared.changeHideProfileMode()
					}, label: {
						Text("dialog_continue")
							.default_text_style_white_600(styleSize: 20)
							.frame(height: 35)
							.frame(maxWidth: .infinity)
					})
					.padding(.horizontal, 20)
					.padding(.vertical, 10)
					.background(Color.orangeMain500)
					.cornerRadius(60)
					.padding(.horizontal)
					.padding(.bottom, geometry.safeAreaInsets.bottom.isEqual(to: 0.0) ? 20 : 0)
					.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
				}
				.frame(minHeight: geometry.size.height)
			}
			.onAppear {
				UserDefaults.standard.set(false, forKey: "display_profile_mode")
				// Skip this view
				SharedMainViewModel.shared.changeHideProfileMode()
			}
			
			if self.isShowPopup {
				PopupView(isShowPopup: $isShowPopup,
						  title: Text(isShowPopupForDefault ? "Default mode" :  "Interoperable mode"),
						  content: Text(
							isShowPopupForDefault
							? "Texte explicatif du default mode : lorem ipsum dolor sit amet, consectetur adipiscing elit."
							+ "Etiam velit sapien, egestas sit amet dictum eget, condimentum a ligula."
							: "Texte explicatif du interoperable mode : lorem ipsum dolor sit amet, consectetur adipiscing elit."
							+ " Etiam velit sapien, egestas sit amet dictum eget, condimentum a ligula."),
						  titleFirstButton: nil,
						  actionFirstButton: {},
						  titleSecondButton: Text("dialog_close"),
						  actionSecondButton: {
					self.isShowPopup.toggle()
				}
				)
				.background(.black.opacity(0.65))
				.onTapGesture {
					self.isShowPopup.toggle()
				}
			}
		}
	}
}

#Preview {
	ProfileModeFragment()
}
