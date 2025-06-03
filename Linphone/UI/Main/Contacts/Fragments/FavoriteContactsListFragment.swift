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

struct FavoriteContactsListFragment: View {
	
	@ObservedObject var contactsManager = ContactsManager.shared
	
	@EnvironmentObject var contactsListViewModel: ContactsListViewModel
    
    @Binding var showingSheet: Bool
    
    var body: some View {
        ScrollView(.horizontal) {
            HStack {
				ForEach(contactsManager.avatarListModel) { contactAvatarModel in
					FavoriteContactRow(contactAvatarModel: contactAvatarModel, showingSheet: $showingSheet)
                }
            }
			.padding(.horizontal, 10)
			.padding(.bottom, 10)
        }
    }
}

struct FavoriteContactRow: View {
	@ObservedObject var contactsManager = ContactsManager.shared
	
	@EnvironmentObject var contactsListViewModel: ContactsListViewModel
	
	@ObservedObject var contactAvatarModel: ContactAvatarModel
	
	@Binding var showingSheet: Bool
	
	var body: some View {
		if contactAvatarModel.starred == true {
			VStack {
				VStack {
					Avatar(contactAvatarModel: contactAvatarModel, avatarSize: 50)
					
					Text(contactAvatarModel.name)
						.default_text_style(styleSize: 16)
						.frame( maxWidth: .infinity, alignment: .center)
				}
			}
			.background(.white)
			.onTapGesture {
				withAnimation {
					SharedMainViewModel.shared.displayedFriend = contactAvatarModel
				}
			}
			.onLongPressGesture(minimumDuration: 0.2) {
				contactsListViewModel.selectedFriend = contactAvatarModel
				showingSheet.toggle()
			}
			.frame(minWidth: 70, maxWidth: 70)
		}
	}
}

#Preview {
    FavoriteContactsListFragment(
        showingSheet: .constant(false))
}
