/*
 * Copyright (c) 2010-2024 Belledonne Communications SARL.
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
import linphonesw
import UniformTypeIdentifiers

struct SideMenu: View {
	
	let width: CGFloat
	let isOpen: Bool
	let menuClose: () -> Void
	let safeAreaInsets: EdgeInsets
	@Binding var isShowLoginFragment: Bool
	@State private var showHelp = false
	
	var body: some View {
		ZStack {
			GeometryReader { _ in
				EmptyView()
			}
			.background(.gray.opacity(0.3))
			.opacity(self.isOpen ? 1.0 : 0.0)
			.onTapGesture {
				self.menuClose()
			}
			VStack {
				VStack {
					HStack {
						Image("linphone")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.orangeMain500)
							.frame(width: 32, height: 32)
							.padding(10)
						Text(Bundle.main.displayName)
							.default_text_style_800(styleSize: 16)
						Spacer()
						Image("x")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c600)
							.frame(width: 24, height: 24)
							.padding(10)
					}
					.padding(.leading, 10)
					.onTapGesture {
						self.menuClose()
					}
					
					List {
						ForEach(0..<CoreContext.shared.accounts.count, id: \.self) { index in
							SideMenuAccountRow(	model: CoreContext.shared.accounts[index])
							.background()
							.listRowInsets(EdgeInsets(top: 0, leading: 16, bottom: 0, trailing: 16))
							.listRowSeparator(.hidden)
						}
					}
					.listStyle(.plain)
					
					if CoreContext.shared.accounts.isEmpty {
						Text("drawer_menu_no_account_configured_yet")
							.text_style(fontSize: 16, fontWeight: 800, fontColor: Color.grayMain2c600)
							.padding(.bottom, 30)
					}
					
					HStack {
						Image("plus-circle")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.orangeMain500)
							.frame(width: 20, height: 20)
						
						Text("drawer_menu_add_account")
							.default_text_style_orange_600(styleSize: 20)
							.frame(height: 35)
					}
					.frame(maxWidth: .infinity)
					.padding(.horizontal, 20)
					.padding(.vertical, 10)
					.cornerRadius(60)
					.overlay(
						RoundedRectangle(cornerRadius: 60)
							.inset(by: 0.5)
							.stroke(Color.orangeMain500, lineWidth: 1)
					)
					.padding(.leading, 16)
					.padding(.trailing, 16)
					.padding(.bottom, 23)
					.background()
					.onTapGesture {
						self.menuClose()
						withAnimation {
							isShowLoginFragment = true
						}
					}
					
					Rectangle()
						.fill(Color.grayMain2c300)
						.padding(.leading, 16)
						.padding(.trailing, 16)
						.frame(height: 1)
					
					VStack(spacing: 19) {
						ForEach(0..<CoreContext.shared.shortcuts.count, id: \.self) { index in
							SideMenuShortcut(shortcutModel: CoreContext.shared.shortcuts[index])
						}
						SideMenuEntry(
							iconName: "gear",
							title: "settings_title"
						)
						SideMenuEntry(
							iconName: "record-fill",
							title: "recordings_title"
						)
						SideMenuEntry(
							iconName: "question",
							title: "help_title"
						).onTapGesture {
							showHelp = true
						}
						.confirmationDialog("Temp Help", isPresented: $showHelp, titleVisibility: .visible) {
							Button("Send Logs") {
								HelpView.sendLogs()
							}
							Button("Clear Logs") {
								HelpView.clearLogs()
							}
							Button("Logout") {
								HelpView.logout()
							}
						}
					}
					.padding(.bottom, safeAreaInsets.bottom + 13)
					.padding(.top, 13)
					.padding(.leading, 16)
					.padding(.trailing, 16)
				}
				.frame(width: self.width - safeAreaInsets.leading)
				.background(.white)
				.offset(x: self.isOpen ? 0 : -self.width)
				
			}
			.frame(maxWidth: .infinity, alignment: .leading)
			.padding(.leading, safeAreaInsets.leading)
			.padding(.top, TelecomManager.shared.callInProgress ? 0 : safeAreaInsets.top)
		}
		.frame(maxWidth: .infinity, maxHeight: .infinity)
	}
	
}

#Preview {
	GeometryReader { geometry in
		@State var triggerNavigateToLogin: Bool = false
		SideMenu(
			width: geometry.size.width / 5 * 4,
			isOpen: true,
			menuClose: {},
			safeAreaInsets: geometry.safeAreaInsets,
			isShowLoginFragment: $triggerNavigateToLogin
		)
		.ignoresSafeArea(.all)
		.zIndex(2)
	}
}
