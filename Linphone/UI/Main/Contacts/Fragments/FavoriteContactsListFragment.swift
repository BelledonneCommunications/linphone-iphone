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
    
    @ObservedObject var magicSearch = MagicSearchSingleton.shared
    
    @ObservedObject var contactViewModel: ContactViewModel
    @ObservedObject var favoriteContactsListViewModel: FavoriteContactsListViewModel
    
    @Binding var showingSheet: Bool
    
    var body: some View {
        ScrollView(.horizontal) {
            HStack {
                ForEach(0..<magicSearch.lastSearch.count, id: \.self) { index in
					if magicSearch.lastSearch[index].friend != nil && magicSearch.lastSearch[index].friend!.starred == true {
						Button {
						} label: {
							VStack {
								if magicSearch.lastSearch[index].friend!.photo != nil
									&& !magicSearch.lastSearch[index].friend!.photo!.isEmpty {
									AsyncImage(url: ContactsManager.shared.getImagePath(friendPhotoPath: magicSearch.lastSearch[index].friend!.photo!)
									) { image in
										switch image {
										case .empty:
											ProgressView()
												.frame(width: 45, height: 45)
										case .success(let image):
											image
												.resizable()
												.aspectRatio(contentMode: .fill)
												.frame(width: 45, height: 45)
												.clipShape(Circle())
										case .failure:
											Image("profil-picture-default")
												.resizable()
												.frame(width: 45, height: 45)
												.clipShape(Circle())
										@unknown default:
											EmptyView()
										}
									}
								} else {
									Image("profil-picture-default")
										.resizable()
										.frame(width: 45, height: 45)
										.clipShape(Circle())
								}
								Text((magicSearch.lastSearch[index].friend?.name)!)
									.default_text_style(styleSize: 16)
									.frame( maxWidth: .infinity, alignment: .center)
							}
						}
						.simultaneousGesture(
							LongPressGesture()
								.onEnded { _ in
									contactViewModel.selectedFriend = magicSearch.lastSearch[index].friend
									showingSheet.toggle()
								}
						)
						.highPriorityGesture(
							TapGesture()
								.onEnded { _ in
									withAnimation {
										contactViewModel.indexDisplayedFriend = index
									}
								}
						)
						.frame(minWidth: 70, maxWidth: 70)
					}
                }
            }
            .padding(.all, 10)
        }
    }
}

#Preview {
    FavoriteContactsListFragment(
        contactViewModel: ContactViewModel(),
        favoriteContactsListViewModel: FavoriteContactsListViewModel(),
        showingSheet: .constant(false))
}
