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
import linphonesw

// swiftlint:disable type_body_length
struct StartCallFragment: View {
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject var magicSearch = MagicSearchSingleton.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@EnvironmentObject var callViewModel: CallViewModel
	@EnvironmentObject var conversationsListViewModel: ConversationsListViewModel
	
	@StateObject private var startCallViewModel = StartCallViewModel()
	
	@State private var contactAvatarModel: ContactAvatarModel? = nil
	
	@State private var transferAddress: Address? = nil
	@State private var isShowTransferPopup: Bool = false
	@State private var isShowSipAddressesPopup: Bool = false
	
	@Binding var showingDialer: Bool
	@Binding var isShowStartCallFragment: Bool
	
	@FocusState var isSearchFieldFocused: Bool
	@State private var delayedColor = Color.white
	
	var resetCallView: () -> Void
	
	var body: some View {
		NavigationView {
			if #available(iOS 16.4, *), idiom != .pad {
				startCall
					.sheet(isPresented: $showingDialer) {
						DialerBottomSheet(
							startCallViewModel: startCallViewModel,
							callViewModel: callViewModel,
							isShowStartCallFragment: $isShowStartCallFragment,
							showingDialer: $showingDialer,
							currentCall: nil
						)
						.presentationDetents([.medium])
						.presentationBackgroundInteraction(.enabled(upThrough: .medium))
					}
			} else {
				startCall
					.halfSheet(showSheet: $showingDialer) {
						DialerBottomSheet(
							startCallViewModel: startCallViewModel,
							callViewModel: callViewModel,
							isShowStartCallFragment: $isShowStartCallFragment,
							showingDialer: $showingDialer,
							currentCall: nil
						)
					} onDismiss: {}
			}
		}
		.navigationViewStyle(StackNavigationViewStyle())
	}
	
	var startCall: some View {
		ZStack {
			VStack(spacing: 1) {
				
				Rectangle()
					.foregroundColor(delayedColor)
					.edgesIgnoringSafeArea(.top)
					.frame(height: 0)
					.task(delayColor)
				
				HStack {
					Image("caret-left")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.orangeMain500)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
						.padding(.top, 2)
						.padding(.leading, -10)
						.onTapGesture {
							startCallViewModel.searchField = ""
							magicSearch.currentFilter = ""
							magicSearch.searchForContacts()
							
							if callViewModel.isTransferInsteadCall == true {
								callViewModel.isTransferInsteadCall = false
							}
							
							resetCallView()
							
							delayColorDismiss()
							withAnimation {
								isShowStartCallFragment.toggle()
							}
						}
					
					let address = callViewModel.currentCall?.remoteAddress

					let nameTmp =
						address?.displayName
						?? address?.username
						?? String(address?.asStringUriOnly().dropFirst(4) ?? "")

					Text(
						!callViewModel.isTransferInsteadCall
						? String(localized: "history_call_start_title")
						: String(
							format: String(localized: "call_transfer_current_call_title"),
							nameTmp
						  )
					)
					.multilineTextAlignment(.leading)
					.default_text_style_orange_800(styleSize: 16)
					
					Spacer()
					
				}
				.frame(maxWidth: .infinity)
				.frame(height: 50)
				.padding(.horizontal)
				.padding(.bottom, 4)
				.background(.white)
				
				VStack(spacing: 0) {
					if callViewModel.isTransferInsteadCall {
						HStack(alignment: .center) {
							Text("call_transfer_active_calls_label")
								.default_text_style_800(styleSize: 16)
							
							Spacer()
						}
						.padding(.vertical, 10)
						.padding(.horizontal, 16)
						
						if let remoteAddress = callViewModel.currentCall?.remoteAddress,
						   !callViewModel.calls.filter({ $0.remoteAddress?.equal(address2: remoteAddress) == false }).isEmpty {
							callsList(remoteAddress: remoteAddress)
								.padding(.bottom, 10)
						} else {
							HStack(alignment: .center) {
								Text("call_transfer_no_active_call_label")
									.default_text_style_700(styleSize: 16)
							}
							.padding(.bottom, 10)
						}
					}
					
					ZStack(alignment: .trailing) {
						TextField("history_call_start_search_bar_filter_hint", text: $startCallViewModel.searchField)
							.default_text_style(styleSize: 15)
							.frame(height: 25)
							.focused($isSearchFieldFocused)
							.padding(.horizontal, 30)
							.onChange(of: startCallViewModel.searchField) { newValue in
								magicSearch.currentFilter = newValue
								magicSearch.searchForContacts()
							}
							.simultaneousGesture(TapGesture().onEnded {
								showingDialer = false
							})
						
						HStack {
							Button(action: {
							}, label: {
								Image("magnifying-glass")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 25, height: 25)
							})
							
							Spacer()
							
							if startCallViewModel.searchField.isEmpty {
								Button(action: {
									if !showingDialer {
										isSearchFieldFocused = false
										
										DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
											showingDialer = true
										}
									} else {
										showingDialer = false
										
										DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
											isSearchFieldFocused = true
										}
									}
								}, label: {
									Image(!showingDialer ? "dialer" : "keyboard")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.grayMain2c500)
										.frame(width: 25, height: 25)
								})
							} else {
								Button(action: {
									startCallViewModel.searchField = ""
									magicSearch.currentFilter = ""
									magicSearch.searchForContacts()
								}, label: {
									Image("x")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.grayMain2c500)
										.frame(width: 25, height: 25)
								})
							}
						}
					}
					.padding(.horizontal, 15)
					.padding(.vertical, 10)
					.cornerRadius(60)
					.overlay(
						RoundedRectangle(cornerRadius: 60)
							.inset(by: 0.5)
							.stroke(isSearchFieldFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
					)
					.padding(.vertical)
					.padding(.horizontal)
					
					
					if !startCallViewModel.hideGroupCallButton {
						NavigationLink(destination: {
							StartGroupCallFragment(isShowStartCallFragment: $isShowStartCallFragment)
								.environmentObject(startCallViewModel)
						}, label: {
							HStack {
								HStack(alignment: .center) {
									Image("video-conference")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(.white)
										.frame(width: 28, height: 28)
								}
								.padding(10)
								.background(Color.orangeMain500)
								.cornerRadius(40)
								
								Text("history_call_start_create_group_call")
									.foregroundStyle(.black)
									.default_text_style_800(styleSize: 16)
								
								Spacer()
								
								Image("caret-right")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 25, height: 25, alignment: .leading)
							}
						})
						.padding(.vertical, 10)
						.padding(.horizontal, 20)
						.background(
							LinearGradient(gradient: Gradient(colors: [.grayMain2c100, .white]), startPoint: .leading, endPoint: .trailing)
								.padding(.vertical, 10)
								.padding(.horizontal, 40)
						)
					}
					
					ZStack {
						ScrollView {
							if !ContactsManager.shared.lastSearch.isEmpty {
								HStack(alignment: .center) {
									Text("contacts_list_all_contacts_title")
										.default_text_style_800(styleSize: 16)
									
									Spacer()
								}
								.padding(.vertical, 10)
								.padding(.horizontal, 16)
							}
							
							ContactsListFragment(showingSheet: .constant(false)
												 , startCallFunc: { addr in
								CoreContext.shared.doOnCoreQueue { core in
									ContactAvatarModel.getAvatarModelFromAddress(address: addr) { contactAvatarModel in
										self.contactAvatarModel = contactAvatarModel
										DispatchQueue.main.async {
											if contactAvatarModel.addresses.count == 1 && contactAvatarModel.phoneNumbersWithLabel.isEmpty {
												if callViewModel.isTransferInsteadCall {
													self.transferAddress = addr
													self.isShowTransferPopup = true
												} else {
													showingDialer = false
													
													startCallViewModel.searchField = ""
													magicSearch.currentFilter = ""
													
													magicSearch.searchForContacts()
													
													if callViewModel.isTransferInsteadCall == true {
														callViewModel.isTransferInsteadCall = false
													}
													
													resetCallView()
													
													delayColorDismiss()
													
													withAnimation {
														isShowStartCallFragment.toggle()
														telecomManager.doCallOrJoinConf(address: addr)
													}
												}
											} else if contactAvatarModel.addresses.isEmpty && contactAvatarModel.phoneNumbersWithLabel.count == 1 {
												if let firstPhoneNumbersWithLabel = contactAvatarModel.phoneNumbersWithLabel.first, let addr = core.interpretUrl(url: firstPhoneNumbersWithLabel.phoneNumber, applyInternationalPrefix: LinphoneUtils.applyInternationalPrefix(core: core)) {
													if callViewModel.isTransferInsteadCall {
														self.transferAddress = addr
														self.isShowTransferPopup = true
													} else {
														showingDialer = false
														
														startCallViewModel.searchField = ""
														magicSearch.currentFilter = ""
														
														magicSearch.searchForContacts()
														
														if callViewModel.isTransferInsteadCall == true {
															callViewModel.isTransferInsteadCall = false
														}
														
														resetCallView()
														
														delayColorDismiss()
														
														withAnimation {
															isShowStartCallFragment.toggle()
															telecomManager.doCallOrJoinConf(address: addr)
														}
													}
												}
											} else {
												DispatchQueue.main.async {
													isShowSipAddressesPopup = true
												}
											}
										}
									}
								}
							})
							.padding(.horizontal, 16)
							
							if !contactsManager.lastSearchSuggestions.isEmpty {
								HStack(alignment: .center) {
									Text("generic_address_picker_suggestions_list_title")
										.default_text_style_800(styleSize: 16)
									
									Spacer()
								}
								.padding(.vertical, 10)
								.padding(.horizontal, 16)
								
								suggestionsList
							}
						}
						
						if magicSearch.isLoading {
							ProgressView()
								.controlSize(.large)
								.progressViewStyle(CircularProgressViewStyle(tint: .orangeMain500))
						}
					}
				}
				.frame(maxWidth: .infinity)
			}
			.background(.white)
			
			if isShowSipAddressesPopup && contactAvatarModel != nil {
				VStack(alignment: .leading) {
					HStack {
						Text("contact_dialog_pick_phone_number_or_sip_address_title")
							.default_text_style_800(styleSize: 16)
							.padding(.bottom, 2)
						
						Spacer()
						
						Image("x")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c600)
							.frame(width: 25, height: 25)
							.padding(.all, 10)
					}
					.frame(maxWidth: .infinity)
					
					ForEach(0..<contactAvatarModel!.addresses.count, id: \.self) { index in
						HStack {
							HStack {
								VStack {
									Text(String(localized: "sip_address") + ":")
										.default_text_style_700(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
									Text(contactAvatarModel!.addresses[index].dropFirst(4))
										.default_text_style(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
										.lineLimit(1)
										.fixedSize(horizontal: false, vertical: true)
								}
								Spacer()
							}
							.padding(.vertical, 15)
							.padding(.horizontal, 10)
						}
						.background(.white)
						.onTapGesture {
							do {
								let addr = try Factory.Instance.createAddress(addr: contactAvatarModel!.addresses[index])
								
								if callViewModel.isTransferInsteadCall {
									self.isShowSipAddressesPopup = false
									self.transferAddress = addr
									self.isShowTransferPopup = true
								} else {
									showingDialer = false
									
									startCallViewModel.searchField = ""
									magicSearch.currentFilter = ""
									
									magicSearch.searchForContacts()
									
									if callViewModel.isTransferInsteadCall == true {
										callViewModel.isTransferInsteadCall = false
									}
									
									resetCallView()
									
									delayColorDismiss()
									
									withAnimation {
										isShowSipAddressesPopup = false
										isShowStartCallFragment.toggle()
										telecomManager.doCallOrJoinConf(address: addr)
									}
								}
							} catch {
								Log.error("[StartCallFragment] unable to create address for a new outgoing call : \(contactAvatarModel!.addresses[index]) \(error) ")
							}
						}
					}
					
					ForEach(0..<contactAvatarModel!.phoneNumbersWithLabel.count, id: \.self) { index in
						HStack {
							HStack {
								VStack {
									Text(String(localized: "phone_number") + ":")
										.default_text_style_700(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
									Text(contactAvatarModel!.phoneNumbersWithLabel[index].phoneNumber)
										.default_text_style(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
										.lineLimit(1)
										.fixedSize(horizontal: false, vertical: true)
								}
								Spacer()
							}
							.padding(.vertical, 15)
							.padding(.horizontal, 10)
						}
						.background(.white)
						.onTapGesture {
							CoreContext.shared.doOnCoreQueue { core in
								if let addr = core.interpretUrl(url: contactAvatarModel!.phoneNumbersWithLabel[index].phoneNumber, applyInternationalPrefix: LinphoneUtils.applyInternationalPrefix(core: core)) {
									DispatchQueue.main.async {
										if callViewModel.isTransferInsteadCall {
											self.isShowSipAddressesPopup = false
											self.transferAddress = addr
											self.isShowTransferPopup = true
										} else {
											showingDialer = false
											
											startCallViewModel.searchField = ""
											magicSearch.currentFilter = ""
											
											magicSearch.searchForContacts()
											
											if callViewModel.isTransferInsteadCall == true {
												callViewModel.isTransferInsteadCall = false
											}
											
											resetCallView()
											
											delayColorDismiss()
											
											withAnimation {
												isShowSipAddressesPopup = false
												isShowStartCallFragment.toggle()
												telecomManager.doCallOrJoinConf(address: addr)
											}
										}
									}
								} else {
									Log.error("[StartCallFragment] unable to create address (interpret Url for phone number) for a new outgoing call : \(contactAvatarModel!.addresses[index])")
								}
							}
						}
					}
				}
				.padding(.horizontal, 20)
				.padding(.vertical, 20)
				.background(.white)
				.cornerRadius(20)
				.frame(maxHeight: .infinity)
				.shadow(color: Color.orangeMain500, radius: 0, x: 0, y: 2)
				.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
				.padding(.horizontal, 20)
				.background(.black.opacity(0.65))
				.zIndex(3)
				.onTapGesture {
					isShowSipAddressesPopup.toggle()
				}
			}
			
			if isShowTransferPopup {
				if let remoteAddress = callViewModel.currentCall?.remoteAddress, let transferAddress = self.transferAddress {
					let currentCallNameTmp = remoteAddress.displayName
					?? remoteAddress.username
					?? String(remoteAddress.asStringUriOnly().dropFirst(4))
					
					let transferAddressNameTmp = transferAddress.displayName
					?? transferAddress.username
					?? String(transferAddress.asStringUriOnly().dropFirst(4))
					
					PopupView(
						isShowPopup: $isShowTransferPopup,
						title: Text("call_transfer_confirm_dialog_tittle"),
						content: Text(String(format: String(localized: "call_transfer_confirm_dialog_message"), currentCallNameTmp, transferAddressNameTmp)),
						titleFirstButton: nil,
						actionFirstButton: {},
						titleSecondButton: Text("dialog_confirm"),
						actionSecondButton: {
							showingDialer = false
							
							startCallViewModel.searchField = ""
							magicSearch.currentFilter = ""
							
							magicSearch.searchForContacts()
							
							if callViewModel.isTransferInsteadCall == true {
								callViewModel.isTransferInsteadCall = false
							}
							
							resetCallView()
							
							delayColorDismiss()
							
							withAnimation {
								isShowStartCallFragment.toggle()
								if let transferAddress = self.transferAddress {
									callViewModel.blindTransferCallTo(toAddress: transferAddress)
									self.transferAddress = nil
								}
							}
							
							self.isShowTransferPopup.toggle()
						},
						titleThirdButton: Text("dialog_cancel"),
						actionThirdButton: {
							self.isShowTransferPopup.toggle()
						}
					)
					.background(.black.opacity(0.65))
					.zIndex(3)
					.onTapGesture {
						self.isShowTransferPopup.toggle()
					}
				}
			}
		}
		.navigationTitle("")
		.navigationBarHidden(true)
		.onAppear {
			if !magicSearch.currentFilter.isEmpty || (self.contactsManager.lastSearch.isEmpty && self.contactsManager.lastSearchSuggestions.isEmpty) {
				magicSearch.currentFilter = ""
				magicSearch.searchForContacts()
			}
		}
	}
	
	@Sendable private func delayColor() async {
		try? await Task.sleep(nanoseconds: 250_000_000)
		delayedColor = Color.orangeMain500
	}
	
	func delayColorDismiss() {
		Task {
			try? await Task.sleep(nanoseconds: 80_000_000)
			delayedColor = .white
		}
	}
	
	func callsList(remoteAddress: Address) -> some View {
		ForEach(0..<callViewModel.calls.filter({ $0.remoteAddress?.equal(address2: remoteAddress) == false }).count, id: \.self) { index in
			Button {
				if callViewModel.isTransferInsteadCall {
					self.transferAddress = callViewModel.calls.filter({ $0.remoteAddress?.equal(address2: remoteAddress) == false })[index].remoteAddress
					self.isShowTransferPopup = true
				}
			} label: {
				HStack {
					if index < callViewModel.calls.count,
					   let remoteAddress = callViewModel.calls.filter({ $0.remoteAddress?.equal(address2: remoteAddress) == false })[index].remoteAddress{
						if remoteAddress.domain != AppServices.corePreferences.defaultDomain {
							Image(uiImage: contactsManager.textToImage(
								firstName: String(remoteAddress.asStringUriOnly().dropFirst(4)),
								lastName: ""))
							.resizable()
							.frame(width: 45, height: 45)
							.clipShape(Circle())
							
							Text(String(remoteAddress.asStringUriOnly().dropFirst(4)))
								.default_text_style(styleSize: 16)
								.lineLimit(1)
								.frame(maxWidth: .infinity, alignment: .leading)
								.foregroundStyle(Color.orangeMain500)
						} else {
							let nameTmp = remoteAddress.displayName
							?? remoteAddress.username
							?? String(remoteAddress.asStringUriOnly().dropFirst(4))
							
							Image(uiImage: contactsManager.textToImage(
								firstName: nameTmp,
								lastName: ""))
							.resizable()
							.frame(width: 45, height: 45)
							.clipShape(Circle())
							
							Text(nameTmp)
								.default_text_style(styleSize: 16)
								.lineLimit(1)
								.frame(maxWidth: .infinity, alignment: .leading)
								.foregroundStyle(Color.orangeMain500)
						}
					} else {
						Image("profil-picture-default")
							.resizable()
							.frame(width: 45, height: 45)
							.clipShape(Circle())
						
						Text("username_error")
							.default_text_style(styleSize: 16)
							.frame(maxWidth: .infinity, alignment: .leading)
							.foregroundStyle(Color.orangeMain500)
					}
				}
				.padding(.horizontal)
			}
			.buttonStyle(.borderless)
			.listRowSeparator(.hidden)
		}
	}
	
	var suggestionsList: some View {
		ForEach(0..<contactsManager.lastSearchSuggestions.count, id: \.self) { index in
			Button {
				if callViewModel.isTransferInsteadCall {
					self.transferAddress = contactsManager.lastSearchSuggestions[index].address
					self.isShowTransferPopup = true
				} else {
					showingDialer = false
					
					startCallViewModel.searchField = ""
					magicSearch.currentFilter = ""
					
					magicSearch.searchForContacts()
					
					if callViewModel.isTransferInsteadCall == true {
						callViewModel.isTransferInsteadCall = false
					}
					
					resetCallView()
					
					delayColorDismiss()
					
					withAnimation {
						isShowStartCallFragment.toggle()
						if contactsManager.lastSearchSuggestions[index].address != nil {
							telecomManager.doCallOrJoinConf(address: contactsManager.lastSearchSuggestions[index].address!)
						}
					}
				}
			} label: {
				HStack {
					if index < contactsManager.lastSearchSuggestions.count
						&& contactsManager.lastSearchSuggestions[index].address != nil {
						if contactsManager.lastSearchSuggestions[index].address!.domain != AppServices.corePreferences.defaultDomain {
							Image(uiImage: contactsManager.textToImage(
								firstName: String(contactsManager.lastSearchSuggestions[index].address!.asStringUriOnly().dropFirst(4)),
								lastName: ""))
							.resizable()
							.frame(width: 45, height: 45)
							.clipShape(Circle())
							
							Text(String(contactsManager.lastSearchSuggestions[index].address!.asStringUriOnly().dropFirst(4)))
								.default_text_style(styleSize: 16)
								.lineLimit(1)
								.frame(maxWidth: .infinity, alignment: .leading)
								.foregroundStyle(Color.orangeMain500)
						} else {
							if let address = contactsManager.lastSearchSuggestions[index].address {
								let nameTmp = address.displayName
								?? address.username
								?? String(address.asStringUriOnly().dropFirst(4))
								
								Image(uiImage: contactsManager.textToImage(
									firstName: nameTmp,
									lastName: ""))
								.resizable()
								.frame(width: 45, height: 45)
								.clipShape(Circle())
								
								Text(nameTmp)
									.default_text_style(styleSize: 16)
									.lineLimit(1)
									.frame(maxWidth: .infinity, alignment: .leading)
									.foregroundStyle(Color.orangeMain500)
							}
						}
					} else {
						Image("profil-picture-default")
							.resizable()
							.frame(width: 45, height: 45)
							.clipShape(Circle())
						
						Text("username_error")
							.default_text_style(styleSize: 16)
							.frame(maxWidth: .infinity, alignment: .leading)
							.foregroundStyle(Color.orangeMain500)
					}
				}
				.padding(.horizontal)
			}
			.buttonStyle(.borderless)
			.listRowSeparator(.hidden)
		}
	}
}

#Preview {
	StartCallFragment(
		showingDialer: .constant(false),
		isShowStartCallFragment: .constant(true),
		resetCallView: {}
	)
}

// swiftlint:enable type_body_length
