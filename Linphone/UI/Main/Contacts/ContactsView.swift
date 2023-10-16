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

struct ContactsView: View {
	
	@ObservedObject var contactViewModel: ContactViewModel
	@ObservedObject var historyViewModel: HistoryViewModel
	
	@State private var orientation = UIDevice.current.orientation
	@State private var selectedIndex = 0
	
	var objects: [Int] = [
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
		20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39
	]
	
	var body: some View {
		NavigationView {
			ZStack(alignment: .bottomTrailing) {
				VStack(spacing: 0) {
					HStack {
						Image("profile-image-example")
							.resizable()
							.frame(width: 40, height: 40)
							.clipShape(Circle())
						
						Text("Contacts")
							.default_text_style_white_800(styleSize: 20)
							.padding(.leading, 10)
						
						Spacer()
						
						Button {
							
						} label: {
							Image("search")
						}
						
						Button {
							
						} label: {
							Image("filtres")
						}
						.padding(.leading)
					}
					.frame(maxWidth: .infinity)
					.frame(height: 50)
					.padding(.horizontal)
					.background(Color.orangeMain500)
					
					VStack {
						List {
							ForEach(objects, id: \.self) { index in
								Button {
									withAnimation {
										contactViewModel.contactTitle = String(index)
									}
								} label: {
									Text("\(index)")
										.frame( maxWidth: .infinity, alignment: .leading)
										.foregroundStyle(Color.orangeMain500)
								}
								.buttonStyle(.borderless)
								.listRowSeparator(.hidden)
							}
						}
						.listStyle(.plain)
						.overlay(
							VStack {
								if objects.isEmpty {
									Spacer()
									Image("illus-belledonne1")
										.resizable()
										.scaledToFit()
										.clipped()
										.padding(.all)
									Text("No contacts for the moment...")
										.default_text_style_800(styleSize: 16)
									Spacer()
									Spacer()
								}
							}
								.padding(.all)
						)
					}
				}
				.onRotate { newOrientation in
					orientation = newOrientation
				}
				
				Button {
					// Action
				} label: {
					Image("contacts")
						.padding()
						.background(.white)
						.clipShape(Circle())
						.shadow(color: .black.opacity(0.2), radius: 4)
					
				}
				.padding()
			}
		}
		.navigationViewStyle(.stack)
	}
}

#Preview {
	ContactsView(contactViewModel: ContactViewModel(), historyViewModel: HistoryViewModel())
}
