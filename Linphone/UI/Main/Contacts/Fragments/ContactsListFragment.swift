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
    
    @ObservedObject var magicSearch = MagicSearchSingleton.shared
    
    @ObservedObject var contactViewModel: ContactViewModel
    @ObservedObject var contactsListViewModel: ContactsListViewModel
    
    @Binding var showingSheet: Bool
    
    var body: some View {
        VStack {
            List {
                ForEach(0..<magicSearch.lastSearch.count, id: \.self) { index in
                    Button {
                    } label: {
                        HStack {
                            if index == 0 
                                || magicSearch.lastSearch[index].friend?.name!.lowercased().folding(
                                    options: .diacriticInsensitive,
                                    locale: .current
                                ).first
                                != magicSearch.lastSearch[index-1].friend?.name!.lowercased().folding(
                                    options: .diacriticInsensitive,
                                    locale: .current
                                ).first {
                                Text(
                                    String(
                                        (magicSearch.lastSearch[index].friend?.name!.uppercased().folding(
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
                            
                            if magicSearch.lastSearch[index].friend!.photo != nil && !magicSearch.lastSearch[index].friend!.photo!.isEmpty {
								AsyncImage(url: ContactsManager.shared.getImagePath(friendPhotoPath: magicSearch.lastSearch[index].friend!.photo!)) { image in
                                    switch image {
                                    case .empty:
                                        ProgressView()
                                            .frame(width: 45, height: 45)
                                    case .success(let image):
                                        image
                                            .resizable()
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
                                .frame(maxWidth: .infinity, alignment: .leading)
                                .foregroundStyle(Color.orangeMain500)
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
                    .buttonStyle(.borderless)
                    .listRowSeparator(.hidden)
                }
            }
            .listStyle(.plain)
            .overlay(
                VStack {
                    if magicSearch.lastSearch.isEmpty {
                        Spacer()
                        Image("illus-belledonne")
                            .resizable()
                            .scaledToFit()
                            .clipped()
                            .padding(.all)
                        Text("No contacts for the moment...")
                            .default_text_style_800(styleSize: 16)
                        Spacer()
                        Spacer()
                    }
                }
                    .padding(.all)
            )
        }
    }
}

#Preview {
    ContactsListFragment(contactViewModel: ContactViewModel(), contactsListViewModel: ContactsListViewModel(), showingSheet: .constant(false))
}
