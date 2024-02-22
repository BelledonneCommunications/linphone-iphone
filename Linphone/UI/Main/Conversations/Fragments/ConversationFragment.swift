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

struct ConversationFragment: View {
	
	@State private var orientation = UIDevice.current.orientation
	
	@ObservedObject var contactsManager = ContactsManager.shared
	
	@ObservedObject var conversationViewModel: ConversationViewModel
	@ObservedObject var conversationsListViewModel: ConversationsListViewModel
	
	@State var isMenuOpen = false
	
	@FocusState var isMessageTextFocused: Bool
	
	var body: some View {
		NavigationView {
			GeometryReader { geometry in
				VStack(spacing: 1) {
					if conversationViewModel.displayedConversation != nil {
						Rectangle()
							.foregroundColor(Color.orangeMain500)
							.edgesIgnoringSafeArea(.top)
							.frame(height: 0)
						
						HStack {
							if !(orientation == .landscapeLeft || orientation == .landscapeRight
								 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
								Image("caret-left")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
									.padding(.top, 4)
									.padding(.leading, -10)
									.onTapGesture {
										withAnimation {
											conversationViewModel.displayedConversation = nil
										}
									}
							}
							
							let addressFriend =
							(conversationViewModel.displayedConversation!.participants.first != nil && conversationViewModel.displayedConversation!.participants.first!.address != nil)
							? contactsManager.getFriendWithAddress(address: conversationViewModel.displayedConversation!.participants.first!.address!)
							: nil
							
							let contactAvatarModel = addressFriend != nil
							? ContactsManager.shared.avatarListModel.first(where: {
								($0.friend!.consolidatedPresence == .Online || $0.friend!.consolidatedPresence == .Busy)
								&& $0.friend!.name == addressFriend!.name
								&& $0.friend!.address!.asStringUriOnly() == addressFriend!.address!.asStringUriOnly()
							})
							: ContactAvatarModel(friend: nil, withPresence: false)
							
							if LinphoneUtils.isChatRoomAGroup(chatRoom: conversationViewModel.displayedConversation!) {
								Image(uiImage: contactsManager.textToImage(
									firstName: conversationViewModel.displayedConversation!.subject!,
									lastName: conversationViewModel.displayedConversation!.subject!.components(separatedBy: " ").count > 1
									? conversationViewModel.displayedConversation!.subject!.components(separatedBy: " ")[1]
									: ""))
								.resizable()
								.frame(width: 50, height: 50)
								.clipShape(Circle())
								.padding(.top, 4)
							} else if addressFriend != nil && addressFriend!.photo != nil && !addressFriend!.photo!.isEmpty {
								if contactAvatarModel != nil {
									Avatar(contactAvatarModel: contactAvatarModel!, avatarSize: 50)
										.padding(.top, 4)
								} else {
									Image("profil-picture-default")
										.resizable()
										.frame(width: 50, height: 50)
										.clipShape(Circle())
										.padding(.top, 4)
								}
							} else {
								if conversationViewModel.displayedConversation!.participants.first != nil
									&& conversationViewModel.displayedConversation!.participants.first!.address != nil {
									if conversationViewModel.displayedConversation!.participants.first!.address!.displayName != nil {
										Image(uiImage: contactsManager.textToImage(
											firstName: conversationViewModel.displayedConversation!.participants.first!.address!.displayName!,
											lastName: conversationViewModel.displayedConversation!.participants.first!.address!.displayName!.components(separatedBy: " ").count > 1
											? conversationViewModel.displayedConversation!.participants.first!.address!.displayName!.components(separatedBy: " ")[1]
											: ""))
										.resizable()
										.frame(width: 50, height: 50)
										.clipShape(Circle())
										.padding(.top, 4)
										
									} else {
										Image(uiImage: contactsManager.textToImage(
											firstName: conversationViewModel.displayedConversation!.participants.first!.address!.username ?? "Username Error",
											lastName: conversationViewModel.displayedConversation!.participants.first!.address!.username!.components(separatedBy: " ").count > 1
											? conversationViewModel.displayedConversation!.participants.first!.address!.username!.components(separatedBy: " ")[1]
											: ""))
										.resizable()
										.frame(width: 50, height: 50)
										.clipShape(Circle())
										.padding(.top, 4)
									}
									
								} else {
									Image("profil-picture-default")
										.resizable()
										.frame(width: 50, height: 50)
										.clipShape(Circle())
										.padding(.top, 4)
								}
							}
							
							if LinphoneUtils.isChatRoomAGroup(chatRoom: conversationViewModel.displayedConversation!) {
								Text(conversationViewModel.displayedConversation!.subject ?? "No Subject")
									.default_text_style(styleSize: 16)
									.frame(maxWidth: .infinity, alignment: .leading)
									.padding(.top, 4)
									.lineLimit(1)
							} else if addressFriend != nil {
								Text(addressFriend!.name!)
									.default_text_style(styleSize: 16)
									.frame(maxWidth: .infinity, alignment: .leading)
									.padding(.top, 4)
									.lineLimit(1)
							} else {
								if conversationViewModel.displayedConversation!.participants.first != nil
									&& conversationViewModel.displayedConversation!.participants.first!.address != nil {
									Text(conversationViewModel.displayedConversation!.participants.first!.address!.displayName != nil
										 ? conversationViewModel.displayedConversation!.participants.first!.address!.displayName!
										 : conversationViewModel.displayedConversation!.participants.first!.address!.username!)
									.default_text_style(styleSize: 16)
									.frame(maxWidth: .infinity, alignment: .leading)
									.padding(.top, 4)
									.lineLimit(1)
								}
							}
							
							Spacer()
							
							Button {
							} label: {
								Image("phone")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
									.padding(.top, 4)
							}
							
							Menu {
								Button {
									isMenuOpen = false
								} label: {
									HStack {
										Text("See contact")
										Spacer()
										Image("user-circle")
											.resizable()
											.frame(width: 25, height: 25, alignment: .leading)
											.padding(.all, 10)
									}
								}
								
								Button {
									isMenuOpen = false
								} label: {
									HStack {
										Text("Copy SIP address")
										Spacer()
										Image("copy")
											.resizable()
											.frame(width: 25, height: 25, alignment: .leading)
											.padding(.all, 10)
									}
								}
								
								Button(role: .destructive) {
									isMenuOpen = false
								} label: {
									HStack {
										Text("Delete history")
										Spacer()
										Image("trash-simple-red")
											.resizable()
											.frame(width: 25, height: 25, alignment: .leading)
											.padding(.all, 10)
									}
								}
							} label: {
								Image("dots-three-vertical")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 25, height: 25, alignment: .leading)
									.padding(.all, 10)
									.padding(.top, 4)
							}
							.onTapGesture {
								isMenuOpen = true
							}
						}
						.frame(maxWidth: .infinity)
						.frame(height: 50)
						.padding(.horizontal)
						.padding(.bottom, 4)
						.background(.white)
						
						
						
						
						
						
						
						List {
							ForEach(0..<conversationViewModel.conversationMessagesList.count, id: \.self) { index in
								ChatBubbleView(conversationViewModel: conversationViewModel, index: index)
									.id(conversationViewModel.conversationMessagesList[index])
									.scaleEffect(x: 1, y: -1, anchor: .center)
									.listRowInsets(EdgeInsets(top: 2, leading: 10, bottom: 2, trailing: 10))
						   			.listRowSeparator(.hidden)
									.transition(.move(edge: .top))
							}
						}
						.scaleEffect(x: 1, y: -1, anchor: .center)
						.listStyle(.plain)
						.frame(maxWidth: .infinity)
						.background(.white)
						.onTapGesture {
							UIApplication.shared.endEditing()
						}
						.onDisappear {
							conversationViewModel.resetMessage()
						}
						
						
						
						
						
						/*
						ScrollViewReader { proxy in
							ScrollView {
								LazyVStack {
									ForEach(0..<conversationViewModel.conversationMessagesList.count, id: \.self) { index in
										ChatBubbleView(conversationViewModel: conversationViewModel, index: index)
											.id(conversationViewModel.conversationMessagesList[index])
									}
								}
								.frame(maxWidth: .infinity)
								.background(.white)
								.onTapGesture {
									UIApplication.shared.endEditing()
								}
								.onAppear {
									if conversationViewModel.conversationMessagesList.last != nil {
										proxy.scrollTo(conversationViewModel.conversationMessagesList.last!, anchor: .bottom)
									}
								}
								.onDisappear {
									conversationViewModel.resetMessage()
								}
							}
						}
						*/
						
						
						
						/*
						ScrollViewReader { proxy in
							if #available(iOS 17.0, *) {
								ScrollView {
									LazyVStack {
										ForEach(0..<conversationViewModel.conversationMessagesList.count, id: \.self) { index in
											ChatBubbleView(conversationViewModel: conversationViewModel, index: index)
												.id(conversationViewModel.conversationMessagesList[index])
										}
									}
									.frame(maxWidth: .infinity)
									.background(.white)
									.onTapGesture {
										UIApplication.shared.endEditing()
									}
									.onAppear {
										conversationViewModel.getMessage()
									}
									.onDisappear {
										conversationViewModel.resetMessage()
									}
								}
								.defaultScrollAnchor(.bottom)
							} else {
								ScrollView {
									LazyVStack {
										ForEach(0..<conversationViewModel.conversationMessagesList.count, id: \.self) { index in
											ChatBubbleView(conversationViewModel: conversationViewModel, index: index)
												.id(conversationViewModel.conversationMessagesList[index])
										}
									}
									.frame(maxWidth: .infinity)
									.background(.white)
									.onTapGesture {
										UIApplication.shared.endEditing()
									}
									.onAppear {
										conversationViewModel.getMessage()
										if conversationViewModel.conversationMessagesList.last != nil {
											DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
												proxy.scrollTo(conversationViewModel.conversationMessagesList.last!, anchor: .bottom)
											}
										}
									}
									.onDisappear {
										conversationViewModel.resetMessage()
									}
								}
							}
						}
						*/
						
						
						
						HStack(spacing: 0) {
							Button {
							} label: {
								Image("smiley")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 28, height: 28, alignment: .leading)
									.padding(.all, 6)
									.padding(.top, 4)
							}
							.padding(.horizontal, isMessageTextFocused ? 0 : 2)
							
							Button {
							} label: {
								Image("paperclip")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: isMessageTextFocused ? 0 : 28, height: isMessageTextFocused ? 0 : 28, alignment: .leading)
									.padding(.all, isMessageTextFocused ? 0 : 6)
									.padding(.top, 4)
							}
							.padding(.horizontal, isMessageTextFocused ? 0 : 2)
								
							Button {
							} label: {
								Image("camera")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: isMessageTextFocused ? 0 : 28, height: isMessageTextFocused ? 0 : 28, alignment: .leading)
									.padding(.all, isMessageTextFocused ? 0 : 6)
									.padding(.top, 4)
							}
							.padding(.horizontal, isMessageTextFocused ? 0 : 2)
							
							HStack {
								 if #available(iOS 16.0, *) {
									TextField("Say something...", text: $conversationViewModel.messageText, axis: .vertical)
										.default_text_style(styleSize: 15)
										.focused($isMessageTextFocused)
										.padding(.vertical, 5)
								 } else {
									 ZStack(alignment: .leading) {
										 TextEditor(text: $conversationViewModel.messageText)
											 .multilineTextAlignment(.leading)
											 .frame(maxHeight: 160)
											 .fixedSize(horizontal: false, vertical: true)
											 .default_text_style(styleSize: 15)
											 .focused($isMessageTextFocused)
										 
										 if conversationViewModel.messageText.isEmpty {
											 Text("Say something...")
												 .padding(.leading, 4)
												 .opacity(conversationViewModel.messageText.isEmpty ? 1 : 0)
												 .foregroundStyle(Color.gray300)
												 .default_text_style(styleSize: 15)
										 }
									 }
									 .onTapGesture {
										 isMessageTextFocused = true
									 }
								 }
								
								if conversationViewModel.messageText.isEmpty {
									Button {
									} label: {
										Image("microphone")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.grayMain2c500)
											.frame(width: 28, height: 28, alignment: .leading)
											.padding(.all, 6)
											.padding(.top, 4)
									}
								} else {
									Button {
										conversationViewModel.sendMessage()
									} label: {
										Image("paper-plane-tilt")
											.renderingMode(.template)
											.resizable()
											.foregroundStyle(Color.orangeMain500)
											.frame(width: 28, height: 28, alignment: .leading)
											.padding(.all, 6)
											.padding(.top, 4)
											.rotationEffect(.degrees(45))
									}
									.padding(.trailing, 4)
								}
							}
							.padding(.leading, 15)
							.padding(.trailing, 5)
							.padding(.vertical, 6)
							.frame(maxWidth: .infinity, minHeight: 55)
							.background(.white)
							.cornerRadius(30)
							.overlay(
								RoundedRectangle(cornerRadius: 30)
									.inset(by: 0.5)
									.stroke(Color.gray200, lineWidth: 1.5)
							)
							.padding(.horizontal, 4)
						}
						.frame(maxWidth: .infinity, minHeight: 60)
						.padding(.top, 12)
						.padding(.bottom, geometry.safeAreaInsets.bottom > 0 ? (isMessageTextFocused ? 12 : 0) : 12)
						.padding(.horizontal, 10)
						.background(Color.gray100)
					}
				}
				.background(.white)
				.navigationBarHidden(true)
				.onRotate { newOrientation in
					orientation = newOrientation
				}
				.onAppear {
					conversationViewModel.addConversationDelegate()
				}
				.onDisappear {
					conversationViewModel.removeConversationDelegate()
				}
			}
		}
		.navigationViewStyle(.stack)
	}
}

extension UIApplication {
	func endEditing() {
		sendAction(#selector(UIResponder.resignFirstResponder), to: nil, from: nil, for: nil)
	}
}
	
#Preview {
	ConversationFragment(conversationViewModel: ConversationViewModel(), conversationsListViewModel: ConversationsListViewModel())
}
