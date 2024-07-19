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

struct ChatBubbleView: View {
	
	@ObservedObject var conversationViewModel: ConversationViewModel
	
	let message: Message
	
	let geometryProxy: GeometryProxy
	
	@State private var ticker = Ticker()
	@State private var isPressed: Bool = false
	@State private var timePassed: TimeInterval?
	
	var body: some View {
		VStack {
			if !message.text.isEmpty || !message.attachments.isEmpty {
				HStack(alignment: .top, content: {
					if message.isOutgoing {
						Spacer()
					}
					
					if conversationViewModel.displayedConversation != nil && conversationViewModel.displayedConversation!.isGroup && !message.isOutgoing && message.isFirstMessage {
						VStack {
							Avatar(
								contactAvatarModel: conversationViewModel.participantConversationModel.first(where: {$0.address == message.address}) ??
								ContactAvatarModel(friend: nil, name: "??", address: "", withPresence: false),
								avatarSize: 35
							)
							.padding(.top, 30)
						}
					} else if conversationViewModel.displayedConversation != nil && conversationViewModel.displayedConversation!.isGroup && !message.isOutgoing {
						VStack {
						}
						.padding(.leading, 43)
					}
					
					VStack(alignment: .leading, spacing: 0) {
						if conversationViewModel.displayedConversation != nil && conversationViewModel.displayedConversation!.isGroup && !message.isOutgoing && message.isFirstMessage {
							Text(conversationViewModel.participantConversationModel.first(where: {$0.address == message.address})?.name ?? "")
								.default_text_style(styleSize: 12)
								.padding(.top, 10)
								.padding(.bottom, 2)
						}
						ZStack {
							
							HStack {
								if message.isOutgoing {
									Spacer()
								}
								
								VStack(alignment: message.isOutgoing ? .trailing : .leading) {
									VStack(alignment: message.isOutgoing ? .trailing : .leading) {
										if !message.attachments.isEmpty {
											messageAttachments()
										}
										
										if !message.text.isEmpty {
											Text(message.text)
												.foregroundStyle(Color.grayMain2c700)
												.default_text_style(styleSize: 16)
										}
										
										HStack(alignment: .center) {
											Text(conversationViewModel.getMessageTime(startDate: message.dateReceived))
												.foregroundStyle(Color.grayMain2c500)
												.default_text_style_300(styleSize: 14)
												.padding(.top, 1)
											
											if (conversationViewModel.displayedConversation != nil && conversationViewModel.displayedConversation!.isGroup) || message.isOutgoing {
												if message.status == .sending {
													ProgressView()
														.progressViewStyle(CircularProgressViewStyle(tint: .orangeMain500))
														.frame(width: 15, height: 15)
														.padding(.top, 1)
												} else if message.status != nil {
													Image(conversationViewModel.getImageIMDN(status: message.status!))
														.renderingMode(.template)
														.resizable()
														.foregroundStyle(Color.orangeMain500)
														.frame(width: 15, height: 15)
														.padding(.top, 1)
												}
											}
										}
										.padding(.top, -4)
									}
									.padding(.all, 15)
									.background(message.isOutgoing ? Color.orangeMain100 : Color.grayMain2c100)
									.clipShape(RoundedRectangle(cornerRadius: 3))
									.roundedCorner(
										16,
										corners: message.isOutgoing && message.isFirstMessage ? [.topLeft, .topRight, .bottomLeft] :
											(!message.isOutgoing && message.isFirstMessage ? [.topRight, .bottomRight, .bottomLeft] : [.allCorners]))
									
									if !message.reactions.isEmpty {
										HStack {
											ForEach(0..<message.reactions.count, id: \.self) { index in
												if message.reactions.firstIndex(of: message.reactions[index]) == index {
													Text(message.reactions[index])
														.default_text_style(styleSize: 14)
														.padding(.horizontal, -2)
												}
											}
											
											if (
												(message.reactions.contains("👍") ? 1 : 0) + 
												(message.reactions.contains("❤️") ? 1 : 0) +
												(message.reactions.contains("😂") ? 1 : 0) +
												(message.reactions.contains("😮") ? 1 : 0) +
												(message.reactions.contains("😢") ? 1 : 0)
											) != message.reactions.count {
												Text("\(message.reactions.count)")
													.default_text_style(styleSize: 14)
													.padding(.horizontal, -2)
											}
										}
										.padding(.vertical, 6)
										.padding(.horizontal, 10)
										.background(message.isOutgoing ? Color.orangeMain100 : Color.grayMain2c100)
										.cornerRadius(20)
										.overlay(
											RoundedRectangle(cornerRadius: 20)
												.stroke(.white, lineWidth: 3)
										)
										.padding(.top, -20)
										.padding(message.isOutgoing ? .trailing : .leading, 5)
									}
								}
								
								if !message.isOutgoing {
									Spacer()
								}
							}
							.frame(maxWidth: .infinity)
						}
						.frame(maxWidth: .infinity)
					}
					
					if !message.isOutgoing {
						Spacer()
					}
				})
				.padding(.leading, message.isOutgoing ? 40 : 0)
				.padding(.trailing, !message.isOutgoing ? 40 : 0)
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
				conversationViewModel.selectedMessage = message
			}
			
		}
	}
	
	@ViewBuilder
	func messageAttachments() -> some View {
		if message.attachments.count == 1 {
			if message.attachments.first!.type == .image || message.attachments.first!.type == .gif || message.attachments.first!.type == .video {
				let result = imageDimensions(url: message.attachments.first!.thumbnail.absoluteString)
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
						.if(result.0 >= result.1 && geometryProxy.size.width > 0 && result.0 >= geometryProxy.size.width - 110 && result.1 >= UIScreen.main.bounds.height/2.5) { view in
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
					
					if message.attachments.first!.type == .image || message.attachments.first!.type == .video {
						if #available(iOS 16.0, *) {
							AsyncImage(url: message.attachments.first!.thumbnail) { phase in
								switch phase {
								case .empty:
									ProgressView()
								case .success(let image):
									ZStack {
										image
											.resizable()
											.interpolation(.medium)
											.aspectRatio(contentMode: .fill)
										
										if message.attachments.first!.type == .video {
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
							AsyncImage(url: message.attachments.first!.thumbnail) { phase in
								switch phase {
								case .empty:
									ProgressView()
								case .success(let image):
									ZStack {
										image
											.resizable()
											.interpolation(.medium)
											.aspectRatio(contentMode: .fill)
										
										if message.attachments.first!.type == .video {
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
					} else if message.attachments.first!.type == .gif {
						if #available(iOS 16.0, *) {
							GifImageView(message.attachments.first!.thumbnail)
								.layoutPriority(-1)
								.clipShape(RoundedRectangle(cornerRadius: 4))
						} else {
							GifImageView(message.attachments.first!.thumbnail)
								.id(UUID())
								.layoutPriority(-1)
								.clipShape(RoundedRectangle(cornerRadius: 4))
						}
					}
				}
				.clipShape(RoundedRectangle(cornerRadius: 4))
				.clipped()
			}
		} else if message.attachments.count > 1 {
			let isGroup = conversationViewModel.displayedConversation != nil && conversationViewModel.displayedConversation!.isGroup
			LazyVGrid(columns: [
				GridItem(.adaptive(minimum: 120), spacing: 1)
			], spacing: 3) {
				ForEach(message.attachments) { attachment in
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
			.frame(
				width: geometryProxy.size.width > 0 && CGFloat(122 * message.attachments.count) > geometryProxy.size.width - 110 - (isGroup ? 40 : 0)
				? 122 * floor(CGFloat(geometryProxy.size.width - 110 - (isGroup ? 40 : 0)) / 122)
				: CGFloat(122 * message.attachments.count)
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

/*
 #Preview {
 ChatBubbleView(conversationViewModel: ConversationViewModel(), index: 0)
 }
 */
