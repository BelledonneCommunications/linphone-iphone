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

// swiftlint:disable type_body_length
// swiftlint:disable cyclomatic_complexity
struct ChatBubbleView: View {
	
	@ObservedObject var conversationViewModel: ConversationViewModel
	
	let eventLogMessage: EventLogMessage
	
	let geometryProxy: GeometryProxy
	
	@State private var ticker = Ticker()
	@State private var isPressed: Bool = false
	@State private var timePassed: TimeInterval?
	
	var body: some View {
		HStack {
			VStack {
				if !eventLogMessage.message.text.isEmpty || !eventLogMessage.message.attachments.isEmpty {
					HStack(alignment: .top, content: {
						if eventLogMessage.message.isOutgoing {
							Spacer()
						}
						if conversationViewModel.displayedConversation != nil && conversationViewModel.displayedConversation!.isGroup 
							&& !eventLogMessage.message.isOutgoing && eventLogMessage.message.isFirstMessage {
							VStack {
								Avatar(
									contactAvatarModel: conversationViewModel.participantConversationModel.first(where: {$0.address == eventLogMessage.message.address}) ??
									ContactAvatarModel(friend: nil, name: "??", address: "", withPresence: false),
									avatarSize: 35
								)
								.padding(.top, 30)
							}
						} else if conversationViewModel.displayedConversation != nil 
									&& conversationViewModel.displayedConversation!.isGroup && !eventLogMessage.message.isOutgoing {
							VStack {
							}
							.padding(.leading, 43)
						}
						
						VStack(alignment: .leading, spacing: 0) {
							if conversationViewModel.displayedConversation != nil && conversationViewModel.displayedConversation!.isGroup 
								&& !eventLogMessage.message.isOutgoing && eventLogMessage.message.isFirstMessage {
								Text(conversationViewModel.participantConversationModel.first(where: {$0.address == eventLogMessage.message.address})?.name ?? "")
									.default_text_style(styleSize: 12)
									.padding(.top, 10)
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
													.default_text_style(styleSize: 16)
													.lineLimit(/*@START_MENU_TOKEN@*/2/*@END_MENU_TOKEN@*/)
											} else if !eventLogMessage.message.replyMessage!.attachmentsNames.isEmpty {
												Text(eventLogMessage.message.replyMessage!.attachmentsNames)
													.foregroundStyle(Color.grayMain2c700)
													.default_text_style(styleSize: 16)
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
											if !eventLogMessage.message.attachments.isEmpty {
												messageAttachments()
											}
											
											if !eventLogMessage.message.text.isEmpty {
												Text(eventLogMessage.message.text)
													   .foregroundStyle(Color.grayMain2c700)
													   .default_text_style(styleSize: 16)
											}
											
											HStack(alignment: .center) {
												Text(conversationViewModel.getMessageTime(startDate: eventLogMessage.message.dateReceived))
													.foregroundStyle(Color.grayMain2c500)
													.default_text_style_300(styleSize: 14)
													.padding(.top, 1)
												
												if (conversationViewModel.displayedConversation != nil && conversationViewModel.displayedConversation!.isGroup) 
													|| eventLogMessage.message.isOutgoing {
													if eventLogMessage.message.status == .sending {
														ProgressView()
															.controlSize(.mini)
															.progressViewStyle(CircularProgressViewStyle(tint: .orangeMain500))
															.frame(width: 10, height: 10)
															.padding(.top, 1)
													} else if eventLogMessage.message.status != nil {
														Image(conversationViewModel.getImageIMDN(status: eventLogMessage.message.status!))
															.renderingMode(.template)
															.resizable()
															.foregroundStyle(Color.orangeMain500)
															.frame(width: 15, height: 15)
															.padding(.top, 1)
													}
												}
											}
											.onTapGesture {
												conversationViewModel.selectedMessageToDisplayDetails = eventLogMessage
												conversationViewModel.prepareBottomSheetForDeliveryStatus()
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
															.default_text_style(styleSize: 14)
															.padding(.horizontal, -2)
													}
												}
												
												if containsDuplicates(strings: eventLogMessage.message.reactions) {
													Text("\(eventLogMessage.message.reactions.count)")
														.default_text_style(styleSize: 14)
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
			.onLongPressGesture(minimumDuration: .infinity, maximumDistance: .infinity, pressing: { (value) in
				self.isPressed = value
				if value == true {
					self.timePassed = 0
					self.ticker.start(interval: 0.2)
				}
				
			}, perform: {})
			.onReceive(ticker.objectWillChange) { (_) in
				// Stop timer and reset the start date if the button in not pressed
				guard self.isPressed else {
					self.ticker.stop()
					return
				}
				
				self.timePassed = self.ticker.timeIntervalSinceStarted
				withAnimation {
					conversationViewModel.selectedMessage = eventLogMessage
				}
				
			}
		}
		.contentShape(Rectangle())
		.onTapGesture {
			if conversationViewModel.selectedMessage != nil {
				conversationViewModel.selectedMessage = nil
			}
			UIApplication.shared.endEditing()
		}
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
							AsyncImage(url: eventLogMessage.message.attachments.first!.thumbnail) { phase in
								switch phase {
								case .empty:
									ProgressView()
								case .success(let image):
									ZStack {
										image
											.resizable()
											.interpolation(.medium)
											.aspectRatio(contentMode: .fill)
										
										if eventLogMessage.message.attachments.first!.type == .video {
											Image("play-fill")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(.white)
												.frame(width: 40, height: 40, alignment: .leading)
										}
									}
								case .failure:
									Image("image-broken")
								@unknown default:
									EmptyView()
								}
							}
							.layoutPriority(-1)
							.clipShape(RoundedRectangle(cornerRadius: 4))
						} else {
							AsyncImage(url: eventLogMessage.message.attachments.first!.thumbnail) { phase in
								switch phase {
								case .empty:
									ProgressView()
								case .success(let image):
									ZStack {
										image
											.resizable()
											.interpolation(.medium)
											.aspectRatio(contentMode: .fill)
										
										if eventLogMessage.message.attachments.first!.type == .video {
											Image("play-fill")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(.white)
												.frame(width: 40, height: 40, alignment: .leading)
										}
									}
								case .failure:
									Image("image-broken")
								@unknown default:
									EmptyView()
								}
							}
							.layoutPriority(-1)
							.clipShape(RoundedRectangle(cornerRadius: 4))
							.id(UUID())
						}
					} else if eventLogMessage.message.attachments.first!.type == .gif {
						if #available(iOS 16.0, *) {
							GifImageView(eventLogMessage.message.attachments.first!.thumbnail)
								.layoutPriority(-1)
								.clipShape(RoundedRectangle(cornerRadius: 4))
						} else {
							GifImageView(eventLogMessage.message.attachments.first!.thumbnail)
								.id(UUID())
								.layoutPriority(-1)
								.clipShape(RoundedRectangle(cornerRadius: 4))
						}
					}
				}
				.clipShape(RoundedRectangle(cornerRadius: 4))
				.clipped()
			} else if eventLogMessage.message.attachments.first!.type == .voiceRecording {
				CustomSlider(
					conversationViewModel: conversationViewModel,
					eventLogMessage: eventLogMessage
				)
				.frame(width: geometryProxy.size.width - 160, height: 50)
			} else {
				HStack {
					VStack {
						Image(getImageOfType(type: eventLogMessage.message.attachments.first!.type))
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c700)
							.frame(width: 60, height: 60, alignment: .leading)
					}
					.frame(width: 100, height: 100)
					.background(Color.grayMain2c200)
					
					VStack {
						Text(eventLogMessage.message.attachments.first!.name)
							.foregroundStyle(Color.grayMain2c700)
							.default_text_style_600(styleSize: 16)
							.truncationMode(.middle)
							.frame(maxWidth: .infinity, alignment: .leading)
							.lineLimit(1)
						
						Text(eventLogMessage.message.attachments.first!.size.formatBytes())
							.default_text_style_300(styleSize: 16)
							.frame(maxWidth: .infinity, alignment: .leading)
							.lineLimit(1)
					}
					.padding(.horizontal, 10)
					.frame(maxWidth: .infinity, alignment: .leading)
				}
				.frame(width: geometryProxy.size.width - 110, height: 100)
				.background(.white)
				.clipShape(RoundedRectangle(cornerRadius: 10))
			}
		} else if eventLogMessage.message.attachments.count > 1 {
			let isGroup = conversationViewModel.displayedConversation != nil && conversationViewModel.displayedConversation!.isGroup
			LazyVGrid(columns: [
				GridItem(.adaptive(minimum: 120), spacing: 1)
			], spacing: 3) {
				ForEach(eventLogMessage.message.attachments) { attachment in
					ZStack {
						Rectangle()
							.fill(Color(.white))
							.frame(width: 120, height: 120)
						
						if #available(iOS 16.0, *) {
							AsyncImage(url: attachment.thumbnail) { image in
								ZStack {
									image
										.resizable()
										.interpolation(.medium)
										.aspectRatio(contentMode: .fill)
									
									if attachment.type == .video {
										Image("play-fill")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(.white)
											.frame(width: 40, height: 40, alignment: .leading)
									}
								}
							} placeholder: {
								ProgressView()
							}
							.layoutPriority(-1)
						} else {
							AsyncImage(url: attachment.thumbnail) { image in
								ZStack {
									image
										.resizable()
										.interpolation(.medium)
										.aspectRatio(contentMode: .fill)
									
									if attachment.type == .video {
										Image("play-fill")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(.white)
											.frame(width: 40, height: 40, alignment: .leading)
									}
								}
							} placeholder: {
								ProgressView()
							}
							.id(UUID())
							.layoutPriority(-1)
						}
					}
					.clipShape(RoundedRectangle(cornerRadius: 4))
					.contentShape(Rectangle())
				}
			}
			.frame( width: geometryProxy.size.width > 0
					&& CGFloat(122 * eventLogMessage.message.attachments.count) > geometryProxy.size.width - 110 - (isGroup ? 40 : 0)
				? 122 * floor(CGFloat(geometryProxy.size.width - 110 - (isGroup ? 40 : 0)) / 122)
				: CGFloat(122 * eventLogMessage.message.attachments.count)
			)
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
	@ObservedObject var conversationViewModel: ConversationViewModel
	
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

/*
 #Preview {
 ChatBubbleView(conversationViewModel: ConversationViewModel(), index: 0)
 }
 */

// swiftlint:enable type_body_length
// swiftlint:enable cyclomatic_complexity
