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

// swiftlint:disable type_body_length
struct DialerBottomSheet: View {
	
	@Environment(\.dismiss) var dismiss
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@ObservedObject private var magicSearch = MagicSearchSingleton.shared
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@ObservedObject var startCallViewModel: StartCallViewModel
	@ObservedObject var callViewModel: CallViewModel
	
	@State private var orientation = UIDevice.current.orientation
	
	@State var dialerField = ""
	
	@Binding var isShowStartCallFragment: Bool
	@Binding var showingDialer: Bool
	
	let currentCall: Call?
	
	var body: some View {
		VStack(alignment: .center, spacing: 0) {
			VStack(alignment: .center, spacing: 0) {
				if idiom != .pad && (orientation == .landscapeLeft
									 || orientation == .landscapeRight
									 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
					Spacer()
					HStack {
						Spacer()
						Button("dialog_close") {
							showingDialer.toggle()
							dismiss()
						}
					}
					.padding(.trailing)
				} else {
					Capsule()
						.fill(currentCall != nil ? .white : Color.grayMain2c300)
						.frame(width: 75, height: 5)
						.padding(15)
				}
				
				if currentCall != nil {
					HStack {
						Text(dialerField)
							.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
							.default_text_style(styleSize: 25)
							.frame(maxWidth: .infinity)
							.padding(.horizontal, 10)
							.lineLimit(1)
							.truncationMode(.head)
						
						Button {
							dialerField = String(dialerField.dropLast())
						} label: {
							Image("backspace-fill")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c500)
								.frame(width: 32, height: 32)
							
						}
						.frame(width: 60, height: 60)
					}
					.padding(.horizontal, 20)
					.padding(.top, 10)
					.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
					
					Spacer()
				} else {
					Spacer()
				}
				
				HStack {
					Button {
						if currentCall != nil {
							let digit = ("1".cString(using: String.Encoding.utf8)?[0])!
							self.sendDtmf(dtmf: digit)
							dialerField += "1"
						} else {
							startCallViewModel.searchField += "1"
						}
					} label: {
						Text("1")
							.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(currentCall != nil ? Color.gray500 : .white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
					
					Spacer()
					
					Button {
						if currentCall != nil {
							let digit = ("2".cString(using: String.Encoding.utf8)?[0])!
							self.sendDtmf(dtmf: digit)
							dialerField += "2"
						} else {
							startCallViewModel.searchField += "2"
						}
					} label: {
						Text("2")
							.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(currentCall != nil ? Color.gray500 : .white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
					
					Spacer()
					
					Button {
						if currentCall != nil {
							let digit = ("3".cString(using: String.Encoding.utf8)?[0])!
							self.sendDtmf(dtmf: digit)
							dialerField += "3"
						} else {
							startCallViewModel.searchField += "3"
						}
					} label: {
						Text("3")
							.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(currentCall != nil ? Color.gray500 : .white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
				}
				.padding(.horizontal, 60)
				.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
				
				HStack {
					Button {
						if currentCall != nil {
							let digit = ("4".cString(using: String.Encoding.utf8)?[0])!
							self.sendDtmf(dtmf: digit)
							dialerField += "4"
						} else {
							startCallViewModel.searchField += "4"
						}
					} label: {
						Text("4")
							.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(currentCall != nil ? Color.gray500 : .white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
					
					Spacer()
					
					Button {
						if currentCall != nil {
							let digit = ("5".cString(using: String.Encoding.utf8)?[0])!
							self.sendDtmf(dtmf: digit)
							dialerField += "5"
						} else {
							startCallViewModel.searchField += "5"
						}
					} label: {
						Text("5")
							.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(currentCall != nil ? Color.gray500 : .white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
					
					Spacer()
					
					Button {
						if currentCall != nil {
							let digit = ("6".cString(using: String.Encoding.utf8)?[0])!
							self.sendDtmf(dtmf: digit)
							dialerField += "6"
						} else {
							startCallViewModel.searchField += "6"
						}
					} label: {
						Text("6")
							.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(currentCall != nil ? Color.gray500 : .white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
				}
				.padding(.horizontal, 60)
				.padding(.top, 10)
				.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
				
				HStack {
					Button {
						if currentCall != nil {
							let digit = ("7".cString(using: String.Encoding.utf8)?[0])!
							self.sendDtmf(dtmf: digit)
							dialerField += "7"
						} else {
							startCallViewModel.searchField += "7"
						}
					} label: {
						Text("7")
							.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(currentCall != nil ? Color.gray500 : .white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
					
					Spacer()
					
					Button {
						if currentCall != nil {
							let digit = ("8".cString(using: String.Encoding.utf8)?[0])!
							self.sendDtmf(dtmf: digit)
							dialerField += "8"
						} else {
							startCallViewModel.searchField += "8"
						}
					} label: {
						Text("8")
							.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(currentCall != nil ? Color.gray500 : .white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
					
					Spacer()
					
					Button {
						if currentCall != nil {
							let digit = ("9".cString(using: String.Encoding.utf8)?[0])!
							self.sendDtmf(dtmf: digit)
							dialerField += "9"
						} else {
							startCallViewModel.searchField += "9"
						}
					} label: {
						Text("9")
							.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(currentCall != nil ? Color.gray500 : .white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
				}
				.padding(.horizontal, 60)
				.padding(.top, 10)
				.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
				
				HStack {
					Button {
						if currentCall != nil {
							let digit = ("*".cString(using: String.Encoding.utf8)?[0])!
							self.sendDtmf(dtmf: digit)
							dialerField += "*"
						} else {
							startCallViewModel.searchField += "*"
						}
					} label: {
						Text("*")
							.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(currentCall != nil ? Color.gray500 : .white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
					
					Spacer()
					
					if currentCall == nil {
						Button {
						} label: {
							ZStack {
								Text("0")
									.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
									.default_text_style(styleSize: 32)
									.multilineTextAlignment(.center)
									.frame(width: 60, height: 75)
									.padding(.top, -15)
									.background(currentCall != nil ? Color.gray500 : .white)
									.clipShape(Circle())
									.shadow(color: .black.opacity(0.2), radius: 4)
								Text("+")
									.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
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
					} else {
						Button {
							let digit = ("0".cString(using: String.Encoding.utf8)?[0])!
							self.sendDtmf(dtmf: digit)
							dialerField += "0"
						} label: {
							Text("0")
								.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
								.default_text_style(styleSize: 32)
								.multilineTextAlignment(.center)
								.frame(width: 60, height: 60)
								.background(currentCall != nil ? Color.gray500 : .white)
								.clipShape(Circle())
								.shadow(color: .black.opacity(0.2), radius: 4)
						}
					}
					
					Spacer()
					
					Button {
						if currentCall != nil {
							let digit = ("#".cString(using: String.Encoding.utf8)?[0])!
							self.sendDtmf(dtmf: digit)
							dialerField += "#"
						} else {
							startCallViewModel.searchField += "#"
						}
					} label: {
						Text("#")
							.foregroundStyle(currentCall != nil ? .white : Color.grayMain2c600)
							.default_text_style(styleSize: 32)
							.multilineTextAlignment(.center)
							.frame(width: 60, height: 60)
							.background(currentCall != nil ? Color.gray500 : .white)
							.clipShape(Circle())
							.shadow(color: .black.opacity(0.2), radius: 4)
					}
				}
				.padding(.horizontal, 60)
				.padding(.top, 10)
				.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
				
				if currentCall == nil {
					HStack {
						HStack {
							
						}
						.frame(width: 60, height: 60)
						
						Spacer()
						
						Button {
							if !startCallViewModel.searchField.isEmpty {
								if callViewModel.isTransferInsteadCall {
									showingDialer = false
									
									magicSearch.currentFilter = ""
									
									magicSearch.searchForContacts()
									
									if callViewModel.isTransferInsteadCall == true {
										callViewModel.isTransferInsteadCall = false
									}
									
									callViewModel.resetCallView()
									
									withAnimation {
										isShowStartCallFragment.toggle()
										startCallViewModel.interpretAndStartCall()
									}
									
									startCallViewModel.searchField = ""
								} else {
									showingDialer = false
									
									magicSearch.currentFilter = ""
									
									magicSearch.searchForContacts()
									
									if callViewModel.isTransferInsteadCall == true {
										callViewModel.isTransferInsteadCall = false
									}
									
										callViewModel.resetCallView()
									
									withAnimation {
										isShowStartCallFragment.toggle()
										startCallViewModel.interpretAndStartCall()
									}
									
									startCallViewModel.searchField = ""
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
					.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
				}
				
				Spacer()
			}
			.frame(maxWidth: .infinity)
			.frame(maxHeight: .infinity)
		}
		.background(currentCall != nil ? Color.gray600 : Color.gray100)
		.frame(maxWidth: .infinity)
		.frame(maxHeight: .infinity)
		.onRotate { newOrientation in
			orientation = newOrientation
		}
	}
	
	func sendDtmf(dtmf: CChar) {
		CoreContext.shared.doOnCoreQueue { core in
			guard let call = self.currentCall, call.state == .StreamsRunning else {
				Log.warn("Cannot send DTMF: call not active")
				return
			}
			
			do {
				try call.sendDtmf(dtmf: dtmf)
			} catch {
				Log.error("Cannot send DTMF \(dtmf) to call \(call.callLog?.callId ?? ""): \(error)")
			}
		}
	}
}

#Preview {
	DialerBottomSheet(
		startCallViewModel: StartCallViewModel()
		, callViewModel: CallViewModel()
		, isShowStartCallFragment: .constant(false)
		, showingDialer: .constant(false)
		, currentCall: nil)
}

// swiftlint:enable type_body_length
