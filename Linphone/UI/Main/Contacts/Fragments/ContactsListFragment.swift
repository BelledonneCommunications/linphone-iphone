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
	
	@EnvironmentObject var contactsListViewModel: ContactsListViewModel
	
	@Binding var showingSheet: Bool
	
    var startCallFunc: (_ addr: Address) -> Void
	
	var body: some View {
		ForEach(Array(contactsManager.avatarListModel.enumerated()), id: \.element.id) { index, contactAvatarModel in
			ContactRow(contactAvatarModel: contactAvatarModel, index: index, showingSheet: $showingSheet, startCallFunc: startCallFunc)
		}
	}
}

struct ContactRow: View {
	@ObservedObject var contactsManager = ContactsManager.shared
	
	@EnvironmentObject var contactsListViewModel: ContactsListViewModel
	
	@ObservedObject var contactAvatarModel: ContactAvatarModel
	
	let index: Int
	
	@Binding var showingSheet: Bool
	
	var startCallFunc: (_ addr: Address) -> Void
	
	var body: some View {
		HStack {
			HStack {
				if index <= 0
					|| (index < contactsManager.avatarListModel.count && contactAvatarModel.name.lowercased().folding(
						options: .diacriticInsensitive,
						locale: .current
					).first
					!= contactsManager.avatarListModel[index-1].name.lowercased().folding(
						options: .diacriticInsensitive,
						locale: .current
					).first) {
					Text(
						String(
							(contactAvatarModel.name.uppercased().folding(
								options: .diacriticInsensitive,
								locale: .current
							).first) ?? "?"))
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
				
				Avatar(contactAvatarModel: contactAvatarModel, avatarSize: 50)
				
				Text(contactAvatarModel.name)
					.default_text_style(styleSize: 16)
					.frame(maxWidth: .infinity, alignment: .leading)
					.foregroundStyle(Color.orangeMain500)
			}
		}
		.frame(height: 50)
		.buttonStyle(.borderless)
		.listRowInsets(EdgeInsets(top: 6, leading: 20, bottom: 6, trailing: 20))
		.listRowSeparator(.hidden)
		.background(.white)
		.onTapGesture {
            if SharedMainViewModel.shared.indexView == 0 {
                withAnimation {
                    SharedMainViewModel.shared.displayedFriend = contactAvatarModel
                }
            }
            
			if contactAvatarModel.friend != nil
				&& contactAvatarModel.friend!.address != nil {
				startCallFunc(contactAvatarModel.friend!.address!)
			}
		}
		.onLongPressGesture(minimumDuration: 0.2) {
            if SharedMainViewModel.shared.indexView == 0 {
                contactsListViewModel.selectedFriend = contactAvatarModel
                showingSheet.toggle()
            }
		}
	}
}

#Preview {
    ContactsListFragment(showingSheet: .constant(false), startCallFunc: {_ in })
}
