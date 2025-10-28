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

struct ContactsFragment: View {
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@EnvironmentObject var contactsListViewModel: ContactsListViewModel
	
	@Binding var isShowDeletePopup: Bool
	
	@State private var showingSheet = false
	@State private var showShareSheet = false
	@Binding var text: String
	
	var body: some View {
		ZStack {
			if #available(iOS 16.0, *), idiom != .pad {
				ContactsInnerFragment(showingSheet: $showingSheet, text: $text)
					.sheet(isPresented: $showingSheet) {
						ContactsListBottomSheet(
							isShowDeletePopup: $isShowDeletePopup,
							showingSheet: $showingSheet,
							showShareSheet: $showShareSheet
						)
						.presentationDetents(contactsListViewModel.selectedFriend?.isReadOnly == true ? [.fraction(0.1)] : [.fraction(0.3)])
					}
					.sheet(isPresented: $showShareSheet) {
						ShareSheet(friendToShare: contactsListViewModel.selectedFriendToShare!)
							.presentationDetents([.medium])
							.edgesIgnoringSafeArea(.bottom)
					}
			} else {
				ContactsInnerFragment(showingSheet: $showingSheet, text: $text)
					.halfSheet(showSheet: $showingSheet) {
						ContactsListBottomSheet(
							isShowDeletePopup: $isShowDeletePopup,
							showingSheet: $showingSheet,
							showShareSheet: $showShareSheet
						)
                        .environmentObject(contactsListViewModel)
					} onDismiss: {}
					.sheet(isPresented: $showShareSheet) {
						ShareSheet(friendToShare: contactsListViewModel.selectedFriendToShare!)
							.edgesIgnoringSafeArea(.bottom)
					}
			}
		}
	}
}

#Preview {
	ContactsFragment(isShowDeletePopup: .constant(false), text: .constant(""))
}
