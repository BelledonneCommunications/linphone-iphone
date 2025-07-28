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
import Contacts

struct ContactFragment: View {
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@EnvironmentObject var contactAvatarModel: ContactAvatarModel
	@EnvironmentObject var contactsListViewModel: ContactsListViewModel
	
	@Binding var isShowDeletePopup: Bool
	@Binding var isShowDismissPopup: Bool
	@Binding var isShowSipAddressesPopup: Bool
	@Binding var isShowSipAddressesPopupType: Int
	@Binding var isShowEditContactFragmentInContactDetails: Bool
	
	@State private var showingSheet = false
	@State private var showShareSheet = false
	
	var body: some View {
		if #available(iOS 16.0, *), idiom != .pad {
			contactInnerContent(contactsListViewModel: contactsListViewModel)
				.sheet(isPresented: $showingSheet) {
					ContactListBottomSheet(contactsListViewModel: contactsListViewModel, showingSheet: $showingSheet)
						.presentationDetents([.fraction(0.1)])
				}
				.sheet(isPresented: $showShareSheet) {
					ShareSheet(friendToShare: contactAvatarModel)
						.presentationDetents([.medium])
						.edgesIgnoringSafeArea(.bottom)
				}
		} else {
			contactInnerContent(contactsListViewModel: contactsListViewModel)
				.halfSheet(showSheet: $showingSheet) {
					ContactListBottomSheet(contactsListViewModel: contactsListViewModel, showingSheet: $showingSheet)
				} onDismiss: {}
				.sheet(isPresented: $showShareSheet) {
					ShareSheet(friendToShare: contactAvatarModel)
						.edgesIgnoringSafeArea(.bottom)
				}
		}
	}
	
	@ViewBuilder
	private func contactInnerContent(contactsListViewModel: ContactsListViewModel) -> some View {
		ContactInnerFragment(
			cnContact: CNContact(),
			isShowDeletePopup: $isShowDeletePopup,
			showingSheet: $showingSheet,
			showShareSheet: $showShareSheet,
			isShowDismissPopup: $isShowDismissPopup,
			isShowSipAddressesPopup: $isShowSipAddressesPopup,
			isShowSipAddressesPopupType: $isShowSipAddressesPopupType,
			isShowEditContactFragmentInContactDetails: $isShowEditContactFragmentInContactDetails
		)
	}
}

#Preview {
	ContactFragment(
		isShowDeletePopup: .constant(false),
		isShowDismissPopup: .constant(false),
		isShowSipAddressesPopup: .constant(false),
		isShowSipAddressesPopupType: .constant(0),
		isShowEditContactFragmentInContactDetails: .constant(false)
	)
}
