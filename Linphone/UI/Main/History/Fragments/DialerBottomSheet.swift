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
import UniformTypeIdentifiers
import linphonesw

struct DialerBottomSheet: View {
	
	@Environment(\.dismiss) var dismiss
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@ObservedObject private var magicSearch = MagicSearchSingleton.shared
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@ObservedObject var startCallViewModel: StartCallViewModel
	
	@State private var orientation = UIDevice.current.orientation
	
	@Binding var showingDialer: Bool
	
	var body: some View {
		VStack(alignment: .center, spacing: 0) {
			VStack(alignment: .center, spacing: 0) {
				if idiom != .pad && (orientation == .landscapeLeft
									 || orientation == .landscapeRight
									 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
					Spacer()
					HStack {
						Spacer()
						Button("Close") {
							showingDialer.toggle()
							dismiss()
						}
					}
					.padding(.trailing)
				} else {
					Capsule()
						.fill(Color.grayMain2c300)
						.frame(width: 75, height: 5)
						.padding(15)
				}
				
				Spacer()
				
				HStack {
					Button {
						startCallViewModel.searchField += "1"
					} label: {
						Text("1")
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(.white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
					
					Spacer()
					
					Button {
						startCallViewModel.searchField += "2"
					} label: {
						Text("2")
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(.white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
					
					Spacer()
					
					Button {
						startCallViewModel.searchField += "3"
					} label: {
						Text("3")
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(.white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
				}
				.padding(.horizontal, 60)
				.frame(maxWidth: sharedMainViewModel.maxWidth)
				
				HStack {
					Button {
						startCallViewModel.searchField += "4"
					} label: {
						Text("4")
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(.white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
					
					Spacer()
					
					Button {
						startCallViewModel.searchField += "5"
					} label: {
						Text("5")
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(.white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
					
					Spacer()
					
					Button {
						startCallViewModel.searchField += "6"
					} label: {
						Text("6")
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(.white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
				}
				.padding(.horizontal, 60)
				.padding(.top, 10)
				.frame(maxWidth: sharedMainViewModel.maxWidth)
				
				HStack {
					Button {
						startCallViewModel.searchField += "7"
					} label: {
						Text("7")
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(.white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
					
					Spacer()
					
					Button {
						startCallViewModel.searchField += "8"
					} label: {
						Text("8")
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(.white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
					
					Spacer()
					
					Button {
						startCallViewModel.searchField += "9"
					} label: {
						Text("9")
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(.white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
				}
				.padding(.horizontal, 60)
				.padding(.top, 10)
				.frame(maxWidth: sharedMainViewModel.maxWidth)
				
				HStack {
					Button {
						startCallViewModel.searchField += "*"
					} label: {
						Text("*")
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(.white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
					
					Spacer()
					
					Button {
					} label: {
						ZStack {
							Text("0")
								.default_text_style(styleSize: 32)
								.multilineTextAlignment(.center)
								.frame(width: 60, height: 75)
								.padding(.top, -15)
								.background(.white)
								.clipShape(Circle())
								.shadow(color: .black.opacity(0.2), radius: 4)
							Text("+")
								.default_text_style(styleSize: 20)
								.multilineTextAlignment(.center)
								.frame(width: 60, height: 85)
								.padding(.bottom, -25)
								.background(.clear)
								.clipShape(Circle())
						}
					}
					.simultaneousGesture(
						LongPressGesture()
							.onEnded { _ in
								startCallViewModel.searchField += "+"
							}
					)
					.highPriorityGesture(
						TapGesture()
							.onEnded { _ in
								startCallViewModel.searchField += "0"
							}
					)
					
					Spacer()
					
					Button {
						startCallViewModel.searchField += "#"
					} label: {
						Text("#")
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(.white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
				}
				.padding(.horizontal, 60)
				.padding(.top, 10)
				.frame(maxWidth: sharedMainViewModel.maxWidth)
				
				HStack {
					
					HStack {
						
					}
					.frame(width: 60, height: 60)
					
					Spacer()
					
					Button {
                        if !startCallViewModel.searchField.isEmpty {
                            do {
                                let address = try Factory.Instance.createAddress(addr: String("sip:" + startCallViewModel.searchField + "@" + startCallViewModel.domain))
								telecomManager.doCallWithCore(addr: address, isVideo: false)
                            } catch {
                                
                            }
                        }
					} label: {
						Image("phone")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(.white)
							.frame(width: 32, height: 32)
						
					}
					.frame(width: 90, height: 60)
					.background(Color.greenSuccess500)
					.cornerRadius(40)
					.shadow(color: .black.opacity(0.2), radius: 4)
					
					Spacer()
					
					Button {
						startCallViewModel.searchField = String(startCallViewModel.searchField.dropLast())
					} label: {
						Image("backspace-fill")
							.resizable()
							.frame(width: 32, height: 32)
						
					}
					.frame(width: 60, height: 60)
				}
				.padding(.horizontal, 60)
				.padding(.top, 20)
				.frame(maxWidth: sharedMainViewModel.maxWidth)
				
				Spacer()
			}
			.frame(maxWidth: .infinity)
			.frame(maxHeight: .infinity)
		}
		.background(Color.gray100)
		.frame(maxWidth: .infinity)
		.frame(maxHeight: .infinity)
		.onRotate { newOrientation in
			orientation = newOrientation
		}
	}
}

#Preview {
	DialerBottomSheet(
		startCallViewModel: StartCallViewModel(), showingDialer: .constant(false)
	)
}
