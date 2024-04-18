//
//  ParticipantsListFragment.swift
//  Linphone
//
//  Created by QuentinArguillere on 16/04/2024.
//

import SwiftUI
import Foundation
import linphonesw

struct AddParticipantsFragment: View {
	
	@Environment(\.dismiss) var dismiss
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	@State private var orientation = UIDevice.current.orientation
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject var magicSearch = MagicSearchSingleton.shared
	@ObservedObject var scheduleMeetingViewModel: ScheduleMeetingViewModel
	
	@State private var delayedColor = Color.white
	
	@Binding var isShowAddParticipantFragment: Bool
	@FocusState var isSearchFieldFocused: Bool
	
	var body: some View {
		ZStack(alignment: .bottomTrailing) {
			VStack(spacing: 16) {
				if #available(iOS 16.0, *) {
					Rectangle()
						.foregroundColor(delayedColor)
						.edgesIgnoringSafeArea(.top)
						.frame(height: 0)
						.task(delayColor)
				} else if idiom != .pad && !(orientation == .landscapeLeft || orientation == .landscapeRight
											 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
					Rectangle()
						.foregroundColor(delayedColor)
						.edgesIgnoringSafeArea(.top)
						.frame(height: 1)
						.task(delayColor)
				}
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
							isShowAddParticipantFragment = false
							scheduleMeetingViewModel.participantsToAdd = []
						}
					
					VStack(alignment: .leading, spacing: 3) {
						Text("Add participants")
							.multilineTextAlignment(.leading)
							.default_text_style_orange_800(styleSize: 16)
							.padding(.top, 20)
						Text("\($scheduleMeetingViewModel.participants.count) selected participants")
							.default_text_style_300(styleSize: 12)
					}
					Spacer()
				}
				.frame(maxWidth: .infinity)
				.frame(height: 50)
				.padding(.horizontal)
				.padding(.bottom, 4)
				.background(.white)
				
				ForEach(0..<scheduleMeetingViewModel.participantsToAdd.count, id: \.self) { index in
					HStack {
						ZStack {
							VStack {
									Avatar(contactAvatarModel: scheduleMeetingViewModel.participantsToAdd[index].avatarModel, avatarSize: 50)
								
								Text(scheduleMeetingViewModel.participantsToAdd[index].avatarModel.name)
								 .default_text_style(styleSize: 16)
								 .frame(maxWidth: .infinity, alignment: .leading)
							}
							Image("x-circle")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c500)
								.frame(width: 25, height: 25, alignment: .leading)
								.padding(.all, 10)
						}
						
					}
				}
				ZStack(alignment: .trailing) {
					TextField("Search contact", text: $scheduleMeetingViewModel.searchField)
						.default_text_style(styleSize: 15)
						.frame(height: 25)
						.focused($isSearchFieldFocused)
						.padding(.horizontal, 30)
						.onChange(of: scheduleMeetingViewModel.searchField) { newValue in
							magicSearch.currentFilterSuggestions = newValue
							magicSearch.searchForSuggestions()
						}
					
					HStack {
						Button(action: {
						}, label: {
							Image("magnifying-glass")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c500)
								.frame(width: 25, height: 25)
						})
						
						Spacer()
						
						if !scheduleMeetingViewModel.searchField.isEmpty {
							Button(action: {
								scheduleMeetingViewModel.searchField = ""
								magicSearch.currentFilterSuggestions = ""
								magicSearch.searchForSuggestions()
								isSearchFieldFocused = false
							}, label: {
								Image("x")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 25, height: 25)
							})
						}
					}
				}
				.padding(.horizontal, 15)
				.padding(.vertical, 10)
				.cornerRadius(60)
				.overlay(
					RoundedRectangle(cornerRadius: 60)
						.inset(by: 0.5)
						.stroke(isSearchFieldFocused ? Color.orangeMain500 : Color.gray200, lineWidth: 1)
				)
				.padding(.vertical)
				.padding(.horizontal)
				
				ForEach(0..<contactsManager.lastSearch.count, id: \.self) { index in
					HStack {
						HStack {
							if index == 0
								|| contactsManager.lastSearch[index].friend?.name!.lowercased().folding(
									options: .diacriticInsensitive,
									locale: .current
								).first
								!= contactsManager.lastSearch[index-1].friend?.name!.lowercased().folding(
									options: .diacriticInsensitive,
									locale: .current
								).first {
								Text(
									String(
										(contactsManager.lastSearch[index].friend?.name!.uppercased().folding(
											options: .diacriticInsensitive,
											locale: .current
										).first)!))
								.contact_text_style_500(styleSize: 20)
								.frame(width: 18)
								.padding(.leading, -5)
								.padding(.trailing, 10)
							} else {
								Text("")
									.contact_text_style_500(styleSize: 20)
									.frame(width: 18)
									.padding(.leading, -5)
									.padding(.trailing, 10)
							}
							
							if index < contactsManager.avatarListModel.count
								&& contactsManager.avatarListModel[index].friend!.photo != nil
								&& !contactsManager.avatarListModel[index].friend!.photo!.isEmpty {
								Avatar(contactAvatarModel: contactsManager.avatarListModel[index], avatarSize: 50)
							} else {
								Image("profil-picture-default")
									.resizable()
									.frame(width: 50, height: 50)
									.clipShape(Circle())
							}
							Text((contactsManager.lastSearch[index].friend?.name)!)
								.default_text_style(styleSize: 16)
								.frame(maxWidth: .infinity, alignment: .leading)
								.foregroundStyle(Color.orangeMain500)
							Spacer()
							
						}
					}
					.background(.white)
					.onTapGesture {
						if contactsManager.lastSearch[index].friend != nil && contactsManager.lastSearch[index].friend!.address != nil {
							scheduleMeetingViewModel.selectParticipant(addr: contactsManager.lastSearch[index].friend!.address!)
						}
					}
					.buttonStyle(.borderless)
					.listRowSeparator(.hidden)
				}
				Spacer()
			}
			Button {
				withAnimation {
					scheduleMeetingViewModel.addParticipants()
					isShowAddParticipantFragment.toggle()
				}
			} label: {
				Image("check")
					.renderingMode(.template)
					.foregroundStyle(.white)
					.padding()
					.background(Color.orangeMain500)
					.clipShape(Circle())
					.shadow(color: .black.opacity(0.2), radius: 4)
				
			}
			.padding()
		}
	}
	
	@Sendable private func delayColor() async {
		try? await Task.sleep(nanoseconds: 250_000_000)
		delayedColor = Color.orangeMain500
	}
	
}

#Preview {
	AddParticipantsFragment(scheduleMeetingViewModel: ScheduleMeetingViewModel()
		, isShowAddParticipantFragment: .constant(true))
}
