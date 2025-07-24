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

struct StartGroupConversationFragment: View {
	
	@EnvironmentObject var startConversationViewModel: StartConversationViewModel
	@EnvironmentObject var conversationsListViewModel: ConversationsListViewModel
	
	@State var addParticipantsViewModel = AddParticipantsViewModel()
	
	@FocusState var isMessageTextFocused: Bool
	
	@Binding var isShowStartConversationFragment: Bool
	
	var body: some View {
		ZStack {
			AddParticipantsFragment(addParticipantsViewModel: addParticipantsViewModel, confirmAddParticipantsFunc: startConversationViewModel.addParticipants, dismissOnCheckClick: false)
				.onAppear {
					addParticipantsViewModel.participantsToAdd = startConversationViewModel.participants
				}
			
			
			if !startConversationViewModel.participants.isEmpty {
				startConversationPopup
					.background(.black.opacity(0.65))
					.onAppear {
						DispatchQueue.main.asyncAfter(deadline: .now() + 0.6) {
							isMessageTextFocused = true
						}
					}
			}
			
			if startConversationViewModel.operationGroupInProgress {
				PopupLoadingView()
					.background(.black.opacity(0.65))
					.onDisappear {
						startConversationViewModel.searchField = ""
						MagicSearchSingleton.shared.currentFilter = ""
						MagicSearchSingleton.shared.searchForContacts()
						//delayColorDismiss()
						
						isShowStartConversationFragment = false
						
						if let displayedConversation = startConversationViewModel.displayedConversation {
							self.conversationsListViewModel.changeDisplayedChatRoom(conversationModel: displayedConversation)
							startConversationViewModel.displayedConversation = nil
						}
					}
			}
		}
	}
	
	var startConversationPopup: some View {
		GeometryReader { geometry in
			VStack(alignment: .leading) {
				Text("conversation_dialog_set_subject")
					.default_text_style_800(styleSize: 16)
					.frame(alignment: .leading)
					.padding(.bottom, 2)
				
				TextField("conversation_dialog_subject_hint", text: $startConversationViewModel.messageText)
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
					startConversationViewModel.participants.removeAll()
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
					startConversationViewModel.createGroupChatRoom()
				}, label: {
					Text("dialog_ok")
						.default_text_style_white_600(styleSize: 20)
						.frame(height: 35)
						.frame(maxWidth: .infinity)
				})
				.padding(.horizontal, 20)
				.padding(.vertical, 10)
				.background(startConversationViewModel.messageText.isEmpty ? Color.orangeMain100 : Color.orangeMain500)
				.cornerRadius(60)
				.disabled(startConversationViewModel.messageText.isEmpty)
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
			.onDisappear {
				startConversationViewModel.messageText = ""
			}
		}
	}
}
