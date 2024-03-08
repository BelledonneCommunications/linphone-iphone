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

struct ChatBubbleView: View {
	
	@ObservedObject var conversationViewModel: ConversationViewModel
	
	//let index: IndexPath
	
	let message: Message
	
    var body: some View {
		/*
		if index < conversationViewModel.conversationMessagesList.count
			&& conversationViewModel.conversationMessagesList[index].eventLog.chatMessage != nil {
			VStack {
				if index == 0 && conversationViewModel.displayedConversationHistorySize > conversationViewModel.conversationMessagesList.count {
				//if index % 30 == 29 && conversationViewModel.displayedConversationHistorySize > conversationViewModel.conversationMessagesList.count {
					ProgressView()
						.frame(idealWidth: .infinity, maxWidth: .infinity, alignment: .center)
						.id(UUID())
				}
				
				HStack {
					if conversationViewModel.conversationMessagesList[index].eventLog.chatMessage!.isOutgoing {
						Spacer()
					}
					
					VStack {
						Text(conversationViewModel.conversationMessagesList[index].eventLog.chatMessage!.utf8Text ?? "")
							.foregroundStyle(Color.grayMain2c700)
							.default_text_style(styleSize: 16)
					}
					.padding(.all, 15)
					.background(conversationViewModel.conversationMessagesList[index].eventLog.chatMessage!.isOutgoing ? Color.orangeMain100 : Color.grayMain2c100)
					.clipShape(RoundedRectangle(cornerRadius: 16))
					
					if !conversationViewModel.conversationMessagesList[index].eventLog.chatMessage!.isOutgoing {
						Spacer()
					}
				}
				.padding(.leading, conversationViewModel.conversationMessagesList[index].eventLog.chatMessage!.isOutgoing ? 40 : 0)
			 	.padding(.trailing, !conversationViewModel.conversationMessagesList[index].eventLog.chatMessage!.isOutgoing ? 40 : 0)
			}
		}
		if conversationViewModel.conversationMessagesSection.count > index.section && conversationViewModel.conversationMessagesSection[index.section].rows.count > index.row {
			VStack {
				HStack {
					if message.isOutgoing {
						Spacer()
					}
					
					VStack {
						Text(message.text
						)
						.foregroundStyle(Color.grayMain2c700)
						.default_text_style(styleSize: 16)
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
		 */
		
		VStack {
			HStack {
				if message.isOutgoing {
					Spacer()
				}
				
				VStack {
					if !message.attachments.isEmpty {
						AsyncImage(url: message.attachments.first!.full) { image in
							image.resizable()
								.scaledToFill()
								//.aspectRatio(1.5, contentMode: .fill)
								//.clipped()
						} placeholder: {
							ProgressView()
						}
						.frame(maxHeight: 400)
						//.frame(width: 50, height: 50)
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
}

/*
#Preview {
	ChatBubbleView(conversationViewModel: ConversationViewModel(), index: 0)
}
*/
