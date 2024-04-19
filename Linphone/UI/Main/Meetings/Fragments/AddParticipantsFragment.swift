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
							scheduleMeetingViewModel.participantsToAdd = []
							dismiss()
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
				
				ScrollView(.horizontal) {
					HStack {
						ForEach(0..<scheduleMeetingViewModel.participantsToAdd.count, id: \.self) { index in
							ZStack(alignment: .topTrailing) {
								VStack {
									Avatar(contactAvatarModel: scheduleMeetingViewModel.participantsToAdd[index].avatarModel, avatarSize: 50)
									
									Text(scheduleMeetingViewModel.participantsToAdd[index].avatarModel.name)
										.default_text_style(styleSize: 12)
										.frame(minWidth: 60, maxWidth:80)
								}
								Image("x-circle")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c500)
									.frame(width: 25, height: 25)
									.onTapGesture {
										scheduleMeetingViewModel.participantsToAdd.remove(at: index)
									}
							}
						}
					}
				}
				.padding(.leading, 16)
				
				ZStack(alignment: .trailing) {
					TextField("Search contact", text: $scheduleMeetingViewModel.searchField)
						.default_text_style(styleSize: 15)
						.frame(height: 25)
						.focused($isSearchFieldFocused)
						.padding(.horizontal, 30)
						.onChange(of: scheduleMeetingViewModel.searchField) { newValue in
							magicSearch.currentFilterSuggestions = newValue
							magicSearch.searchForSuggestions()
						}.onAppear {
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
				
				ScrollView {
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
							if let addr = contactsManager.lastSearch[index].address {
								scheduleMeetingViewModel.selectParticipant(addr: addr)
							}
						}
						.buttonStyle(.borderless)
						.listRowSeparator(.hidden)
					}
					
					HStack(alignment: .center) {
						Text("Suggestions")
							.default_text_style_800(styleSize: 16)
						
						Spacer()
					}
					.padding(.vertical, 10)
					.padding(.horizontal, 16)
					
					suggestionsList
				}
			}
			Button {
				withAnimation {
					scheduleMeetingViewModel.addParticipants()
					dismiss()
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
		.navigationTitle("")
		.navigationBarHidden(true)
	}
	
	@Sendable private func delayColor() async {
		try? await Task.sleep(nanoseconds: 250_000_000)
		delayedColor = Color.orangeMain500
	}
	
	var suggestionsList: some View {
		ForEach(0..<contactsManager.lastSearchSuggestions.count, id: \.self) { index in
			Button {
				if let addr = contactsManager.lastSearchSuggestions[index].address {
					scheduleMeetingViewModel.selectParticipant(addr: addr)
				}
			} label: {
				HStack {
					if index < contactsManager.lastSearchSuggestions.count
						&& contactsManager.lastSearchSuggestions[index].address != nil
						&& contactsManager.lastSearchSuggestions[index].address!.username != nil {
						
						Image(uiImage: contactsManager.textToImage(
							firstName: contactsManager.lastSearchSuggestions[index].address!.username!,
							lastName: ""))
						.resizable()
						.frame(width: 45, height: 45)
						.clipShape(Circle())
						
						Text(contactsManager.lastSearchSuggestions[index].address?.username ?? "")
							.default_text_style(styleSize: 16)
							.frame(maxWidth: .infinity, alignment: .leading)
							.foregroundStyle(Color.orangeMain500)
					} else {
						Image("profil-picture-default")
							.resizable()
							.frame(width: 45, height: 45)
							.clipShape(Circle())
						
						Text("Username error")
							.default_text_style(styleSize: 16)
							.frame(maxWidth: .infinity, alignment: .leading)
							.foregroundStyle(Color.orangeMain500)
					}
				}
				.padding(.horizontal)
			}
			.buttonStyle(.borderless)
			.listRowSeparator(.hidden)
		}
	}
}

#Preview {
	AddParticipantsFragment(scheduleMeetingViewModel: ScheduleMeetingViewModel())
}
