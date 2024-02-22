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
	
	let index: Int
	
    var body: some View {
		if index < conversationViewModel.conversationMessagesList.count 
			&& conversationViewModel.conversationMessagesList[index].eventLog.chatMessage != nil {
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
}

#Preview {
	ChatBubbleView(conversationViewModel: ConversationViewModel(), index: 0)
}
