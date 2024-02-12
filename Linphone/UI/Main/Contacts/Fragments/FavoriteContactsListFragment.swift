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
	
    @ObservedObject var contactViewModel: ContactViewModel
    @ObservedObject var favoriteContactsListViewModel: FavoriteContactsListViewModel
    
    @Binding var showingSheet: Bool
    
    var body: some View {
        ScrollView(.horizontal) {
            HStack {
                ForEach(0..<contactsManager.lastSearch.count, id: \.self) { index in
					if contactsManager.lastSearch[index].friend != nil && contactsManager.lastSearch[index].friend!.starred == true {
						VStack {
							VStack {
								if contactsManager.lastSearch[index].friend!.photo != nil
									&& !contactsManager.lastSearch[index].friend!.photo!.isEmpty {
									Avatar(contactAvatarModel: contactsManager.avatarListModel[index], avatarSize: 50)
								} else {
									Image("profil-picture-default")
										.resizable()
										.frame(width: 50, height: 50)
										.clipShape(Circle())
								}
								Text((contactsManager.lastSearch[index].friend?.name)!)
									.default_text_style(styleSize: 16)
									.frame( maxWidth: .infinity, alignment: .center)
							}
						}
						.background(.white)
						.onTapGesture {
							withAnimation {
								contactViewModel.indexDisplayedFriend = index
							}
						}
						.onLongPressGesture(minimumDuration: 0.2) {
							contactViewModel.selectedFriend = contactsManager.lastSearch[index].friend
							showingSheet.toggle()
						}
						.frame(minWidth: 70, maxWidth: 70)
					}
                }
            }
			.padding(.horizontal, 10)
			.padding(.bottom, 10)
        }
    }
}

#Preview {
    FavoriteContactsListFragment(
        contactViewModel: ContactViewModel(),
        favoriteContactsListViewModel: FavoriteContactsListViewModel(),
        showingSheet: .constant(false))
}
