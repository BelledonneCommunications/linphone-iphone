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

struct ContactsListFragment: View {
	
	@ObservedObject var contactsManager = ContactsManager.shared
	
	@ObservedObject var contactViewModel: ContactViewModel
	@ObservedObject var contactsListViewModel: ContactsListViewModel
	
	@Binding var showingSheet: Bool
	
    var startCallFunc: (_ addr: Address) -> Void
	
	var body: some View {
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
						Avatar(contactAvatarModel: contactsManager.avatarListModel[index], avatarSize: 45)
					} else {
						Image("profil-picture-default")
							.resizable()
							.frame(width: 45, height: 45)
							.clipShape(Circle())
					}
					Text((contactsManager.lastSearch[index].friend?.name)!)
						.default_text_style(styleSize: 16)
						.frame(maxWidth: .infinity, alignment: .leading)
						.foregroundStyle(Color.orangeMain500)
				}
			}
			.background(.white)
			.onTapGesture {
				withAnimation {
					contactViewModel.indexDisplayedFriend = index
				}
				if contactsManager.lastSearch[index].friend != nil && contactsManager.lastSearch[index].friend!.address != nil {
					startCallFunc(contactsManager.lastSearch[index].friend!.address!)
				}
		  	}
		  	.onLongPressGesture(minimumDuration: 0.2) {
				contactViewModel.selectedFriend = contactsManager.lastSearch[index].friend
				showingSheet.toggle()
			}
			.buttonStyle(.borderless)
			.listRowSeparator(.hidden)
		}
	}
}

#Preview {
    ContactsListFragment(contactViewModel: ContactViewModel()
						 , contactsListViewModel: ContactsListViewModel()
						 , showingSheet: .constant(false)
						 , startCallFunc: {_ in })
}
