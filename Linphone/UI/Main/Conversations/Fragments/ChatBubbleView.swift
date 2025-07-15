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
import WebKit
import QuickLook

// swiftlint:disable type_body_length
// swiftlint:disable cyclomatic_complexity
struct ChatBubbleView: View {
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@EnvironmentObject var conversationViewModel: ConversationViewModel
	
	let eventLogMessage: EventLogMessage
	
	let geometryProxy: GeometryProxy
	
	@State private var ticker = Ticker()
	@State private var isPressed: Bool = false
	@State private var didLongPress = false
	@State private var timePassed: TimeInterval?
	
	@State private var timer: Timer?
	@State private var ephemeralLifetime: String = ""
	
	@State private var selectedAttachment: Bool = false
	@State private var selectedAttachmentIndex: Int = 0
	
	@State private var selectedURLAttachment: URL?
	
	@State private var showShareSheet = false
	
	var body: some View {
		HStack {
			if eventLogMessage.eventModel.eventLogType == .ConferenceChatMessage {
				VStack {
					if !eventLogMessage.message.text.isEmpty || !eventLogMessage.message.attachments.isEmpty || eventLogMessage.message.isIcalendar {
						HStack(alignment: .top, content: {
							if eventLogMessage.message.isOutgoing {
								Spacer()
							}
							if SharedMainViewModel.shared.displayedConversation != nil && SharedMainViewModel.shared.displayedConversation!.isGroup
								&& !eventLogMessage.message.isOutgoing && eventLogMessage.message.isFirstMessage {
								VStack {
									Avatar(
										contactAvatarModel: conversationViewModel.participantConversationModel.first(where: {$0.address == eventLogMessage.message.address}) ??
										ContactAvatarModel(friend: nil, name: "??", address: "", withPresence: false),
										avatarSize: 35
									)
									.padding(.top, 30)
								}
							} else if SharedMainViewModel.shared.displayedConversation != nil
										&& SharedMainViewModel.shared.displayedConversation!.isGroup && !eventLogMessage.message.isOutgoing {
								VStack {
								}
								.padding(.leading, 43)
							}
							
							VStack(alignment: .leading, spacing: 0) {
								if SharedMainViewModel.shared.displayedConversation != nil && SharedMainViewModel.shared.displayedConversation!.isGroup
									&& !eventLogMessage.message.isOutgoing && eventLogMessage.message.isFirstMessage {
									Text(conversationViewModel.participantConversationModel.first(where: {$0.address == eventLogMessage.message.address})?.name ?? "")
										.default_text_style(styleSize: 12)
										.padding(.top, 5)
										.padding(.bottom, 2)
								}
								
								if eventLogMessage.message.isForward {
									HStack {
										if eventLogMessage.message.isOutgoing {
											Spacer()
										}
										
										VStack(alignment: eventLogMessage.message.isOutgoing ? .trailing : .leading, spacing: 0) {
											HStack {
												Image("forward")
													.resizable()
													.frame(width: 15, height: 15, alignment: .leading)
												
												Text("message_forwarded_label")
													.default_text_style(styleSize: 12)
											}
											.padding(.bottom, 2)
										}
										
										if !eventLogMessage.message.isOutgoing {
											Spacer()
										}
									}
									.frame(maxWidth: .infinity)
								}
								
								if eventLogMessage.message.replyMessage != nil {
									HStack {
										if eventLogMessage.message.isOutgoing {
											Spacer()
										}
										
										VStack(alignment: eventLogMessage.message.isOutgoing ? .trailing : .leading, spacing: 0) {
											HStack {
												Image("reply")
													.resizable()
													.frame(width: 15, height: 15, alignment: .leading)
												
												Text(conversationViewModel.participantConversationModel.first(
													where: {$0.address == eventLogMessage.message.replyMessage!.address})?.name ?? "")
												.default_text_style(styleSize: 12)
											}
											.padding(.bottom, 2)
											
											VStack(alignment: eventLogMessage.message.isOutgoing ? .trailing : .leading) {
												if !eventLogMessage.message.replyMessage!.text.isEmpty {
													Text(eventLogMessage.message.replyMessage!.text)
														.foregroundStyle(Color.grayMain2c700)
														.default_text_style(styleSize: 14)
														.lineLimit(/*@START_MENU_TOKEN@*/2/*@END_MENU_TOKEN@*/)
												} else if !eventLogMessage.message.replyMessage!.attachmentsNames.isEmpty {
													Text(eventLogMessage.message.replyMessage!.attachmentsNames)
														.foregroundStyle(Color.grayMain2c700)
														.default_text_style(styleSize: 14)
														.lineLimit(/*@START_MENU_TOKEN@*/2/*@END_MENU_TOKEN@*/)
												}
											}
											.padding(.all, 15)
											.padding(.bottom, 15)
											.background(Color.gray200)
											.clipShape(RoundedRectangle(cornerRadius: 1))
											.roundedCorner(
												16,
												corners: eventLogMessage.message.isOutgoing ? [.topLeft, .topRight, .bottomLeft] : [.topLeft, .topRight, .bottomRight]
											)
										}
										.onTapGesture {
											conversationViewModel.scrollToMessage(message: eventLogMessage.message)
										}
										
										if !eventLogMessage.message.isOutgoing {
											Spacer()
										}
									}
									.frame(maxWidth: .infinity)
									.padding(.bottom, -20)
								}
								
								ZStack {
									HStack {
										if eventLogMessage.message.isOutgoing {
											Spacer()
										}
										
										VStack(alignment: eventLogMessage.message.isOutgoing ? .trailing : .leading) {
											VStack(alignment: eventLogMessage.message.isOutgoing ? .trailing : .leading) {
												if !eventLogMessage.message.attachments.isEmpty && !eventLogMessage.message.isIcalendar {
													messageAttachments()
												}
												
												if !eventLogMessage.message.text.isEmpty {
													DynamicLinkText(text: eventLogMessage.message.text)
												}
												
												if eventLogMessage.message.isIcalendar && eventLogMessage.message.messageConferenceInfo != nil {
													VStack(spacing: 0) {
														VStack {
															if eventLogMessage.message.messageConferenceInfo!.meetingState != .new {
																if eventLogMessage.message.messageConferenceInfo!.meetingState == .updated {
																	Text("conversation_message_meeting_updated_label")
																		.foregroundStyle(Color.orangeWarning600)
																		.default_text_style_600(styleSize: 12)
																		.lineLimit(1)
																		.frame(maxWidth: .infinity, alignment: .leading)
																		.padding(.bottom, 5)
																} else {
																	Text("conversation_message_meeting_cancelled_label")
																		.foregroundStyle(Color.redDanger500)
																		.default_text_style_600(styleSize: 12)
																		.lineLimit(1)
																		.frame(maxWidth: .infinity, alignment: .leading)
																		.padding(.bottom, 5)
																}
															}
															
															HStack {
																VStack(spacing: 0) {
																	Text(eventLogMessage.message.messageConferenceInfo!.meetingDay)
																		.default_text_style(styleSize: 16)
																	
																	Text(eventLogMessage.message.messageConferenceInfo!.meetingDayNumber)
																		.foregroundStyle(.white)
																		.default_text_style_800(styleSize: 18)
																		.lineLimit(1)
																		.frame(width: 30, height: 30, alignment: .center)
																		.background(Color.orangeMain500)
																		.clipShape(Circle())
																	
																}
																.padding(.all, 10)
																.frame(width: 70, height: 70)
																.background(.white)
																.cornerRadius(15)
																.shadow(color: .black.opacity(0.1), radius: 15)
																
																VStack {
																	HStack {
																		Image("video-conference")
																			.renderingMode(.template)
																			.resizable()
																			.foregroundStyle(Color.grayMain2c600)
																			.frame(width: 25, height: 25)
																		
																		Text(eventLogMessage.message.messageConferenceInfo!.meetingSubject)
																			.default_text_style_800(styleSize: 15)
																			.lineLimit(1)
																			.frame(maxWidth: .infinity, alignment: .leading)
																	}
																	.frame(maxWidth: .infinity, alignment: .leading)
																	
																	Text(eventLogMessage.message.messageConferenceInfo!.meetingDate)
																		.default_text_style_300(styleSize: 14)
																		.lineLimit(1)
																		.frame(maxWidth: .infinity, alignment: .leading)
																	
																	Text(eventLogMessage.message.messageConferenceInfo!.meetingTime)
																		.default_text_style_300(styleSize: 14)
																		.lineLimit(1)
																		.frame(maxWidth: .infinity, alignment: .leading)
																}
																.padding(.leading, 5)
															}
															.frame(maxWidth: .infinity)
														}
														.padding(.all, 15)
														.frame(maxWidth: .infinity)
														.background(Color.gray100)
														
														VStack(spacing: 2) {
															if !eventLogMessage.message.messageConferenceInfo!.meetingDescription.isEmpty {
																Text("meeting_schedule_description_title")
																	.default_text_style(styleSize: 14)
																	.frame(maxWidth: .infinity, alignment: .leading)
																
																Text(eventLogMessage.message.messageConferenceInfo!.meetingDescription)
																	.default_text_style_300(styleSize: 14)
																	.frame(maxWidth: .infinity, alignment: .leading)
															}
															
															if eventLogMessage.message.messageConferenceInfo!.meetingState != .cancelled {
																HStack {
																	Image("users")
																		.renderingMode(.template)
																		.resizable()
																		.foregroundStyle(Color.grayMain2c600)
																		.frame(width: 20, height: 20)
																	
																	Text(eventLogMessage.message.messageConferenceInfo!.meetingParticipants)
																		.default_text_style(styleSize: 14)
																		.frame(maxWidth: .infinity, alignment: .leading)
																	
																	Button(action: {
																		conversationViewModel.joinMeetingInvite(addressUri: eventLogMessage.message.messageConferenceInfo!.meetingConferenceUri)
																	}, label: {
																		Text("meeting_waiting_room_join")
																			.default_text_style_white_600(styleSize: 14)
																	})
																	.padding(.horizontal, 15)
																	.padding(.vertical, 10)
																	.background(Color.orangeMain500)
																	.cornerRadius(60)
																}
																.padding(.top, !eventLogMessage.message.messageConferenceInfo!.meetingDescription.isEmpty ? 10 : 0)
															}
														}
														.padding(.all,
															eventLogMessage.message.messageConferenceInfo!.meetingState != .cancelled
															|| !eventLogMessage.message.messageConferenceInfo!.meetingDescription.isEmpty
															? 15
															: 0
														)
														.frame(maxWidth: .infinity)
														.background(.white)
													}
													.frame(width: geometryProxy.size.width >= 110 ? geometryProxy.size.width - 110 : geometryProxy.size.width)
													.background(.white)
													.cornerRadius(10)
												}
												
												HStack(alignment: .center) {
													if eventLogMessage.message.isEphemeral && eventLogMessage.message.isOutgoing {
														Text(ephemeralLifetime)
															.foregroundStyle(Color.grayMain2c500)
															.default_text_style_300(styleSize: 12)
															.padding(.top, 1)
															.padding(.trailing, -4)
															.onAppear {
																updateEphemeralTimer()
															}
															.onChange(of: eventLogMessage.message.ephemeralExpireTime) { ephemeralExpireTimeTmp in
																if ephemeralExpireTimeTmp > 0 {
																	updateEphemeralTimer()
																}
															}
														
														Image("clock-countdown")
															.renderingMode(.template)
															.resizable()
															.foregroundStyle(Color.grayMain2c500)
															.frame(width: 15, height: 15)
															.padding(.top, 1)
													}
													
													Text(conversationViewModel.getMessageTime(startDate: eventLogMessage.message.dateReceived))
														.foregroundStyle(Color.grayMain2c500)
														.default_text_style_300(styleSize: 12)
														.padding(.top, 1)
														.padding(.trailing, -4)
													
													if (SharedMainViewModel.shared.displayedConversation != nil && SharedMainViewModel.shared.displayedConversation!.isGroup)
														|| eventLogMessage.message.isOutgoing {
														if eventLogMessage.message.status == .sending {
															ProgressView()
																.controlSize(.mini)
																.progressViewStyle(CircularProgressViewStyle(tint: .orangeMain500))
																.frame(width: 10, height: 10)
																.padding(.top, 1)
														} else if eventLogMessage.message.status != nil && !(CoreContext.shared.imdnToEverybodyThreshold && !eventLogMessage.message.isOutgoing) {
															Image(conversationViewModel.getImageIMDN(status: eventLogMessage.message.status!))
																.renderingMode(.template)
																.resizable()
																.foregroundStyle(Color.orangeMain500)
																.frame(width: 15, height: 15)
																.padding(.top, 1)
														}
													}
													
													if eventLogMessage.message.isEphemeral && !eventLogMessage.message.isOutgoing {
														Image("clock-countdown")
															.renderingMode(.template)
															.resizable()
															.foregroundStyle(Color.grayMain2c500)
															.frame(width: 15, height: 15)
															.padding(.top, 1)
															.padding(.trailing, -4)
														
														Text(ephemeralLifetime)
															.foregroundStyle(Color.grayMain2c500)
															.default_text_style_300(styleSize: 12)
															.padding(.top, 1)
															.onAppear {
																updateEphemeralTimer()
															}
															.onChange(of: eventLogMessage.message.ephemeralExpireTime) { ephemeralExpireTimeTmp in
																if ephemeralExpireTimeTmp > 0 {
																	updateEphemeralTimer()
																}
															}
													}
												}
												.onTapGesture {
													if !(CoreContext.shared.imdnToEverybodyThreshold && !eventLogMessage.message.isOutgoing) {
														conversationViewModel.selectedMessageToDisplayDetails = eventLogMessage
														conversationViewModel.prepareBottomSheetForDeliveryStatus()
													}
												}
												.disabled(conversationViewModel.selectedMessage != nil)
												.padding(.top, -4)
											}
											.padding(.all, 15)
											.background(eventLogMessage.message.isOutgoing ? Color.orangeMain100 : Color.grayMain2c100)
											.clipShape(RoundedRectangle(cornerRadius: 3))
											.roundedCorner(
												16,
												corners: eventLogMessage.message.isOutgoing && eventLogMessage.message.isFirstMessage ? [.topLeft, .topRight, .bottomLeft] :
													(!eventLogMessage.message.isOutgoing && eventLogMessage.message.isFirstMessage ? [.topRight, .bottomRight, .bottomLeft] : [.allCorners]))
											
											if !eventLogMessage.message.reactions.isEmpty {
												HStack {
													ForEach(0..<eventLogMessage.message.reactions.count, id: \.self) { index in
														if eventLogMessage.message.reactions.firstIndex(of: eventLogMessage.message.reactions[index]) == index {
															Text(eventLogMessage.message.reactions[index])
																.default_text_style(styleSize: 12)
																.padding(.horizontal, -2)
														}
													}
													
													if containsDuplicates(strings: eventLogMessage.message.reactions) {
														Text("\(eventLogMessage.message.reactions.count)")
															.default_text_style(styleSize: 12)
															.padding(.horizontal, -2)
													}
												}
												.onTapGesture {
													conversationViewModel.selectedMessageToDisplayDetails = eventLogMessage
													conversationViewModel.prepareBottomSheetForReactions()
												}
												.disabled(conversationViewModel.selectedMessage != nil)
												.padding(.vertical, 6)
												.padding(.horizontal, 10)
												.background(eventLogMessage.message.isOutgoing ? Color.orangeMain100 : Color.grayMain2c100)
												.cornerRadius(20)
												.overlay(
													RoundedRectangle(cornerRadius: 20)
														.stroke(.white, lineWidth: 3)
												)
												.padding(.top, -20)
												.padding(eventLogMessage.message.isOutgoing ? .trailing : .leading, 5)
											}
										}
										
										if !eventLogMessage.message.isOutgoing {
											Spacer()
										}
									}
									.frame(maxWidth: .infinity)
								}
								.frame(maxWidth: .infinity)
							}
							
							if !eventLogMessage.message.isOutgoing {
								Spacer()
							}
						})
						.padding(.leading, eventLogMessage.message.isOutgoing ? 40 : 0)
						.padding(.trailing, !eventLogMessage.message.isOutgoing ? 40 : 0)
					}
				}
				.onTapGesture {}
				.onLongPressGesture(minimumDuration: .infinity, maximumDistance: .infinity, pressing: { value in
					if !self.conversationViewModel.isSwiping {
						self.isPressed = value
						if !value {
							if !didLongPress {
								self.isPressed = false
							}
						} else {
							self.timePassed = 0
							self.ticker.start(interval: 0.2)
							self.didLongPress = false
						}
					} else {
						self.ticker.stop()
						return
					}
				}, perform: {})
				.onReceive(ticker.objectWillChange) { _ in
					guard isPressed else {
						ticker.stop()
						return
					}

					timePassed = ticker.timeIntervalSinceStarted

					if let timePassed = timePassed, timePassed >= 0.2 {
						didLongPress = true
						if !conversationViewModel.isSwiping {
							withAnimation {
								conversationViewModel.selectedMessage = eventLogMessage
							}
						}
					}
				}
			} else if !eventLogMessage.eventModel.text.isEmpty {
				HStack {
					Spacer()
					
					HStack {
						if eventLogMessage.eventModel.icon != nil {
							eventLogMessage.eventModel.icon!
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c500)
								.frame(width: 25, height: 25, alignment: .leading)
						}
						
						Text(eventLogMessage.eventModel.text)
							.foregroundStyle(Color.grayMain2c500)
							.default_text_style(styleSize: 12)
					}
					.padding(.horizontal, 10)
					.padding(.vertical, 4)
					.overlay(
						RoundedRectangle(cornerRadius: 6)
							.stroke(Color.grayMain2c200, lineWidth: 1)
					)
					
					Spacer()
				}
				.padding(.vertical, 10)
			}
		}
		.contentShape(Rectangle())
		.onTapGesture {
			if conversationViewModel.selectedMessage != nil {
				conversationViewModel.selectedMessage = nil
			}
			UIApplication.shared.endEditing()
		}
		.quickLookPreview($selectedURLAttachment, in: conversationViewModel.attachments.map { $0.full })
	}
	
	func containsDuplicates(strings: [String]) -> Bool {
		let uniqueStrings = Set(strings)
		return uniqueStrings.count != strings.count
	}
	
	@ViewBuilder
	func messageAttachments() -> some View {
		if eventLogMessage.message.attachments.count == 1 {
			if eventLogMessage.message.attachments.first!.type == .image || eventLogMessage.message.attachments.first!.type == .gif 
				|| eventLogMessage.message.attachments.first!.type == .video {
				let result = imageDimensions(url: eventLogMessage.message.attachments.first!.thumbnail.absoluteString)
				ZStack {
					Rectangle()
						.fill(Color(.white))
						.aspectRatio(result.0/result.1, contentMode: .fit)
						.if(result.0 < geometryProxy.size.width - 110) { view in
							view.frame(maxWidth: result.0)
						}
						.if(result.1 < UIScreen.main.bounds.height/2) { view in
							view.frame(maxHeight: result.1)
						}
						.if(result.0 >= result.1 && geometryProxy.size.width > 0 && result.0 >= geometryProxy.size.width - 110 
							&& result.1 >= UIScreen.main.bounds.height/2.5) { view in
							view.frame(
								maxWidth: geometryProxy.size.width - 110,
								maxHeight: result.1 * ((geometryProxy.size.width - 110) / result.0)
							)
						}
						.if(result.0 < result.1 && geometryProxy.size.width > 0 && result.1 >= UIScreen.main.bounds.height/2.5) { view in
							view.frame(
								maxWidth: result.0 * ((UIScreen.main.bounds.height/2.5) / result.1),
								maxHeight: UIScreen.main.bounds.height/2.5
							)
						}
					
					if eventLogMessage.message.attachments.first!.type == .image || eventLogMessage.message.attachments.first!.type == .video {
						if #available(iOS 16.0, *) {
							CachedAsyncImage(
								url: eventLogMessage.message.attachments.first!.thumbnail,
								placeholder: ProgressView(),
								onImageTapped: {
									if !isPressed && !didLongPress {
										selectedURLAttachment = eventLogMessage.message.attachments.first!.full
									}
								})
							.overlay(
								Group {
									if eventLogMessage.message.attachments.first!.type == .video {
										Image("play-fill")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(.white)
											.frame(width: 40, height: 40)
									}
								}
							)
							.layoutPriority(-1)
							.clipShape(RoundedRectangle(cornerRadius: 4))
						} else {
							CachedAsyncImage(
								url: eventLogMessage.message.attachments.first!.thumbnail,
								placeholder: ProgressView(),
								onImageTapped: {
									if !isPressed && !didLongPress {
										selectedURLAttachment = eventLogMessage.message.attachments.first!.full
									}
								})
							
							.overlay(
								Group {
									if eventLogMessage.message.attachments.first!.type == .video {
										Image("play-fill")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(.white)
											.frame(width: 40, height: 40)
									}
								}
							)
							.layoutPriority(-1)
							.clipShape(RoundedRectangle(cornerRadius: 4))
							.id(UUID())
						}
					} else if eventLogMessage.message.attachments.first!.type == .gif {
						if #available(iOS 16.0, *) {
							GifImageView(eventLogMessage.message.attachments.first!.thumbnail)
								.layoutPriority(-1)
								.clipShape(RoundedRectangle(cornerRadius: 4))
								.contentShape(Rectangle())
								.onTapGesture {
									if !isPressed && !didLongPress {
										selectedURLAttachment = eventLogMessage.message.attachments.first!.full
									}
							 	}
						} else {
							GifImageView(eventLogMessage.message.attachments.first!.thumbnail)
								.id(UUID())
								.layoutPriority(-1)
								.clipShape(RoundedRectangle(cornerRadius: 4))
								.contentShape(Rectangle())
								.onTapGesture {
									if !isPressed && !didLongPress {
										selectedURLAttachment = eventLogMessage.message.attachments.first!.full
									}
								}
						}
					}
				}
				.clipShape(RoundedRectangle(cornerRadius: 4))
				.clipped()
			} else if eventLogMessage.message.attachments.first!.type == .voiceRecording {
				CustomSlider(
					eventLogMessage: eventLogMessage
				)
				.environmentObject(conversationViewModel)
				.frame(width: geometryProxy.size.width - 160, height: 50)
			} else {
				HStack {
					VStack {
						if conversationViewModel.attachmentTransferInProgress != nil && conversationViewModel.attachmentTransferInProgress!.id == eventLogMessage.message.attachments.first!.id {
							CircularProgressView(progress: Double(conversationViewModel.attachmentTransferInProgress!.transferProgressIndication) / 100.0)
								.frame(width: 80, height: 80)
						} else {
							Image(getImageOfType(type: eventLogMessage.message.attachments.first!.type))
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c700)
								.frame(width: 60, height: 60, alignment: .leading)
						}
					}
					.frame(width: 100, height: 100)
					.background(Color.grayMain2c200)
					.onTapGesture {
						if eventLogMessage.message.attachments.first!.type == .fileTransfer && eventLogMessage.message.attachments.first!.transferProgressIndication == -1 {
							CoreContext.shared.doOnCoreQueue { _ in
								if let chatMessage = eventLogMessage.eventModel.eventLog.chatMessage {
									if let firstContent = chatMessage.contents.first, firstContent.type != "text" {
										conversationViewModel.downloadContent(
											chatMessage: chatMessage,
											content: firstContent
										)
									} else if chatMessage.contents.count >= 2 {
										let secondContent = chatMessage.contents[1]
										conversationViewModel.downloadContent(
											chatMessage: chatMessage,
											content: secondContent
										)
									}
								}
							}
						} else {
							if !isPressed && !didLongPress {
								selectedURLAttachment = eventLogMessage.message.attachments.first!.full
							}
						}
					}
					
					VStack {
						Text(eventLogMessage.message.attachments.first!.name)
							.foregroundStyle(Color.grayMain2c700)
							.default_text_style_600(styleSize: 14)
							.truncationMode(.middle)
							.frame(maxWidth: .infinity, alignment: .leading)
							.lineLimit(1)
						
						if eventLogMessage.message.attachments.first!.size > 0 {
							Text(eventLogMessage.message.attachments.first!.size.formatBytes())
							 .default_text_style_300(styleSize: 14)
							 .frame(maxWidth: .infinity, alignment: .leading)
							 .lineLimit(1)
						} else {
							if let size = self.getFileSize(atPath: eventLogMessage.message.attachments.first!.full.path) {
								Text(size.formatBytes())
									.default_text_style_300(styleSize: 14)
									.frame(maxWidth: .infinity, alignment: .leading)
									.lineLimit(1)
							} else {
								Text(eventLogMessage.message.attachments.first!.size.formatBytes())
									.default_text_style_300(styleSize: 14)
									.frame(maxWidth: .infinity, alignment: .leading)
									.lineLimit(1)
							}
						}
					}
					.padding(.horizontal, 10)
					.frame(maxWidth: .infinity, alignment: .leading)
				}
				.background(.white)
				.clipShape(RoundedRectangle(cornerRadius: 10))
				.onTapGesture {
					if !isPressed && !didLongPress {
						selectedURLAttachment = eventLogMessage.message.attachments.first!.full
					}
				}
			}
		} else if eventLogMessage.message.attachments.count > 1 {
			let sizeCard = ((geometryProxy.size.width - 150)/2)-2
			let columns = [GridItem(.adaptive(minimum: sizeCard), spacing: 1)]
			
			VStack {
				LazyVGrid(columns: columns) {
					ForEach(eventLogMessage.message.attachments.filter({ $0.type == .image || $0.type == .gif
						|| $0.type == .video }), id: \.id) { attachment in
							ZStack {
								Rectangle()
									.fill(Color(.white))
									.frame(width: sizeCard, height: sizeCard)
								
								if #available(iOS 16.0, *) {
									CachedAsyncImage(
										url: attachment.thumbnail,
										placeholder: ProgressView(),
										onImageTapped: {
											if !isPressed && !didLongPress {
												selectedURLAttachment = attachment.full
											}
										})
									
									.overlay(
										Group {
											if attachment.type == .video {
												Image("play-fill")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(.white)
													.frame(width: 40, height: 40)
											}
										}
									)
									.layoutPriority(-1)
								} else {
									CachedAsyncImage(
										url: attachment.thumbnail,
										placeholder: ProgressView(),
										onImageTapped: {
											if !isPressed && !didLongPress {
												selectedURLAttachment = attachment.full
											}
										})
									
									.overlay(
										Group {
											if attachment.type == .video {
												Image("play-fill")
													.renderingMode(.template)
													.resizable()
													.foregroundStyle(.white)
													.frame(width: 40, height: 40)
											}
										}
									)
									.id(UUID())
									.layoutPriority(-1)
								}
							}
							.clipShape(RoundedRectangle(cornerRadius: 4))
							.contentShape(Rectangle())
						}
				}
				
				ForEach(eventLogMessage.message.attachments.filter({ $0.type != .image && $0.type != .gif
					&& $0.type != .video }), id: \.id) { attachment in
					HStack {
						VStack {
							if conversationViewModel.attachmentTransferInProgress != nil && conversationViewModel.attachmentTransferInProgress!.id == attachment.id {
								CircularProgressView(progress: Double(conversationViewModel.attachmentTransferInProgress!.transferProgressIndication) / 100.0)
									.frame(width: 80, height: 80)
							} else {
								Image(getImageOfType(type: attachment.type))
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c700)
									.frame(width: 60, height: 60, alignment: .leading)
							}
						}
						.frame(width: 100, height: 100)
						.background(Color.grayMain2c200)
						.onTapGesture {
							if conversationViewModel.attachmentTransferInProgress == nil {
								if attachment.type == .fileTransfer && attachment.transferProgressIndication == -1 {
									CoreContext.shared.doOnCoreQueue { _ in
										if let content = eventLogMessage.eventModel.eventLog.chatMessage!.contents.first(where: {$0.name == attachment.name && $0.isFileTransfer}) {
											conversationViewModel.downloadContent(
												chatMessage: eventLogMessage.eventModel.eventLog.chatMessage!,
												content: content
											)
										}
									}
								} else {
									if !isPressed && !didLongPress {
										selectedURLAttachment = attachment.full
									}
								}
							}
						}
						
						VStack {
							Text(attachment.name)
								.foregroundStyle(Color.grayMain2c700)
								.default_text_style_600(styleSize: 14)
								.truncationMode(.middle)
								.frame(maxWidth: .infinity, alignment: .leading)
								.lineLimit(1)
							
							if attachment.size > 0 {
								Text(attachment.size.formatBytes())
								 .default_text_style_300(styleSize: 14)
								 .frame(maxWidth: .infinity, alignment: .leading)
								 .lineLimit(1)
							} else {
								if let size = self.getFileSize(atPath: attachment.full.path) {
									Text(size.formatBytes())
										.default_text_style_300(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
										.lineLimit(1)
								} else {
									Text(attachment.size.formatBytes())
										.default_text_style_300(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
										.lineLimit(1)
								}
							}
						}
						.padding(.horizontal, 10)
						.frame(maxWidth: .infinity, alignment: .leading)
					}
					.background(.white)
					.clipShape(RoundedRectangle(cornerRadius: 10))
					.onTapGesture {
						if !isPressed && !didLongPress {
							selectedURLAttachment = attachment.full
						}
					}
				}
			}
			.frame(width: geometryProxy.size.width - 150)
		}
	}
	
	func imageDimensions(url: String) -> (CGFloat, CGFloat) {
		if let imageSource = CGImageSourceCreateWithURL(URL(string: url)! as CFURL, nil) {
			if let imageProperties = CGImageSourceCopyPropertiesAtIndex(imageSource, 0, nil) as Dictionary? {
				let pixelWidth = imageProperties[kCGImagePropertyPixelWidth] as? CGFloat
				let pixelHeight = imageProperties[kCGImagePropertyPixelHeight] as? CGFloat
				let orientation = imageProperties[kCGImagePropertyOrientation] as? Int
				return orientation != nil && orientation == 6 ? (pixelHeight ?? 0, pixelWidth ?? 0) : (pixelWidth ?? 0, pixelHeight ?? 0)
			}
		}
		return (100, 100)
	}
	
	func getImageOfType(type: AttachmentType) -> String {
		if type == .audio {
			return "file-audio"
		} else if type == .pdf {
			return "file-pdf"
		} else if type == .text {
			return "file-text"
		} else if type == .fileTransfer {
			return "download-simple"
		} else {
			return "file"
		}
	}
	
	private func updateEphemeralTimer() {
		if eventLogMessage.message.isEphemeral {
			if eventLogMessage.message.ephemeralExpireTime == 0 {
				// Message hasn't been read by all participants yet
				self.ephemeralLifetime = eventLogMessage.message.ephemeralLifetime.convertDurationToString()
			} else {
				let remaining = eventLogMessage.message.ephemeralExpireTime - Int(Date().timeIntervalSince1970)
				self.ephemeralLifetime = remaining.convertDurationToString()
				
				if timer == nil {
					timer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { _ in
						let updatedRemaining = eventLogMessage.message.ephemeralExpireTime - Int(Date().timeIntervalSince1970)
						if updatedRemaining <= 0 {
							timer?.invalidate()
							timer = nil
						} else {
							self.ephemeralLifetime = updatedRemaining.convertDurationToString()
						}
					}
				}
			}
		}
	}
	
	private func getFileSize(atPath path: String) -> Int? {
		do {
			let attributes = try FileManager.default.attributesOfItem(atPath: path)
			if let fileSize = attributes[.size] as? Int {
				return fileSize
			}
		} catch {
			print("Error: \(error)")
		}
		return nil
	}
}

struct DynamicLinkText: View {
	let text: String
	
	var body: some View {
		let components = text.components(separatedBy: " ")
		
		Text(makeAttributedString(from: components))
			.fixedSize(horizontal: false, vertical: true)
			.multilineTextAlignment(.leading)
			.lineLimit(nil)
			.foregroundStyle(Color.grayMain2c700)
			.default_text_style(styleSize: 14)
	}
	
	// Function to create an AttributedString with clickable links
	private func makeAttributedString(from components: [String]) -> AttributedString {
		var result = AttributedString("")
		for (index, component) in components.enumerated() {
			if let url = URL(string: component.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed) ?? ""),
			   url.scheme == "http" || url.scheme == "https" {
				var attributedText = AttributedString(component)
				attributedText.link = url
				attributedText.foregroundColor = .blue
				attributedText.underlineStyle = .single
				result.append(attributedText)
			} else {
				result.append(AttributedString(component))
			}
			
			// Add space between words except for the last one
			if index < components.count - 1 {
				result.append(AttributedString(" "))
			}
		}
		return result
	}
}

enum URLType {
	case name(String) // local file name of gif
	case url(URL) // remote url
	
	var url: URL? {
		switch self {
		case .name(let name):
			return Bundle.main.url(forResource: name, withExtension: "gif")
		case .url(let remoteURL):
			return remoteURL
		}
	}
}

struct GifImageView: UIViewRepresentable {
	private let name: URL
	init(_ name: URL) {
		self.name = name
	}
	
	func makeUIView(context: Context) -> WKWebView {
		let webview = WKWebView()
		let url = name
		let data = try? Data(contentsOf: url)
		if data != nil {
			webview.load(data!, mimeType: "image/gif", characterEncodingName: "UTF-8", baseURL: url.deletingLastPathComponent())
			webview.scrollView.isScrollEnabled = false
			webview.isUserInteractionEnabled = false
		}
		return webview
	}
	
	func updateUIView(_ uiView: WKWebView, context: Context) {
		uiView.reload()
	}
}

class Ticker: ObservableObject {

	var startedAt: Date = Date()

	var timeIntervalSinceStarted: TimeInterval {
		return Date().timeIntervalSince(startedAt)
	}

	private var timer: Timer?
	func start(interval: TimeInterval) {
		stop()
		startedAt = Date()
		timer = Timer.scheduledTimer(withTimeInterval: interval, repeats: true) { _ in
			self.objectWillChange.send()
		}
	}

	func stop() {
		timer?.invalidate()
	}

	deinit {
		timer?.invalidate()
	}

}

struct RoundedCorner: Shape {
	var radius: CGFloat = .infinity
	var corners: UIRectCorner = .allCorners
	
	func path(in rect: CGRect) -> Path {
		let path = UIBezierPath(roundedRect: rect, byRoundingCorners: corners, cornerRadii: CGSize(width: radius, height: radius))
		return Path(path.cgPath)
	}
}

extension View {
	func roundedCorner(_ radius: CGFloat, corners: UIRectCorner) -> some View {
		clipShape(RoundedCorner(radius: radius, corners: corners) )
	}
}

struct CustomSlider: View {
	@EnvironmentObject var conversationViewModel: ConversationViewModel
	
	let eventLogMessage: EventLogMessage
	
	@State private var value: Double = 0.0
	@State private var isPlaying: Bool = false
	@State private var timer: Timer?
	
	var minTrackColor: Color = .white.opacity(0.5)
	var maxTrackGradient: Gradient = Gradient(colors: [Color.orangeMain300, Color.orangeMain500])
	
	var body: some View {
		GeometryReader { geometry in
			let radius = geometry.size.height * 0.5
			ZStack(alignment: .leading) {
				LinearGradient(
					gradient: maxTrackGradient,
					startPoint: .leading,
					endPoint: .trailing
				)
				.frame(width: geometry.size.width, height: geometry.size.height)
				HStack {
					Rectangle()
						.foregroundColor(minTrackColor)
						.frame(width: self.value * geometry.size.width / 100, height: geometry.size.height)
						.animation(self.value > 0 ? .linear(duration: 0.1) : nil, value: self.value)
				}
				
				HStack {
					Button(
						action: {
							if isPlaying {
								conversationViewModel.pauseVoiceRecordPlayer()
								pauseProgress()
							} else {
								conversationViewModel.startVoiceRecordPlayer(voiceRecordPath: eventLogMessage.message.attachments.first!.full)
								playProgress()
							}
						},
						label: {
							Image(isPlaying ? "pause-fill" : "play-fill")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.orangeMain500)
								.frame(width: 20, height: 20)
						}
					)
					.padding(8)
					.background(.white)
					.clipShape(RoundedRectangle(cornerRadius: 25))
					
					Spacer()
					
					HStack {
						Text((eventLogMessage.message.attachments.first!.duration/1000).convertDurationToString())
							.default_text_style(styleSize: 16)
							.padding(.horizontal, 5)
					}
					.padding(8)
					.background(.white)
					.clipShape(RoundedRectangle(cornerRadius: 25))
				}
				.padding(.horizontal, 10)
			}
			.clipShape(RoundedRectangle(cornerRadius: radius))
			.onDisappear {
				resetProgress()
			}
		}
	}
	
	private func playProgress() {
		isPlaying = true
		self.value = conversationViewModel.getPositionVoiceRecordPlayer(voiceRecordPath: eventLogMessage.message.attachments.first!.full)
		timer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { _ in
			if self.value < 100.0 {
				let valueTmp = conversationViewModel.getPositionVoiceRecordPlayer(voiceRecordPath: eventLogMessage.message.attachments.first!.full)
				if self.value > 90 && self.value == valueTmp {
					self.value = 100
				} else {
					if valueTmp == 0 && !conversationViewModel.isPlayingVoiceRecordPlayer(voiceRecordPath: eventLogMessage.message.attachments.first!.full) {
						stopProgress()
						value = 0.0
						isPlaying = false
					} else {
						self.value = valueTmp
					}
				}
			} else {
				resetProgress()
			}
		}
	}
	
	// Pause the progress
	private func pauseProgress() {
		isPlaying = false
		stopProgress()
	}
	
	// Reset the progress
	private func resetProgress() {
		conversationViewModel.stopVoiceRecordPlayer()
		stopProgress()
		value = 0.0
		isPlaying = false
	}
	
	// Stop the progress and invalidate the timer
	private func stopProgress() {
		timer?.invalidate()
		timer = nil
	}
}

struct CircularProgressView: View {
	var progress: Double

	var body: some View {
		ZStack {
			Circle()
				.stroke(Color(.systemGray4), lineWidth: 5)
			Circle()
				.trim(from: 0, to: progress)
				.stroke(
					Color.orangeMain500,
					style: StrokeStyle(lineWidth: 5, lineCap: .round))
				.rotationEffect(Angle(degrees: -90))
				.animation(.easeInOut(duration: 0.5), value: progress)
				.overlay(
					Text("\(Int(progress * 100))%")
						.font(.system(size: 15, weight: .bold, design: .rounded))
						.foregroundColor(Color.orangeMain500)
				)
		}
		.padding()
	}
}

class ImageCache {
	static let shared = NSCache<NSURL, UIImage>()
}

struct CachedAsyncImage<Placeholder: View>: View {
	let url: URL
	let placeholder: Placeholder
	let onImageTapped: (() -> Void)?

	@State private var image: UIImage?

	var body: some View {
		ZStack {
			if let image = image {
				Image(uiImage: image)
					.resizable()
					.interpolation(.medium)
					.aspectRatio(contentMode: .fill)
					.onTapGesture {
						onImageTapped?()
					}
			} else {
				placeholder
					.onAppear {
						loadImage()
					}
			}
		}
	}

	private func loadImage() {
		if let cachedImage = ImageCache.shared.object(forKey: url as NSURL) {
			self.image = cachedImage
			return
		}

		Task {
			do {
				let (data, _) = try await URLSession.shared.data(from: url)
				if let downloadedImage = UIImage(data: data) {
					ImageCache.shared.setObject(downloadedImage, forKey: url as NSURL)
					await MainActor.run {
						self.image = downloadedImage
					}
				}
			} catch {
				print("Error loading image: \(error.localizedDescription)")
			}
		}
	}
}

/*
 #Preview {
 ChatBubbleView(index: 0)
 }
 */

// swiftlint:enable type_body_length
// swiftlint:enable cyclomatic_complexity
