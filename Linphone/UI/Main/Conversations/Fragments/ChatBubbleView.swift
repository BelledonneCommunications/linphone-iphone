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
	
	var body: some View {
		VStack {
			HStack {
				if message.isOutgoing {
					Spacer()
				}
				
				VStack {
					if !message.attachments.isEmpty {
						if message.attachments.count == 1 {
							let result = imageDimensions(url: message.attachments.first!.full.absoluteString)
							if message.attachments.first!.type != .gif {
								AsyncImage(url: message.attachments.first!.full) { image in
									image.resizable()
										.interpolation(.low)
										.scaledToFit()
										.clipShape(RoundedRectangle(cornerRadius: 4))
								} placeholder: {
									ProgressView()
								}
								.frame(
									height: result.0 > result.1
									? (result.1 / (result.0 / (UIScreen.main.bounds.height > UIScreen.main.bounds.width ? UIScreen.main.bounds.height / 3 : UIScreen.main.bounds.width / 3)))
									: (UIScreen.main.bounds.height > UIScreen.main.bounds.width ? UIScreen.main.bounds.height / 3 : UIScreen.main.bounds.width / 3)
								)
							} else {
								if result.0 < result.1 {
									GifImageView(message.attachments.first!.full)
										.clipShape(RoundedRectangle(cornerRadius: 4))
										.frame(
											width: result.1 > UIScreen.main.bounds.height / 3
											? (result.0 / (result.0 / (UIScreen.main.bounds.height > UIScreen.main.bounds.width ? UIScreen.main.bounds.height / 3 : UIScreen.main.bounds.width / 3)))
											: result.0,
											height: result.1 > UIScreen.main.bounds.height / 3
											? (result.1 / (result.0 / (UIScreen.main.bounds.height > UIScreen.main.bounds.width ? UIScreen.main.bounds.height / 3 : UIScreen.main.bounds.width / 3)))
											: result.1
										)
								} else {
									GifImageView(message.attachments.first!.full)
										.clipShape(RoundedRectangle(cornerRadius: 4))
									 	.frame(maxWidth: .infinity)
										.frame(
											height: result.1 / (result.0 / (UIScreen.main.bounds.height > UIScreen.main.bounds.width ? UIScreen.main.bounds.height / 3 : UIScreen.main.bounds.width / 3))
										)
								}
							}
						} else {
						}
					}
					
					if !message.text.isEmpty {
						Text(message.text)
							.foregroundStyle(Color.grayMain2c700)
							.default_text_style(styleSize: 16)
					}
				}
				.padding(.all, 15)
				.background(message.isOutgoing ? Color.orangeMain100 : Color.grayMain2c100)
				.clipShape(RoundedRectangle(cornerRadius: 16))
				
				if !message.isOutgoing {
					Spacer()
				}
			}
			.padding(.leading, message.isOutgoing ? 40 : 0)
			.padding(.trailing, !message.isOutgoing ? 40 : 0)
		}
	}
	
	func imageDimensions(url: String) -> (CGFloat, CGFloat) {
		if let imageSource = CGImageSourceCreateWithURL(URL(string: url)! as CFURL, nil) {
			if let imageProperties = CGImageSourceCopyPropertiesAtIndex(imageSource, 0, nil) as Dictionary? {
				let pixelWidth = imageProperties[kCGImagePropertyPixelWidth] as? CGFloat
				let pixelHeight = imageProperties[kCGImagePropertyPixelHeight] as? CGFloat
				return (pixelWidth ?? 0, pixelHeight ?? 0)
			}
		}
		return (0, 0)
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

/*
 #Preview {
 ChatBubbleView(conversationViewModel: ConversationViewModel(), index: 0)
 }
 */
