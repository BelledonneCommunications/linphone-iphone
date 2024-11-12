//
//  PopupViewWithTextField.swift
//  Linphone
//
//  Created by Benoît Martins on 12/11/2024.
//

import SwiftUI

struct PopupViewWithTextField: View {
	
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	
	@ObservedObject var conversationViewModel: ConversationViewModel
	
	@FocusState var isMessageTextFocused: Bool
	
    var body: some View {
		GeometryReader { geometry in
			VStack(alignment: .leading) {
				Text("conversation_dialog_edit_subject")
					.default_text_style_800(styleSize: 16)
					.frame(alignment: .leading)
					.padding(.bottom, 2)
				
				TextField("conversation_dialog_subject_hint", text: $conversationViewModel.conversationInfoPopupText)
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
					conversationViewModel.isShowConversationInfoPopup = false
				}, label: {
					Text("Cancel")
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
					conversationViewModel.setNewChatRoomSubject()
				}, label: {
					Text("Confirm")
						.default_text_style_white_600(styleSize: 20)
						.frame(height: 35)
						.frame(maxWidth: .infinity)
				})
				.padding(.horizontal, 20)
				.padding(.vertical, 10)
				.background(conversationViewModel.conversationInfoPopupText.isEmpty ? Color.orangeMain100 : Color.orangeMain500)
				.cornerRadius(60)
				.disabled(conversationViewModel.conversationInfoPopupText.isEmpty)
			}
			.padding(.horizontal, 20)
			.padding(.vertical, 20)
			.background(.white)
			.cornerRadius(20)
			.padding(.horizontal)
			.frame(maxHeight: .infinity)
			.shadow(color: Color.orangeMain500, radius: 0, x: 0, y: 2)
			.frame(maxWidth: sharedMainViewModel.maxWidth)
			.position(x: geometry.size.width / 2, y: geometry.size.height / 2)
		}
    }
}

#Preview {
	PopupViewWithTextField(conversationViewModel: ConversationViewModel())
}
