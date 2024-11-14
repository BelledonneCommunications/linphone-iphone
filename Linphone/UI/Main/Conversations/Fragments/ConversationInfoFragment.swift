/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
 *
 * This file is part of Linphone
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

// swiftlint:disable type_body_length
struct ConversationInfoFragment: View {
	@State private var orientation = UIDevice.current.orientation
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	
	@ObservedObject var conversationViewModel: ConversationViewModel
	@ObservedObject var conversationsListViewModel: ConversationsListViewModel
	@ObservedObject var contactViewModel: ContactViewModel
	@ObservedObject var editContactViewModel: EditContactViewModel
	@ObservedObject var meetingViewModel: MeetingViewModel
	
	@State var addParticipantsViewModel = AddParticipantsViewModel()
	
	@Binding var isMuted: Bool
	@Binding var isShowEphemeralFragment: Bool
	@Binding var isShowStartCallGroupPopup: Bool
	@Binding var isShowInfoConversationFragment: Bool
	@Binding var isShowEditContactFragment: Bool
	@Binding var indexPage: Int
	
	@Binding var isShowScheduleMeetingFragment: Bool
	
	@State private var participantListIsOpen = true
	
	var body: some View {
		NavigationView {
			GeometryReader { geometry in
				if conversationViewModel.displayedConversation != nil {
					VStack(spacing: 1) {
						Rectangle()
							.foregroundColor(Color.orangeMain500)
							.edgesIgnoringSafeArea(.top)
							.frame(height: 0)
						
						HStack {
							Image("caret-left")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.orangeMain500)
								.frame(width: 25, height: 25, alignment: .leading)
								.padding(.all, 10)
								.padding(.top, 2)
								.padding(.leading, -10)
								.onTapGesture {
									withAnimation {
										isShowInfoConversationFragment = false
									}
								}
							
							Spacer()
						}
						.frame(maxWidth: .infinity)
						.frame(height: 50)
						.padding(.horizontal)
						.padding(.bottom, 4)
						.background(.white)
						
						ScrollView {
							VStack(spacing: 0) {
								VStack(spacing: 0) {
									if #unavailable(iOS 16.0) {
										Rectangle()
											.foregroundColor(Color.gray100)
											.frame(height: 7)
									}
									
									VStack(spacing: 0) {
										if conversationViewModel.displayedConversation != nil && !conversationViewModel.displayedConversation!.isGroup {
											
											Avatar(contactAvatarModel: conversationViewModel.displayedConversation!.avatarModel, avatarSize: 100)
												.padding(.top, 4)
											
											Text(conversationViewModel.displayedConversation!.avatarModel.name)
												.foregroundStyle(Color.grayMain2c700)
												.multilineTextAlignment(.center)
												.default_text_style(styleSize: 14)
												.frame(maxWidth: .infinity)
												.padding(.top, 10)
											
											Text(conversationViewModel.participantConversationModel.first?.address ?? "")
												.foregroundStyle(Color.grayMain2c700)
												.multilineTextAlignment(.center)
												.default_text_style(styleSize: 14)
												.frame(maxWidth: .infinity)
												.padding(.top, 5)
											
											if !conversationViewModel.displayedConversation!.avatarModel.lastPresenceInfo.isEmpty {
												Text(conversationViewModel.displayedConversation!.avatarModel.lastPresenceInfo)
													.foregroundStyle(conversationViewModel.displayedConversation!.avatarModel.lastPresenceInfo == "Online"
																	 ? Color.greenSuccess500
																	 : Color.orangeWarning600)
													.multilineTextAlignment(.center)
													.default_text_style_300(styleSize: 12)
													.frame(maxWidth: .infinity)
													.frame(height: 20)
													.padding(.top, 5)
											} else {
												Text("")
													.multilineTextAlignment(.center)
													.default_text_style_300(styleSize: 12)
													.frame(maxWidth: .infinity)
													.frame(height: 20)
											}
										} else {
											Avatar(contactAvatarModel: conversationViewModel.displayedConversation!.avatarModel, avatarSize: 100)
												.padding(.top, 4)
											
											HStack {
												Text(conversationViewModel.displayedConversation!.avatarModel.name)
													.foregroundStyle(Color.grayMain2c700)
													.multilineTextAlignment(.center)
													.default_text_style(styleSize: 14)
													.padding(.top, 10)
												
												if conversationViewModel.isUserAdmin {
													Button(
														action: {
															conversationViewModel.isShowConversationInfoPopup = true
														},
														label: {
															Image("pencil-simple")
																.renderingMode(.template)
																.resizable()
																.foregroundStyle(Color.orangeMain500)
																.frame(width: 20, height: 20)
														}
													)
													.padding(.top, 10)
												}
											}
											.padding(.leading, conversationViewModel.isUserAdmin ? 20 : 0)
										}
									}
									.frame(minHeight: 150)
									.frame(maxWidth: .infinity)
									.padding(.top, 10)
									.padding(.bottom, 2)
									.background(Color.gray100)
									
									if !conversationViewModel.displayedConversation!.isReadOnly {
										HStack {
											Spacer()
											
											Button(action: {
												conversationViewModel.displayedConversation!.toggleMute()
												isMuted = !isMuted
											}, label: {
												VStack {
													HStack(alignment: .center) {
														Image(isMuted ? "bell-simple" : "bell-simple-slash")
															.renderingMode(.template)
															.resizable()
															.foregroundStyle(Color.grayMain2c600)
															.frame(width: 25, height: 25)
													}
													.padding(16)
													.background(Color.grayMain2c200)
													.cornerRadius(40)
													
													Text(isMuted ? "conversation_action_unmute" : "conversation_action_mute")
														.default_text_style(styleSize: 14)
														.frame(minWidth: 80)
														.lineLimit(1)
												}
											})
											.frame(width: geometry.size.width / 4)
											
											Spacer()
											
											Button(action: {
												if conversationViewModel.displayedConversation!.isGroup {
													isShowStartCallGroupPopup.toggle()
												} else {
													conversationViewModel.displayedConversation!.call()
												}
											}, label: {
												VStack {
													HStack(alignment: .center) {
														Image("phone")
															.renderingMode(.template)
															.resizable()
															.foregroundStyle(Color.grayMain2c600)
															.frame(width: 25, height: 25)
													}
													.padding(16)
													.background(Color.grayMain2c200)
													.cornerRadius(40)
													
													Text("conversation_action_call")
														.default_text_style(styleSize: 14)
														.frame(minWidth: 80)
														.lineLimit(1)
												}
											})
											.frame(width: geometry.size.width / 4)
											
											Spacer()
											
											Button(action: {
												if conversationViewModel.displayedConversation != nil {
													meetingViewModel.subject = conversationViewModel.displayedConversation!.subject
													meetingViewModel.participants = conversationViewModel.participants
													conversationViewModel.displayedConversation = nil
													indexPage = 3
													withAnimation {
														isShowScheduleMeetingFragment = true
													}
												}
											}, label: {
												VStack {
													HStack(alignment: .center) {
														Image("video-conference")
															.renderingMode(.template)
															.resizable()
															.foregroundStyle(Color.grayMain2c600)
															.frame(width: 25, height: 25)
													}
													.padding(16)
													.background(Color.grayMain2c200)
													.cornerRadius(40)
													
													Text("meeting_schedule_meeting_label")
														.default_text_style(styleSize: 14)
														.frame(minWidth: 80)
														.lineLimit(1)
												}
											})
											.frame(width: geometry.size.width / 4)
											
											Spacer()
										}
										.padding(.top, 20)
										.padding(.bottom, 10)
										.frame(maxWidth: .infinity)
										.background(Color.gray100)
									}
									
									if conversationViewModel.displayedConversation!.isGroup {
										HStack(alignment: .center) {
											Text("conversation_info_participants_list_title")
												.default_text_style_800(styleSize: 18)
												.frame(maxWidth: .infinity, alignment: .leading)
											
											Spacer()
											
											Image(participantListIsOpen ? "caret-up" : "caret-down")
												.renderingMode(.template)
												.resizable()
												.foregroundStyle(Color.grayMain2c600)
												.frame(width: 25, height: 25, alignment: .leading)
												.padding(.all, 10)
										}
										.padding(.top, 30)
										.padding(.bottom, 10)
										.padding(.horizontal, 20)
										.background(Color.gray100)
										.onTapGesture {
											withAnimation {
												participantListIsOpen.toggle()
											}
										}
										
										if participantListIsOpen {
											VStack(spacing: 0) {
												ForEach(conversationViewModel.participantConversationModel) { participantConversationModel in
													HStack {
														Avatar(contactAvatarModel: participantConversationModel, avatarSize: 50)
														
														VStack {
															Text(participantConversationModel.name)
																.foregroundStyle(Color.grayMain2c700)
																.default_text_style(styleSize: 14)
																.frame(maxWidth: .infinity, alignment: .leading)
																.lineLimit(1)
															
															let participantConversationModelIsAdmin = conversationViewModel.participantConversationModelAdmin.first(
																where: {$0.address == participantConversationModel.address})
															
															if participantConversationModelIsAdmin != nil {
																Text("conversation_info_participant_is_admin_label")
																	.foregroundStyle(Color.grayMain2c400)
																	.default_text_style(styleSize: 12)
																	.frame(maxWidth: .infinity, alignment: .leading)
																	.lineLimit(1)
															}
														}
														
														if conversationViewModel.myParticipantConversationModel != nil && conversationViewModel.myParticipantConversationModel!.address != participantConversationModel.address {
															Menu {
																Button(
																	action: {
																		let addressConv = participantConversationModel.address
																		
																		let friendIndex = contactsManager.lastSearch.firstIndex(
																			where: {$0.friend!.addresses.contains(where: {$0.asStringUriOnly() == addressConv})})
																		if friendIndex != nil {
																			withAnimation {
																				conversationViewModel.displayedConversation = nil
																				indexPage = 0
																				contactViewModel.indexDisplayedFriend = friendIndex
																			}
																		} else {
																			withAnimation {
																				conversationViewModel.displayedConversation = nil
																				indexPage = 0
																				
																				isShowEditContactFragment.toggle()
																				editContactViewModel.sipAddresses.removeAll()
																				editContactViewModel.sipAddresses.append(String(participantConversationModel.address.dropFirst(4) ?? ""))
																				editContactViewModel.sipAddresses.append("")
																			}
																		}
																	},
																	label: {
																		HStack {
																			let addressConv = participantConversationModel.address
																			
																			let friendIndex = contactsManager.lastSearch.firstIndex(
																				where: {$0.friend!.addresses.contains(where: {$0.asStringUriOnly() == addressConv})})
																			if friendIndex != nil {
																				Image("address-book")
																					.renderingMode(.template)
																					.resizable()
																					.foregroundStyle(Color.grayMain2c600)
																					.frame(width: 25, height: 25)
																				
																				Text("conversation_info_menu_go_to_contact")
																					.default_text_style(styleSize: 16)
																					.frame(maxWidth: .infinity, alignment: .leading)
																					.lineLimit(1)
																			} else {
																				Image("user-plus")
																					.renderingMode(.template)
																					.resizable()
																					.foregroundStyle(Color.grayMain2c600)
																					.frame(width: 25, height: 25)
																				
																				Text("conversation_info_menu_add_to_contacts")
																					.default_text_style(styleSize: 16)
																					.frame(maxWidth: .infinity, alignment: .leading)
																					.lineLimit(1)
																			}
																		}
																	}
																)
																
																if conversationViewModel.isUserAdmin {
																	let participantConversationModelIsAdmin = conversationViewModel.participantConversationModelAdmin.first(
																		where: {$0.address == participantConversationModel.address})
																	
																	Button {
																		conversationViewModel.toggleAdminRights(address: participantConversationModel.address)
																	} label: {
																		HStack {
																			Text(participantConversationModelIsAdmin != nil ? "conversation_info_admin_menu_unset_participant_admin" : "conversation_info_admin_menu_set_participant_admin")
																			Spacer()
																			Image("user-circle")
																				.renderingMode(.template)
																				.resizable()
																				.foregroundStyle(Color.grayMain2c500)
																				.frame(width: 25, height: 25, alignment: .leading)
																				.padding(.all, 10)
																		}
																	}
																	
																	Button(role: .destructive) {
																		conversationViewModel.removeParticipant(address: participantConversationModel.address)
																	} label: {
																		HStack {
																			Text("conversation_info_admin_menu_remove_participant")
																			Spacer()
																			Image("trash-simple-red")
																				.renderingMode(.template)
																				.resizable()
																				.foregroundStyle(Color.grayMain2c500)
																				.frame(width: 25, height: 25, alignment: .leading)
																				.padding(.all, 10)
																		}
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
														}
													}
													.padding(.vertical, 15)
													.padding(.horizontal, 20)
												}
												
												if conversationViewModel.isUserAdmin {
													NavigationLink(destination: {
														AddParticipantsFragment(addParticipantsViewModel: addParticipantsViewModel, confirmAddParticipantsFunc: conversationViewModel.addParticipants)
															.onAppear {
																conversationViewModel.getParticipants()
																addParticipantsViewModel.participantsToAdd = conversationViewModel.participants
															}
													}, label: {
														HStack {
															Image("plus-circle")
																.renderingMode(.template)
																.resizable()
																.foregroundStyle(Color.orangeMain500)
																.frame(width: 20, height: 20)
															
															Text("conversation_info_add_participants_label")
																.default_text_style_orange_500(styleSize: 14)
																.frame(height: 35)
														}
														
													})
													.padding(.horizontal, 20)
													.padding(.vertical, 5)
													.background(Color.orangeMain100)
													.cornerRadius(60)
													.padding(.top, 10)
													.padding(.bottom, 20)
													
													/*
													Button(
														action: {
														},
														label: {
															HStack {
																Image("plus-circle")
																	.renderingMode(.template)
																	.resizable()
																	.foregroundStyle(Color.orangeMain500)
																	.frame(width: 20, height: 20)
																
																Text("conversation_info_add_participants_label")
																	.default_text_style_orange_500(styleSize: 14)
																	.frame(height: 35)
															}
														}
													)
													.padding(.horizontal, 20)
													.padding(.vertical, 5)
													.background(Color.orangeMain100)
													.cornerRadius(60)
													.padding(.top, 10)
													.padding(.bottom, 20)
													 */
												}
											}
											.background(.white)
											.cornerRadius(15)
											.padding(.horizontal)
											.zIndex(-1)
											.transition(.move(edge: .top))
										}
									}
									
									Text("contact_details_actions_title")
										.default_text_style_800(styleSize: 18)
										.frame(maxWidth: .infinity, alignment: .leading)
										.padding(.horizontal, 20)
										.padding(.top, 20)
									
									VStack(spacing: 0) {
										if !conversationViewModel.displayedConversation!.isReadOnly {
											if !conversationViewModel.displayedConversation!.isGroup {
												Button(
													action: {
														if conversationViewModel.displayedConversation != nil {
															
															let addressConv = conversationViewModel.participantConversationModel.first?.address ?? ""
															
															let friendIndex = contactsManager.lastSearch.firstIndex(
																where: {$0.friend!.addresses.contains(where: {$0.asStringUriOnly() == addressConv})})
															if friendIndex != nil {
																withAnimation {
																	conversationViewModel.displayedConversation = nil
																	indexPage = 0
																	contactViewModel.indexDisplayedFriend = friendIndex
																}
															} else {
																withAnimation {
																	conversationViewModel.displayedConversation = nil
																	indexPage = 0
																	
																	isShowEditContactFragment.toggle()
																	editContactViewModel.sipAddresses.removeAll()
																	editContactViewModel.sipAddresses.append(String(conversationViewModel.participantConversationModel.first?.address.dropFirst(4) ?? ""))
																	editContactViewModel.sipAddresses.append("")
																}
															}
														}
													},
													label: {
														HStack {
															let addressConv = conversationViewModel.participantConversationModel.first?.address ?? ""
															
															let friendIndex = contactsManager.lastSearch.firstIndex(
																where: {$0.friend!.addresses.contains(where: {$0.asStringUriOnly() == addressConv})})
															if friendIndex != nil {
																Image("address-book")
																 .renderingMode(.template)
																 .resizable()
																 .foregroundStyle(Color.grayMain2c600)
																 .frame(width: 25, height: 25)
															 
															 Text("conversation_info_menu_go_to_contact")
																 .default_text_style(styleSize: 16)
																 .frame(maxWidth: .infinity, alignment: .leading)
																 .lineLimit(1)
															} else {
																Image("user-plus")
																	.renderingMode(.template)
																	.resizable()
																	.foregroundStyle(Color.grayMain2c600)
																	.frame(width: 25, height: 25)
																
																Text("conversation_info_menu_add_to_contacts")
																	.default_text_style(styleSize: 16)
																	.frame(maxWidth: .infinity, alignment: .leading)
																	.lineLimit(1)
															}
														}
													}
												)
												.frame(height: 60)
												
												Divider()
											}
											
											Button(
												action: {
													withAnimation {
														isShowEphemeralFragment = true
													}
												},
												label: {
													HStack {
														Image("clock-countdown")
															.renderingMode(.template)
															.resizable()
															.foregroundStyle(Color.grayMain2c600)
															.frame(width: 25, height: 25)
														
														Text("conversation_action_configure_ephemeral_messages")
															.default_text_style(styleSize: 16)
															.frame(maxWidth: .infinity, alignment: .leading)
															.lineLimit(1)
														
													}
												}
											)
											.frame(height: 60)
											
											Divider()
											
											if conversationViewModel.displayedConversation!.isGroup {
												Button(
													action: {
														conversationViewModel.displayedConversation!.leave()
														conversationViewModel.displayedConversation!.isReadOnly = true
														isShowInfoConversationFragment = false
													},
													label: {
														HStack {
															Image("sign-out")
																.renderingMode(.template)
																.resizable()
																.foregroundStyle(Color.grayMain2c600)
																.frame(width: 25, height: 25)
															
															Text("conversation_action_leave_group")
																.default_text_style(styleSize: 16)
																.frame(maxWidth: .infinity, alignment: .leading)
																.lineLimit(1)
															
														}
													}
												)
												.frame(height: 60)
												
												Divider()
											}
										}
										
										Button(
											action: {
												conversationViewModel.displayedConversation!.deleteChatRoom()
												conversationsListViewModel.computeChatRoomsList(filter: "")
												conversationViewModel.displayedConversation = nil
											},
											label: {
												HStack {
													Image("trash-simple")
														.renderingMode(.template)
														.resizable()
														.foregroundStyle(Color.redDanger500)
														.frame(width: 25, height: 25)
													
													Text("conversation_info_delete_history_action")
														.foregroundStyle(Color.redDanger500)
														.default_text_style(styleSize: 16)
														.frame(maxWidth: .infinity, alignment: .leading)
														.lineLimit(1)
													
												}
											}
										)
										.frame(height: 60)
									}
									.padding(.horizontal, 20)
									.padding(.vertical, 4)
									.background(.white)
									.cornerRadius(15)
									.padding(.all)
								}
								.frame(maxWidth: sharedMainViewModel.maxWidth)
							}
							.frame(maxWidth: .infinity)
							.padding(.top, 2)
						}
						.background(Color.gray100)
					}
					.background(.white)
					.navigationBarHidden(true)
					.onAppear {
						conversationViewModel.getParticipants()
					}
					.onRotate { newOrientation in
						orientation = newOrientation
					}
				}
			}
		}
		.navigationViewStyle(.stack)
	}
}

#Preview {
	ConversationInfoFragment(
		conversationViewModel: ConversationViewModel(),
		conversationsListViewModel: ConversationsListViewModel(),
		contactViewModel: ContactViewModel(),
		editContactViewModel: EditContactViewModel(),
		meetingViewModel: MeetingViewModel(),
		addParticipantsViewModel: AddParticipantsViewModel(),
		isMuted: .constant(false),
		isShowEphemeralFragment: .constant(false),
		isShowStartCallGroupPopup: .constant(false),
		isShowInfoConversationFragment: .constant(true),
		isShowEditContactFragment: .constant(false),
		indexPage: .constant(0),
		isShowScheduleMeetingFragment: .constant(false)
	)
}
// swiftlint:enable type_body_length
