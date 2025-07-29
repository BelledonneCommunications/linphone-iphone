/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
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

struct ParticipantsListFragment: View {
	
	@ObservedObject private var coreContext = CoreContext.shared
	@ObservedObject private var contactsManager = ContactsManager.shared
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@ObservedObject var callViewModel: CallViewModel
	
	@ObservedObject var addParticipantsViewModel: AddParticipantsViewModel
	
	@State private var delayedColor = Color.white
	
	@Binding var isShowParticipantsListFragment: Bool
	
	@State private var isShowPopup = false
	@State private var indexToRemove = -1
	
	var body: some View {
		NavigationView {
			ZStack {
				VStack(spacing: 1) {
					Rectangle()
						.foregroundColor(delayedColor)
						.edgesIgnoringSafeArea(.top)
						.frame(height: 0)
						.task(delayColor)
					
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
								delayColorDismiss()
								withAnimation {
									isShowParticipantsListFragment.toggle()
								}
							}
						
						Text("\(callViewModel.participantList.count + 1) \(callViewModel.participantList.isEmpty ? "Participant" : "Participants")")
							.multilineTextAlignment(.leading)
							.default_text_style_orange_800(styleSize: 16)
						
						Spacer()
						
					}
					.frame(maxWidth: .infinity)
					.frame(height: 50)
					.padding(.horizontal)
					.padding(.bottom, 4)
					.background(.white)
					
					participantsList
					
					HStack {
						Spacer()
						
						if callViewModel.myParticipantModel != nil && callViewModel.myParticipantModel!.isAdmin {
							NavigationLink(destination: {
								AddParticipantsFragment(addParticipantsViewModel: addParticipantsViewModel, confirmAddParticipantsFunc: callViewModel.addParticipants, dismissOnCheckClick: true)
									.onAppear {
										addParticipantsViewModel.participantsToAdd = []
									}
							}, label: {
								Image("plus")
									.resizable()
									.renderingMode(.template)
									.frame(width: 25, height: 25)
									.foregroundStyle(.white)
									.padding()
									.background(Color.orangeMain500)
									.clipShape(Circle())
									.shadow(color: .black.opacity(0.2), radius: 4)
								
							})
							.padding()
						}
					}
					.padding(.trailing, 10)
				}
				.background(.white)
				
				if self.isShowPopup {
					let contentPopup = Text(String(format: String(localized: "meeting_call_remove_participant_confirmation_message"), callViewModel.participantList[indexToRemove].name))
					PopupView(isShowPopup: $isShowPopup,
							  title: Text("meeting_call_remove_participant_confirmation_title"),
							  content: contentPopup,
							  titleFirstButton: Text("dialog_no"),
							  actionFirstButton: {self.isShowPopup.toggle()},
							  titleSecondButton: Text("dialog_yes"),
							  actionSecondButton: {
						callViewModel.removeParticipant(index: indexToRemove)
						self.isShowPopup.toggle()
						indexToRemove = -1
					})
					.background(.black.opacity(0.65))
					.onTapGesture {
						self.isShowPopup.toggle()
						indexToRemove = -1
					}
				}
			}
			.navigationBarHidden(true)
		}
	}
	
	@Sendable private func delayColor() async {
		try? await Task.sleep(nanoseconds: 250_000_000)
		delayedColor = Color.orangeMain500
	}
	
	func delayColorDismiss() {
		Task {
			try? await Task.sleep(nanoseconds: 80_000_000)
			delayedColor = .white
		}
	}
	
	var participantsList: some View {
		VStack {
			List {
				HStack {
					HStack {
						if callViewModel.myParticipantModel != nil {
							Avatar(contactAvatarModel: callViewModel.myParticipantModel!.avatarModel, avatarSize: 50, hidePresence: true)
							
							Text(callViewModel.myParticipantModel!.name)
								.default_text_style(styleSize: 16)
								.frame(maxWidth: .infinity, alignment: .leading)
								.lineLimit(1)
							
							Spacer()
							
							if callViewModel.myParticipantModel!.isAdmin {
								Text("conversation_info_participant_is_admin_label")
									.foregroundStyle(Color.grayMain2c300)
									.default_text_style(styleSize: 12)
									.frame(maxWidth: .infinity, alignment: .trailing)
									.lineLimit(1)
							}
							
							if callViewModel.myParticipantModel!.isAdmin {
								Toggle("", isOn: .constant(true))
									.tint(Color.greenSuccess700)
									.labelsHidden()
									.padding(.horizontal, 10)
								
								HStack(alignment: .center, spacing: 10) {
									Image("x")
										.renderingMode(.template)
										.foregroundStyle(Color.grayMain2c400)
										.frame(maxWidth: .infinity, maxHeight: .infinity)
								}
								.frame(width: 30, height: 30, alignment: .center)
								.hidden()
							}
						}
					}
				}
				.buttonStyle(.borderless)
				.listRowInsets(EdgeInsets(top: 10, leading: 20, bottom: 10, trailing: 20))
				.listRowSeparator(.hidden)
				.background(.white)
				
				ForEach(0..<callViewModel.participantList.count, id: \.self) { index in
					HStack {
						HStack {
							if index < callViewModel.participantList.count {
								Avatar(contactAvatarModel: callViewModel.participantList[index].avatarModel, avatarSize: 50, hidePresence: true)
								
								Text(callViewModel.participantList[index].name)
									.default_text_style(styleSize: 16)
									.frame(maxWidth: .infinity, alignment: .leading)
									.lineLimit(1)
								
								Spacer()
								
								if callViewModel.participantList[index].isAdmin {
									Text("conversation_info_participant_is_admin_label")
										.foregroundStyle(Color.grayMain2c300)
										.default_text_style(styleSize: 12)
										.frame(maxWidth: .infinity, alignment: .trailing)
										.lineLimit(1)
								}
								
								if callViewModel.myParticipantModel!.isAdmin {
									Toggle("", isOn: $callViewModel.participantList[index].isAdmin)
										.tint(Color.greenSuccess500)
										.labelsHidden()
										.padding(.horizontal, 10)
										.onTapGesture {
											callViewModel.toggleAdminParticipant(index: index)
										}
									
									Button {
										indexToRemove = index
										isShowPopup.toggle()
									} label: {
										Image("x")
											.renderingMode(.template)
											.foregroundStyle(Color.grayMain2c400)
											.frame(maxWidth: .infinity, maxHeight: .infinity)
									}
									.frame(width: 30, height: 30, alignment: .center)
									.background(Color.grayMain2c100)
									.cornerRadius(50)
								}
							}
						}
					}
					.buttonStyle(.borderless)
					.listRowInsets(EdgeInsets(top: 10, leading: 20, bottom: 10, trailing: 20))
					.listRowSeparator(.hidden)
					.background(.white)
				}
			}
			.listStyle(.plain)
			.overlay(
				VStack {
					if callViewModel.myParticipantModel == nil && callViewModel.calls.isEmpty {
						Spacer()
						Image("illus-belledonne")
							.resizable()
							.scaledToFit()
							.clipped()
							.padding(.all)
						Text("meeting_call_remove_no_participants")
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
	ParticipantsListFragment(callViewModel: CallViewModel(), 
							 addParticipantsViewModel: AddParticipantsViewModel(),
							 isShowParticipantsListFragment: .constant(true))
}
