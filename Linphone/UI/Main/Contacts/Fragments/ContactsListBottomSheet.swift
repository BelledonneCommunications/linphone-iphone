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
import Contacts

struct ContactsListBottomSheet: View {
	
	@Environment(\.dismiss) var dismiss
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@EnvironmentObject var contactsListViewModel: ContactsListViewModel
	
	@State private var orientation = UIDevice.current.orientation
	
	@Binding var isShowDeletePopup: Bool
	@Binding var showingSheet: Bool
	@Binding var showShareSheet: Bool
	
	var body: some View {
		VStack(alignment: .leading) {
			if idiom != .pad && (orientation == .landscapeLeft
								 || orientation == .landscapeRight
								 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
				Spacer()
				HStack {
					Spacer()
					Button("dialog_close") {
						if #available(iOS 16.0, *) {
							showingSheet.toggle()
						} else {
							showingSheet.toggle()
							dismiss()
						}
					}
				}
				.padding(.trailing)
			}
			
			if !contactsListViewModel.selectedFriend!.isReadOnly {
				Spacer()
				
				Button {
					self.contactsListViewModel.toggleStarredSelectedFriend()
					
					if #available(iOS 16.0, *) {
						if idiom != .pad {
							showingSheet.toggle()
						} else {
							showingSheet.toggle()
							dismiss()
						}
					} else {
						showingSheet.toggle()
						dismiss()
					}
				} label: {
					HStack {
						Image(contactsListViewModel.selectedFriend != nil && contactsListViewModel.selectedFriend!.starred == true ? "heart-fill" : "heart")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(
								contactsListViewModel.selectedFriend != nil && contactsListViewModel.selectedFriend!.starred == true
								? Color.redDanger500
								: Color.grayMain2c500
							)
							.frame(width: 25, height: 25, alignment: .leading)
							.padding(.all, 10)
						Text(contactsListViewModel.selectedFriend != nil && contactsListViewModel.selectedFriend!.starred == true
							 ? "contact_details_remove_from_favourites"
							 : "contact_details_add_to_favourites")
						.default_text_style(styleSize: 16)
						Spacer()
					}
					.frame(maxHeight: .infinity)
				}
				.padding(.horizontal, 30)
				.background(Color.gray100)
				
				VStack {
					Divider()
				}
				.frame(maxWidth: .infinity)
			}
			
			Button {
				if #available(iOS 16.0, *) {
					if idiom != .pad {
						showingSheet.toggle()
					} else {
						showingSheet.toggle()
						dismiss()
					}
				} else {
					showingSheet.toggle()
					dismiss()
				}
                
                contactsListViewModel.selectedFriendToShare = contactsListViewModel.selectedFriend
				
				DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
					showShareSheet.toggle()
				}
				
			} label: {
				HStack {
					Image("share-network")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.grayMain2c500)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
					Text("contact_details_share")
						.default_text_style(styleSize: 16)
					Spacer()
				}
				.frame(maxHeight: .infinity)
			}
			.padding(.horizontal, 30)
			.background(Color.gray100)
			
			if !contactsListViewModel.selectedFriend!.isReadOnly {
				VStack {
					Divider()
				}
				.frame(maxWidth: .infinity)
				
				Button {
					if contactsListViewModel.selectedFriend != nil {
						isShowDeletePopup.toggle()
					}
					
					if #available(iOS 16.0, *) {
						if idiom != .pad {
							showingSheet.toggle()
						} else {
							showingSheet.toggle()
							dismiss()
						}
					} else {
						showingSheet.toggle()
						dismiss()
					}
				} label: {
					HStack {
						Image("trash-simple")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.redDanger500)
							.frame(width: 25, height: 25, alignment: .leading)
							.padding(.all, 10)
						Text("contact_details_delete")
							.foregroundStyle(Color.redDanger500)
							.default_text_style(styleSize: 16)
						Spacer()
					}
					.frame(maxHeight: .infinity)
				}
				.padding(.horizontal, 30)
				.background(Color.gray100)
			}
		}
		.background(Color.gray100)
		.frame(maxWidth: .infinity)
		.onRotate { newOrientation in
			orientation = newOrientation
		}
		.onDisappear {
			contactsListViewModel.selectedFriend = nil
		}
	}
}
