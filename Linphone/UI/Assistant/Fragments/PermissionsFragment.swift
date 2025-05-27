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

struct PermissionsFragment: View {
	
	var permissionManager = PermissionManager.shared
	
	@Environment(\.dismiss) var dismiss
	
	var body: some View {
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
		}
		.navigationViewStyle(StackNavigationViewStyle())
		.navigationBarHidden(true)
		.edgesIgnoringSafeArea(.bottom)
		.edgesIgnoringSafeArea(.horizontal)
		.onReceive(permissionManager.$allPermissionsHaveBeenDisplayed, perform: { (granted) in
			if granted {
				withAnimation {
					SharedMainViewModel.shared.changeWelcomeView()
				}
			}
		})
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
				
				Text("assistant_permissions_title")
					.default_text_style_800(styleSize: 20)
			}
			.frame(width: geometry.size.width)
			.padding(.top, 10)
			.padding(.bottom, 20)
			
			Text(String(format: String(localized: "assistant_permissions_subtitle"), Bundle.main.displayName))
				.default_text_style(styleSize: 15)
				.multilineTextAlignment(.center)
			
			Spacer()
			
			VStack(alignment: .leading) {
				HStack {
					HStack(alignment: .center) {
						Image("bell-ringing")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c500)
							.frame(width: 20, height: 20, alignment: .leading)
					}
					.padding(16)
					.background(Color.grayMain2c200)
					.cornerRadius(40)
					
					Text("assistant_permissions_post_notifications_title")
						.default_text_style(styleSize: 15)
						.padding(.leading, 10)
				}
				.padding(.bottom)
				
				HStack {
					HStack(alignment: .center) {
						Image("address-book")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c500)
							.frame(width: 20, height: 20, alignment: .leading)
					}
					.padding(16)
					.background(Color.grayMain2c200)
					.cornerRadius(40)
					Text(.init(String(format: String(localized: "assistant_permissions_read_contacts_title"), Bundle.main.displayName)))
						.default_text_style(styleSize: 15)
						.padding(.leading, 10)
				}
				.padding(.bottom)
				
				HStack {
					HStack(alignment: .center) {
						Image("microphone")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c500)
							.frame(width: 20, height: 20, alignment: .leading)
					}
					.padding(16)
					.background(Color.grayMain2c200)
					.cornerRadius(40)
					
					Text("assistant_permissions_record_audio_title")
						.default_text_style(styleSize: 15)
						.padding(.leading, 10)
				}
				.padding(.bottom)
				
				HStack {
					HStack(alignment: .center) {
						Image("video-camera")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c500)
							.frame(width: 20, height: 20, alignment: .leading)
					}
					.padding(16)
					.background(Color.grayMain2c200)
					.cornerRadius(40)
					
					Text("assistant_permissions_access_camera_title")
						.default_text_style(styleSize: 15)
						.padding(.leading, 10)
				}
				.padding(.bottom)
			}
			.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
			.frame(maxHeight: .infinity)
			.padding(.horizontal, 20)
			
			Spacer()
			
			Button(action: {
				withAnimation {
					SharedMainViewModel.shared.changeWelcomeView()
				}
			}, label: {
				Text("assistant_permissions_skip_permissions")
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
			.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
			.padding(.horizontal)
			
			Button {
				permissionManager.getPermissions()
			} label: {
				Text("assistant_permissions_grant_all_of_them")
					.default_text_style_white_600(styleSize: 20)
					.frame(height: 35)
					.frame(maxWidth: .infinity)
			}
			.padding(.horizontal, 20)
			.padding(.vertical, 10)
			.background(Color.orangeMain500)
			.cornerRadius(60)
			.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
			.padding(.horizontal)
			.padding(.bottom)
			
			Image("mountain2")
				.resizable()
				.scaledToFill()
				.frame(width: geometry.size.width, height: 60)
				.clipped()
		}
		.frame(minHeight: geometry.size.height)
	}
}

#Preview {
	PermissionsFragment()
}
