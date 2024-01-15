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

struct RegisterFragment: View {
	
	@Environment(\.dismiss) var dismiss
	
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
							
							Text("Register")
								.default_text_style_white_800(styleSize: 20)
								.padding(.top, 20)
						}
						.padding(.top, 35)
						.padding(.bottom, 10)
						
					}
				}
			}
			.navigationTitle("")
			.navigationBarHidden(true)
		}
		.navigationViewStyle(StackNavigationViewStyle())
		.navigationTitle("")
		.navigationBarHidden(true)
	}
}

#Preview {
	RegisterFragment()
}
