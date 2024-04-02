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
import CallKit
import AVFAudio
import linphonesw

// swiftlint:disable type_body_length
// swiftlint:disable line_length
struct CallView: View {
	
	@ObservedObject private var coreContext = CoreContext.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	@ObservedObject private var contactsManager = ContactsManager.shared
	
	@ObservedObject var callViewModel: CallViewModel
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	let pub = NotificationCenter.default.publisher(for: AVAudioSession.routeChangeNotification)
	
	@State var audioRouteSheet: Bool = false
	@State var options: Int = 1
	@State var imageAudioRoute: String = ""
	@State var angleDegree = 0.0
	@State var showingDialer = false
	@State var minBottomSheetHeight: CGFloat = 0.16
	@State var maxBottomSheetHeight: CGFloat = 0.5
	@State private var pointingUp: CGFloat = 0.0
	@State private var currentOffset: CGFloat = 0.0
	@State private var viewIsDisplayed = false
	
	@Binding var fullscreenVideo: Bool
	@Binding var isShowCallsListFragment: Bool
	@Binding var isShowStartCallFragment: Bool
	
	var body: some View {
		GeometryReader { geo in
			ZStack {
				if #available(iOS 16.4, *), idiom != .pad {
					innerView(geometry: geo)
						.sheet(isPresented: $audioRouteSheet, onDismiss: {
							audioRouteSheet = false
						}) {
							innerBottomSheet()
								.presentationDetents([.fraction(0.3)])
						}
						.sheet(isPresented: $showingDialer) {
							DialerBottomSheet(
								startCallViewModel: StartCallViewModel(),
								showingDialer: $showingDialer,
								currentCall: callViewModel.currentCall
							)
							.presentationDetents([.medium])
							.presentationBackgroundInteraction(.enabled(upThrough: .medium))
						}
				} else if #available(iOS 16.0, *), idiom != .pad {
					innerView(geometry: geo)
						.sheet(isPresented: $audioRouteSheet, onDismiss: {
							audioRouteSheet = false
						}) {
							innerBottomSheet()
								.presentationDetents([.fraction(0.3)])
						}
						.sheet(isPresented: $showingDialer) {
							DialerBottomSheet(
								startCallViewModel: StartCallViewModel(),
								showingDialer: $showingDialer,
								currentCall: callViewModel.currentCall
							)
							.presentationDetents([.medium])
						}
				} else {
					innerView(geometry: geo)
						.halfSheet(showSheet: $audioRouteSheet) {
							innerBottomSheet()
						} onDismiss: {
							audioRouteSheet = false
						}
						.halfSheet(showSheet: $showingDialer) {
							DialerBottomSheet(
								startCallViewModel: StartCallViewModel(),
								showingDialer: $showingDialer,
								currentCall: callViewModel.currentCall
							)
						} onDismiss: {}
				}
				
				if isShowCallsListFragment {
					CallsListFragment(callViewModel: callViewModel, isShowCallsListFragment: $isShowCallsListFragment)
					.zIndex(4)
					.transition(.move(edge: .bottom))
				}
				
				if callViewModel.zrtpPopupDisplayed == true {
					ZRTPPopup(callViewModel: callViewModel)
						.background(.black.opacity(0.65))
						.onTapGesture {
							callViewModel.zrtpPopupDisplayed = false
						}
				}
				
				if telecomManager.remainingCall {
					HStack {}
					.onAppear {
						callViewModel.resetCallView()
						callViewModel.getCallsList()
					}
				}
			}
			.onAppear {
				callViewModel.enableAVAudioSession()
			}
			.onDisappear {
				callViewModel.disableAVAudioSession()
			}
		}
	}
	
	@ViewBuilder
	func innerBottomSheet() -> some View {
		VStack(spacing: 0) {
			Button(action: {
				options = 1
				
				do {
					try AVAudioSession.sharedInstance().overrideOutputAudioPort(.none)
					if callViewModel.isHeadPhoneAvailable() {
						try AVAudioSession.sharedInstance().setPreferredInput(AVAudioSession.sharedInstance().availableInputs?.filter({ $0.portType.rawValue.contains("Receiver") }).first)
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
					
					Text(!callViewModel.isHeadPhoneAvailable() ? "Earpiece" : "Headphones")
						.default_text_style_white(styleSize: 15)
					
					Spacer()
					
					Image(!callViewModel.isHeadPhoneAvailable() ? "ear" : "headset")
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
					
					Text("Speaker")
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
					try AVAudioSession.sharedInstance().setPreferredInput(AVAudioSession.sharedInstance().availableInputs?.filter({ $0.portType.rawValue.contains("Bluetooth") }).first)
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
					
					Text("Bluetooth")
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
	
	@ViewBuilder
	// swiftlint:disable:next cyclomatic_complexity
	func innerView(geometry: GeometryProxy) -> some View {
		ZStack {
			VStack {
				if !fullscreenVideo || (fullscreenVideo && telecomManager.isPausedByRemote) {
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
					ZStack {
						HStack {
							Button {
								withAnimation {
									telecomManager.callDisplayed = false
								}
							} label: {
								Image("caret-left")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(.white)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
							}
							
							Text(callViewModel.displayName)
							 .default_text_style_white_800(styleSize: 16)
							
							if !telecomManager.outgoingCallStarted && telecomManager.callInProgress {
								Text("|")
									.default_text_style_white_800(styleSize: 16)
								
								ZStack {
									Text(callViewModel.timeElapsed.convertDurationToString())
										.onReceive(callViewModel.timer) { _ in
											callViewModel.timeElapsed = callViewModel.currentCall?.duration ?? 0
										}
										.default_text_style_white_800(styleSize: 16)
										.if(callViewModel.isPaused || telecomManager.isPausedByRemote) { view in
											view.hidden()
										}
									
									if callViewModel.isPaused {
										Text("Paused")
											.default_text_style_white_800(styleSize: 16)
									} else if telecomManager.isPausedByRemote {
										Text("Paused by remote")
											.default_text_style_white_800(styleSize: 16)
									}
								}
							}
							
							Spacer()
							
							Button {
							} label: {
								Image("cell-signal-full")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(.white)
									.frame(width: 30, height: 30)
									.padding(.all, 10)
							}
							
							if !callViewModel.isConference && telecomManager.remoteVideo {
									Button {
										callViewModel.switchCamera()
									} label: {
										Image("camera-rotate")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(.white)
											.frame(width: 30, height: 30)
											.padding(.horizontal)
									}
							} else if callViewModel.isConference && callViewModel.videoDisplayed {
								Button {
									callViewModel.switchCamera()
								} label: {
									Image("camera-rotate")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(.white)
										.frame(width: 30, height: 30)
										.padding(.horizontal)
								}
							}
						}
						.frame(height: 40)
						.zIndex(1)
						
						if callViewModel.isMediaEncrypted {
							HStack {
								Image("lock_simple")
									.resizable()
									.frame(width: 15, height: 15, alignment: .leading)
									.padding(.leading, 50)
									.padding(.top, 35)
								
								Text("Appel chiffré de bout en bout")
									.foregroundStyle(Color.blueInfo500)
									.default_text_style_white(styleSize: 12)
									.padding(.top, 35)
								
								Spacer()
							}
							.onTapGesture {
								callViewModel.showZrtpSasDialogIfPossible()
							}
							.frame(height: 40)
						 	.zIndex(1)
						}
					}
				}
				
				simpleCallView(geometry: geometry)
				
				Spacer()
			}
			.frame(height: geometry.size.height)
			.frame(maxWidth: .infinity)
			.background(Color.gray900)
			.if(fullscreenVideo && !telecomManager.isPausedByRemote) { view in
				view.ignoresSafeArea(.all)
			}
			
			if !fullscreenVideo || (fullscreenVideo && telecomManager.isPausedByRemote) {
				if telecomManager.callStarted {
					let scene = UIApplication.shared.connectedScenes.first as? UIWindowScene
					let bottomInset = scene?.windows.first?.safeAreaInsets
					
					BottomSheetView(
						content: bottomSheetContent(geo: geometry),
						minHeight: (minBottomSheetHeight * geometry.size.height > 80 ? minBottomSheetHeight * geometry.size.height : 78),
						maxHeight: (maxBottomSheetHeight * geometry.size.height),
						currentOffset: $currentOffset,
						pointingUp: $pointingUp,
						bottomSafeArea: bottomInset?.bottom ?? 0
					)
					.onAppear {
						currentOffset = (minBottomSheetHeight * geometry.size.height > 80 ? minBottomSheetHeight * geometry.size.height : 78)
						pointingUp = -(((currentOffset - (minBottomSheetHeight * geometry.size.height > 80 ? minBottomSheetHeight * geometry.size.height : 78)) / ((maxBottomSheetHeight * geometry.size.height) - (minBottomSheetHeight * geometry.size.height > 80 ? minBottomSheetHeight * geometry.size.height : 78))) - 0.5) * 2
					}
					.edgesIgnoringSafeArea(.bottom)
				}
			}
		}
	}
	
	// swiftlint:disable:next cyclomatic_complexity
	func simpleCallView(geometry: GeometryProxy) -> some View {
		ZStack {
			if !callViewModel.isConference {
				VStack {
					Spacer()
					ZStack {
						
						if callViewModel.isRemoteDeviceTrusted {
							Circle()
								.fill(Color.blueInfo500)
								.frame(width: 206, height: 206)
						}
						
						if callViewModel.remoteAddress != nil {
							let addressFriend = contactsManager.getFriendWithAddress(address: callViewModel.remoteAddress!)
							
							let contactAvatarModel = addressFriend != nil
							? ContactsManager.shared.avatarListModel.first(where: {
								($0.friend!.consolidatedPresence == .Online || $0.friend!.consolidatedPresence == .Busy)
								&& $0.friend!.name == addressFriend!.name
								&& $0.friend!.address!.asStringUriOnly() == addressFriend!.address!.asStringUriOnly()
							})
							: ContactAvatarModel(friend: nil, name: "", withPresence: false)
							
							if addressFriend != nil && addressFriend!.photo != nil && !addressFriend!.photo!.isEmpty {
								if contactAvatarModel != nil {
									Avatar(contactAvatarModel: contactAvatarModel!, avatarSize: 200, hidePresence: true)
								}
							} else {
								if callViewModel.remoteAddress!.displayName != nil {
									Image(uiImage: contactsManager.textToImage(
										firstName: callViewModel.remoteAddress!.displayName!,
										lastName: callViewModel.remoteAddress!.displayName!.components(separatedBy: " ").count > 1
										? callViewModel.remoteAddress!.displayName!.components(separatedBy: " ")[1]
										: ""))
									.resizable()
									.frame(width: 200, height: 200)
									.clipShape(Circle())
									
								} else {
									Image(uiImage: contactsManager.textToImage(
										firstName: callViewModel.remoteAddress!.username ?? "Username Error",
										lastName: callViewModel.remoteAddress!.username!.components(separatedBy: " ").count > 1
										? callViewModel.remoteAddress!.username!.components(separatedBy: " ")[1]
										: ""))
									.resizable()
									.frame(width: 200, height: 200)
									.clipShape(Circle())
								}
								
							}
						} else {
							Image("profil-picture-default")
								.resizable()
								.frame(width: 200, height: 200)
								.clipShape(Circle())
						}
						
						if callViewModel.isRemoteDeviceTrusted {
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
					
					Text(callViewModel.displayName)
						.padding(.top)
						.default_text_style_white(styleSize: 22)
					
					Text(callViewModel.remoteAddressString)
						.default_text_style_white_300(styleSize: 16)
					
					Spacer()
				}
			} else {
				VStack {
					Spacer()
					ZStack {
						if callViewModel.activeSpeakerParticipant?.address != nil {
							let addressFriend = contactsManager.getFriendWithAddress(address: callViewModel.activeSpeakerParticipant!.address)
							
							let contactAvatarModel = addressFriend != nil
							? ContactsManager.shared.avatarListModel.first(where: {
								($0.friend!.consolidatedPresence == .Online || $0.friend!.consolidatedPresence == .Busy)
								&& $0.friend!.name == addressFriend!.name
								&& $0.friend!.address!.asStringUriOnly() == addressFriend!.address!.asStringUriOnly()
							})
							: ContactAvatarModel(friend: nil, name: "", withPresence: false)
							
							if addressFriend != nil && addressFriend!.photo != nil && !addressFriend!.photo!.isEmpty {
								if contactAvatarModel != nil {
									Avatar(contactAvatarModel: contactAvatarModel!, avatarSize: 200, hidePresence: true)
								}
							} else {
								if callViewModel.activeSpeakerParticipant!.address.displayName != nil {
									Image(uiImage: contactsManager.textToImage(
										firstName: callViewModel.activeSpeakerParticipant!.address.displayName!,
										lastName: callViewModel.activeSpeakerParticipant!.address.displayName!.components(separatedBy: " ").count > 1
										? callViewModel.activeSpeakerParticipant!.address.displayName!.components(separatedBy: " ")[1]
										: ""))
									.resizable()
									.frame(width: 200, height: 200)
									.clipShape(Circle())
									
								} else {
									Image(uiImage: contactsManager.textToImage(
										firstName: callViewModel.activeSpeakerParticipant!.address.username ?? "Username Error",
										lastName: callViewModel.activeSpeakerParticipant!.address.username!.components(separatedBy: " ").count > 1
										? callViewModel.activeSpeakerParticipant!.address.username!.components(separatedBy: " ")[1]
										: ""))
									.resizable()
									.frame(width: 200, height: 200)
									.clipShape(Circle())
								}
								
							}
						} else {
							Image("profil-picture-default")
								.resizable()
								.frame(width: 200, height: 200)
								.clipShape(Circle())
						}
					}
					
					Spacer()
				}
			}
			
			if !callViewModel.isConference {
				LinphoneVideoViewHolder { view in
					coreContext.doOnCoreQueue { core in
						core.nativeVideoWindowId = UnsafeMutableRawPointer(Unmanaged.passRetained(view).toOpaque())
					}
				}
				.frame(
					width:
						angleDegree == 0
					? (fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.width : geometry.size.width - 8)
					: (fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.height + geometry.safeAreaInsets.top + geometry.safeAreaInsets.bottom : geometry.size.height - (minBottomSheetHeight * geometry.size.height > 80 ? minBottomSheetHeight * geometry.size.height : 78) - 40 - 20 + geometry.safeAreaInsets.bottom),
					height:
						angleDegree == 0
					? (fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.height + geometry.safeAreaInsets.top + geometry.safeAreaInsets.bottom : geometry.size.height - (minBottomSheetHeight * geometry.size.height > 80 ? minBottomSheetHeight * geometry.size.height : 78) - 40 - 20 + geometry.safeAreaInsets.bottom)
					: (fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.width : geometry.size.width - 8)
				)
				.scaledToFill()
				.clipped()
				.onTapGesture {
					if telecomManager.remoteVideo {
						fullscreenVideo.toggle()
					}
				}
				
				if telecomManager.remoteVideo {
					HStack {
						Spacer()
						VStack {
							Spacer()
							LinphoneVideoViewHolder { view in
								coreContext.doOnCoreQueue { core in
									core.nativePreviewWindow = view
								}
							}
							.frame(width: angleDegree == 0 ? 120*1.2 : 160*1.2, height: angleDegree == 0 ? 160*1.2 : 120*1.2)
							.cornerRadius(20)
							.padding(10)
							.padding(.trailing, abs(angleDegree/2))
						}
					}
					.frame(
						maxWidth: fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.width : geometry.size.width - 8,
						maxHeight: fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.height + geometry.safeAreaInsets.top + geometry.safeAreaInsets.bottom : geometry.size.height - (minBottomSheetHeight * geometry.size.height > 80 ? minBottomSheetHeight * geometry.size.height : 78) - 40 - 20 + geometry.safeAreaInsets.bottom
					)
				}
			} else {
				/*
				if !viewIsDisplayed {
					VStack {
						
					}
					.onAppear {
						DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
							viewIsDisplayed = true
						}
					}
				}
				
				if viewIsDisplayed && (callViewModel.receiveVideo || telecomManager.remoteConfVideo) {
				 */
				if (callViewModel.receiveVideo || telecomManager.remoteConfVideo) && !telecomManager.outgoingCallStarted {
					LinphoneVideoViewHolder { view in
						coreContext.doOnCoreQueue { core in
							//core.nativeVideoWindow = view
							core.nativeVideoWindowId = UnsafeMutableRawPointer(Unmanaged.passRetained(view).toOpaque())
						}
					}
					.frame(
						width:
							angleDegree == 0
						? (fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.width : geometry.size.width - 8)
						: (fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.height + geometry.safeAreaInsets.top + geometry.safeAreaInsets.bottom : geometry.size.height - (minBottomSheetHeight * geometry.size.height > 80 ? minBottomSheetHeight * geometry.size.height : 78) - 40 - 20 + geometry.safeAreaInsets.bottom),
						height:
							angleDegree == 0
						? (fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.height + geometry.safeAreaInsets.top + geometry.safeAreaInsets.bottom : geometry.size.height - (minBottomSheetHeight * geometry.size.height > 80 ? minBottomSheetHeight * geometry.size.height : 78) - 40 - 20 + geometry.safeAreaInsets.bottom)
						: (fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.width : geometry.size.width - 8)
					)
					.scaledToFill()
					.clipped()
					.onTapGesture {
						if telecomManager.remoteVideo {
							fullscreenVideo.toggle()
						}
					}
					
					/*
					HStack {
						Spacer()
						VStack {
							Spacer()
							LinphoneVideoViewHolder { view in
								coreContext.doOnCoreQueue { core in
									core.nativePreviewWindow = view
								}
							}
							.frame(width: angleDegree == 0 ? 120*1.2 : 160*1.2, height: angleDegree == 0 ? 160*1.2 : 120*1.2)
							.cornerRadius(20)
							.padding(10)
							.padding(.trailing, abs(angleDegree/2))
						}
					}
					.frame(
						maxWidth: fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.width : geometry.size.width - 8,
						maxHeight: fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.height + geometry.safeAreaInsets.top + geometry.safeAreaInsets.bottom : geometry.size.height - (minBottomSheetHeight * geometry.size.height > 80 ? minBottomSheetHeight * geometry.size.height : 78) - 40 - 20 + geometry.safeAreaInsets.bottom
					)
					 */
				}
				
				if !telecomManager.outgoingCallStarted {
					HStack {
						Spacer()
						VStack {
							Spacer()
							ScrollView(.horizontal) {
								HStack {
									ZStack {
										VStack {
											Spacer()
											
											if callViewModel.myParticipantModel != nil {
												Avatar(contactAvatarModel: callViewModel.myParticipantModel!.avatarModel, avatarSize: 50)
											}
											
											Spacer()
										}
										
										VStack(alignment: .leading) {
											Spacer()
											
											if callViewModel.myParticipantModel != nil {
												Text(callViewModel.myParticipantModel!.name)
													.frame(maxWidth: .infinity, alignment: .leading)
													.foregroundStyle(Color.white)
													.default_text_style_500(styleSize: 14)
													.lineLimit(1)
													.padding(.horizontal, 10)
													.padding(.bottom, 6)
											}
										}
										.frame(maxWidth: .infinity)
										
										LinphoneVideoViewHolder { view in
											coreContext.doOnCoreQueue { core in
												core.nativePreviewWindow = view
											}
										}
									}
									.frame(width: 140, height: 140)
									.background(Color.gray600)
									.cornerRadius(20)
									
									ForEach(0..<callViewModel.participantList.count, id: \.self) { index in
										ZStack {
											VStack {
												Spacer()
												
												Avatar(contactAvatarModel: callViewModel.participantList[index].avatarModel, avatarSize: 50)
												
												Spacer()
											}
											
											VStack(alignment: .leading) {
												Spacer()
												
												Text(callViewModel.participantList[index].name)
													.frame(maxWidth: .infinity, alignment: .leading)
													.foregroundStyle(Color.white)
													.default_text_style_500(styleSize: 14)
													.lineLimit(1)
													.padding(.horizontal, 10)
													.padding(.bottom, 6)
											}
											.frame(maxWidth: .infinity)
										}
										.frame(width: 140, height: 140)
										.background(Color.gray600)
										.cornerRadius(20)
									}
								}
							}
						}
					}
					.frame(
						maxWidth: fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.width : geometry.size.width - 8,
						maxHeight: fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.height + geometry.safeAreaInsets.top + geometry.safeAreaInsets.bottom : geometry.size.height - (minBottomSheetHeight * geometry.size.height > 80 ? minBottomSheetHeight * geometry.size.height : 78) - 40 - 20 + geometry.safeAreaInsets.bottom
					)
					.padding(.bottom, 10)
				}
			}
			
			if callViewModel.isRecording {
				HStack {
					VStack {
						Image("record-fill")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.redDanger500)
							.frame(width: 32, height: 32)
							.padding(10)
							.if(fullscreenVideo && !telecomManager.isPausedByRemote) { view in
								view.padding(.top, 30)
							}
						Spacer()
					}
					Spacer()
				}
				.frame(
					maxWidth: fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.width : geometry.size.width - 8,
					maxHeight: fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.height + geometry.safeAreaInsets.top + geometry.safeAreaInsets.bottom : geometry.size.height - (minBottomSheetHeight * geometry.size.height > 80 ? minBottomSheetHeight * geometry.size.height : 78) - 40 - 20 + geometry.safeAreaInsets.bottom
				)
			}
			
			if telecomManager.outgoingCallStarted {
				VStack {
					ActivityIndicator()
						.frame(width: 20, height: 20)
						.padding(.top, 60)
					
					Text(callViewModel.counterToMinutes())
						.onAppear {
							callViewModel.timeElapsed = 0
						}
						.onReceive(callViewModel.timer) { _ in
							callViewModel.timeElapsed = callViewModel.currentCall?.duration ?? 0
							
						}
						.onDisappear {
							callViewModel.timeElapsed = 0
						}
						.padding(.top)
						.foregroundStyle(.white)
					
					Spacer()
				}
				.background(.clear)
				.frame(
					maxWidth: fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.width : geometry.size.width - 8,
					maxHeight: fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.height + geometry.safeAreaInsets.top + geometry.safeAreaInsets.bottom : geometry.size.height - (minBottomSheetHeight * geometry.size.height > 80 ? minBottomSheetHeight * geometry.size.height : 78) - 40 - 20 + geometry.safeAreaInsets.bottom
				)
			}
		}
		.frame(
			maxWidth: fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.width : geometry.size.width - 8,
			maxHeight: fullscreenVideo && !telecomManager.isPausedByRemote ? geometry.size.height + geometry.safeAreaInsets.top + geometry.safeAreaInsets.bottom : geometry.size.height - (minBottomSheetHeight * geometry.size.height > 80 ? minBottomSheetHeight * geometry.size.height : 78) - 40 - 20 + geometry.safeAreaInsets.bottom
		)
		.background(Color.gray900)
		.cornerRadius(20)
		.padding(.horizontal, fullscreenVideo && !telecomManager.isPausedByRemote ? 0 : 4)
		.onRotate { newOrientation in
			let oldOrientation = orientation
			orientation = newOrientation
			if orientation == .portrait || orientation == .portraitUpsideDown {
				angleDegree = 0
			} else {
				if orientation == .landscapeLeft {
					angleDegree = -90
				} else if orientation == .landscapeRight {
					angleDegree = 90
				}
			}
			
			if (oldOrientation != orientation && oldOrientation != .faceUp) || (oldOrientation == .faceUp && (orientation == .landscapeLeft || orientation == .landscapeRight)) {
				telecomManager.callStarted = false
				
				DispatchQueue.global().asyncAfter(deadline: .now() + 0.5) {
					telecomManager.callStarted = true
				}
			}
			
			callViewModel.orientationUpdate(orientation: orientation)
		}
		.onAppear {
			if orientation == .portrait && orientation == .portraitUpsideDown {
				angleDegree = 0
			} else {
				if orientation == .landscapeLeft {
					angleDegree = -90
				} else if orientation == .landscapeRight {
					angleDegree = 90
				}
			}
			
			telecomManager.callStarted = false
			
			DispatchQueue.global().asyncAfter(deadline: .now() + 0.5) {
				telecomManager.callStarted = true
			}
			
			callViewModel.orientationUpdate(orientation: orientation)
		}
	}
	
	// swiftlint:disable function_body_length
	func bottomSheetContent(geo: GeometryProxy) -> some View {
		GeometryReader { _ in
			VStack(spacing: 0) {
				Button {
					withAnimation {
						if currentOffset < (maxBottomSheetHeight * geo.size.height) {
							currentOffset = (maxBottomSheetHeight * geo.size.height)
						} else {
							currentOffset = (minBottomSheetHeight * geo.size.height > 80 ? minBottomSheetHeight * geo.size.height : 78)
						}
						
						pointingUp = -(((currentOffset - (minBottomSheetHeight * geo.size.height > 80 ? minBottomSheetHeight * geo.size.height : 78)) / ((maxBottomSheetHeight * geo.size.height) - (minBottomSheetHeight * geo.size.height > 80 ? minBottomSheetHeight * geo.size.height : 78))) - 0.5) * 2
					}
				} label: {
					ChevronShape(pointingUp: pointingUp)
						.stroke(style: StrokeStyle(lineWidth: 4, lineCap: .round))
						.frame(width: 40, height: 6)
						.foregroundStyle(.white)
						.contentShape(Rectangle())
						.padding(.top, 15)
				}
				
				HStack(spacing: 12) {
					Button {
						callViewModel.terminateCall()
					} label: {
						Image("phone-disconnect")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(.white)
							.frame(width: 32, height: 32)
						
					}
					.frame(width: 90, height: 60)
					.background(Color.redDanger500)
					.cornerRadius(40)
					
					Spacer()
					
					if !callViewModel.isConference {
						Button {
							callViewModel.toggleVideo()
						} label: {
							HStack {
								Image(telecomManager.remoteVideo ? "video-camera" : "video-camera-slash")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle((callViewModel.isPaused || telecomManager.isPausedByRemote) ? Color.gray500 : .white)
									.frame(width: 32, height: 32)
							}
						}
						.buttonStyle(PressedButtonStyle())
						.frame(width: 60, height: 60)
						.background((callViewModel.isPaused || telecomManager.isPausedByRemote) ? .white : Color.gray500)
						.cornerRadius(40)
						.disabled(callViewModel.isPaused || telecomManager.isPausedByRemote)
					} else {
						Button {
							callViewModel.displayMyVideo()
						} label: {
							HStack {
								Image(callViewModel.videoDisplayed ? "video-camera" : "video-camera-slash")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle((callViewModel.isPaused || telecomManager.isPausedByRemote) ? Color.gray500 : .white)
									.frame(width: 32, height: 32)
							}
						}
						.buttonStyle(PressedButtonStyle())
						.frame(width: 60, height: 60)
						.background((callViewModel.isPaused || telecomManager.isPausedByRemote) ? .white : Color.gray500)
						.cornerRadius(40)
						.disabled(callViewModel.isPaused || telecomManager.isPausedByRemote)
					}
					
					Button {
						callViewModel.toggleMuteMicrophone()
					} label: {
						HStack {
							Image(callViewModel.micMutted ? "microphone-slash" : "microphone")
							 .renderingMode(.template)
							 .resizable()
							 .foregroundStyle(.white)
							 .frame(width: 32, height: 32)
						}
					}
					.buttonStyle(PressedButtonStyle())
					.frame(width: 60, height: 60)
					.background(callViewModel.micMutted ? Color.redDanger500 : Color.gray500)
					.cornerRadius(40)
					
					Button {
						if AVAudioSession.sharedInstance().availableInputs != nil
							&& !AVAudioSession.sharedInstance().availableInputs!.filter({ $0.portType.rawValue.contains("Bluetooth") }).isEmpty {
							
							DispatchQueue.global().asyncAfter(deadline: .now() + 0.5) {
								audioRouteSheet = true
							}
						} else {
							do {
								try AVAudioSession.sharedInstance().overrideOutputAudioPort(AVAudioSession.sharedInstance().currentRoute.outputs.filter({ $0.portType.rawValue == "Speaker" }).isEmpty ? .speaker : .none)
							} catch _ {
								
							}
						}
						
					} label: {
						HStack {
							Image(imageAudioRoute)
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
					.buttonStyle(PressedButtonStyle())
					.frame(width: 60, height: 60)
					.background(Color.gray500)
					.cornerRadius(40)
				}
				.frame(height: geo.size.height * 0.15)
				.padding(.horizontal, 20)
				.padding(.top, -12)
				
				if orientation != .landscapeLeft && orientation != .landscapeRight {
					HStack(spacing: 0) {
						if !callViewModel.isConference {
							VStack {
								Button {
									if callViewModel.calls.count < 2 {
										withAnimation {
											callViewModel.isTransferInsteadCall = true
											MagicSearchSingleton.shared.searchForSuggestions()
											isShowStartCallFragment.toggle()
										}
									} else {
										callViewModel.transferClicked()
									}
								} label: {
									HStack {
										Image("phone-transfer")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(.white)
											.frame(width: 32, height: 32)
									}
								}
								.buttonStyle(PressedButtonStyle())
								.frame(width: 60, height: 60)
								.background(Color.gray500)
								.cornerRadius(40)
								
								Text(callViewModel.calls.count < 2 ? "Transfer" : "Attended transfer")
									.foregroundStyle(.white)
									.default_text_style(styleSize: 15)
							}
							.frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
							
							VStack {
								Button {
									withAnimation {
										MagicSearchSingleton.shared.searchForSuggestions()
										isShowStartCallFragment.toggle()
									}
								} label: {
									HStack {
										Image("phone-plus")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(.white)
											.frame(width: 32, height: 32)
									}
								}
								.buttonStyle(PressedButtonStyle())
								.frame(width: 60, height: 60)
								.background(Color.gray500)
								.cornerRadius(40)
								
								Text("New call")
									.foregroundStyle(.white)
									.default_text_style(styleSize: 15)
							}
							.frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
						} else {
							VStack {
								Button {
								} label: {
									HStack {
										Image("screencast")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.gray500)
											.frame(width: 32, height: 32)
									}
								}
								.buttonStyle(PressedButtonStyle())
								.frame(width: 60, height: 60)
								.background(.white)
								.cornerRadius(40)
								.disabled(true)
								
								Text("Partage d'écran")
									.foregroundStyle(.white)
									.default_text_style(styleSize: 15)
							}
							.frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
							
							VStack {
								Button {
								} label: {
									HStack {
										Image("users")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(.white)
											.frame(width: 32, height: 32)
									}
								}
								.buttonStyle(PressedButtonStyle())
								.frame(width: 60, height: 60)
								.background(Color.gray500)
								.cornerRadius(40)
								
								Text("Participants")
									.foregroundStyle(.white)
									.default_text_style(styleSize: 15)
							}
							.frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
						}
						VStack {
							ZStack {
								Button {
									callViewModel.getCallsList()
									withAnimation {
										isShowCallsListFragment.toggle()
									}
								} label: {
									HStack {
										Image("phone-list")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(.white)
											.frame(width: 32, height: 32)
									}
								}
								.buttonStyle(PressedButtonStyle())
								.frame(width: 60, height: 60)
								.background(Color.gray500)
								.cornerRadius(40)
								
								if callViewModel.calls.count > 1 {
									VStack {
										HStack {
											Spacer()
											
											VStack {
												Text("\(callViewModel.calls.count)")
													.foregroundStyle(.white)
													.default_text_style(styleSize: 15)
											}
											.frame(width: 20, height: 20)
											.background(Color.redDanger500)
											.cornerRadius(10)
										}
										
										Spacer()
									}
									.frame(width: 60, height: 60)
								}
							}
							
							Text("Call list")
								.foregroundStyle(.white)
								.default_text_style(styleSize: 15)
						}
						.frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
						
						VStack {
							Button {
								showingDialer.toggle()
							} label: {
								HStack {
									Image("dialer")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(.white)
										.frame(width: 32, height: 32)
								}
							}
							.buttonStyle(PressedButtonStyle())
							.frame(width: 60, height: 60)
							.background(Color.gray500)
							.cornerRadius(40)
							
							Text("Dialer")
								.foregroundStyle(.white)
								.default_text_style(styleSize: 15)
						}
						.frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
					}
					.frame(height: geo.size.height * 0.15)
					
					HStack(spacing: 0) {
						VStack {
							Button {
							} label: {
								HStack {
									Image("chat-teardrop-text")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.gray500)
										.frame(width: 32, height: 32)
								}
							}
							.buttonStyle(PressedButtonStyle())
							.frame(width: 60, height: 60)
							.background(.white)
							.cornerRadius(40)
							.disabled(true)
							
							Text("Messages")
								.foregroundStyle(.white)
								.default_text_style(styleSize: 15)
						}
						.frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
						
						VStack {
							Button {
								callViewModel.togglePause()
							} label: {
								HStack {
									Image(callViewModel.isPaused ? "play" : "pause")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(telecomManager.isPausedByRemote ? Color.gray500 : .white)
										.frame(width: 32, height: 32)
								}
							}
							.buttonStyle(PressedButtonStyle())
							.frame(width: 60, height: 60)
							.background(telecomManager.isPausedByRemote ? .white : (callViewModel.isPaused ? Color.greenSuccess500 : Color.gray500))
							.cornerRadius(40)
							.disabled(telecomManager.isPausedByRemote)
							
							Text("Pause")
								.foregroundStyle(.white)
								.default_text_style(styleSize: 15)
						}
						.frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
						
						VStack {
							Button {
								callViewModel.toggleRecording()
							} label: {
								HStack {
									Image("record-fill")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle((callViewModel.isPaused || telecomManager.isPausedByRemote) ? Color.gray500 : .white)
										.frame(width: 32, height: 32)
								}
							}
							.buttonStyle(PressedButtonStyle())
							.frame(width: 60, height: 60)
							.background((callViewModel.isPaused || telecomManager.isPausedByRemote) ? .white : (callViewModel.isRecording ? Color.redDanger500 : Color.gray500))
							.cornerRadius(40)
							.disabled(callViewModel.isPaused || telecomManager.isPausedByRemote)
							
							Text("Record")
								.foregroundStyle(.white)
								.default_text_style(styleSize: 15)
						}
						.frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
						
						VStack {
							Button {
							} label: {
								HStack {
									Image("video-camera")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(.white)
										.frame(width: 32, height: 32)
								}
							}
							.buttonStyle(PressedButtonStyle())
							.frame(width: 60, height: 60)
							.background(Color.gray500)
							.cornerRadius(40)
							
							Text("Disposition")
								.foregroundStyle(.white)
								.default_text_style(styleSize: 15)
						}
						.frame(width: geo.size.width * 0.25, height: geo.size.width * 0.25)
						.hidden()
					}
					.frame(height: geo.size.height * 0.15)
				} else {
					HStack {
						VStack {
							Button {
								withAnimation {
									callViewModel.isTransferInsteadCall = true
									MagicSearchSingleton.shared.searchForSuggestions()
									isShowStartCallFragment.toggle()
								}
							} label: {
								HStack {
									Image("phone-transfer")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(.white)
										.frame(width: 32, height: 32)
								}
							}
							.buttonStyle(PressedButtonStyle())
							.frame(width: 60, height: 60)
							.background(Color.gray500)
							.cornerRadius(40)
							
							Text(callViewModel.calls.count < 2 ? "Transfer" : "Attended transfer")
								.foregroundStyle(.white)
								.default_text_style(styleSize: 15)
						}
						.frame(width: geo.size.width * 0.125, height: geo.size.width * 0.125)
						
						VStack {
							Button {
								withAnimation {
									MagicSearchSingleton.shared.searchForSuggestions()
									isShowStartCallFragment.toggle()
								}
							} label: {
								HStack {
									Image("phone-plus")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(.white)
										.frame(width: 32, height: 32)
								}
							}
							.buttonStyle(PressedButtonStyle())
							.frame(width: 60, height: 60)
							.background(Color.gray500)
							.cornerRadius(40)
							
							Text("New call")
								.foregroundStyle(.white)
								.default_text_style(styleSize: 15)
						}
						.frame(width: geo.size.width * 0.125, height: geo.size.width * 0.125)
						
						VStack {
							ZStack {
								Button {
								} label: {
									HStack {
										Image("phone-list")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(.white)
											.frame(width: 32, height: 32)
									}
								}
								.buttonStyle(PressedButtonStyle())
								.frame(width: 60, height: 60)
								.background(Color.gray500)
								.cornerRadius(40)
								
								if callViewModel.calls.count > 1 {
									VStack {
										HStack {
											Spacer()
											
											VStack {
												Text("\(callViewModel.calls.count)")
													.foregroundStyle(.white)
													.default_text_style(styleSize: 15)
											}
											.frame(width: 20, height: 20)
											.background(Color.redDanger500)
											.cornerRadius(10)
										}
										
										Spacer()
									}
									.frame(width: 60, height: 60)
								}
							}
							
							Text("Call list")
								.foregroundStyle(.white)
								.default_text_style(styleSize: 15)
						}
						.frame(width: geo.size.width * 0.125, height: geo.size.width * 0.125)
						
						VStack {
							Button {
								showingDialer.toggle()
							} label: {
								HStack {
									Image("dialer")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(.white)
										.frame(width: 32, height: 32)
								}
							}
							.buttonStyle(PressedButtonStyle())
							.frame(width: 60, height: 60)
							.background(Color.gray500)
							.cornerRadius(40)
							
							Text("Dialer")
								.foregroundStyle(.white)
								.default_text_style(styleSize: 15)
						}
						.frame(width: geo.size.width * 0.125, height: geo.size.width * 0.125)
						
						VStack {
							Button {
							} label: {
								HStack {
									Image("chat-teardrop-text")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.gray500)
										.frame(width: 32, height: 32)
								}
							}
							.buttonStyle(PressedButtonStyle())
							.frame(width: 60, height: 60)
							.background(.white)
							.cornerRadius(40)
							.disabled(true)
							
							Text("Messages")
								.foregroundStyle(.white)
								.default_text_style(styleSize: 15)
						}
						.frame(width: geo.size.width * 0.125, height: geo.size.width * 0.125)
						
						VStack {
							Button {
								callViewModel.togglePause()
							} label: {
								HStack {
									Image(callViewModel.isPaused ? "play" : "pause")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(telecomManager.isPausedByRemote ? Color.gray500 : .white)
										.frame(width: 32, height: 32)
								}
							}
							.buttonStyle(PressedButtonStyle())
							.frame(width: 60, height: 60)
							.background(telecomManager.isPausedByRemote ? .white : (callViewModel.isPaused ? Color.greenSuccess500 : Color.gray500))
							.cornerRadius(40)
							.disabled(telecomManager.isPausedByRemote)
							
							Text("Pause")
								.foregroundStyle(.white)
								.default_text_style(styleSize: 15)
						}
						.frame(width: geo.size.width * 0.125, height: geo.size.width * 0.125)
						
						VStack {
							Button {
								callViewModel.toggleRecording()
							} label: {
								HStack {
									Image("record-fill")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle((callViewModel.isPaused || telecomManager.isPausedByRemote) ? Color.gray500 : .white)
										.frame(width: 32, height: 32)
								}
							}
							.buttonStyle(PressedButtonStyle())
							.frame(width: 60, height: 60)
							.background((callViewModel.isPaused || telecomManager.isPausedByRemote) ? .white : (callViewModel.isRecording ? Color.redDanger500 : Color.gray500))
							.cornerRadius(40)
							.disabled(callViewModel.isPaused || telecomManager.isPausedByRemote)
							
							Text("Record")
								.foregroundStyle(.white)
								.default_text_style(styleSize: 15)
						}
						.frame(width: geo.size.width * 0.125, height: geo.size.width * 0.125)
					}
					.frame(height: geo.size.height * 0.15)
					.padding(.horizontal, 20)
					.padding(.top, 30)
				}
				
				Spacer()
			}
			.background(Color.gray600)
			.frame(maxHeight: .infinity, alignment: .top)
		}
	}
	// swiftlint:enable function_body_length
	
	func getAudioRouteImage() {
		imageAudioRoute = AVAudioSession.sharedInstance().currentRoute.outputs.filter({ $0.portType.rawValue == "Speaker" }).isEmpty
		? (
			AVAudioSession.sharedInstance().currentRoute.outputs.filter({ $0.portType.rawValue.contains("Bluetooth") }).isEmpty
			? (
				callViewModel.isHeadPhoneAvailable()
				? "headset"
				: "speaker-slash"
			)
			: "bluetooth"
		)
		: "speaker-high"
	}
}

struct BottomSheetView<Content: View>: View {
	let content: Content
	
	@State var minHeight: CGFloat
	@State var maxHeight: CGFloat
	
	@Binding var currentOffset: CGFloat
	@Binding var pointingUp: CGFloat
	
	@State var bottomSafeArea: CGFloat
	
	var body: some View {
		GeometryReader { geometry in
			VStack(spacing: 0.0) {
				content
			}
			.frame(
				width: geometry.size.width,
				height: maxHeight,
				alignment: .top
			)
			.clipShape(
				Path(
					UIBezierPath(
						roundedRect: CGRect(x: 0.0, y: 0.0, width: geometry.size.width, height: maxHeight),
						byRoundingCorners: [.topLeft, .topRight],
						cornerRadii: CGSize(width: 16.0, height: 16.0)
					)
					.cgPath
				)
			)
			.frame(
				height: geometry.size.height,
				alignment: .bottom
			)
			.highPriorityGesture(
				DragGesture()
					.onChanged { value in
						currentOffset -= value.translation.height
						currentOffset = min(max(currentOffset, minHeight), maxHeight)
						pointingUp = -(((currentOffset - minHeight) / (maxHeight - minHeight)) - 0.5) * 2
					}
					.onEnded { _ in
						withAnimation {
							currentOffset = (currentOffset - minHeight <= maxHeight - currentOffset) ? minHeight : maxHeight
							pointingUp = -(((currentOffset - minHeight) / (maxHeight - minHeight)) - 0.5) * 2
						}
					}
			)
			.offset(y: maxHeight - currentOffset)
		}
	}
}

struct ChevronShape: Shape {
	var pointingUp: CGFloat
	
	var animatableData: CGFloat {
		get { return pointingUp }
		set { pointingUp = newValue }
	}
	
	func path(in rect: CGRect) -> Path {
		var path = Path()
		
		let width = rect.width
		let height = rect.height
		
		let horizontalCenter = width / 2
		let horizontalCenterOffset = width * 0.05
		let arrowTipStartingPoint = height - pointingUp * height * 0.9
		
		path.move(to: .init(x: 0, y: height))
		
		path.addLine(to: .init(x: horizontalCenter - horizontalCenterOffset, y: arrowTipStartingPoint))
		path.addQuadCurve(to: .init(x: horizontalCenter + horizontalCenterOffset, y: arrowTipStartingPoint), control: .init(x: horizontalCenter, y: height * (1 - pointingUp)))
		
		path.addLine(to: .init(x: width, y: height))

		return path
	}
}

struct PressedButtonStyle: ButtonStyle {
	func makeBody(configuration: Self.Configuration) -> some View {
		configuration.label
		.frame(width: 60, height: 60)
		.background(configuration.isPressed ? .white : .clear)
		.cornerRadius(40)
	}
}

#Preview {
	CallView(callViewModel: CallViewModel(), fullscreenVideo: .constant(false), isShowCallsListFragment: .constant(false), isShowStartCallFragment: .constant(false))
}
// swiftlint:enable type_body_length
// swiftlint:enable line_length
