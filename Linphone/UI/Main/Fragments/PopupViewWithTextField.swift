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

struct PopupViewWithTextField: View {
	
	@ObservedObject var sharedMainViewModel = SharedMainViewModel.shared
	
	@Binding var isShowConversationInfoPopup: Bool
	@Binding var conversationInfoPopupText: String
	
	@FocusState var isMessageTextFocused: Bool
	
    var body: some View {
		GeometryReader { geometry in
			VStack(alignment: .leading) {
				Text("conversation_dialog_edit_subject")
					.default_text_style_800(styleSize: 16)
					.frame(alignment: .leading)
					.padding(.bottom, 2)
				
				TextField("conversation_dialog_subject_hint", text: $conversationInfoPopupText)
					.default_text_style(styleSize: 15)
					.frame(height: 25)
					.padding(.horizontal, 20)
					.padding(.vertical, 15)
					.cornerRadius(60)
					.overlay(
						RoundedRectangle(cornerRadius: 60)
							.inset(by: 0.5)
							.stroke(isMessageTextFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
					)
					.padding(.bottom)
					.focused($isMessageTextFocused)
				
				Button(action: {
					isShowConversationInfoPopup = false
				}, label: {
					Text("dialog_cancel")
						.default_text_style_orange_600(styleSize: 20)
						.frame(height: 35)
						.frame(maxWidth: .infinity)
				})
				.padding(.horizontal, 20)
				.padding(.vertical, 10)
				.cornerRadius(60)
				.overlay(
					RoundedRectangle(cornerRadius: 60)
						.inset(by: 0.5)
						.stroke(Color.orangeMain500, lineWidth: 1)
				)
				.padding(.bottom, 10)
				
				Button(action: {
					setNewChatRoomSubject()
					isShowConversationInfoPopup = false
				}, label: {
					Text("dialog_ok")
						.default_text_style_white_600(styleSize: 20)
						.frame(height: 35)
						.frame(maxWidth: .infinity)
				})
				.padding(.horizontal, 20)
				.padding(.vertical, 10)
				.background(conversationInfoPopupText.isEmpty ? Color.orangeMain100 : Color.orangeMain500)
				.cornerRadius(60)
				.disabled(conversationInfoPopupText.isEmpty)
			}
			.padding(.horizontal, 20)
			.padding(.vertical, 20)
			.background(.white)
			.cornerRadius(20)
			.padding(.horizontal)
			.frame(maxHeight: .infinity)
			.shadow(color: Color.orangeMain500, radius: 0, x: 0, y: 2)
			.frame(maxWidth: SharedMainViewModel.shared.maxWidth)
			.position(x: geometry.size.width / 2, y: geometry.size.height / 2)
		}
		.onAppear {
			self.conversationInfoPopupText = self.sharedMainViewModel.displayedConversation?.subject ?? ""
		}
    }
	
	func setNewChatRoomSubject() {
		if let displayedConversation = self.sharedMainViewModel.displayedConversation, self.conversationInfoPopupText != displayedConversation.subject {
			
			CoreContext.shared.doOnCoreQueue { _ in
				displayedConversation.chatRoom.subject = self.conversationInfoPopupText
			}
			
			displayedConversation.subject = self.conversationInfoPopupText
			displayedConversation.avatarModel = ContactAvatarModel(
				friend: displayedConversation.avatarModel.friend,
				name: self.conversationInfoPopupText,
				address: displayedConversation.avatarModel.address,
				withPresence: false
			)
		}
	}
}

#Preview {
	PopupViewWithTextField(isShowConversationInfoPopup: .constant(true), conversationInfoPopupText: .constant(""))
}
