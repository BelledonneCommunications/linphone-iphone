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

// swiftlint:disable type_body_length
struct CallsListFragment: View {
	
	@ObservedObject private var coreContext = CoreContext.shared
	@ObservedObject private var contactsManager = ContactsManager.shared
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@ObservedObject var callViewModel: CallViewModel
	
	@State private var delayedColor = Color.white
	@State var isShowCallsListBottomSheet: Bool = false
	@State private var isShowPopup = false
	
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
					
					Text("calls_list_title")
						.multilineTextAlignment(.leading)
						.default_text_style_orange_800(styleSize: 16)
					
					Spacer()
					
					if callViewModel.callsCounter > 1 {
						Button {
							self.isShowPopup = true
						} label: {
							Image("arrows-merge")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.orangeMain500)
								.frame(width: 25, height: 25, alignment: .leading)
								.padding(.all, 10)
						}
					}
				}
				.frame(maxWidth: .infinity)
				.frame(height: 50)
				.padding(.horizontal)
				.padding(.bottom, 4)
				.background(.white)
				
				if #available(iOS 16.0, *), idiom != .pad {
					callsList
						.sheet(isPresented: $isShowCallsListBottomSheet, onDismiss: {
						}, content: {
							innerBottomSheet()
								.presentationDetents([.fraction(0.2)])
						})
				} else {
					callsList
						.halfSheet(showSheet: $isShowCallsListBottomSheet) {
							innerBottomSheet()
						} onDismiss: {}
				}
			}
			.background(.white)
			
			if self.isShowPopup {
				PopupView(isShowPopup: $isShowPopup,
						  title: Text("calls_list_dialog_merge_into_conference_title"),
						  content: nil,
						  titleFirstButton: Text("dialog_cancel"),
						  actionFirstButton: {self.isShowPopup.toggle()},
						  titleSecondButton: Text("calls_list_dialog_merge_into_conference_label"),
						  actionSecondButton: {
					callViewModel.mergeCallsIntoConference()
					self.isShowPopup.toggle()
					isShowCallsListFragment.toggle()
				})
				.background(.black.opacity(0.65))
				.onTapGesture {
					self.isShowPopup.toggle()
				}
			}
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
						CoreContext.shared.doOnCoreQueue { core in
							if callViewModel.currentCall!.state == .StreamsRunning {
								TelecomManager.shared.setHeldOtherCalls(core: core, exceptCallid: "")
							} else {
								TelecomManager.shared.setHeldOtherCalls(core: core, exceptCallid: callViewModel.currentCall?.callLog?.callId ?? "")
							}
						}
						TelecomManager.shared.setHeld(call: callViewModel.selectedCall!, hold: false)
                        
                        DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
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
                        .frame(width: 35, height: 35)
                        
                        Text((callViewModel.selectedCall!.state == .PausedByRemote
                              || callViewModel.selectedCall!.state == .Pausing
                              || callViewModel.selectedCall!.state == .Paused) ? "call_action_resume_call" : "call_action_pause_call")
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
                                .foregroundStyle(Color.redDanger500)
                                .frame(width: 30, height: 30)
                        }
						.frame(width: 35, height: 35)
                        
                        Text("call_action_hang_up")
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
				ForEach(Array(callViewModel.calls.enumerated()), id: \.element.callLog?.callId) { index, call in
					if let callLog = call.callLog, let remoteAddress = callLog.remoteAddress {
						HStack {
							if let avatarModel = callViewModel.callsContactAvatarModel[index], callLog.conferenceInfo == nil {
								Avatar(contactAvatarModel: avatarModel, avatarSize: 50)
							} else {
								Image("video-conference")
									.renderingMode(.template)
									.resizable()
									.frame(width: 28, height: 28)
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 50, height: 50)
									.background(Color.grayMain2c200)
									.clipShape(Circle())
							}

							// Display name or conference subject
							let name = callLog.conferenceInfo == nil
								? callViewModel.callsContactAvatarModel[index]?.name ?? "Unknown"
								: callLog.conferenceInfo?.subject ?? String(localized: "conference_name_error")

							Text(name)
								.default_text_style(styleSize: 16)
								.frame(maxWidth: .infinity, alignment: .leading)
								.lineLimit(1)

							Spacer()

							HStack {
								let state = call.state
								let isPaused = state == .PausedByRemote || state == .Pausing || state == .Paused
								let isResuming = state == .Resuming

								Text(isResuming ? String(localized: "call_state_resuming") :
									 isPaused ? String(localized: "call_state_paused") :
									 String(localized: "call_state_connected"))
									.default_text_style_300(styleSize: 14)
									.lineLimit(1)
									.padding(.horizontal, 4)

								Image(isPaused || isResuming ? "pause" : "phone-call")
									.resizable()
									.frame(width: 25, height: 25)
							}
						}
						.buttonStyle(.borderless)
						.listRowInsets(EdgeInsets(top: 10, leading: 20, bottom: 10, trailing: 20))
						.listRowSeparator(.hidden)
						.background(.white)
						.onTapGesture {
							handleTap(on: call)
						}
						.onLongPressGesture(minimumDuration: 0.2) {
							callViewModel.selectedCall = call
							isShowCallsListBottomSheet = true
						}
					}
				}
			}
			.listStyle(.plain)
			.overlay(
				VStack {
					if callViewModel.callsCounter == 0 {
						Spacer()
						Image("illus-belledonne")
							.resizable()
							.scaledToFit()
							.clipped()
							.padding()
						Text("history_list_empty_history")
							.default_text_style_800(styleSize: 16)
						Spacer()
						Spacer()
					}
				}
			)
		}
		.navigationTitle("")
		.navigationBarHidden(true)
	}
	
	private func handleTap(on call: Call) {
		guard let current = callViewModel.currentCall else { return }

		if call.callLog?.callId == current.callLog?.callId {
			if current.state == .StreamsRunning {
				try? current.pause()
				DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
					callViewModel.isPaused = true
				}
			} else {
				try? current.resume()
				DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
					callViewModel.isPaused = false
				}
			}
		} else {
			CoreContext.shared.doOnCoreQueue { core in
				if current.state == .StreamsRunning {
					TelecomManager.shared.setHeldOtherCalls(core: core, exceptCallid: "")
				} else {
					TelecomManager.shared.setHeldOtherCalls(core: core, exceptCallid: current.callLog?.callId ?? "")
				}
			}
			TelecomManager.shared.setHeld(call: call, hold: false)

			DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
				callViewModel.resetCallView()
			}
		}
	}

}

#Preview {
	CallsListFragment(callViewModel: CallViewModel(), isShowCallsListFragment: .constant(true))
}
// swiftlint:enable type_body_length
