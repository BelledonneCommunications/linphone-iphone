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

struct ContentView: View {
	
	@ObservedObject var sharedMainViewModel: SharedMainViewModel
	@ObservedObject private var coreContext = CoreContext.shared
	
	@State var index = 0
	@State private var orientation = UIDevice.current.orientation
	
	var body: some View {
		if !sharedMainViewModel.welcomeViewDisplayed {
			WelcomeView(sharedMainViewModel: sharedMainViewModel)
		} else if coreContext.mCore.defaultAccount == nil || sharedMainViewModel.displayProfileMode {
			AssistantView(sharedMainViewModel: sharedMainViewModel)
		} else {
			GeometryReader { geometry in
				NavigationView {
					if orientation == .landscapeLeft || orientation == .landscapeRight || geometry.size.width > geometry.size.height {
						HStack(spacing: 0) {
							VStack {
								Group {
									Spacer()
									Button(action: {
										self.index = 0
									}, label: {
										VStack {
											Image("address-book")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(self.index == 0 ? Color.orangeMain500 : Color.grayMain2c600)
												.frame(width: 25, height: 25)
											if self.index == 0 {
												Text("Contacts")
													.default_text_style_700(styleSize: 10)
											} else {
												Text("Contacts")
													.default_text_style(styleSize: 10)
											}
										}
									})
									
									Spacer()
									
									Button(action: {
										self.index = 1
									}, label: {
										VStack {
											Image("phone")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(self.index == 1 ? Color.orangeMain500 : Color.grayMain2c600)
												.frame(width: 25, height: 25)
											if self.index == 1 {
												Text("Calls")
													.default_text_style_700(styleSize: 10)
											} else {
												Text("Calls")
													.default_text_style(styleSize: 10)
											}
										}
									})
									
									Spacer()
								}
							}
							.frame(width: 60)
							.padding(.leading, orientation == .landscapeRight && geometry.safeAreaInsets.bottom > 0 ? -geometry.safeAreaInsets.leading : 0)
							.background(Color.gray100)
							
							VStack {
								if self.index == 0 {
									ContactsView()
								} else if self.index == 1 {
									HistoryView()
								}
							}
							.frame(maxWidth: .infinity)
						}
					} else {
						VStack(spacing: 0) {
							VStack {
								if self.index == 0 {
									ContactsView()
								} else if self.index == 1 {
									HistoryView()
								}
							}
							.frame(maxWidth: .infinity)
							
							HStack {
								Group {
									Spacer()
									Button(action: {
										self.index = 0
									}, label: {
										VStack {
											Image("address-book")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(self.index == 0 ? Color.orangeMain500 : Color.grayMain2c600)
												.frame(width: 25, height: 25)
											if self.index == 0 {
												Text("Contacts")
													.default_text_style_700(styleSize: 10)
											} else {
												Text("Contacts")
													.default_text_style(styleSize: 10)
											}
										}
									})
									.padding(.top)
									
									Spacer()
									
									Button(action: {
										self.index = 1
									}, label: {
										VStack {
											Image("phone")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(self.index == 1 ? Color.orangeMain500 : Color.grayMain2c600)
												.frame(width: 25, height: 25)
											if self.index == 1 {
												Text("Calls")
													.default_text_style_700(styleSize: 10)
											} else {
												Text("Calls")
													.default_text_style(styleSize: 10)
											}
										}
									})
									.padding(.top)
									
									Spacer()
								}
							}
							.background(Color.gray100)
						}
					}
				}
				.onRotate { newOrientation in
					orientation = newOrientation
				}
			}
		}
	}
}

struct DeviceRotationViewModifier: ViewModifier {
	let action: (UIDeviceOrientation) -> Void
	
	func body(content: Content) -> some View {
		content
			.onAppear()
			.onReceive(NotificationCenter.default.publisher(for: UIDevice.orientationDidChangeNotification)) { _ in
				if UIDevice.current.orientation == .landscapeLeft
					|| UIDevice.current.orientation == .landscapeRight
					|| UIDevice.current.orientation == .portrait {
					action(UIDevice.current.orientation)
				}
			}
	}
}

extension View {
	func onRotate(perform action: @escaping (UIDeviceOrientation) -> Void) -> some View {
		self.modifier(DeviceRotationViewModifier(action: action))
	}
}

#Preview {
	ContentView(sharedMainViewModel: SharedMainViewModel())
}
