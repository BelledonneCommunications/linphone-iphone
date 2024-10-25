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
import linphonesw

struct ConversationsListFragment: View {
	
	@EnvironmentObject var navigationManager: NavigationManager
	
	@ObservedObject var conversationViewModel: ConversationViewModel
	@ObservedObject var conversationsListViewModel: ConversationsListViewModel
	
	@Binding var showingSheet: Bool
	@Binding var text: String
	
	var body: some View {
		VStack {
			List {
				ForEach(conversationsListViewModel.conversationsList) { conversation in
					ConversationRow(
						navigationManager: _navigationManager,
						conversation: conversation,
						conversationViewModel: conversationViewModel,
						conversationsListViewModel: conversationsListViewModel,
						showingSheet: $showingSheet,
						text: $text
					)
				}
			}
			.safeAreaInset(edge: .top, content: {
				Spacer()
					.frame(height: 12)
			})
			.listStyle(.plain)
			.overlay(
				VStack {
					if conversationsListViewModel.conversationsList.isEmpty {
						Spacer()
						Image("illus-belledonne")
							.resizable()
							.scaledToFit()
							.clipped()
							.padding(.all)
						Text(!text.isEmpty ? "list_filter_no_result_found" : "conversations_list_empty")
							.default_text_style_800(styleSize: 16)
						Spacer()
						Spacer()
					}
				}
					.padding(.all)
			)
		}
		.navigationTitle("")
		.navigationBarHidden(true)
	}
}

struct ConversationRow: View {
	@EnvironmentObject var navigationManager: NavigationManager
	
	@ObservedObject var conversation: ConversationModel
	@ObservedObject var conversationViewModel: ConversationViewModel
	@ObservedObject var conversationsListViewModel: ConversationsListViewModel
	
	@Binding var showingSheet: Bool
	@Binding var text: String
	
	var body: some View {
		let pub = NotificationCenter.default
				.publisher(for: NSNotification.Name("ChatRoomsComputed"))
		HStack {
			Avatar(contactAvatarModel: conversation.avatarModel, avatarSize: 50)
			
			VStack(spacing: 0) {
				Spacer()
				
				Text(conversation.subject)
					.foregroundStyle(Color.grayMain2c800)
					.if(conversation.unreadMessagesCount > 0) { view in
						view.default_text_style_700(styleSize: 14)
					}
					.default_text_style(styleSize: 14)
					.frame(maxWidth: .infinity, alignment: .leading)
					.lineLimit(1)
				
				Text(conversation.lastMessageText)
					.foregroundStyle(Color.grayMain2c400)
					.if(conversation.unreadMessagesCount > 0) { view in
						view.default_text_style_700(styleSize: 14)
					}
					.default_text_style(styleSize: 14)
					.frame(maxWidth: .infinity, alignment: .leading)
					.lineLimit(1)
				
				Spacer()
			}
			
			Spacer()
			
			VStack(alignment: .trailing, spacing: 0) {
				Spacer()
				
				HStack {
					if !conversation.encryptionEnabled {
						Image("warning-circle")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.redDanger500)
							.frame(width: 18, height: 18, alignment: .trailing)
					}
					
					Text(conversationsListViewModel.getCallTime(startDate: conversation.lastUpdateTime))
						.foregroundStyle(Color.grayMain2c400)
						.default_text_style(styleSize: 14)
						.lineLimit(1)
				}
				
				Spacer()
				
				HStack {
					if conversation.isMuted == false
						&& !(!conversation.lastMessageText.isEmpty
							 && conversation.lastMessageIsOutgoing == true)
						&& conversation.unreadMessagesCount == 0 {
						Text("")
							.frame(width: 18, height: 18, alignment: .trailing)
					}
					
					if conversation.isMuted {
						Image("bell-slash")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.orangeMain500)
							.frame(width: 18, height: 18, alignment: .trailing)
					}
					
					if !conversation.lastMessageText.isEmpty
						&& conversation.lastMessageIsOutgoing == true {
						let imageName = LinphoneUtils.getChatIconState(chatState: conversation.lastMessageState)
						Image(imageName)
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.orangeMain500)
							.frame(width: 18, height: 18, alignment: .trailing)
					}
					
					if conversation.unreadMessagesCount > 0 {
						HStack {
							Text(
								conversation.unreadMessagesCount < 99
								? String(conversation.unreadMessagesCount)
								: "99+"
							)
							.foregroundStyle(.white)
							.default_text_style(styleSize: 10)
							.lineLimit(1)
						}
						.frame(width: 18, height: 18)
						.background(Color.redDanger500)
						.cornerRadius(50)
					}
				}
				
				Spacer()
			}
			.padding(.trailing, 10)
		}
		.frame(height: 50)
		.buttonStyle(.borderless)
		.listRowInsets(EdgeInsets(top: 6, leading: 20, bottom: 6, trailing: 20))
		.listRowSeparator(.hidden)
		.background(.white)
		.onReceive(pub) { _ in
			if CoreContext.shared.enteredForeground && conversationViewModel.displayedConversation != nil
				&& (navigationManager.peerAddr == nil || navigationManager.peerAddr == conversationViewModel.displayedConversation!.remoteSipUri) {
				if conversationViewModel.displayedConversation != nil {
					conversationViewModel.resetDisplayedChatRoom(conversationsList: conversationsListViewModel.conversationsList)
				}
			}
			
			CoreContext.shared.enteredForeground = false
			
			if navigationManager.peerAddr != nil
				&& conversation.remoteSipUri.contains(navigationManager.peerAddr!) {
				conversationViewModel.getChatRoomWithStringAddress(conversationsList: conversationsListViewModel.conversationsList, stringAddr: navigationManager.peerAddr!)
				navigationManager.peerAddr = nil
			}
		}
		.onTapGesture {
			conversationViewModel.changeDisplayedChatRoom(conversationModel: conversation)
		}
		.onLongPressGesture(minimumDuration: 0.2) {
			conversationsListViewModel.selectedConversation = conversation
			showingSheet.toggle()
		}
	}
}

#Preview {
	ConversationsListFragment(
		conversationViewModel: ConversationViewModel(),
		conversationsListViewModel: ConversationsListViewModel(),
		showingSheet: .constant(false),
		text: .constant("")
	)
}
