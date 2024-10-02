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
	@Environment(\.scenePhase) var scenePhase
	@State private var enteredForeground: Bool = false
	
	@ObservedObject var conversationViewModel: ConversationViewModel
	@ObservedObject var conversationsListViewModel: ConversationsListViewModel
	
	@Binding var showingSheet: Bool
	@Binding var text: String
	
	var body: some View {
		let pub = NotificationCenter.default
				.publisher(for: NSNotification.Name("ChatRoomsComputed"))
		
		VStack {
			List {
				ForEach(0..<conversationsListViewModel.conversationsList.count, id: \.self) { index in
					if index < conversationsListViewModel.conversationsList.count {
						HStack {
							Avatar(contactAvatarModel: conversationsListViewModel.conversationsList[index].avatarModel, avatarSize: 50)
							
							VStack(spacing: 0) {
								Spacer()
								
								Text(conversationsListViewModel.conversationsList[index].subject)
									.foregroundStyle(Color.grayMain2c800)
									.if(conversationsListViewModel.conversationsList[index].unreadMessagesCount > 0) { view in
										view.default_text_style_700(styleSize: 14)
									}
									.default_text_style(styleSize: 14)
									.frame(maxWidth: .infinity, alignment: .leading)
									.lineLimit(1)
								
								Text(conversationsListViewModel.conversationsList[index].lastMessageText)
									.foregroundStyle(Color.grayMain2c400)
									.if(conversationsListViewModel.conversationsList[index].unreadMessagesCount > 0) { view in
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
									if !conversationsListViewModel.conversationsList[index].encryptionEnabled {
										Image("warning-circle")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.redDanger500)
											.frame(width: 18, height: 18, alignment: .trailing)
									}
									
									Text(conversationsListViewModel.getCallTime(startDate: conversationsListViewModel.conversationsList[index].lastUpdateTime))
										.foregroundStyle(Color.grayMain2c400)
										.default_text_style(styleSize: 14)
										.lineLimit(1)
								}
								
								Spacer()
								
								HStack {
									if conversationsListViewModel.conversationsList[index].isMuted == false
										&& !(!conversationsListViewModel.conversationsList[index].lastMessageText.isEmpty
											 && conversationsListViewModel.conversationsList[index].lastMessageIsOutgoing == true)
										&& conversationsListViewModel.conversationsList[index].unreadMessagesCount == 0 {
										Text("")
											.frame(width: 18, height: 18, alignment: .trailing)
									}
									
									if conversationsListViewModel.conversationsList[index].isMuted {
										Image("bell-slash")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.orangeMain500)
											.frame(width: 18, height: 18, alignment: .trailing)
									}
									
									if !conversationsListViewModel.conversationsList[index].lastMessageText.isEmpty
										&& conversationsListViewModel.conversationsList[index].lastMessageIsOutgoing == true {
										let imageName = LinphoneUtils.getChatIconState(chatState: conversationsListViewModel.conversationsList[index].lastMessageState)
										Image(imageName)
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.orangeMain500)
											.frame(width: 18, height: 18, alignment: .trailing)
									}
									
									if conversationsListViewModel.conversationsList[index].unreadMessagesCount > 0 {
										HStack {
											Text(
												conversationsListViewModel.conversationsList[index].unreadMessagesCount < 99
												? String(conversationsListViewModel.conversationsList[index].unreadMessagesCount)
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
							if enteredForeground && conversationViewModel.displayedConversation != nil
								&& (navigationManager.peerAddr == nil || navigationManager.peerAddr == conversationViewModel.displayedConversation!.remoteSipUri) {
								if conversationViewModel.displayedConversation != nil {
									conversationViewModel.resetDisplayedChatRoom(conversationsList: conversationsListViewModel.conversationsList)
								}
							}
							
							enteredForeground = false
							
							if navigationManager.peerAddr != nil && conversationsListViewModel.conversationsList[index].remoteSipUri.contains(navigationManager.peerAddr!) {
								conversationViewModel.getChatRoomWithStringAddress(conversationsList: conversationsListViewModel.conversationsList, stringAddr: navigationManager.peerAddr!)
								navigationManager.peerAddr = nil
							}
						}
						.onTapGesture {
							if index < conversationsListViewModel.conversationsList.count {
								if conversationViewModel.displayedConversation != nil {
									conversationViewModel.removeConversationDelegate()
									DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
										conversationViewModel.selectedMessage = nil
										conversationViewModel.resetMessage()
										withAnimation {
											conversationViewModel.changeDisplayedChatRoom(conversationModel: conversationsListViewModel.conversationsList[index])
										}
										conversationViewModel.getMessages()
									}
								} else {
									conversationViewModel.selectedMessage = nil
									withAnimation {
										conversationViewModel.changeDisplayedChatRoom(conversationModel: conversationsListViewModel.conversationsList[index])
									}
								}
							}
						}
						.onLongPressGesture(minimumDuration: 0.2) {
							if index < conversationsListViewModel.conversationsList.count {
								conversationsListViewModel.selectedConversation = conversationsListViewModel.conversationsList[index]
								showingSheet.toggle()
							}
						}
					}
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
		.onChange(of: scenePhase) { newPhase in
			enteredForeground = newPhase == .active
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
