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
import AVFAudio

// swiftlint:disable type_body_length
// swiftlint:disable cyclomatic_complexity

struct MeetingWaitingRoomFragment: View {
	
	@ObservedObject private var coreContext = CoreContext.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@StateObject private var meetingWaitingRoomViewModel = MeetingWaitingRoomViewModel()
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	let pub = NotificationCenter.default.publisher(for: AVAudioSession.routeChangeNotification)
	
	@State var audioRouteSheet: Bool = false
	@State var options: Int = 1
	@State var angleDegree = 0.0
	
	var body: some View {
		GeometryReader { geometry in
			
			if #available(iOS 16.0, *), idiom != .pad {
				innerView(geometry: geometry)
					.sheet(isPresented: $audioRouteSheet, onDismiss: {
						audioRouteSheet = false
					}, content: {
						innerBottomSheet().presentationDetents([.fraction(0.3)])
					})
					.onAppear {
						meetingWaitingRoomViewModel.enableAVAudioSession()
						if AVAudioSession.sharedInstance().currentRoute.outputs.filter({
							$0.portType.rawValue.contains("Bluetooth") || $0.portType.rawValue.contains("Headphones")
						}).isEmpty {
							do {
								try AVAudioSession.sharedInstance().overrideOutputAudioPort(.speaker)
							} catch _ {
								
							}
						}
					}
					.onDisappear {
						meetingWaitingRoomViewModel.disableAVAudioSession()
					}
			} else {
				innerView(geometry: geometry)
					.halfSheet(showSheet: $audioRouteSheet) {
						innerBottomSheet()
					} onDismiss: {
						audioRouteSheet = false
					}
					.onAppear {
						meetingWaitingRoomViewModel.enableAVAudioSession()
						if AVAudioSession.sharedInstance().currentRoute.outputs.filter({
							$0.portType.rawValue.contains("Bluetooth") || $0.portType.rawValue.contains("Headphones")
						}).isEmpty {
							do {
								try AVAudioSession.sharedInstance().overrideOutputAudioPort(.speaker)
							} catch _ {
								
							}
						}
					}
					.onDisappear {
						meetingWaitingRoomViewModel.disableAVAudioSession()
					}
			}
		}
	}
	
	@ViewBuilder
	// swiftlint:disable:next function_body_length
	func innerView(geometry: GeometryProxy) -> some View {
		VStack {
			if #available(iOS 16.0, *) {
				Rectangle()
					.foregroundColor(Color.orangeMain500)
					.edgesIgnoringSafeArea(.top)
					.frame(height: 0)
			} else if idiom != .pad && !(orientation == .landscapeLeft || orientation == .landscapeRight
										 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
				Rectangle()
					.foregroundColor(Color.orangeMain500)
					.edgesIgnoringSafeArea(.top)
					.frame(height: 1)
			}
			
			HStack {
				Button {
					withAnimation {
						meetingWaitingRoomViewModel.disableVideoPreview()
						telecomManager.meetingWaitingRoomSelected = nil
						telecomManager.meetingWaitingRoomDisplayed = false
					}
				} label: {
					Image("caret-left")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.orangeMain500)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
				}
				
				Text(telecomManager.meetingWaitingRoomName)
					.default_text_style_white_800(styleSize: 16)
				
				Spacer()
			}
			.frame(height: 40)
			.zIndex(1)
			
			HStack {
				Button {
				} label: {
					Image("caret-left")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.orangeMain500)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
				}
				.hidden()
				
				Text(meetingWaitingRoomViewModel.meetingDate)
					.foregroundStyle(.white)
					.default_text_style_white(styleSize: 12)
				
				Spacer()
			}
			.frame(height: 40)
			.padding(.top, -25)
			.zIndex(1)
			
			if !telecomManager.callStarted {
				ZStack {
					VStack {
						Spacer()
						
						if meetingWaitingRoomViewModel.avatarDisplayed {
							ZStack {
								
								if meetingWaitingRoomViewModel.isRemoteDeviceTrusted {
									Circle()
										.fill(Color.blueInfo500)
										.frame(width: 206, height: 206)
								}
								
								if meetingWaitingRoomViewModel.avatarModel != nil {
									Avatar(contactAvatarModel: meetingWaitingRoomViewModel.avatarModel!, avatarSize: 200, hidePresence: true)
								} else {
									Image("profil-picture-default")
										.resizable()
										.frame(width: 200, height: 200)
										.clipShape(Circle())
								}
								
								if meetingWaitingRoomViewModel.isRemoteDeviceTrusted {
									VStack {
										Spacer()
										HStack {
											Image("trusted")
												.resizable()
												.frame(width: 25, height: 25)
												.padding(.all, 15)
											Spacer()
										}
									}
									.frame(width: 200, height: 200)
								}
							}
						}
						
						Spacer()
					}
					.frame(
						width:
							angleDegree == 0
						? 120 * ceil((geometry.size.width - 20) / 120)
						: 160 * ceil((geometry.size.height - 160) / 160),
						height:
							angleDegree == 0
						? 160 * ceil((geometry.size.width - 20) / 120)
						: 120 * ceil((geometry.size.height - 160) / 160)
					)
					
					LinphoneVideoViewHolder { view in
						coreContext.doOnCoreQueue { core in
							core.nativePreviewWindow = view
						}
					}
					.frame(
						width:
							angleDegree == 0
						? 120 * ceil((geometry.size.width - 20) / 120)
						: 160 * ceil((geometry.size.height - 160) / 160),
						height:
							angleDegree == 0
						? 160 * ceil((geometry.size.width - 20) / 120)
						: 120 * ceil((geometry.size.height - 160) / 160)
					)
					
					VStack {
						HStack {
							Spacer()
							
							if meetingWaitingRoomViewModel.videoDisplayed {
								Button {
									meetingWaitingRoomViewModel.switchCamera()
								} label: {
									Image("camera-rotate")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(.white)
										.frame(width: 30, height: 30)
								}
							}
						}
						
						Spacer()
						
						HStack {
							Text(meetingWaitingRoomViewModel.userName)
								.foregroundStyle(.white)
								.default_text_style_white(styleSize: 20)
							Spacer()
						}
					}
					.padding(.all, 10)
					.frame(maxWidth: geometry.size.width - 20, maxHeight: geometry.size.height - (angleDegree == 0 ? 250 : 160))
				}
				.background(Color.gray600)
				.frame(maxWidth: geometry.size.width - 20, maxHeight: geometry.size.height - (angleDegree == 0 ? 250 : 160))
				.cornerRadius(20)
				.padding(.horizontal, 10)
				.onDisappear {
					meetingWaitingRoomViewModel.disableVideoPreview()
				}
				
				if angleDegree != 0 {
					Spacer()
				}
				
				HStack {
					Spacer()
					
					Button {
						!meetingWaitingRoomViewModel.videoDisplayed
						? meetingWaitingRoomViewModel.enableVideoPreview() : meetingWaitingRoomViewModel.disableVideoPreview()
					} label: {
						HStack {
							Image(meetingWaitingRoomViewModel.videoDisplayed ? "video-camera" : "video-camera-slash")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(.white)
								.frame(width: 32, height: 32)
						}
					}
					.buttonStyle(PressedButtonStyle(buttonSize: 60))
					.frame(width: 60, height: 60)
					.background(Color.gray500)
					.cornerRadius(40)
					.padding(.horizontal, 5)
					
					Button {
						meetingWaitingRoomViewModel.toggleMuteMicrophone()
					} label: {
						HStack {
							Image(meetingWaitingRoomViewModel.micMutted ? "microphone-slash" : "microphone")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(.white)
								.frame(width: 32, height: 32)
						}
					}
					.buttonStyle(PressedButtonStyle(buttonSize: 60))
					.frame(width: 60, height: 60)
					.background(Color.gray500)
					.cornerRadius(40)
					.padding(.horizontal, 5)
					
					Button {
						if AVAudioSession.sharedInstance().availableInputs != nil
							&& !AVAudioSession.sharedInstance().availableInputs!.filter({ $0.portType.rawValue.contains("Bluetooth") }).isEmpty {
							
							audioRouteSheet = true
						} else {
							do {
								try AVAudioSession.sharedInstance().overrideOutputAudioPort(
									AVAudioSession.sharedInstance().currentRoute
										.outputs.filter({ $0.portType.rawValue == "Speaker" }).isEmpty ? .speaker : .none)
							} catch _ {}
						}
					} label: {
						HStack {
							Image(meetingWaitingRoomViewModel.imageAudioRoute)
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(.white)
								.frame(width: 32, height: 32)
								.onAppear(perform: getAudioRouteImage)
								.onReceive(pub) { _ in
									self.getAudioRouteImage()
								}
						}
					}
					.buttonStyle(PressedButtonStyle(buttonSize: 60))
					.frame(width: 60, height: 60)
					.background(Color.gray500)
					.cornerRadius(40)
					.padding(.horizontal, 5)
					
					Spacer()
					
					if angleDegree != 0 {
						Button(action: {
							meetingWaitingRoomViewModel.joinMeeting()
						}, label: {
							Text("meeting_waiting_room_join")
								.default_text_style_white_600(styleSize: 20)
								.frame(height: 35)
								.frame(maxWidth: .infinity)
						})
						.padding(.horizontal, 20)
						.padding(.vertical, 10)
						.background(Color.orangeMain500)
						.cornerRadius(60)
						.padding(.horizontal, 10)
						.frame(width: (geometry.size.width - 20) / 2)
					}
				}
				.padding(.all, 10)
				
				if angleDegree == 0 {
					Spacer()
					
					Button(action: {
						meetingWaitingRoomViewModel.joinMeeting()
					}, label: {
						Text("meeting_waiting_room_join")
							.default_text_style_white_600(styleSize: 20)
							.frame(height: 35)
							.frame(maxWidth: .infinity)
					})
					.padding(.horizontal, 20)
					.padding(.vertical, 10)
					.background(Color.orangeMain500)
					.cornerRadius(60)
					.padding(.bottom)
					.padding(.horizontal, 10)
					
				}
			} else {
				VStack {
					Spacer()
					
					Text("meeting_waiting_room_joining_title")
						.default_text_style_white_600(styleSize: 24)
						.multilineTextAlignment(.center)
						.padding(.bottom, 10)
					
					Text("meeting_waiting_room_joining_subtitle")
						.default_text_style_white(styleSize: 16)
						.multilineTextAlignment(.center)
						.padding(.bottom, 20)
					
					ActivityIndicator(color: Color.orangeMain500)
						.frame(width: 35, height: 35)
					
					Spacer()
					
					Button(action: {
						meetingWaitingRoomViewModel.cancelMeeting()
					}, label: {
						Text("meeting_waiting_room_cancel")
							.default_text_style_white_600(styleSize: 20)
							.frame(height: 35)
							.frame(maxWidth: .infinity)
					})
					.padding(.horizontal, 20)
					.padding(.vertical, 10)
					.background(Color.orangeMain500)
					.cornerRadius(60)
					.padding(.bottom)
					.padding(.horizontal, 10)
				}
			}
			
			Spacer()
		}
		.background(Color.gray900)
		.onRotate { newOrientation in
			orientation = newOrientation
			if orientation == .portrait || orientation == .portraitUpsideDown {
				angleDegree = 0
			} else {
				if orientation == .landscapeLeft {
					angleDegree = -90
				} else if orientation == .landscapeRight {
					angleDegree = 90
				} else if UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height {
					angleDegree = 90
				}
			}
			
			meetingWaitingRoomViewModel.orientationUpdate(orientation: orientation)
		}
		.onAppear {
			if orientation == .portrait || orientation == .portraitUpsideDown {
				angleDegree = 0
			} else {
				if orientation == .landscapeLeft {
					angleDegree = -90
				} else if orientation == .landscapeRight {
					angleDegree = 90
				} else if UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height {
					angleDegree = 90
				}
			}
			
			meetingWaitingRoomViewModel.orientationUpdate(orientation: orientation)
		}
	}
	
	@ViewBuilder
	func innerBottomSheet() -> some View {
		VStack(spacing: 0) {
			Button(action: {
				options = 1
				
				do {
					try AVAudioSession.sharedInstance().overrideOutputAudioPort(.none)
					if meetingWaitingRoomViewModel.isHeadPhoneAvailable() {
						try AVAudioSession.sharedInstance().setPreferredInput(AVAudioSession.sharedInstance()
							.availableInputs?.filter({ $0.portType.rawValue.contains("Receiver") }).first)
					} else {
						try AVAudioSession.sharedInstance().setPreferredInput(AVAudioSession.sharedInstance().availableInputs?.first)
					}
				} catch _ {
					
				}
			}, label: {
				HStack {
					Image(options == 1 ? "radio-button-fill" : "radio-button")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
					
					Text(!meetingWaitingRoomViewModel.isHeadPhoneAvailable() ? "call_audio_device_type_earpiece" : "call_audio_device_type_headphones")
						.default_text_style_white(styleSize: 15)
					
					Spacer()
					
					Image(!meetingWaitingRoomViewModel.isHeadPhoneAvailable() ? "ear" : "headset")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
				}
			})
			.frame(maxHeight: .infinity)
			
			Button(action: {
				options = 2
				
				do {
					try AVAudioSession.sharedInstance().overrideOutputAudioPort(.speaker)
				} catch _ {
					
				}
			}, label: {
				HStack {
					Image(options == 2 ? "radio-button-fill" : "radio-button")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
					
					Text("call_audio_device_type_speaker")
						.default_text_style_white(styleSize: 15)
					
					Spacer()
					
					Image("speaker-high")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
				}
			})
			.frame(maxHeight: .infinity)
			
			Button(action: {
				options = 3
				
				do {
					try AVAudioSession.sharedInstance().overrideOutputAudioPort(.none)
					try AVAudioSession.sharedInstance().setPreferredInput(AVAudioSession.sharedInstance().availableInputs?
						.filter({ $0.portType.rawValue.contains("Bluetooth") }).first)
				} catch _ {
					
				}
			}, label: {
				HStack {
					Image(options == 3 ? "radio-button-fill" : "radio-button")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
					
					Text(String(format: String(localized: "call_audio_device_type_bluetooth"),
								AVAudioSession.sharedInstance().currentRoute.outputs.first?.portName ?? ""))
						.default_text_style_white(styleSize: 15)
					
					Spacer()
					
					Image("bluetooth")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(.white)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
				}
			})
			.frame(maxHeight: .infinity)
		}
		.padding(.horizontal, 20)
		.background(Color.gray600)
		.frame(maxHeight: .infinity)
	}
	
	func getAudioRouteImage() {
		if !AVAudioSession.sharedInstance().currentRoute.outputs.filter({ $0.portType.rawValue == "Speaker" }).isEmpty {
			meetingWaitingRoomViewModel.imageAudioRoute = "speaker-high"
			options = 2
		} else if !AVAudioSession.sharedInstance().currentRoute.outputs.filter({ $0.portType.rawValue.contains("Bluetooth") }).isEmpty {
			meetingWaitingRoomViewModel.imageAudioRoute = "bluetooth"
			options = 3
		} else {
			meetingWaitingRoomViewModel.imageAudioRoute = meetingWaitingRoomViewModel.isHeadPhoneAvailable() ? "headset" : "speaker-slash"
			options = 1
		}
	}
}

#Preview {
	MeetingWaitingRoomFragment()
}
// swiftlint:enable type_body_length
// swiftlint:enable cyclomatic_complexity
