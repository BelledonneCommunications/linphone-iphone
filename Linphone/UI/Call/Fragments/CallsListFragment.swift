/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
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

struct CallsListFragment: View {
	
	@ObservedObject private var coreContext = CoreContext.shared
	@ObservedObject private var contactsManager = ContactsManager.shared
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@ObservedObject var callViewModel: CallViewModel
	
	@State private var delayedColor = Color.white
	@State var isShowCallsListBottomSheet: Bool = false
	
	@Binding var isShowCallsListFragment: Bool
	
	var body: some View {
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
							delayColorDismiss()
							withAnimation {
								isShowCallsListFragment.toggle()
							}
						}
					
					Text("Call list")
						.multilineTextAlignment(.leading)
						.default_text_style_orange_800(styleSize: 16)
					
					Spacer()
					
				}
				.frame(maxWidth: .infinity)
				.frame(height: 50)
				.padding(.horizontal)
				.padding(.bottom, 4)
				.background(.white)
				
				if #available(iOS 16.0, *), idiom != .pad {
					callsList
						.sheet(isPresented: $isShowCallsListBottomSheet, onDismiss: {
						}) {
							innerBottomSheet()
								.presentationDetents([.fraction(0.2)])
						}
				} else {
					callsList
						.halfSheet(showSheet: $isShowCallsListBottomSheet) {
							innerBottomSheet()
						} onDismiss: {}
				}
			}
			.background(.white)
		}
		.navigationBarHidden(true)
	}
	
	@ViewBuilder
    func innerBottomSheet() -> some View {
		VStack(spacing: 0) {
            if callViewModel.selectedCall != nil {
                Button(action: {
                    if callViewModel.currentCall != nil && callViewModel.selectedCall!.callLog!.callId == callViewModel.currentCall!.callLog!.callId {
                        if callViewModel.currentCall!.state == .StreamsRunning {
                            do {
                                try callViewModel.currentCall!.pause()
                                
                                DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
                                    callViewModel.isPaused = true
                                }
                            } catch {
                                
                            }
                        } else {
                            do {
                                try callViewModel.currentCall!.resume()
                                
                                DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
                                    callViewModel.isPaused = false
                                }
                            } catch {
                                
                            }
                        }
                        
                        isShowCallsListBottomSheet = false
                    } else {
                        TelecomManager.shared.setHeldOtherCallsWithCore(exceptCallid: "")
                         TelecomManager.shared.setHeld(call: callViewModel.selectedCall!, hold: callViewModel.selectedCall!.state == .StreamsRunning)
                        
                        DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
                            callViewModel.resetCallView()
                        }
                    }
                }, label: {
                    HStack {
                        HStack {
                            Image((callViewModel.selectedCall!.state == .PausedByRemote
                                    || callViewModel.selectedCall!.state == .Pausing
                                    || callViewModel.selectedCall!.state == .Paused) ? "play" : "pause")
                                .resizable()
                                .frame(width: 30, height: 30)
                            
                        }
                        .frame(width: 35, height: 30)
                        .background(.clear)
                        .cornerRadius(40)
                        
                        Text((callViewModel.selectedCall!.state == .PausedByRemote
                              || callViewModel.selectedCall!.state == .Pausing
                              || callViewModel.selectedCall!.state == .Paused) ? "Resume" : "Pause")
                            .default_text_style(styleSize: 15)
                        
                        Spacer()
                    }
                })
                .padding(.horizontal, 30)
                .frame(maxHeight: .infinity)
                
                VStack {
                    Divider()
                }
                .frame(maxWidth: .infinity)
                
                Button(action: {
                    do {
                        try callViewModel.selectedCall!.terminate()
                        isShowCallsListBottomSheet = false
                    } catch _ {
                        
                    }
                }, label: {
                    HStack {
                        HStack {
                            Image("phone-disconnect")
                                .renderingMode(.template)
                                .resizable()
                                .foregroundStyle(.white)
                                .frame(width: 20, height: 20)
                            
                        }
                        .frame(width: 35, height: 30)
                        .background(Color.redDanger500)
                        .cornerRadius(40)
                        
                        Text("Hang up call")
                            .foregroundStyle(Color.redDanger500)
                            .default_text_style_white(styleSize: 15)
                        
                        Spacer()
                    }
                })
                .padding(.horizontal, 30)
                .frame(maxHeight: .infinity)
            }
		}
		.frame(maxHeight: .infinity)
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
	
	var callsList: some View {
		VStack {
			List {
				ForEach(0..<callViewModel.calls.count, id: \.self) { index in
					HStack {
						HStack {
							if callViewModel.calls[index].callLog != nil && callViewModel.calls[index].callLog!.remoteAddress != nil {
								let addressFriend = contactsManager.getFriendWithAddress(address: callViewModel.calls[index].callLog!.remoteAddress!)
								
								let contactAvatarModel = addressFriend != nil
								? ContactsManager.shared.avatarListModel.first(where: {
									($0.friend!.consolidatedPresence == .Online || $0.friend!.consolidatedPresence == .Busy)
									&& $0.friend!.name == addressFriend!.name
									&& $0.friend!.address!.asStringUriOnly() == addressFriend!.address!.asStringUriOnly()
								})
								: ContactAvatarModel(friend: nil, withPresence: false)
								
								if addressFriend != nil && addressFriend!.photo != nil && !addressFriend!.photo!.isEmpty {
									if contactAvatarModel != nil {
										Avatar(contactAvatarModel: contactAvatarModel!, avatarSize: 45)
									} else {
										Image("profil-picture-default")
											.resizable()
											.frame(width: 45, height: 45)
											.clipShape(Circle())
									}
								} else {
									if callViewModel.calls[index].callLog!.remoteAddress!.displayName != nil {
										Image(uiImage: contactsManager.textToImage(
											firstName: callViewModel.calls[index].callLog!.remoteAddress!.displayName!,
											lastName: callViewModel.calls[index].callLog!.remoteAddress!.displayName!.components(separatedBy: " ").count > 1
											? callViewModel.calls[index].callLog!.remoteAddress!.displayName!.components(separatedBy: " ")[1]
											: ""))
										.resizable()
										.frame(width: 45, height: 45)
										.clipShape(Circle())
										
									} else {
										Image(uiImage: contactsManager.textToImage(
											firstName: callViewModel.calls[index].callLog!.remoteAddress!.username ?? "Username Error",
											lastName: callViewModel.calls[index].callLog!.remoteAddress!.username!.components(separatedBy: " ").count > 1
											? callViewModel.calls[index].callLog!.remoteAddress!.username!.components(separatedBy: " ")[1]
											: ""))
										.resizable()
										.frame(width: 45, height: 45)
										.clipShape(Circle())
									}
								}
								
								if addressFriend != nil {
									Text(addressFriend!.name!)
										.default_text_style(styleSize: 16)
										.frame(maxWidth: .infinity, alignment: .leading)
										.lineLimit(1)
								} else {
									Text(callViewModel.calls[index].callLog!.remoteAddress!.displayName != nil
										 ? callViewModel.calls[index].callLog!.remoteAddress!.displayName!
										 : callViewModel.calls[index].callLog!.remoteAddress!.username!)
									.default_text_style(styleSize: 16)
									.frame(maxWidth: .infinity, alignment: .leading)
									.lineLimit(1)
								}
								
								Spacer()
								
								HStack {
									if callViewModel.calls[index].state == .PausedByRemote
										|| callViewModel.calls[index].state == .Pausing
										|| callViewModel.calls[index].state == .Paused
										|| callViewModel.calls[index].state == .Resuming {
										Text(callViewModel.calls[index].state == .Resuming ? "Resuming" : "Paused")
										.default_text_style_300(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .trailing)
										.lineLimit(1)
										.padding(.horizontal, 4)
										
										Image("pause")
											.resizable()
											.frame(width: 25, height: 25)
									} else {
										Text("Active")
										.default_text_style_300(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .trailing)
										.lineLimit(1)
										.padding(.horizontal, 4)
										
										Image("phone-call")
											.resizable()
											.frame(width: 25, height: 25)
									}
								}
							}
						}
					}
					.buttonStyle(.borderless)
					.listRowInsets(EdgeInsets(top: 10, leading: 20, bottom: 10, trailing: 20))
					.listRowSeparator(.hidden)
					.background(.white)
					.onTapGesture {
						if callViewModel.currentCall != nil && callViewModel.calls[index].callLog!.callId == callViewModel.currentCall!.callLog!.callId {
							if callViewModel.currentCall!.state == .StreamsRunning {
								do {
									try callViewModel.currentCall!.pause()
									
									DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
										callViewModel.isPaused = true
									}
								} catch {
									
								}
							} else {
								do {
									try callViewModel.currentCall!.resume()
									
									DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
										callViewModel.isPaused = false
									}
								} catch {
									
								}
							}
						} else {
							TelecomManager.shared.setHeldOtherCallsWithCore(exceptCallid: "")
						 	TelecomManager.shared.setHeld(call: callViewModel.calls[index], hold: callViewModel.calls[index].state == .StreamsRunning)
							
							DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
								callViewModel.resetCallView()
							}
						}
					}
					.onLongPressGesture(minimumDuration: 0.2) {
                        callViewModel.selectedCall = callViewModel.calls[index]
						isShowCallsListBottomSheet = true
					}
				}
			}
			.listStyle(.plain)
			.overlay(
				VStack {
					if callViewModel.calls.isEmpty {
						Spacer()
						Image("illus-belledonne")
							.resizable()
							.scaledToFit()
							.clipped()
							.padding(.all)
						Text("No call for the moment...")
							.default_text_style_800(styleSize: 16)
						Spacer()
						Spacer()
					}
				}
					.padding(.all)
			)
		}
		.navigationTitle("")
		.navigationBarHidden(true)
	}
}

#Preview {
	CallsListFragment(callViewModel: CallViewModel(), isShowCallsListFragment: .constant(true))
}
