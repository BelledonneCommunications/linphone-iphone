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
import AVFAudio


struct BottomSheetContent: View {
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@EnvironmentObject var callViewModel: CallViewModel
	
	let pub = NotificationCenter.default.publisher(for: AVAudioSession.routeChangeNotification)
	
	let geo: GeometryProxy
	
	@Binding var buttonSize: Double
	
	@Binding var pointingUp: CGFloat
	@Binding var currentOffset: CGFloat
	var minBottomSheetHeight: CGFloat
	var maxBottomSheetHeight: CGFloat
	
	@Binding var optionsAudioRoute: Int
	@Binding var optionsChangeLayout: Int
	
	@Binding var showingDialer: Bool
	@Binding var audioRouteSheet: Bool
	@Binding var changeLayoutSheet: Bool
	@Binding var isShowStartCallFragment: Bool
	@Binding var isShowCallsListFragment: Bool
	@Binding var isShowParticipantsListFragment: Bool
	
	@Binding var imageAudioRoute: String
    
	var body: some View {
		let minHeight = minBottomSheetHeight * UIScreen.main.bounds.height
		let maxHeight = maxBottomSheetHeight * UIScreen.main.bounds.height
		
		let basePortraitSize = geo.size.width * 0.24
		let baseLandscapeSize = geo.size.width * 0.125
		
		let buttonPortraitDimension = min(max(basePortraitSize, 80), 140)
		let buttonLandscapeDimension = min(max(baseLandscapeSize, 80), 140)
		
		VStack(spacing: 0) {
            VStack {
                Button {
					withAnimation {
						currentOffset = currentOffset < (minHeight + maxHeight) / 2 ? maxHeight : minHeight
						
						pointingUp = -(((currentOffset - minHeight) / (maxHeight - minHeight)) - 0.5) * 2
					}
                } label: {
                    ChevronShape(pointingUp: pointingUp)
                        .stroke(style: StrokeStyle(lineWidth: 4, lineCap: .round))
                        .frame(width: 40, height: 6)
                        .foregroundStyle(.white)
                        .contentShape(Rectangle())
                        .padding(.top, 15)
                }
                .padding(10)
                
                Spacer()
                
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
                    .frame(width: buttonSize == 60 ? 90 : 70, height: buttonSize)
                    .background(Color.redDanger500)
                    .cornerRadius(40)
                    
                    Spacer()
                    
                    if !SharedMainViewModel.shared.disableVideoCall {
                        ZStack {
                            Button {
                                if optionsChangeLayout == 3 {
                                    optionsChangeLayout = 2
                                    callViewModel.toggleVideoMode(isAudioOnlyMode: false)
                                } else {
                                    callViewModel.displayMyVideo()
                                }
                            } label: {
                                HStack {
                                    Image(callViewModel.videoDisplayed ? "video-camera" : "video-camera-slash")
                                        .renderingMode(.template)
                                        .resizable()
                                        .foregroundStyle(.white)
                                        .frame(width: 32, height: 32)
                                }
                            }
                            .buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
                            .frame(width: buttonSize, height: buttonSize)
                            .background(Color.gray500)
                            .cornerRadius(40)
                            .disabled(callViewModel.isPaused || telecomManager.isPausedByRemote || telecomManager.outgoingCallStarted || optionsChangeLayout == 3)
                            
                            if callViewModel.isPaused || telecomManager.isPausedByRemote || telecomManager.outgoingCallStarted || optionsChangeLayout == 3 {
                                Color.gray600.opacity(0.8)
                                    .cornerRadius(40)
                                    .allowsHitTesting(false)
                            }
                        }
                        .frame(width: buttonSize, height: buttonSize)
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
                    .buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
                    .frame(width: buttonSize, height: buttonSize)
                    .background(callViewModel.micMutted ? Color.redDanger500 : Color.gray500)
                    .cornerRadius(40)
                    
                    if !callViewModel.hasAudioRouteRestriction {
                        Button {
                            if AVAudioSession.sharedInstance().availableInputs != nil
                                && !AVAudioSession.sharedInstance().availableInputs!.filter({ $0.portType.rawValue.contains("Bluetooth") }).isEmpty {
                                
                                DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
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
                            }
                        }
                        .buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
                        .frame(width: buttonSize, height: buttonSize)
                        .background(Color.gray500)
                        .cornerRadius(40)
                    }
                    Color.clear
                        .frame(width: 0, height: 0)
                        .onAppear {
                            self.getAudioRouteImage()
                            callViewModel.enforceEarpieceIfNeeded()
                        }
                        .onReceive(pub) { _ in
                            self.getAudioRouteImage()
                            callViewModel.enforceEarpieceIfNeeded()
                        }
                }
                .padding(.horizontal, 20)
                
                Spacer()
            }
            .frame(height: minHeight)
            .padding(.bottom, 10)
            
			ZStack(alignment: .top) {
				VStack(spacing: 0) {
					if geo.size.width < geo.size.height {
						HStack(spacing: 0) {
							if callViewModel.isOneOneCall {
								VStack {
									Button {
										withAnimation {
											callViewModel.isTransferInsteadCall = true
											isShowStartCallFragment.toggle()
										}
										
										DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
											telecomManager.callStarted = false
											DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
												telecomManager.callStarted = true
											}
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
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(Color.gray500)
									.cornerRadius(40)
									
									Text("call_action_blind_transfer")
										.foregroundStyle(.white)
										.default_text_style(styleSize: 15)
								}
								.frame(width: basePortraitSize, height: buttonPortraitDimension)
								
								VStack {
									Button {
										withAnimation {
											isShowStartCallFragment.toggle()
										}
										
										DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
											telecomManager.callStarted = false
											DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
												telecomManager.callStarted = true
											}
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
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(Color.gray500)
									.cornerRadius(40)
									
									Text("call_action_start_new_call")
										.foregroundStyle(.white)
										.default_text_style(styleSize: 15)
								}
								.frame(width: basePortraitSize, height: buttonPortraitDimension)
							} else {
								ZStack {
									VStack {
										Button {
										} label: {
											HStack {
												Image("screencast")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(.white)
													.frame(width: 32, height: 32)
											}
										}
										.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
										.frame(width: buttonSize, height: buttonSize)
										.background(Color.gray500)
										.cornerRadius(40)
										.disabled(true)
										
										Text("conference_action_screen_sharing")
											.foregroundStyle(.white)
											.default_text_style(styleSize: 15)
									}
									.frame(width: basePortraitSize, height: buttonPortraitDimension)
									
									if true {
										Color.gray600.opacity(0.8)
											.allowsHitTesting(false)
									}
								}
								.frame(width: basePortraitSize, height: buttonPortraitDimension)
								
								VStack {
									Button {
										withAnimation {
											isShowParticipantsListFragment.toggle()
										}
									} label: {
										HStack {
											Image("users")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(.white)
												.frame(width: 32, height: 32)
										}
									}
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(Color.gray500)
									.cornerRadius(40)
									
									Text("conference_action_show_participants")
										.foregroundStyle(.white)
										.default_text_style(styleSize: 15)
								}
								.frame(width: basePortraitSize, height: buttonPortraitDimension)
							}
							VStack {
								ZStack {
									Button {
										callViewModel.getCallsList()
										withAnimation {
											isShowCallsListFragment.toggle()
										}
										
										DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
											telecomManager.callStarted = false
											DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
												telecomManager.callStarted = true
											}
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
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(Color.gray500)
									.cornerRadius(40)
									
									if callViewModel.callsCounter > 1 {
										VStack {
											HStack {
												Spacer()
												
												VStack {
													Text("\(callViewModel.callsCounter)")
														.foregroundStyle(.white)
														.default_text_style(styleSize: 15)
												}
												.frame(width: 20, height: 20)
												.background(Color.redDanger500)
												.cornerRadius(10)
											}
											
											Spacer()
										}
										.frame(width: buttonSize, height: buttonSize)
									}
								}
								
								Text("call_action_go_to_calls_list")
									.foregroundStyle(.white)
									.default_text_style(styleSize: 15)
							}
							.frame(width: basePortraitSize, height: buttonPortraitDimension)
							
							if callViewModel.isOneOneCall {
								VStack {
									Button {
										showingDialer.toggle()
										DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
											telecomManager.callStarted = false
											DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
												telecomManager.callStarted = true
											}
										}
									} label: {
										HStack {
											Image("dialer")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(.white)
												.frame(width: 32, height: 32)
										}
									}
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(Color.gray500)
									.cornerRadius(40)
									
									Text("call_action_show_dialer")
										.foregroundStyle(.white)
										.default_text_style(styleSize: 15)
								}
								.frame(width: basePortraitSize, height: buttonPortraitDimension)
							} else {
								ZStack {
									VStack {
										Button {
											changeLayoutSheet = true
										} label: {
											HStack {
												Image("layout")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(.white)
													.frame(width: 32, height: 32)
											}
										}
										.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
										.frame(width: buttonSize, height: buttonSize)
										.background(Color.gray500)
										.cornerRadius(40)
										.disabled(callViewModel.activeSpeakerParticipant?.isScreenSharing == true)
										
										Text("call_action_change_layout")
											.foregroundStyle(.white)
											.default_text_style(styleSize: 15)
									}
									.frame(width: basePortraitSize, height: buttonPortraitDimension)
									
									if callViewModel.activeSpeakerParticipant?.isScreenSharing == true {
										Color.gray600.opacity(0.8)
											.allowsHitTesting(false)
									}
								}
								.frame(width: basePortraitSize, height: buttonPortraitDimension)
							}
						}
						
						HStack(spacing: 0) {
							if !AppServices.corePreferences.disableChatFeature && callViewModel.chatEnabled {
								VStack {
									Button {
										callViewModel.createConversation()
									} label: {
										HStack {
											if !callViewModel.operationInProgress {
												Image("chat-teardrop-text")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(.white)
													.frame(width: 32, height: 32)
											} else {
												ProgressView()
													.controlSize(.mini)
													.progressViewStyle(CircularProgressViewStyle(tint: .white))
													.frame(width: 32, height: 32, alignment: .center)
													.onDisappear {
														if SharedMainViewModel.shared.displayedConversation != nil {
															SharedMainViewModel.shared.changeIndexView(indexViewInt: 2)
															callViewModel.changeDisplayedChatRoom(conversationModel: SharedMainViewModel.shared.displayedConversation!)
															SharedMainViewModel.shared.displayedConversation = nil
															withAnimation {
																telecomManager.callDisplayed = false
															}
														}
													}
											}
										}
									}
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(Color.gray500)
									.cornerRadius(40)
									
									Text("call_action_show_messages")
										.foregroundStyle(.white)
										.default_text_style(styleSize: 15)
								}
								.frame(width: basePortraitSize, height: buttonPortraitDimension)
							}
							
							ZStack {
								VStack {
									Button {
										callViewModel.togglePause()
									} label: {
										HStack {
											Image(callViewModel.isPaused ? "play" : "pause")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(.white)
												.frame(width: 32, height: 32)
										}
									}
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(callViewModel.isPaused ? Color.greenSuccess500 : Color.gray500)
									.cornerRadius(40)
									.disabled(telecomManager.isPausedByRemote)
									
									Text("call_action_pause_call")
										.foregroundStyle(.white)
										.default_text_style(styleSize: 15)
								}
								.frame(width: basePortraitSize, height: buttonPortraitDimension)
								
								if telecomManager.isPausedByRemote {
									Color.gray600.opacity(0.8)
										.allowsHitTesting(false)
								}
							}
							.frame(width: basePortraitSize, height: buttonPortraitDimension)
							
							if callViewModel.isOneOneCall {
								ZStack {
									VStack {
										Button {
											callViewModel.toggleRecording()
										} label: {
											HStack {
												Image("record-fill")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(.white)
													.frame(width: 32, height: 32)
											}
										}
										.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
										.frame(width: buttonSize, height: buttonSize)
										.background(callViewModel.isRecording ? Color.redDanger500 : Color.gray500)
										.cornerRadius(40)
										.disabled(AppServices.corePreferences.disableCallRecordings || callViewModel.isPaused || telecomManager.isPausedByRemote)
										
										Text("call_action_record_call")
											.foregroundStyle(.white)
											.default_text_style(styleSize: 15)
									}
									.frame(width: basePortraitSize, height: buttonPortraitDimension)
									
									if AppServices.corePreferences.disableCallRecordings || callViewModel.isPaused || telecomManager.isPausedByRemote {
										Color.gray600.opacity(0.8)
											.allowsHitTesting(false)
									}
								}
								.frame(width: basePortraitSize, height: buttonPortraitDimension)
							} else {
								ZStack {
									VStack {
										Button {
										} label: {
											HStack {
												Image("record-fill")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(.white)
													.frame(width: 32, height: 32)
											}
										}
										.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
										.frame(width: buttonSize, height: buttonSize)
										.background(Color.gray500)
										.cornerRadius(40)
										.disabled(true)
										
										Text("call_action_record_call")
											.foregroundStyle(.white)
											.default_text_style(styleSize: 15)
									}
									.frame(width: basePortraitSize, height: buttonPortraitDimension)
									
									Color.gray600.opacity(0.8)
										.allowsHitTesting(false)
								}
								.frame(width: basePortraitSize, height: buttonPortraitDimension)
							}
							
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
								.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
								.frame(width: buttonSize, height: buttonSize)
								.background(Color.gray500)
								.cornerRadius(40)
								
								Text("call_action_change_layout")
									.foregroundStyle(.white)
									.default_text_style(styleSize: 15)
							}
							.frame(width: basePortraitSize, height: buttonPortraitDimension)
							.hidden()
							
							if AppServices.corePreferences.disableChatFeature || !callViewModel.chatEnabled {
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
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(Color.gray500)
									.cornerRadius(40)
									
									Text("call_action_change_layout")
										.foregroundStyle(.white)
										.default_text_style(styleSize: 15)
								}
								.frame(width: basePortraitSize, height: buttonPortraitDimension)
								.hidden()
							}
						}
					} else {
						HStack {
							if callViewModel.isOneOneCall {
								VStack {
									Button {
										withAnimation {
											callViewModel.isTransferInsteadCall = true
											isShowStartCallFragment.toggle()
										}
										
										DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
											telecomManager.callStarted = false
											DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
												telecomManager.callStarted = true
											}
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
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(Color.gray500)
									.cornerRadius(40)
									
									Text("call_action_blind_transfer")
										.foregroundStyle(.white)
										.default_text_style(styleSize: 15)
								}
								.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
								
								VStack {
									Button {
										withAnimation {
											isShowStartCallFragment.toggle()
										}
										
										DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
											telecomManager.callStarted = false
											DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
												telecomManager.callStarted = true
											}
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
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(Color.gray500)
									.cornerRadius(40)
									
									Text("call_action_start_new_call")
										.foregroundStyle(.white)
										.default_text_style(styleSize: 15)
								}
								.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
							} else {
								ZStack {
									VStack {
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
											.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
											.frame(width: buttonSize, height: buttonSize)
											.background(.white)
											.cornerRadius(40)
											.disabled(true)
											
											Text("conference_action_screen_sharing")
												.foregroundStyle(.white)
												.default_text_style(styleSize: 15)
										}
									}
									.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
									
									if true {
										Color.gray600.opacity(0.8)
											.allowsHitTesting(false)
									}
								}
								.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
								
								VStack {
									Button {
										withAnimation {
											isShowParticipantsListFragment.toggle()
										}
									} label: {
										HStack {
											Image("users")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(.white)
												.frame(width: 32, height: 32)
										}
									}
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(Color.gray500)
									.cornerRadius(40)
									
									Text("conference_action_show_participants")
										.foregroundStyle(.white)
										.default_text_style(styleSize: 15)
								}
								.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
							}
							
							VStack {
								ZStack {
									Button {
										callViewModel.getCallsList()
										withAnimation {
											isShowCallsListFragment.toggle()
										}
										
										DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
											telecomManager.callStarted = false
											DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
												telecomManager.callStarted = true
											}
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
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(Color.gray500)
									.cornerRadius(40)
									
									if callViewModel.callsCounter > 1 {
										VStack {
											HStack {
												Spacer()
												
												VStack {
													Text("\(callViewModel.callsCounter)")
														.foregroundStyle(.white)
														.default_text_style(styleSize: 15)
												}
												.frame(width: 20, height: 20)
												.background(Color.redDanger500)
												.cornerRadius(10)
											}
											
											Spacer()
										}
										.frame(width: buttonSize, height: buttonSize)
									}
								}
								
								Text("call_action_go_to_calls_list")
									.foregroundStyle(.white)
									.default_text_style(styleSize: 15)
							}
							.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
							
							if callViewModel.isOneOneCall {
								VStack {
									Button {
										showingDialer.toggle()
										DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
											telecomManager.callStarted = false
											DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
												telecomManager.callStarted = true
											}
										}
									} label: {
										HStack {
											Image("dialer")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(.white)
												.frame(width: 32, height: 32)
										}
									}
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(Color.gray500)
									.cornerRadius(40)
									
									Text("call_action_show_dialer")
										.foregroundStyle(.white)
										.default_text_style(styleSize: 15)
								}
								.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
							} else {
								VStack {
									Button {
										changeLayoutSheet = true
									} label: {
										HStack {
											Image("layout")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(.white)
												.frame(width: 32, height: 32)
										}
									}
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(Color.gray500)
									.cornerRadius(40)
									
									Text("call_action_change_layout")
										.foregroundStyle(.white)
										.default_text_style(styleSize: 15)
								}
								.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
							}
							
							if !AppServices.corePreferences.disableChatFeature && callViewModel.chatEnabled {
								VStack {
									Button {
										callViewModel.createConversation()
									} label: {
										HStack {
											if !callViewModel.operationInProgress {
												Image("chat-teardrop-text")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(.white)
													.frame(width: 32, height: 32)
											} else {
												ProgressView()
													.controlSize(.mini)
													.progressViewStyle(CircularProgressViewStyle(tint: .white))
													.frame(width: 32, height: 32, alignment: .center)
													.onDisappear {
														if SharedMainViewModel.shared.displayedConversation != nil {
															SharedMainViewModel.shared.changeIndexView(indexViewInt: 2)
															callViewModel.changeDisplayedChatRoom(conversationModel: SharedMainViewModel.shared.displayedConversation!)
															SharedMainViewModel.shared.displayedConversation = nil
															withAnimation {
																telecomManager.callDisplayed = false
															}
														}
													}
											}
										}
									}
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(Color.gray500)
									.cornerRadius(40)
									
									Text("call_action_show_messages")
										.foregroundStyle(.white)
										.default_text_style(styleSize: 15)
								}
								.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
							}
							
							ZStack {
								VStack {
									Button {
										callViewModel.togglePause()
									} label: {
										HStack {
											Image(callViewModel.isPaused ? "play" : "pause")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(.white)
												.frame(width: 32, height: 32)
										}
									}
									.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
									.frame(width: buttonSize, height: buttonSize)
									.background(callViewModel.isPaused ? Color.greenSuccess500 : Color.gray500)
									.cornerRadius(40)
									.disabled(telecomManager.isPausedByRemote)
									
									Text("call_action_pause_call")
										.foregroundStyle(.white)
										.default_text_style(styleSize: 15)
								}
								.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
								
								if telecomManager.isPausedByRemote {
									Color.gray600.opacity(0.8)
										.allowsHitTesting(false)
								}
							}
							.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
							
							if callViewModel.isOneOneCall {
								ZStack {
									VStack {
										Button {
											callViewModel.toggleRecording()
										} label: {
											HStack {
												Image("record-fill")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(.white)
													.frame(width: 32, height: 32)
											}
										}
										.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
										.frame(width: buttonSize, height: buttonSize)
										.background(callViewModel.isRecording ? Color.redDanger500 : Color.gray500)
										.cornerRadius(40)
										.disabled(AppServices.corePreferences.disableCallRecordings || callViewModel.isPaused || telecomManager.isPausedByRemote)
										
										Text("call_action_record_call")
											.foregroundStyle(.white)
											.default_text_style(styleSize: 15)
									}
									.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
									
									if AppServices.corePreferences.disableCallRecordings || callViewModel.isPaused || telecomManager.isPausedByRemote {
										Color.gray600.opacity(0.8)
											.allowsHitTesting(false)
									}
								}
								.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
							} else {
								ZStack {
									VStack {
										Button {
										} label: {
											HStack {
												Image("record-fill")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(Color.gray500)
													.frame(width: 32, height: 32)
											}
										}
										.buttonStyle(PressedButtonStyle(buttonSize: buttonSize))
										.frame(width: buttonSize, height: buttonSize)
										.background(.white)
										.cornerRadius(40)
										.disabled(true)
										
										Text("call_action_record_call")
											.foregroundStyle(.white)
											.default_text_style(styleSize: 15)
									}
									.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
									
									if true {
										Color.gray600.opacity(0.8)
											.allowsHitTesting(false)
									}
								}
								.frame(width: baseLandscapeSize, height: buttonLandscapeDimension)
							}
						}
						.padding(.horizontal, 20)
					}
					
					Spacer()
				}
				
				if currentOffset <= minHeight {
					Color.gray600
				}
			}
			.frame(height: maxHeight - minHeight)
			
		}
		.frame(height: maxHeight)
		.background(Color.gray600)
	}
	
	func getAudioRouteImage() {
		if !AVAudioSession.sharedInstance().currentRoute.outputs.filter({ $0.portType.rawValue == "Speaker" }).isEmpty {
			imageAudioRoute = "speaker-high"
			optionsAudioRoute = 2
		} else if !AVAudioSession.sharedInstance().currentRoute.outputs.filter({ $0.portType.rawValue.contains("Bluetooth") }).isEmpty {
			imageAudioRoute = "bluetooth"
			optionsAudioRoute = 3
		} else {
			imageAudioRoute = callViewModel.isHeadPhoneAvailable() ? "headset" : "speaker-slash"
			optionsAudioRoute = 1
		}
	}
}

struct BottomSheetView<Content: View>: View {
	let content: Content
	
	@State var minHeight: CGFloat
	@State var maxHeight: CGFloat
	
	@Binding var currentOffset: CGFloat
	@Binding var pointingUp: CGFloat
	
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
	var buttonSize: CGFloat
	func makeBody(configuration: Self.Configuration) -> some View {
		configuration.label
		.frame(width: buttonSize, height: buttonSize)
		.background(configuration.isPressed ? .white : .clear)
		.cornerRadius(40)
	}
}
