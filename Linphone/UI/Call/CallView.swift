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
import UniformTypeIdentifiers

// swiftlint:disable function_body_length
// swiftlint:disable type_body_length
// swiftlint:disable line_length
// swiftlint:disable file_length
struct CallView: View {
	
	@ObservedObject private var coreContext = CoreContext.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	@ObservedObject private var contactsManager = ContactsManager.shared
	
	@EnvironmentObject private var callViewModel: CallViewModel
	
	@State private var addParticipantsViewModel: AddParticipantsViewModel?
    
    @Environment(\.scenePhase) var scenePhase
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	@State var audioRouteSheet: Bool = false
	@State var changeLayoutSheet: Bool = false
	@State var mediaEncryptedSheet: Bool = false
	@State var callStatisticsSheet: Bool = false
	@State var optionsAudioRoute: Int = 1
	@State var optionsChangeLayout: Int = 2
	@State var imageAudioRoute: String = ""
	@State var angleDegree = 0.0
	@State var showingDialer = false
	@State var topBarHeight: CGFloat = 50.0
    // @State var minBottomSheetHeight: CGFloat = 0.15
	// @State var maxBottomSheetHeight: CGFloat = 0.4
	@State var pointingUp: CGFloat = 0.0
	@State var currentOffset: CGFloat = 0.0
	@State var displayVideo = false
	@State private var previewVideoOffset = CGSize.zero
	@State private var previewVideoOffsetPreviousDrag = CGSize.zero
	
	@Binding var fullscreenVideo: Bool
	@State var isShowCallsListFragment: Bool = false
	@State var isShowParticipantsListFragment: Bool = false
	@Binding var isShowStartCallFragment: Bool
	@Binding var isShowConversationFragment: Bool
	@Binding var isShowStartCallGroupPopup: Bool
	
	@State var buttonSize = 60.0
	
	@Binding var isShowEditContactFragment: Bool
	
	@Binding var isShowScheduleMeetingFragment: Bool
	
	@State private var didInit = false
	
	var body: some View {
		GeometryReader { geo in
			let isLandscape = geo.size.width > geo.size.height
			
			let topBarCallCounter: CGFloat = (isLandscape && (
				(telecomManager.callInProgress
				 && !fullscreenVideo
				 && ((!telecomManager.callDisplayed && callViewModel.callsCounter == 1)
					 || callViewModel.callsCounter > 1))
				|| isShowConversationFragment
			)) ? 40.0 : 0.0
			
			let minBottomSheetHeight: CGFloat = isLandscape ? (idiom != .pad ? 0.3 : 0.2) : 0.15
			let maxBottomSheetHeight: CGFloat = isLandscape && idiom != .pad ? 0.6 : 0.4
			
			let minHeight = minBottomSheetHeight * UIScreen.main.bounds.height + topBarCallCounter
			let maxHeight = maxBottomSheetHeight * UIScreen.main.bounds.height + topBarCallCounter
			
			ZStack {
				if #available(iOS 16.4, *), idiom != .pad {
					innerView(
						geometry: geo,
						isLandscape: isLandscape,
						minHeight: minHeight,
						maxHeight: maxHeight,
						topBarCallCounter: topBarCallCounter,
						minBottomSheetHeight: minBottomSheetHeight,
						maxBottomSheetHeight: maxBottomSheetHeight
					)
					.sheet(isPresented: $mediaEncryptedSheet, onDismiss: {
						mediaEncryptedSheet = false
					}, content: {
						MediaEncryptedSheetBottomSheet(callViewModel: callViewModel, mediaEncryptedSheet: $mediaEncryptedSheet)
							.presentationDetents([.medium])
					})
					.sheet(isPresented: $callStatisticsSheet, onDismiss: {
						callStatisticsSheet = false
					}, content: {
						CallStatisticsSheetBottomSheet(callViewModel: callViewModel, callStatisticsSheet: $callStatisticsSheet)
							.presentationDetents(!callViewModel.callStatsModel.isVideoEnabled ? [.fraction(0.3)] : [.medium])
					})
					.sheet(isPresented: $audioRouteSheet, onDismiss: {
						audioRouteSheet = false
					}, content: {
						AudioRouteBottomSheet(callViewModel: callViewModel, optionsAudioRoute: $optionsAudioRoute)
							.presentationDetents([.fraction(0.3)])
					})
					.sheet(isPresented: $changeLayoutSheet, onDismiss: {
						changeLayoutSheet = false
					}, content: {
						ChangeLayoutBottomSheet(callViewModel: callViewModel, changeLayoutSheet: $changeLayoutSheet, optionsChangeLayout: $optionsChangeLayout)
							.presentationDetents([.fraction(0.3)])
					})
					.sheet(isPresented: $showingDialer) {
						DialerBottomSheet(
							startCallViewModel: StartCallViewModel(),
							callViewModel: callViewModel,
							isShowStartCallFragment: $isShowStartCallFragment,
							showingDialer: $showingDialer,
							transferAddress: .constant(nil),
							isShowTransferPopup: .constant(false),
							currentCall: callViewModel.currentCall
						)
						.presentationDetents([.medium])
						.presentationBackgroundInteraction(.enabled(upThrough: .medium))
					}
				} else if #available(iOS 16.0, *), idiom != .pad {
					innerView(
						geometry: geo,
						isLandscape: isLandscape,
						minHeight: minHeight,
						maxHeight: maxHeight,
						topBarCallCounter: topBarCallCounter,
						minBottomSheetHeight: minBottomSheetHeight,
						maxBottomSheetHeight: maxBottomSheetHeight
					)
					.sheet(isPresented: $mediaEncryptedSheet, onDismiss: {
						mediaEncryptedSheet = false
					}, content: {
						MediaEncryptedSheetBottomSheet(callViewModel: callViewModel, mediaEncryptedSheet: $mediaEncryptedSheet)
							.presentationDetents([.medium])
					})
					.sheet(isPresented: $callStatisticsSheet, onDismiss: {
						callStatisticsSheet = false
					}, content: {
						CallStatisticsSheetBottomSheet(callViewModel: callViewModel, callStatisticsSheet: $callStatisticsSheet)
							.presentationDetents(!callViewModel.callStatsModel.isVideoEnabled ? [.fraction(0.3)] : [.medium])
					})
					.sheet(isPresented: $audioRouteSheet, onDismiss: {
						audioRouteSheet = false
					}, content: {
						AudioRouteBottomSheet(callViewModel: callViewModel, optionsAudioRoute: $optionsAudioRoute)
							.presentationDetents([.fraction(0.3)])
					})
					.sheet(isPresented: $changeLayoutSheet, onDismiss: {
						changeLayoutSheet = false
					}, content: {
						ChangeLayoutBottomSheet(callViewModel: callViewModel, changeLayoutSheet: $changeLayoutSheet, optionsChangeLayout: $optionsChangeLayout)
							.presentationDetents([.fraction(0.3)])
					})
					.sheet(isPresented: $showingDialer) {
						DialerBottomSheet(
							startCallViewModel: StartCallViewModel(),
							callViewModel: callViewModel,
							isShowStartCallFragment: $isShowStartCallFragment,
							showingDialer: $showingDialer,
							transferAddress: .constant(nil),
							isShowTransferPopup: .constant(false),
							currentCall: callViewModel.currentCall
						)
						.presentationDetents([.medium])
					}
				} else {
					innerView(
						geometry: geo,
						isLandscape: isLandscape,
						minHeight: minHeight,
						maxHeight: maxHeight,
						topBarCallCounter: topBarCallCounter,
						minBottomSheetHeight: minBottomSheetHeight,
						maxBottomSheetHeight: maxBottomSheetHeight
					)
					.halfSheet(showSheet: $mediaEncryptedSheet) {
						MediaEncryptedSheetBottomSheet(callViewModel: callViewModel, mediaEncryptedSheet: $mediaEncryptedSheet)
					} onDismiss: {
						mediaEncryptedSheet = false
					}
					.halfSheet(showSheet: $callStatisticsSheet) {
						CallStatisticsSheetBottomSheet(callViewModel: callViewModel, callStatisticsSheet: $callStatisticsSheet)
					} onDismiss: {
						callStatisticsSheet = false
					}
					.halfSheet(showSheet: $audioRouteSheet) {
						AudioRouteBottomSheet(callViewModel: callViewModel, optionsAudioRoute: $optionsAudioRoute)
					} onDismiss: {
						audioRouteSheet = false
					}
					.halfSheet(showSheet: $changeLayoutSheet) {
						ChangeLayoutBottomSheet(callViewModel: callViewModel, changeLayoutSheet: $changeLayoutSheet, optionsChangeLayout: $optionsChangeLayout)
					} onDismiss: {
						changeLayoutSheet = false
					}
					.halfSheet(showSheet: $showingDialer) {
						DialerBottomSheet(
							startCallViewModel: StartCallViewModel(),
							callViewModel: callViewModel,
							isShowStartCallFragment: $isShowStartCallFragment,
							showingDialer: $showingDialer,
							transferAddress: .constant(nil),
							isShowTransferPopup: .constant(false),
							currentCall: callViewModel.currentCall
						)
					} onDismiss: {}
				}
				
				if isShowCallsListFragment {
					CallsListFragment(callViewModel: callViewModel, isShowCallsListFragment: $isShowCallsListFragment)
						.zIndex(4)
						.transition(.move(edge: .bottom))
				}
				
				if isShowParticipantsListFragment {
					ParticipantsListFragment(callViewModel: callViewModel, addParticipantsViewModel: addParticipantsViewModel ?? AddParticipantsViewModel(), isShowParticipantsListFragment: $isShowParticipantsListFragment)
						.zIndex(4)
						.transition(.move(edge: .bottom))
						.onAppear {
							addParticipantsViewModel = AddParticipantsViewModel()
						}
				}
				
				if callViewModel.zrtpPopupDisplayed == true {
					if idiom != .pad
						&& geo.size.width > geo.size.height
						&& buttonSize != 45 {
						ZRTPPopup(callViewModel: callViewModel, resizeView: 1.5)
							.background(.black.opacity(0.65))
					} else {
						ZRTPPopup(callViewModel: callViewModel, resizeView: buttonSize == 45 ? 1.5 : 1)
							.background(.black.opacity(0.65))
					}
				}
				
				if telecomManager.remainingCall {
					HStack {}
						.onAppear {
							callViewModel.resetCallView()
						}
				}
			}
			.background(Color.gray900)
			.onAppear {
				UIApplication.shared.endEditing()
				fullscreenVideo = false
				if geo.size.width < 350 || geo.size.height < 350 {
					buttonSize = 45.0
				}
			}
			.onChange(of: scenePhase) { newPhase in
				switch newPhase {
				case .active:
					callViewModel.resetCallView()
				default:
					break
				}
			}
		}
		.id("\(telecomManager.callInProgress)-\(telecomManager.callDisplayed)-\(callViewModel.callsCounter)-\(isShowConversationFragment)")
	}
	
	@ViewBuilder
	func innerView(geometry: GeometryProxy, isLandscape: Bool, minHeight: CGFloat, maxHeight: CGFloat, topBarCallCounter: CGFloat, minBottomSheetHeight: CGFloat, maxBottomSheetHeight: CGFloat) -> some View {
		ZStack(alignment: .bottom) {
			VStack(spacing: 0) {
				if !fullscreenVideo || (fullscreenVideo && telecomManager.isPausedByRemote) {
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
                                .lineLimit(1)
							
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
										Text("call_state_paused")
											.default_text_style_white_800(styleSize: 16)
											.lineLimit(1)
									} else if telecomManager.isPausedByRemote {
										Text("call_state_paused_by_remote")
											.default_text_style_white_800(styleSize: 16)
											.lineLimit(1)
									}
								}
							}
							
							Spacer()
							
							if callViewModel.isPaused || telecomManager.isPausedByRemote {
								Button {
								} label: {
									Image("pause")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(Color.orangeMain500)
										.frame(width: 30, height: 30)
										.padding(.all, 10)
								}
							} else {
								if callViewModel.videoDisplayed {
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
								
								Button {
									callStatisticsSheet = true
								} label: {
									Image(callViewModel.qualityIcon)
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(.white)
										.frame(width: 30, height: 30)
										.padding(.all, 10)
								}
							}
						}
						.frame(height: topBarHeight)
						.zIndex(1)
						
						if !telecomManager.outgoingCallStarted && telecomManager.callInProgress {
							// Compute the image, text, and color before the HStack
							let encryptionInfo: (image: String, textKey: LocalizedStringKey, color: Color) = {
								if callViewModel.isMediaEncrypted && callViewModel.isRemoteDeviceTrusted && callViewModel.isZrtp {
									// Encrypted call, ZRTP, device trusted
									let key: LocalizedStringKey = {
										if callViewModel.isConference {
											if callViewModel.isEndToEndEncrypted {
												return LocalizedStringKey("call_conference_end_to_end_encrypted")
											} else if callViewModel.isZrtp {
												return LocalizedStringKey("call_zrtp_point_to_point_encrypted")
											} else {
												return LocalizedStringKey("call_srtp_point_to_point_encrypted")
											}
										} else {
											if callViewModel.isZrtp {
												return LocalizedStringKey("call_zrtp_end_to_end_encrypted")
											} else {
												return LocalizedStringKey("call_srtp_point_to_point_encrypted")
											}
										}
									}()
									return ("lock-key", key, Color.blueInfo500)
									
								} else if callViewModel.isMediaEncrypted && !callViewModel.isZrtp {
									// Encrypted call, SRTP
									return ("lock_simple", LocalizedStringKey("call_srtp_point_to_point_encrypted"), Color.blueInfo500)
									
								} else if callViewModel.isMediaEncrypted && (!callViewModel.isRemoteDeviceTrusted && callViewModel.isZrtp) || callViewModel.cacheMismatch {
									// ZRTP warning
									return ("warning-circle", LocalizedStringKey("call_zrtp_sas_validation_required"), Color.orangeWarning600)
									
								} else if callViewModel.isNotEncrypted {
									// Not encrypted
									return ("lock-simple-open", LocalizedStringKey("call_not_encrypted"), .white)
									
								} else {
									// Waiting for encryption info
									return ("progress", LocalizedStringKey("call_waiting_for_encryption_info"), .white)
								}
							}()

							HStack {
								if encryptionInfo.image == "progress" {
									ProgressView()
										.controlSize(.mini)
										.progressViewStyle(CircularProgressViewStyle(tint: encryptionInfo.color))
										.frame(width: 15, height: 15, alignment: .leading)
										.padding(.leading, 50)
										.padding(.top, 35)
								} else {
									Image(encryptionInfo.image)
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(encryptionInfo.color)
										.frame(width: 15, height: 15, alignment: .leading)
										.padding(.leading, 50)
										.padding(.top, 35)
								}
								
								Text(encryptionInfo.textKey)
									.foregroundStyle(encryptionInfo.color)
									.default_text_style_white(styleSize: 12)
									.padding(.top, 35)
								
								Spacer()
							}
							.onTapGesture {
								mediaEncryptedSheet = true
							}
							.frame(height: topBarHeight)
							.zIndex(1)
						}
					}
                    .frame(height: topBarHeight)
					.background(Color.gray900)
				}
				
				simpleCallView(geometry: geometry, minBottomSheetHeight: minBottomSheetHeight, isLandscape: isLandscape, topBarCallCounter: topBarCallCounter)
					.background(Color.gray900)
					.safeAreaInset(edge: .bottom) {
						if !fullscreenVideo || (fullscreenVideo && telecomManager.isPausedByRemote) {
							Color.clear.frame(height: minHeight)
						}
					}
				
				if !fullscreenVideo || (fullscreenVideo && telecomManager.isPausedByRemote) {
					Color.gray600
						.frame(height: 1)
				}
			}
			
			if !fullscreenVideo || (fullscreenVideo && telecomManager.isPausedByRemote) {
				if telecomManager.callStarted {
					BottomSheetView(
						content: BottomSheetContent(
							geo: geometry,
							buttonSize: $buttonSize,
							pointingUp: $pointingUp,
							currentOffset: $currentOffset,
							minBottomSheetHeight: minBottomSheetHeight,
							maxBottomSheetHeight: maxBottomSheetHeight,
							optionsAudioRoute: $optionsAudioRoute,
							optionsChangeLayout: $optionsChangeLayout,
							showingDialer: $showingDialer,
							audioRouteSheet: $audioRouteSheet,
							changeLayoutSheet: $changeLayoutSheet,
							isShowStartCallFragment: $isShowStartCallFragment,
							isShowCallsListFragment: $isShowCallsListFragment,
							isShowParticipantsListFragment: $isShowParticipantsListFragment,
							imageAudioRoute: $imageAudioRoute
						),
						minHeight: minHeight,
						maxHeight: maxHeight,
						currentOffset: $currentOffset,
						pointingUp: $pointingUp
					)
					.onAppear {
						currentOffset = minHeight
						pointingUp = -(((currentOffset - minHeight) / (maxHeight - minHeight)) - 0.5) * 2
					}
					.onChange(of: optionsChangeLayout) { _ in
						currentOffset = minHeight
						pointingUp = -(((currentOffset - minHeight) / (maxHeight - minHeight)) - 0.5) * 2
					}
				}
			}
		}
		.background(!fullscreenVideo || (fullscreenVideo && telecomManager.isPausedByRemote) ? Color.gray600 : Color.gray900)
	}
	
	// swiftlint:disable:next cyclomatic_complexity
	func simpleCallView(geometry: GeometryProxy, minBottomSheetHeight: Double, isLandscape: Bool, topBarCallCounter: CGFloat) -> some View {
		ZStack {
			if callViewModel.isOneOneCall {
				let avatarSize = min(topBarCallCounter == 40.0 && isLandscape && idiom != .pad ? geometry.size.height * 0.15 : geometry.size.height * 0.25, 220)

					VStack {
						Spacer()

						ZStack {
							if callViewModel.isRemoteDeviceTrusted {
								Circle()
									.fill(Color.blueInfo500)
									.frame(width: avatarSize + 6, height: avatarSize + 6)
							}

							if let avatar = callViewModel.avatarModel {
								Avatar(
									contactAvatarModel: avatar,
									avatarSize: avatarSize,
									hidePresence: true
								)
							}

							if callViewModel.isRemoteDeviceTrusted {
								VStack {
									Spacer()

									HStack {
										Image("trusted")
											.resizable()
											.scaledToFit()
											.frame(width: avatarSize * 0.12)
											.padding(avatarSize * 0.07)

										Spacer()
									}
								}
								.frame(width: avatarSize, height: avatarSize)
							}
						}

						Text(callViewModel.displayName)
							.padding(.top)
							.default_text_style_white(styleSize: 22)

						if !AppServices.corePreferences.hideSipAddresses {
							Text(callViewModel.remoteAddressCleanedString)
								.default_text_style_white_300(styleSize: 16)
						}

						Spacer()
					}
				
				if telecomManager.remoteConfVideo {
					VStack {
						LinphoneVideoViewHolder { view in
							coreContext.doOnCoreQueue { core in
								core.nativeVideoWindow = view
								DispatchQueue.main.async {
									CoreContext.shared.pipViewModel.setupPiPViewController(remoteView: view)
								}
							}
						}
						.onTapGesture {
							if telecomManager.remoteConfVideo {
								fullscreenVideo.toggle()
							}
						}
						.onAppear {
							if callViewModel.videoDisplayed {
								if coreContext.pipViewModel.pipController?.isPictureInPictureActive ?? false {
									coreContext.pipViewModel.pipController?.stopPictureInPicture()
								}
								callViewModel.videoDisplayed = false
								DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
									callViewModel.videoDisplayed = true
								}
							}
						}
						.onDisappear {
							coreContext.doOnCoreQueue { core in
                                if !telecomManager.remoteConfVideo {
                                    core.nativeVideoWindow = nil
                                }
							}
							
							if callViewModel.videoDisplayed {
								if !callViewModel.isPaused && TelecomManager.shared.callInProgress
									&& !(coreContext.pipViewModel.pipController?.isPictureInPictureActive ?? false) {
									// TODO: Enable PIP in 6.1
									//coreContext.pipViewModel.pipController?.startPictureInPicture()
								}
								callViewModel.videoDisplayed = false
								DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
									callViewModel.videoDisplayed = true
								}
							}
							
							fullscreenVideo = false
						}
					}
					.cornerRadius(fullscreenVideo && !telecomManager.isPausedByRemote ? 0 : 20)
					.ignoresSafeArea(fullscreenVideo && !telecomManager.isPausedByRemote ? .all : [])
				}
				
				if callViewModel.videoDisplayed {
					HStack {
						Spacer()
						VStack {
							Spacer()
							HStack {
								LinphoneVideoViewHolder { view in
									coreContext.doOnCoreQueue { core in
										core.nativePreviewWindow = view
									}
								}
								.onDisappear {
									coreContext.doOnCoreQueue { core in
										core.nativePreviewWindow = nil
									}
								}
								.aspectRatio(callViewModel.callStatsModel.sentVideoWindow.widthFactor/callViewModel.callStatsModel.sentVideoWindow.heightFactor, contentMode: .fill)
								.frame(maxWidth: callViewModel.callStatsModel.sentVideoWindow.widthFactor * 256,
									   maxHeight: callViewModel.callStatsModel.sentVideoWindow.heightFactor * 256)
								.clipped()
							}
							.frame(width: angleDegree == 0 ? 120*1.2 : 160*1.2, height: angleDegree == 0 ? 160*1.2 : 120*1.2) // 144*192
							.cornerRadius(20)
                            .gesture(
                                DragGesture(coordinateSpace: .global)
                                    .onChanged { value in
                                        previewVideoOffset = CGSize(width: previewVideoOffsetPreviousDrag.width + value.translation.width,
                                                                    height: previewVideoOffsetPreviousDrag.height + value.translation.height)
                                    }
                                    .onEnded { _ in
                                        previewVideoOffsetPreviousDrag = previewVideoOffset
                                    }
                            )
                            
							.offset(x: previewVideoOffset.width, y: previewVideoOffset.height)
						}
						.padding(10)
						.padding(.trailing, abs(angleDegree/2))
					}
				}
				
				if telecomManager.outgoingCallStarted {
					VStack {
						ActivityIndicator(color: .white)
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
				}
			} else if callViewModel.isConference && !telecomManager.outgoingCallStarted && callViewModel.activeSpeakerParticipant != nil {
				if optionsChangeLayout == 1 && callViewModel.participantList.count <= 5 && callViewModel.activeSpeakerParticipant?.isScreenSharing == false {
					mosaicMode()
				} else if optionsChangeLayout == 3 && callViewModel.activeSpeakerParticipant?.isScreenSharing == false {
					audioOnlyMode()
				} else {
					activeSpeakerMode(geometry: geometry)
						.onAppear {
							guard !didInit else { return }
							didInit = true
							
							DispatchQueue.main.async {
								callViewModel.resetCallView()
							}
						}
				}
			} else if callViewModel.isConference && !telecomManager.outgoingCallStarted && callViewModel.participantList.isEmpty {
				VStack {
					Spacer()
					
					Text("conference_call_empty")
						.frame(maxWidth: .infinity, alignment: .center)
						.foregroundStyle(Color.white)
						.default_text_style_300(styleSize: 25)
						.lineLimit(1)
						.padding(.bottom, 4)
					
					Button(action: {
						UIPasteboard.general.setValue(
							callViewModel.remoteAddressString,
							forPasteboardType: UTType.plainText.identifier
						)
						
						DispatchQueue.main.async {
							ToastViewModel.shared.show("Success_address_copied_into_clipboard")
						}
					}, label: {
						HStack {
							Image("share-network")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c400)
								.frame(width: 30, height: 30)
							
							Text("conference_share_link_title")
								.foregroundStyle(Color.grayMain2c400)
								.default_text_style(styleSize: 25)
								.frame(height: 40)
						}
					})
					.padding(.horizontal, 20)
					.padding(.vertical, 10)
					.cornerRadius(60)
					.overlay(
						RoundedRectangle(cornerRadius: 60)
							.inset(by: 0.5)
							.stroke(Color.grayMain2c400, lineWidth: 1)
					)
					
					Spacer()
				}
				.onAppear {
					fullscreenVideo = false
					
					guard !didInit && geometry.safeAreaInsets.bottom > 0 else { return }
					didInit = true
					
					DispatchQueue.main.async {
						callViewModel.resetCallView()
					}
				}
				
				HStack {
					Spacer()
					VStack {
						Spacer()
						HStack {
							LinphoneVideoViewHolder { view in
								coreContext.doOnCoreQueue { core in
									core.nativePreviewWindow = view
								}
							}
							.onDisappear {
								coreContext.doOnCoreQueue { core in
									core.nativePreviewWindow = nil
								}
							}
							.aspectRatio(callViewModel.callStatsModel.sentVideoWindow.widthFactor/callViewModel.callStatsModel.sentVideoWindow.heightFactor, contentMode: .fill)
							.frame(maxWidth: callViewModel.callStatsModel.sentVideoWindow.widthFactor * 256,
								   maxHeight: callViewModel.callStatsModel.sentVideoWindow.heightFactor * 256)
							.clipped()
						}
						.frame(width: angleDegree == 0 ? 120*1.2 : 160*1.2, height: angleDegree == 0 ? 160*1.2 : 120*1.2) // 144*192
						.cornerRadius(20)
						.gesture(
							DragGesture(coordinateSpace: .global)
								.onChanged { value in
									previewVideoOffset = CGSize(width: previewVideoOffsetPreviousDrag.width + value.translation.width,
																height: previewVideoOffsetPreviousDrag.height + value.translation.height)
								}
								.onEnded { _ in
									previewVideoOffsetPreviousDrag = previewVideoOffset
								}
						)
									
						.offset(x: previewVideoOffset.width, y: previewVideoOffset.height)
					}
					.padding(10)
					.padding(.trailing, abs(angleDegree/2))
				}
			} else if telecomManager.outgoingCallStarted {
				VStack {
					Spacer()
					
					ProgressView()
						.progressViewStyle(CircularProgressViewStyle(tint: .white))
						.frame(width: 60, height: 60, alignment: .center)
						.onDisappear {
							callViewModel.resetCallView()
						}
					
					Spacer()
				}
			} else {
				VStack {
					Spacer()
					
					ProgressView()
						.progressViewStyle(CircularProgressViewStyle(tint: .white))
						.frame(width: 60, height: 60, alignment: .center)
					
					Spacer()
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
			}
		}
		.frame(maxWidth: .infinity)
		.background(Color.gray900)
		.padding(.top, fullscreenVideo ? 0 : 6)
		.padding(.bottom, fullscreenVideo ? 0 : 2)
		.onRotate { newOrientation in
			let oldOrientation = orientation
			orientation = newOrientation
			if orientation == .portrait || orientation == .portraitUpsideDown {
				angleDegree = 0
			} else if orientation == .landscapeLeft {
				angleDegree = -90
			} else if orientation == .landscapeRight {
				angleDegree = 90
			} else if geometry.size.width > geometry.size.height {
				angleDegree = 90
			}
			
			if oldOrientation != orientation &&
			   (oldOrientation != .faceUp || orientation.isLandscape) {
				telecomManager.callStarted = false
				
				DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
					telecomManager.callStarted = true
				}
			}
			
			callViewModel.orientationUpdate(orientation: orientation)
		}
		.onAppear {
			if orientation == .portrait || orientation == .portraitUpsideDown {
				angleDegree = 0
			} else if orientation == .landscapeLeft {
				angleDegree = -90
			} else if orientation == .landscapeRight {
				angleDegree = 90
			} else if geometry.size.width > geometry.size.height {
				angleDegree = 90
			}
			
			callViewModel.orientationUpdate(orientation: orientation)
		}
		.onReceive(telecomManager.$remoteConfVideo, perform: { videoOn in
			if videoOn {
				fullscreenVideo = videoOn
			}
		})
	}
	
	// swiftlint:disable:next cyclomatic_complexity
	func activeSpeakerMode(geometry: GeometryProxy) -> some View {
		ZStack {
			let isLandscapeMode = geometry.size.width > geometry.size.height
			if callViewModel.activeSpeakerParticipant!.onPause {
				VStack {
					VStack {
						Spacer()
						
						Image("pause")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(.white)
							.frame(width: 40, height: 40)
						
						Text("conference_participant_paused_text")
							.frame(maxWidth: .infinity, alignment: .center)
							.foregroundStyle(Color.white)
							.default_text_style_500(styleSize: 14)
							.lineLimit(1)
							.padding(.horizontal, 10)
						
						Spacer()
					}
					Spacer()
				}
			} else if callViewModel.activeSpeakerParticipant!.isJoining {
				VStack {
					VStack {
						Spacer()
						
						ActivityIndicator(color: .white)
							.frame(width: 40, height: 40)
							.padding(.bottom, 5)
						
						Text("conference_participant_joining_text")
							.frame(maxWidth: .infinity, alignment: .center)
							.foregroundStyle(Color.white)
							.default_text_style_500(styleSize: 14)
							.lineLimit(1)
							.padding(.horizontal, 10)
						
						Spacer()
					}
					Spacer()
				}
			} else {
				VStack {
					Spacer()
					HStack {
						if callViewModel.activeSpeakerParticipant != nil {
							Avatar(contactAvatarModel: callViewModel.activeSpeakerParticipant!.avatarModel, avatarSize: 200, hidePresence: true)
								.onAppear {
									DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
										displayVideo = true
									}
								}
						}
					}
					
					Spacer()
				}
				
				if telecomManager.remoteConfVideo && !telecomManager.outgoingCallStarted && callViewModel.activeSpeakerParticipant != nil && displayVideo {
					VStack {
						LinphoneVideoViewHolder { view in
							coreContext.doOnCoreQueue { core in
								core.nativeVideoWindow = view
								DispatchQueue.main.async {
									CoreContext.shared.pipViewModel.setupPiPViewController(remoteView: view)
								}
							}
						}
						.onAppear {
							if coreContext.pipViewModel.pipController?.isPictureInPictureActive ?? false {
								coreContext.pipViewModel.pipController?.stopPictureInPicture()
							}
						}
						.onDisappear {
							coreContext.doOnCoreQueue { core in
                                if !telecomManager.remoteConfVideo {
                                    core.nativeVideoWindow = nil
                                }
							}
							if !callViewModel.isPaused && TelecomManager.shared.callInProgress
								&& !(coreContext.pipViewModel.pipController?.isPictureInPictureActive ?? false) {
								// TODO: Enable PIP in 6.1
								//coreContext.pipViewModel.pipController?.startPictureInPicture()
							}
						}
					}
					.cornerRadius(fullscreenVideo && !telecomManager.isPausedByRemote ? 0 : 20)
					.ignoresSafeArea(fullscreenVideo && !telecomManager.isPausedByRemote ? .all : [])
				}
			}
			
			if callViewModel.isConference && !telecomManager.outgoingCallStarted && callViewModel.activeSpeakerParticipant != nil && callViewModel.activeSpeakerParticipant!.isMuted {
				VStack {
					HStack {
						Spacer()
						
						HStack(alignment: .center) {
							Image("microphone-slash")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c800)
								.frame(width: 20, height: 20)
						}
						.padding(5)
						.background(.white)
						.cornerRadius(40)
						
						if isLandscapeMode {
							Spacer()
								.frame(width: 160)
						}
					}
					Spacer()
				}
				.frame(maxWidth: .infinity)
				.padding(.all, 20)
			}
			
			if callViewModel.isConference {
				HStack {
					Spacer()
					VStack {
						Spacer()
						
						Text(callViewModel.activeSpeakerName)
							.frame(maxWidth: .infinity, alignment: .leading)
							.foregroundStyle(Color.white)
							.default_text_style_500(styleSize: 20)
							.lineLimit(1)
							.padding(.horizontal, 10)
						
						if !isLandscapeMode {
							ScrollView(.horizontal) {
								HStack {
									ZStack {
										VStack {
											Spacer()
											
											if callViewModel.myParticipantModel != nil {
												Avatar(contactAvatarModel: callViewModel.myParticipantModel!.avatarModel, avatarSize: 50, hidePresence: true)
											}
											
											Spacer()
										}
										.frame(width: 140, height: 140)
										
										if callViewModel.videoDisplayed {
											LinphoneVideoViewHolder { view in
												coreContext.doOnCoreQueue { core in
													core.nativePreviewWindow = view
												}
											}
											.onDisappear {
												coreContext.doOnCoreQueue { core in
													core.nativePreviewWindow = nil
												}
											}
											.frame(width: angleDegree == 0 ? 120*1.2 : 160*1.2, height: angleDegree == 0 ? 160*1.2 : 120*1.2)
											.scaledToFill()
											.clipped()
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
										.frame(width: 140, height: 140)
									}
									.frame(width: 140, height: 140)
									.background(Color.gray600)
									.overlay(
										RoundedRectangle(cornerRadius: 20)
											.stroke(callViewModel.myParticipantModel != nil && callViewModel.myParticipantModel!.isSpeaking ? .white : .clear, lineWidth: 4)
									)
									.cornerRadius(20)
									
									ForEach(0..<callViewModel.participantList.count, id: \.self) { index in
										if callViewModel.activeSpeakerParticipant != nil && (!callViewModel.participantList[index].address.weakEqual(address2: callViewModel.activeSpeakerParticipant!.address) || callViewModel.activeSpeakerParticipant!.isScreenSharing) {
											ZStack {
												if callViewModel.participantList[index].isJoining {
													VStack {
														Spacer()
														
														ActivityIndicator(color: .white)
															.frame(width: 40, height: 40)
															.padding(.bottom, 5)
														
														Text("conference_participant_joining_text")
															.frame(maxWidth: .infinity, alignment: .center)
															.foregroundStyle(Color.white)
															.default_text_style_500(styleSize: 14)
															.lineLimit(1)
															.padding(.horizontal, 10)
														
														Spacer()
													}
												} else if callViewModel.participantList[index].onPause {
													VStack {
														Spacer()
														
														Image("pause")
															.renderingMode(.template)
															.resizable()
															.foregroundStyle(.white)
															.frame(width: 40, height: 40)
														
														Text("conference_participant_paused_text")
															.frame(maxWidth: .infinity, alignment: .center)
															.foregroundStyle(Color.white)
															.default_text_style_500(styleSize: 14)
															.lineLimit(1)
															.padding(.horizontal, 10)
														
														Spacer()
													}
												} else {
													VStack {
														Spacer()
														
														Avatar(contactAvatarModel: callViewModel.participantList[index].avatarModel, avatarSize: 50, hidePresence: true)
														
														Spacer()
													}
													
													LinphoneVideoViewHolder { view in
														coreContext.doOnCoreQueue { core in
															if index < callViewModel.participantList.count {
																let participantVideo = core.currentCall?.conference?.participantList.first(where: {$0.address!.weakEqual(address2: callViewModel.participantList[index].address)})
																if participantVideo != nil && participantVideo!.devices.first != nil {
																	participantVideo!.devices.first!.nativeVideoWindowId = UnsafeMutableRawPointer(Unmanaged.passRetained(view).toOpaque())
																}
															}
														}
													}
													
													if callViewModel.participantList[index].isMuted {
														VStack {
															HStack {
																Spacer()
																
																HStack(alignment: .center) {
																	Image("microphone-slash")
																		.renderingMode(.template)
																		.resizable()
																		.foregroundStyle(Color.grayMain2c800)
																		.frame(width: 12, height: 12)
																}
																.padding(2)
																.background(.white)
																.cornerRadius(40)
															}
															Spacer()
														}
														.frame(maxWidth: .infinity)
														.padding(.all, 10)
													}
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
											.overlay(
												RoundedRectangle(cornerRadius: 20)
													.stroke(callViewModel.participantList[index].isSpeaking ? .white : .clear, lineWidth: 4)
											)
											.cornerRadius(20)
										}
									}
								}
								.padding(.leading, 8)
								.padding(.trailing, 6)
							}
							.padding(.bottom, 6)
							.padding(.leading, -10)
						}
					}
				}
				
				if isLandscapeMode {
					HStack {
						Spacer()
						ScrollView(.vertical) {
							VStack {
								ZStack {
									VStack {
										Spacer()
										
										if callViewModel.myParticipantModel != nil {
											Avatar(contactAvatarModel: callViewModel.myParticipantModel!.avatarModel, avatarSize: 50, hidePresence: true)
										}
										
										Spacer()
									}
									.frame(width: 140, height: 140)
									
									if callViewModel.videoDisplayed {
										LinphoneVideoViewHolder { view in
											coreContext.doOnCoreQueue { core in
												core.nativePreviewWindow = view
											}
										}
										.onDisappear {
											coreContext.doOnCoreQueue { core in
												core.nativePreviewWindow = nil
											}
										}
										.frame(width: angleDegree == 0 ? 120*1.2 : 160*1.2, height: angleDegree == 0 ? 160*1.2 : 120*1.2)
										.scaledToFill()
										.clipped()
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
									.frame(width: 140, height: 140)
								}
								.frame(width: 140, height: 140)
								.background(Color.gray600)
								.overlay(
									RoundedRectangle(cornerRadius: 20)
										.stroke(callViewModel.myParticipantModel != nil && callViewModel.myParticipantModel!.isSpeaking ? .white : .clear, lineWidth: 4)
								)
								.cornerRadius(20)
								
								ForEach(0..<callViewModel.participantList.count, id: \.self) { index in
                                    if callViewModel.activeSpeakerParticipant != nil && (!callViewModel.participantList[index].address.weakEqual(address2: callViewModel.activeSpeakerParticipant!.address) || callViewModel.activeSpeakerParticipant!.isScreenSharing) {
										ZStack {
											if callViewModel.participantList[index].isJoining {
												VStack {
													Spacer()
													
													ActivityIndicator(color: .white)
														.frame(width: 40, height: 40)
														.padding(.bottom, 5)
													
													Text("conference_participant_joining_text")
														.frame(maxWidth: .infinity, alignment: .center)
														.foregroundStyle(Color.white)
														.default_text_style_500(styleSize: 14)
														.lineLimit(1)
														.padding(.horizontal, 10)
													
													Spacer()
												}
											} else if callViewModel.participantList[index].onPause {
												VStack {
													Spacer()
													
													Image("pause")
														.renderingMode(.template)
														.resizable()
														.foregroundStyle(.white)
														.frame(width: 40, height: 40)
													
													Text("conference_participant_paused_text")
														.frame(maxWidth: .infinity, alignment: .center)
														.foregroundStyle(Color.white)
														.default_text_style_500(styleSize: 14)
														.lineLimit(1)
														.padding(.horizontal, 10)
													
													Spacer()
												}
											} else {
												VStack {
													Spacer()
													
													Avatar(contactAvatarModel: callViewModel.participantList[index].avatarModel, avatarSize: 50, hidePresence: true)
													
													Spacer()
												}
												
												LinphoneVideoViewHolder { view in
													coreContext.doOnCoreQueue { core in
														if index < callViewModel.participantList.count {
															let participantVideo = core.currentCall?.conference?.participantList.first(where: {$0.address!.weakEqual(address2: callViewModel.participantList[index].address)})
															if participantVideo != nil && participantVideo!.devices.first != nil {
																participantVideo!.devices.first!.nativeVideoWindowId = UnsafeMutableRawPointer(Unmanaged.passRetained(view).toOpaque())
															}
														}
													}
												}
												
												if callViewModel.participantList[index].isMuted {
													VStack {
														HStack {
															Spacer()
															
															HStack(alignment: .center) {
																Image("microphone-slash")
																	.renderingMode(.template)
																	.resizable()
																	.foregroundStyle(Color.grayMain2c800)
																	.frame(width: 12, height: 12)
															}
															.padding(2)
															.background(.white)
															.cornerRadius(40)
														}
														Spacer()
													}
													.frame(maxWidth: .infinity)
													.padding(.all, 10)
												}
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
										.overlay(
											RoundedRectangle(cornerRadius: 20)
												.stroke(callViewModel.participantList[index].isSpeaking ? .white : .clear, lineWidth: 4)
										)
										.cornerRadius(20)
									}
								}
							}
							.padding(.all, 6)
						}
					}
				}
			}
		}
		.frame(maxWidth: .infinity, maxHeight: .infinity)
		.background(Color.gray900)
		.onTapGesture {
			fullscreenVideo.toggle()
		}
		.onAppear {
			optionsChangeLayout = 2
		}
	}
	
	// swiftlint:disable:next cyclomatic_complexity
	func mosaicMode() -> some View {
		GeometryReader { geometry in
			VStack {
				if geometry.size.width < geometry.size.height {
					let height = geometry.size.height
					let width = geometry.size.width
					let participantCount = Double(callViewModel.participantList.count + 1)
					let rows = ceil(participantCount / 2.0)

					let optionA = (height - (participantCount - 1) * 10) / participantCount
                    let optionB = min((width - 10) / 2.0, (height - (rows - 1) * 10) / rows)

					let maxValue = max(optionA, optionB)
					
					LazyVGrid(columns: [
						GridItem(.adaptive(
							minimum: maxValue
						))
					], spacing: 10) {
						if callViewModel.myParticipantModel != nil {
							ZStack {
								if callViewModel.myParticipantModel!.isJoining {
									VStack {
										Spacer()
										
										ActivityIndicator(color: .white)
											.frame(width: maxValue/4, height: maxValue/4)
											.padding(.bottom, 5)
										
										Text("conference_participant_joining_text")
											.frame(maxWidth: .infinity, alignment: .center)
											.foregroundStyle(Color.white)
											.default_text_style_500(styleSize: 14)
											.lineLimit(1)
											.padding(.horizontal, 10)
										
										Spacer()
									}
								} else if callViewModel.myParticipantModel!.onPause {
									VStack {
										Spacer()
										
										Image("pause")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(.white)
											.frame(width: maxValue/4, height: maxValue/4)
										
										Text("conference_participant_paused_text")
											.frame(maxWidth: .infinity, alignment: .center)
											.foregroundStyle(Color.white)
											.default_text_style_500(styleSize: 14)
											.lineLimit(1)
											.padding(.horizontal, 10)
										
										Spacer()
									}
								} else {
									VStack {
										Spacer()
										
										if callViewModel.myParticipantModel != nil {
											Avatar(contactAvatarModel: callViewModel.myParticipantModel!.avatarModel, avatarSize: maxValue/2, hidePresence: true)
										}
										
										Spacer()
									}
									.frame(width: maxValue, height: maxValue)
									
									if callViewModel.videoDisplayed {
										LinphoneVideoViewHolder { view in
											coreContext.doOnCoreQueue { core in
												core.nativePreviewWindow = view
											}
										}
										.onDisappear {
											coreContext.doOnCoreQueue { core in
												core.nativePreviewWindow = nil
											}
										}
										.frame(
											width: 120 * ceil(maxValue / 120),
											height: 160 * ceil(maxValue / 120)
										)
										.scaledToFill()
										.clipped()
									}
									
									if callViewModel.myParticipantModel!.isMuted {
										VStack {
											HStack {
												Spacer()
												
												HStack(alignment: .center) {
													Image("microphone-slash")
														.renderingMode(.template)
														.resizable()
														.foregroundStyle(Color.grayMain2c800)
														.frame(width: 12, height: 12)
												}
												.padding(2)
												.background(.white)
												.cornerRadius(40)
											}
											Spacer()
										}
										.frame(maxWidth: .infinity)
										.padding(.all, 10)
									}
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
								.frame(width: maxValue, height: maxValue)
							}
							.frame(
								width: maxValue,
								height: maxValue,
								alignment: .center
							)
							.background(Color.gray600)
							.overlay(
								RoundedRectangle(cornerRadius: 20)
									.stroke(callViewModel.myParticipantModel!.isSpeaking ? .white : .clear, lineWidth: 4)
							)
							.cornerRadius(20)
						}
						
						ForEach(0..<callViewModel.participantList.count, id: \.self) { index in
							if index < callViewModel.participantList.count {
								ZStack {
									if callViewModel.participantList[index].isJoining {
										VStack {
											Spacer()
											
											ActivityIndicator(color: .white)
												.frame(width: maxValue/4, height: maxValue/4)
												.padding(.bottom, 5)
											
											Text("conference_participant_joining_text")
												.frame(maxWidth: .infinity, alignment: .center)
												.foregroundStyle(Color.white)
												.default_text_style_500(styleSize: 14)
												.lineLimit(1)
												.padding(.horizontal, 10)
											
											Spacer()
										}
									} else if callViewModel.participantList[index].onPause {
										VStack {
											Spacer()
											
											Image("pause")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(.white)
												.frame(width: maxValue/4, height: maxValue/4)
											
											Text("conference_participant_paused_text")
												.frame(maxWidth: .infinity, alignment: .center)
												.foregroundStyle(Color.white)
												.default_text_style_500(styleSize: 14)
												.lineLimit(1)
												.padding(.horizontal, 10)
											
											Spacer()
										}
									} else {
										VStack {
											Spacer()
											
											Avatar(contactAvatarModel: callViewModel.participantList[index].avatarModel, avatarSize: maxValue/2, hidePresence: true)
											
											Spacer()
										}
										.frame(width: maxValue, height: maxValue)
										
										LinphoneVideoViewHolder { view in
											coreContext.doOnCoreQueue { core in
												if index < callViewModel.participantList.count {
													let participantVideo = core.currentCall?.conference?.participantList.first(where: {$0.address!.weakEqual(address2: callViewModel.participantList[index].address)})
													if participantVideo != nil && participantVideo!.devices.first != nil {
														participantVideo!.devices.first!.nativeVideoWindowId = UnsafeMutableRawPointer(Unmanaged.passRetained(view).toOpaque())
													}
												}
											}
										}
										
										if callViewModel.participantList[index].isMuted {
											VStack {
												HStack {
													Spacer()
													
													HStack(alignment: .center) {
														Image("microphone-slash")
															.renderingMode(.template)
															.resizable()
															.foregroundStyle(Color.grayMain2c800)
															.frame(width: 12, height: 12)
													}
													.padding(2)
													.background(.white)
													.cornerRadius(40)
												}
												Spacer()
											}
											.frame(maxWidth: .infinity)
											.padding(.all, 10)
										}
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
									.frame(width: maxValue, height: maxValue)
								}
								.frame(
									width: maxValue,
									height: maxValue,
									alignment: .center
								)
								.background(Color.gray600)
								.overlay(
									RoundedRectangle(cornerRadius: 20)
										.stroke(callViewModel.participantList[index].isSpeaking ? .white : .clear, lineWidth: 4)
								)
								.cornerRadius(20)
							}
						}
					}
				} else {
					let height = geometry.size.height
					let width = geometry.size.width
					let participantCount = Double(callViewModel.participantList.count + 1)
					let rows = ceil(participantCount / 2.0)

					let optionA = min(height, (width - (participantCount - 1) * 10) / participantCount)
                    let optionB = min((height - 10) / 2.0, (width - (rows - 1) * 10) / rows)

					let maxValue = max(optionA, optionB)
					
					LazyHGrid(rows: [
						GridItem(.adaptive(
							minimum: maxValue
						))
					], spacing: 10) {
						if callViewModel.myParticipantModel != nil {
							ZStack {
								if callViewModel.myParticipantModel!.isJoining {
									VStack {
										Spacer()
										
										ActivityIndicator(color: .white)
											.frame(width: maxValue/4, height: maxValue/4)
											.padding(.bottom, 5)
										
										Text("conference_participant_joining_text")
											.frame(maxWidth: .infinity, alignment: .center)
											.foregroundStyle(Color.white)
											.default_text_style_500(styleSize: 14)
											.lineLimit(1)
											.padding(.horizontal, 10)
										
										Spacer()
									}
								} else if callViewModel.myParticipantModel!.onPause {
									VStack {
										Spacer()
										
										Image("pause")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(.white)
											.frame(width: maxValue/4, height: maxValue/4)
										
										Text("conference_participant_paused_text")
											.frame(maxWidth: .infinity, alignment: .center)
											.foregroundStyle(Color.white)
											.default_text_style_500(styleSize: 14)
											.lineLimit(1)
											.padding(.horizontal, 10)
										
										Spacer()
									}
								} else {
									VStack {
										Spacer()
										
										if callViewModel.myParticipantModel != nil {
											Avatar(contactAvatarModel: callViewModel.myParticipantModel!.avatarModel, avatarSize: maxValue/2, hidePresence: true)
										}
										
										Spacer()
									}
									.frame(width: maxValue, height: maxValue)
									
									if callViewModel.videoDisplayed {
										LinphoneVideoViewHolder { view in
											coreContext.doOnCoreQueue { core in
												core.nativePreviewWindow = view
											}
										}
										.onDisappear {
											coreContext.doOnCoreQueue { core in
												core.nativePreviewWindow = nil
											}
										}
										.frame(
											width: 160 * ceil(maxValue / 120),
											height: 120 * ceil(maxValue / 120)
										)
										.scaledToFill()
										.clipped()
									}
									
									if callViewModel.myParticipantModel!.isMuted {
										VStack {
											HStack {
												Spacer()
												
												HStack(alignment: .center) {
													Image("microphone-slash")
														.renderingMode(.template)
														.resizable()
														.foregroundStyle(Color.grayMain2c800)
														.frame(width: 12, height: 12)
												}
												.padding(2)
												.background(.white)
												.cornerRadius(40)
											}
											Spacer()
										}
										.frame(maxWidth: .infinity)
										.padding(.all, 10)
									}
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
								.frame(width: maxValue, height: maxValue)
							}
							.frame(
								width: maxValue,
								height: maxValue,
								alignment: .center
							)
							.background(Color.gray600)
							.overlay(
								RoundedRectangle(cornerRadius: 20)
									.stroke(callViewModel.myParticipantModel!.isSpeaking ? .white : .clear, lineWidth: 4)
							)
							.cornerRadius(20)
						}
						
						ForEach(0..<callViewModel.participantList.count, id: \.self) { index in
							if index < callViewModel.participantList.count {
								ZStack {
									if callViewModel.participantList[index].isJoining {
										VStack {
											Spacer()
											
											ActivityIndicator(color: .white)
												.frame(width: maxValue/4, height: maxValue/4)
												.padding(.bottom, 5)
											
											Text("conference_participant_joining_text")
												.frame(maxWidth: .infinity, alignment: .center)
												.foregroundStyle(Color.white)
												.default_text_style_500(styleSize: 14)
												.lineLimit(1)
												.padding(.horizontal, 10)
											
											Spacer()
										}
									} else if callViewModel.participantList[index].onPause {
										VStack {
											Spacer()
											
											Image("pause")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(.white)
												.frame(width: maxValue/4, height: maxValue/4)
											
											Text("conference_participant_paused_text")
												.frame(maxWidth: .infinity, alignment: .center)
												.foregroundStyle(Color.white)
												.default_text_style_500(styleSize: 14)
												.lineLimit(1)
												.padding(.horizontal, 10)
											
											Spacer()
										}
									} else {
										VStack {
											Spacer()
											
											Avatar(contactAvatarModel: callViewModel.participantList[index].avatarModel, avatarSize: maxValue/2, hidePresence: true)
											
											Spacer()
										}
										.frame(width: maxValue, height: maxValue)
										
										LinphoneVideoViewHolder { view in
											coreContext.doOnCoreQueue { core in
												if index < callViewModel.participantList.count {
													let participantVideo = core.currentCall?.conference?.participantList.first(where: {$0.address!.weakEqual(address2: callViewModel.participantList[index].address)})
													if participantVideo != nil && participantVideo!.devices.first != nil {
														participantVideo!.devices.first!.nativeVideoWindowId = UnsafeMutableRawPointer(Unmanaged.passRetained(view).toOpaque())
													}
												}
											}
										}
										
										if callViewModel.participantList[index].isMuted {
											VStack {
												HStack {
													Spacer()
													
													HStack(alignment: .center) {
														Image("microphone-slash")
															.renderingMode(.template)
															.resizable()
															.foregroundStyle(Color.grayMain2c800)
															.frame(width: 12, height: 12)
													}
													.padding(2)
													.background(.white)
													.cornerRadius(40)
												}
												Spacer()
											}
											.frame(maxWidth: .infinity)
											.padding(.all, 10)
										}
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
									.frame(width: maxValue, height: maxValue)
								}
								.frame(
									width: maxValue,
									height: maxValue,
									alignment: .center
								)
								.background(Color.gray600)
								.overlay(
									RoundedRectangle(cornerRadius: 20)
										.stroke(callViewModel.participantList[index].isSpeaking ? .white : .clear, lineWidth: 4)
								)
								.cornerRadius(20)
							}
						}
					}
				}
			}
			.frame(maxWidth: .infinity, maxHeight: .infinity)
			.background(Color.gray900)
			.onTapGesture {
				fullscreenVideo.toggle()
			}
		}
	}
	
	func audioOnlyMode() -> some View {
		GeometryReader { geometry in
			VStack {
				let layout = [
					GridItem(.fixed((geometry.size.width/2)-10)),
					GridItem(.fixed((geometry.size.width/2)-10))
				]
				ScrollView {
					LazyVGrid(columns: layout) {
						if callViewModel.myParticipantModel != nil {
							HStack {
								Avatar(contactAvatarModel: callViewModel.myParticipantModel!.avatarModel, avatarSize: 50, hidePresence: true)
								
								Text(callViewModel.myParticipantModel!.name)
									.frame(maxWidth: .infinity, alignment: .leading)
									.foregroundStyle(Color.white)
									.default_text_style_500(styleSize: 14)
									.lineLimit(1)
									.padding(.horizontal, 10)
								
								if callViewModel.myParticipantModel!.isMuted {
									HStack(alignment: .center) {
										Image("microphone-slash")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c800)
											.frame(width: 20, height: 20)
									}
									.padding(2)
									.background(.white)
									.cornerRadius(40)
								}
								
								if callViewModel.myParticipantModel!.onPause {
									Image("pause")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(.white)
										.frame(width: 25, height: 25)
								}
							}
							.frame(height: 80)
							.padding(.all, 10)
							.background(Color.gray600)
							.overlay(
								RoundedRectangle(cornerRadius: 20)
									.stroke(callViewModel.myParticipantModel!.isSpeaking ? .white : .clear, lineWidth: 4)
							)
							.cornerRadius(20)
						}
						
						ForEach(0..<callViewModel.participantList.count, id: \.self) { index in
							HStack {
								Avatar(contactAvatarModel: callViewModel.participantList[index].avatarModel, avatarSize: 50, hidePresence: true)
								
								Text(callViewModel.participantList[index].name)
									.frame(maxWidth: .infinity, alignment: .leading)
									.foregroundStyle(Color.white)
									.default_text_style_500(styleSize: 14)
									.lineLimit(1)
									.padding(.horizontal, 10)
								
								if callViewModel.participantList[index].isMuted {
									HStack(alignment: .center) {
										Image("microphone-slash")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c800)
											.frame(width: 20, height: 20)
									}
									.padding(2)
									.background(.white)
									.cornerRadius(40)
								}
								
								if callViewModel.participantList[index].onPause {
									Image("pause")
										.renderingMode(.template)
										.resizable()
										.foregroundStyle(.white)
										.frame(width: 25, height: 25)
								}
							}
							.frame(height: 80)
							.padding(.all, 10)
							.background(Color.gray600)
							.overlay(
								RoundedRectangle(cornerRadius: 20)
									.stroke(callViewModel.participantList[index].isSpeaking ? .white : .clear, lineWidth: 4)
							)
							.cornerRadius(20)
						}
					}
				}
				.frame(maxWidth: .infinity, maxHeight: .infinity)
			}
			.background(Color.gray900)
			.onTapGesture {
				if fullscreenVideo {
					fullscreenVideo.toggle()
				}
			}
		}
	}
}

#Preview {
	CallView(
		fullscreenVideo: .constant(false),
		isShowStartCallFragment: .constant(false),
		isShowConversationFragment: .constant(false),
		isShowStartCallGroupPopup: .constant(false),
		isShowEditContactFragment: .constant(false),
		isShowScheduleMeetingFragment: .constant(false)
	)
}
// swiftlint:enable type_body_length
// swiftlint:enable line_length
// swiftlint:enable function_body_length
// swiftlint:enable file_length
