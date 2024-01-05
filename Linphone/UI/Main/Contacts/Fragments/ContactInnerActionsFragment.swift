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

struct ContactInnerActionsFragment: View {
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@ObservedObject var contactViewModel: ContactViewModel
	@ObservedObject var editContactViewModel: EditContactViewModel
	
	@State private var informationIsOpen = true
	
	@Binding var showingSheet: Bool
	@Binding var showShareSheet: Bool
	@Binding var isShowDeletePopup: Bool
	@Binding var isShowDismissPopup: Bool
	
	var actionEditButton: () -> Void
	
    var body: some View {
		HStack(alignment: .center) {
			Text("Information")
				.default_text_style_800(styleSize: 16)
			
			Spacer()
			
			Image(informationIsOpen ? "caret-up" : "caret-down")
				.renderingMode(.template)
				.resizable()
				.foregroundStyle(Color.grayMain2c600)
				.frame(width: 25, height: 25, alignment: .leading)
		}
		.padding(.top, 30)
		.padding(.bottom, 10)
		.padding(.horizontal, 16)
		.background(Color.gray100)
		.onTapGesture {
			withAnimation {
				informationIsOpen.toggle()
			}
		}
		
		if informationIsOpen {
			VStack(spacing: 0) {
				if contactViewModel.indexDisplayedFriend != nil && contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil {
					ForEach(0..<contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.addresses.count, id: \.self) { index in
						HStack {
							HStack {
								VStack {
									Text("SIP address :")
										.default_text_style_700(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
									Text(contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.addresses[index].asStringUriOnly().dropFirst(4))
										.default_text_style(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
										.lineLimit(1)
										.fixedSize(horizontal: false, vertical: true)
								}
								Spacer()
								
								Image("phone")
									.renderingMode(.template)
									.resizable()
									.foregroundStyle(Color.grayMain2c600)
									.frame(width: 25, height: 25)
							}
							.padding(.vertical, 15)
							.padding(.horizontal, 20)
						}
						.background(.white)
						.onTapGesture {
							withAnimation {
								telecomManager.doCallWithCore(
									addr: contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.addresses[index]
								)
							}
						}
						.onLongPressGesture(minimumDuration: 0.2) {
							contactViewModel.stringToCopy = contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.addresses[index].asStringUriOnly()
							showingSheet.toggle()
						}
						
						if !contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbers.isEmpty
							|| index < contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.addresses.count - 1 {
							VStack {
								Divider()
							}
							.padding(.horizontal)
						}
					}
					
					ForEach(0..<contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbers.count, id: \.self) { index in
						HStack {
							HStack {
								VStack {
									if contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].label != nil
										&& !contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].label!.isEmpty {
										Text("Phone (\(contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].label!)) :")
											.default_text_style_700(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
									} else {
										Text("Phone :")
											.default_text_style_700(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
									}
									Text(contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].phoneNumber)
										.default_text_style(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
										.lineLimit(1)
										.fixedSize(horizontal: false, vertical: true)
								}
								Spacer()
							}
							.padding(.vertical, 15)
							.padding(.horizontal, 20)
						}
						.background(.white)
						.onLongPressGesture(minimumDuration: 0.2) {
							contactViewModel.stringToCopy =
							   contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbersWithLabel[index].phoneNumber
							showingSheet.toggle()
						}
						
						if index < contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.phoneNumbers.count - 1 {
							VStack {
								Divider()
							}
							.padding(.horizontal)
						}
					}
				}
			}
			.background(.white)
			.cornerRadius(15)
			.padding(.horizontal)
			.zIndex(-1)
			.transition(.move(edge: .top))
		}
		
		if contactViewModel.indexDisplayedFriend != nil
			&& contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
			&& ((contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization != nil
				 && !contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization!.isEmpty)
				|| (contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle != nil
					&& !contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle!.isEmpty)) {
			VStack {
				if contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization != nil
					&& !contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization!.isEmpty {
					Text("**Company :** \(contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization!)")
						.default_text_style(styleSize: 14)
						.padding(.vertical, 15)
						.padding(.horizontal, 20)
						.frame(maxWidth: .infinity, alignment: .leading)
				}
				
				if contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle != nil
					&& !contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle!.isEmpty {
					Text("**Job :** \(contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.jobTitle!)")
						.default_text_style(styleSize: 14)
						.padding(.top, 
								 contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization != nil
								 && !contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.organization!.isEmpty 
								 ? 0 : 15
						)
						.padding(.bottom, 15)
						.padding(.horizontal, 20)
						.frame(maxWidth: .infinity, alignment: .leading)
				}
			}
			.background(.white)
			.cornerRadius(15)
			.padding(.top)
			.padding(.horizontal)
			.zIndex(-1)
			.transition(.move(edge: .top))
		}
		
		// TODO Trust Fragment
		
		// TODO Medias Fragment
		
		HStack(alignment: .center) {
			Text("Other actions")
				.default_text_style_800(styleSize: 16)
			
			Spacer()
		}
		.padding(.vertical, 10)
		.padding(.horizontal, 16)
		.background(Color.gray100)
		
		VStack(spacing: 0) {
			if contactViewModel.indexDisplayedFriend != nil && contactViewModel.indexDisplayedFriend! < contactsManager.lastSearch.count
				&& contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
				&& contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.nativeUri != nil
				&& !contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.nativeUri!.isEmpty {
				Button {
					actionEditButton()
				} label: {
					HStack {
						Image("pencil-simple")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c600)
							.frame(width: 25, height: 25)
						
						Text("Edit")
							.default_text_style(styleSize: 14)
							.frame(maxWidth: .infinity, alignment: .leading)
							.lineLimit(1)
							.fixedSize(horizontal: false, vertical: true)
						Spacer()
					}
					.padding(.vertical, 15)
					.padding(.horizontal, 20)
				}
			} else {
				NavigationLink(destination: EditContactFragment(
						editContactViewModel: editContactViewModel,
						contactViewModel: contactViewModel,
						isShowEditContactFragment: .constant(false),
						isShowDismissPopup: $isShowDismissPopup)) {
						HStack {
							Image("pencil-simple")
								.renderingMode(.template)
								.resizable()
								.foregroundStyle(Color.grayMain2c600)
								.frame(width: 25, height: 25)
							
							Text("Edit")
								.default_text_style(styleSize: 14)
								.frame(maxWidth: .infinity, alignment: .leading)
								.lineLimit(1)
								.fixedSize(horizontal: false, vertical: true)
							Spacer()
						}
						.padding(.vertical, 15)
						.padding(.horizontal, 20)
				}
				.simultaneousGesture(
					TapGesture().onEnded {
						editContactViewModel.selectedEditFriend = contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend
						editContactViewModel.resetValues()
					}
				)
			}
			
			VStack {
				Divider()
			}
			.padding(.horizontal)
			
			Button {
				if contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil {
					contactViewModel.objectWillChange.send()
					contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.starred.toggle()
				}
			} label: {
				HStack {
					Image(contactViewModel.indexDisplayedFriend != nil && contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
						  && contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.starred == true ? "heart-fill" : "heart")
					.renderingMode(.template)
					.resizable()
					.foregroundStyle(contactViewModel.indexDisplayedFriend != nil && contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
									 && contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.starred == true ? Color.redDanger500 : Color.grayMain2c500)
					.frame(width: 25, height: 25)
					Text(contactViewModel.indexDisplayedFriend != nil
						 && contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil
						 && contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend!.starred == true
						 ? "Remove from favourites"
						 : "Add to favourites")
					.default_text_style(styleSize: 14)
					.frame(maxWidth: .infinity, alignment: .leading)
					.lineLimit(1)
					.fixedSize(horizontal: false, vertical: true)
					Spacer()
				}
				.padding(.vertical, 15)
				.padding(.horizontal, 20)
			}
			
			VStack {
				Divider()
			}
			.padding(.horizontal)
			
			Button {
				showShareSheet.toggle()
			} label: {
				HStack {
					Image("share-network")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.grayMain2c600)
						.frame(width: 25, height: 25)
					
					Text("Share")
						.default_text_style(styleSize: 14)
						.frame(maxWidth: .infinity, alignment: .leading)
						.lineLimit(1)
						.fixedSize(horizontal: false, vertical: true)
					Spacer()
				}
				.padding(.vertical, 15)
				.padding(.horizontal, 20)
			}
			
			VStack {
				Divider()
			}
			.padding(.horizontal)
			
			/*
			 Button {
			 } label: {
			 HStack {
			 Image("bell-simple-slash")
			 .renderingMode(.template)
			 .resizable()
			 .foregroundStyle(Color.grayMain2c600)
			 .frame(width: 25, height: 25)
			 
			 Text("Mute")
			 .default_text_style(styleSize: 14)
			 .frame(maxWidth: .infinity, alignment: .leading)
			 .lineLimit(1)
			 .fixedSize(horizontal: false, vertical: true)
			 Spacer()
			 }
			 .padding(.vertical, 15)
			 .padding(.horizontal, 20)
			 }
			 
			 VStack {
			 Divider()
			 }
			 .padding(.horizontal)
			 
			 Button {
			 } label: {
			 HStack {
			 Image("x-circle")
			 .renderingMode(.template)
			 .resizable()
			 .foregroundStyle(Color.grayMain2c600)
			 .frame(width: 25, height: 25)
			 
			 Text("Block")
			 .default_text_style(styleSize: 14)
			 .frame(maxWidth: .infinity, alignment: .leading)
			 .lineLimit(1)
			 .fixedSize(horizontal: false, vertical: true)
			 Spacer()
			 }
			 .padding(.vertical, 15)
			 .padding(.horizontal, 20)
			 }
			 
			 VStack {
			 Divider()
			 }
			 .padding(.horizontal)
			 */
			
			Button {
				if contactsManager.lastSearch[contactViewModel.indexDisplayedFriend!].friend != nil {
					isShowDeletePopup.toggle()
				}
			} label: {
				HStack {
					Image("trash-simple")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.redDanger500)
						.frame(width: 25, height: 25)
					
					Text("Delete this contact")
						.foregroundStyle(Color.redDanger500)
						.default_text_style(styleSize: 14)
						.frame(maxWidth: .infinity, alignment: .leading)
						.lineLimit(1)
						.fixedSize(horizontal: false, vertical: true)
					Spacer()
				}
				.padding(.vertical, 15)
				.padding(.horizontal, 20)
			}
		}
		.background(.white)
		.cornerRadius(15)
		.padding(.horizontal)
		.zIndex(-1)
		.transition(.move(edge: .top))
    }
}

#Preview {
	ContactInnerActionsFragment(
		contactViewModel: ContactViewModel(),
		editContactViewModel: EditContactViewModel(),
		showingSheet: .constant(false),
		showShareSheet: .constant(false),
		isShowDeletePopup: .constant(false),
		isShowDismissPopup: .constant(false),
		actionEditButton: {}
	)
}
