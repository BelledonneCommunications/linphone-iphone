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
import Photos

struct PopupView: View {
	
	var permissionManager = PermissionManager.shared
	
	@Binding var isShowPopup: Bool
	var title: Text
	var content: Text?
	
	var titleFirstButton: Text?
	var actionFirstButton: () -> Void
	
	var titleSecondButton: Text?
	var actionSecondButton: () -> Void
	
	var body: some View {
		GeometryReader { geometry in
			VStack(alignment: .leading) {
				title
					.default_text_style_800(styleSize: 16)
					.frame(alignment: .leading)
					.padding(.bottom, 2)
				
				if content != nil {
					content
						.tint(Color.grayMain2c600)
						.default_text_style(styleSize: 15)
						.padding(.bottom, 20)
				}
				
				if titleFirstButton != nil {
					Button(action: {
						actionFirstButton()
					}, label: {
						titleFirstButton
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
				}
				
				if titleSecondButton != nil {
					Button(action: {
						actionSecondButton()
					}, label: {
						titleSecondButton
							.default_text_style_white_600(styleSize: 20)
							.frame(height: 35)
							.frame(maxWidth: .infinity)
					})
					.padding(.horizontal, 20)
					.padding(.vertical, 10)
					.background(Color.orangeMain500)
					.cornerRadius(60)
				}
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
		}
	}
}

#Preview {
	PopupView(isShowPopup: .constant(true),
			  title: Text("Title"),
			  content: Text("Content"),
			  titleFirstButton: Text("Deny all"),
			  actionFirstButton: {},
			  titleSecondButton: Text("Accept all"),
			  actionSecondButton: {})
	.background(.black.opacity(0.65))
}
