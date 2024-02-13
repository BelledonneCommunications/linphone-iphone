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
	
	@ObservedObject var contactsManager = ContactsManager.shared
	
	@ObservedObject var conversationsListViewModel: ConversationsListViewModel
	
	var body: some View {
		VStack {
			List {
				ForEach(0..<conversationsListViewModel.conversationsList.count, id: \.self) { index in
					HStack {
						let addressFriend =
						(conversationsListViewModel.conversationsList[index].participants.first != nil && conversationsListViewModel.conversationsList[index].participants.first!.address != nil)
						? contactsManager.getFriendWithAddress(address: conversationsListViewModel.conversationsList[index].participants.first!.address!)
						: nil
						HStack {
							let contactAvatarModel = addressFriend != nil
							? ContactsManager.shared.avatarListModel.first(where: {
								($0.friend!.consolidatedPresence == .Online || $0.friend!.consolidatedPresence == .Busy)
								&& $0.friend!.name == addressFriend!.name
								&& $0.friend!.address!.asStringUriOnly() == addressFriend!.address!.asStringUriOnly()
							})
							: ContactAvatarModel(friend: nil, withPresence: false)
							
							if LinphoneUtils.isChatRoomAGroup(chatRoom: conversationsListViewModel.conversationsList[index]) {
								Image(uiImage: contactsManager.textToImage(
									firstName: conversationsListViewModel.conversationsList[index].subject!,
									lastName: conversationsListViewModel.conversationsList[index].subject!.components(separatedBy: " ").count > 1
									? conversationsListViewModel.conversationsList[index].subject!.components(separatedBy: " ")[1]
									: ""))
								.resizable()
								.frame(width: 50, height: 50)
								.clipShape(Circle())
							} else if addressFriend != nil && addressFriend!.photo != nil && !addressFriend!.photo!.isEmpty {
								if contactAvatarModel != nil {
									Avatar(contactAvatarModel: contactAvatarModel!, avatarSize: 50)
								} else {
									Image("profil-picture-default")
										.resizable()
										.frame(width: 50, height: 50)
										.clipShape(Circle())
								}
							} else {
								if conversationsListViewModel.conversationsList[index].participants.first != nil
									&& conversationsListViewModel.conversationsList[index].participants.first!.address != nil {
									if conversationsListViewModel.conversationsList[index].participants.first!.address!.displayName != nil {
										Image(uiImage: contactsManager.textToImage(
											firstName: conversationsListViewModel.conversationsList[index].participants.first!.address!.displayName!,
											lastName: conversationsListViewModel.conversationsList[index].participants.first!.address!.displayName!.components(separatedBy: " ").count > 1
											? conversationsListViewModel.conversationsList[index].participants.first!.address!.displayName!.components(separatedBy: " ")[1]
											: ""))
										.resizable()
										.frame(width: 50, height: 50)
										.clipShape(Circle())
										
									} else {
										Image(uiImage: contactsManager.textToImage(
											firstName: conversationsListViewModel.conversationsList[index].participants.first!.address!.username ?? "Username Error",
											lastName: conversationsListViewModel.conversationsList[index].participants.first!.address!.username!.components(separatedBy: " ").count > 1
											? conversationsListViewModel.conversationsList[index].participants.first!.address!.username!.components(separatedBy: " ")[1]
											: ""))
										.resizable()
										.frame(width: 50, height: 50)
										.clipShape(Circle())
									}
									
								} else {
									Image("profil-picture-default")
										.resizable()
										.frame(width: 50, height: 50)
										.clipShape(Circle())
								}
							}
							
							VStack(spacing: 0) {
								Spacer()
								
								if LinphoneUtils.isChatRoomAGroup(chatRoom: conversationsListViewModel.conversationsList[index]) {
									Text(conversationsListViewModel.conversationsList[index].subject ?? "No Subject")
										.foregroundStyle(Color.grayMain2c800)
										.if(conversationsListViewModel.conversationsList[index].unreadMessagesCount > 0) { view in
											view.default_text_style_700(styleSize: 14)
										}
										.default_text_style(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
										.lineLimit(1)
								} else if addressFriend != nil {
									Text(addressFriend!.name!)
										.foregroundStyle(Color.grayMain2c800)
										.if(conversationsListViewModel.conversationsList[index].unreadMessagesCount > 0) { view in
											view.default_text_style_700(styleSize: 14)
										}
										.default_text_style(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
										.lineLimit(1)
								} else {
									if conversationsListViewModel.conversationsList[index].participants.first != nil
										&& conversationsListViewModel.conversationsList[index].participants.first!.address != nil {
										Text(conversationsListViewModel.conversationsList[index].participants.first!.address!.displayName != nil
											 ? conversationsListViewModel.conversationsList[index].participants.first!.address!.displayName!
											 : conversationsListViewModel.conversationsList[index].participants.first!.address!.username!)
										.foregroundStyle(Color.grayMain2c800)
										.if(conversationsListViewModel.conversationsList[index].unreadMessagesCount > 0) { view in
											view.default_text_style_700(styleSize: 14)
										}
										.default_text_style(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
										.lineLimit(1)
									}
								}
								
								Text(
									conversationsListViewModel.conversationsList[index].lastMessageInHistory != nil
									? conversationsListViewModel.getContentTextMessage(message: conversationsListViewModel.conversationsList[index].lastMessageInHistory!)
									: ""
								)
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
									if conversationsListViewModel.conversationsList[index].currentParams != nil
										&& !conversationsListViewModel.conversationsList[index].currentParams!.encryptionEnabled {
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
									if conversationsListViewModel.conversationsList[index].lastMessageInHistory != nil
										&& conversationsListViewModel.conversationsList[index].lastMessageInHistory!.isOutgoing == true {
										let imageName = LinphoneUtils.getChatIconState(chatState: conversationsListViewModel.conversationsList[index].lastMessageInHistory!.state)
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
									
									if !(conversationsListViewModel.conversationsList[index].lastMessageInHistory != nil
										&& conversationsListViewModel.conversationsList[index].lastMessageInHistory!.isOutgoing == true)
										&& conversationsListViewModel.conversationsList[index].unreadMessagesCount == 0 {
										Text("")
											.frame(width: 18, height: 18, alignment: .trailing)
									}
								}
								
								Spacer()
							}
							.padding(.trailing, 10)
						}
					}
					.buttonStyle(.borderless)
					.listRowInsets(EdgeInsets(top: 6, leading: 20, bottom: 6, trailing: 20))
					.listRowSeparator(.hidden)
					.background(.white)
					.onTapGesture {
					}
					.onLongPressGesture(minimumDuration: 0.2) {
					}
				}
			}
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
						Text("No conversation for the moment...")
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

#Preview {
	ConversationsListFragment(conversationsListViewModel: ConversationsListViewModel())
}
