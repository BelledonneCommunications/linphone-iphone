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

struct HistoryListFragment: View {
	
	@ObservedObject var historyListViewModel: HistoryListViewModel
	@ObservedObject var historyViewModel: HistoryViewModel
	
	@Binding var showingSheet: Bool
	
	var body: some View {
		VStack {
			List {
				ForEach(0..<historyListViewModel.callLogs.count, id: \.self) { index in
					Button {
					} label: {
						HStack {
							let fromAddressFriend = ContactsManager.shared.getFriendWithAddress(address: historyListViewModel.callLogs[index].fromAddress!)
							let toAddressFriend = ContactsManager.shared.getFriendWithAddress(address: historyListViewModel.callLogs[index].toAddress!)
							
							if historyListViewModel.callLogs[index].dir == .Incoming && fromAddressFriend != nil && fromAddressFriend!.photo != nil && !fromAddressFriend!.photo!.isEmpty {
								AsyncImage(url:
											ContactsManager.shared.getImagePath(
												friendPhotoPath: fromAddressFriend!.photo!)) { image in
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
							} else if historyListViewModel.callLogs[index].dir == .Outgoing && toAddressFriend != nil && toAddressFriend!.photo != nil && !toAddressFriend!.photo!.isEmpty {
								AsyncImage(url:
											ContactsManager.shared.getImagePath(
												friendPhotoPath: toAddressFriend!.photo!)) { image in
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
								if historyListViewModel.callLogs[index].dir == .Outgoing && historyListViewModel.callLogs[index].toAddress != nil {
									if historyListViewModel.callLogs[index].toAddress!.displayName != nil {
										Image(uiImage: ContactsManager.shared.textToImage(
										 firstName: historyListViewModel.callLogs[index].toAddress!.displayName!,
										 lastName: historyListViewModel.callLogs[index].toAddress!.displayName!.components(separatedBy: " ").count > 1
											? historyListViewModel.callLogs[index].toAddress!.displayName!.components(separatedBy: " ")[1]
											: ""))
										 .resizable()
										 .frame(width: 45, height: 45)
										 .clipShape(Circle())
										
									} else {
										Image(uiImage: ContactsManager.shared.textToImage(
											firstName: historyListViewModel.callLogs[index].toAddress!.username ?? "Username Error",
											lastName: historyListViewModel.callLogs[index].toAddress!.username!.components(separatedBy: " ").count > 1
											? historyListViewModel.callLogs[index].toAddress!.username!.components(separatedBy: " ")[1]
											: ""))
										 .resizable()
										 .frame(width: 45, height: 45)
										 .clipShape(Circle())
									}
									
								} else if historyListViewModel.callLogs[index].fromAddress != nil {
									if historyListViewModel.callLogs[index].fromAddress!.displayName != nil {
										Image(uiImage: ContactsManager.shared.textToImage(
											firstName: historyListViewModel.callLogs[index].fromAddress!.displayName!,
											lastName: historyListViewModel.callLogs[index].fromAddress!.displayName!.components(separatedBy: " ").count > 1 
											? historyListViewModel.callLogs[index].fromAddress!.displayName!.components(separatedBy: " ")[1]
											: ""))
											.resizable()
											.frame(width: 45, height: 45)
											.clipShape(Circle())
									} else {
										Image(uiImage: ContactsManager.shared.textToImage(
											firstName: historyListViewModel.callLogs[index].fromAddress!.username ?? "Username Error",
											lastName: historyListViewModel.callLogs[index].fromAddress!.username!.components(separatedBy: " ").count > 1 
											? historyListViewModel.callLogs[index].fromAddress!.username!.components(separatedBy: " ")[1]
											: ""))
											.resizable()
											.frame(width: 45, height: 45)
											.clipShape(Circle())
									}
								}
							}
							
							VStack(spacing: 0) {
								Spacer()
								
								let fromAddressFriend = ContactsManager.shared.getFriendWithAddress(address: historyListViewModel.callLogs[index].fromAddress!)
								let toAddressFriend = ContactsManager.shared.getFriendWithAddress(address: historyListViewModel.callLogs[index].toAddress!)
								
								if historyListViewModel.callLogs[index].dir == .Incoming && fromAddressFriend != nil {
									Text(fromAddressFriend!.name!)
									 .default_text_style(styleSize: 14)
									 .frame(maxWidth: .infinity, alignment: .leading)
									 .lineLimit(1)
								} else if historyListViewModel.callLogs[index].dir == .Outgoing && toAddressFriend != nil {
									Text(toAddressFriend!.name!)
									 .default_text_style(styleSize: 14)
									 .frame(maxWidth: .infinity, alignment: .leading)
									 .lineLimit(1)
								} else {
									if historyListViewModel.callLogs[index].dir == .Outgoing && historyListViewModel.callLogs[index].toAddress != nil {
										Text(historyListViewModel.callLogs[index].toAddress!.displayName != nil
											 ? historyListViewModel.callLogs[index].toAddress!.displayName!
											 : historyListViewModel.callLogs[index].toAddress!.username!)
											.default_text_style(styleSize: 14)
											.frame(maxWidth: .infinity, alignment: .leading)
											.lineLimit(1)
									} else if historyListViewModel.callLogs[index].fromAddress != nil {
										Text(historyListViewModel.callLogs[index].fromAddress!.displayName != nil
											 ? historyListViewModel.callLogs[index].fromAddress!.displayName!
											 : historyListViewModel.callLogs[index].fromAddress!.username!)
										 .default_text_style(styleSize: 14)
										 .frame(maxWidth: .infinity, alignment: .leading)
										 .lineLimit(1)
									}
								}
								HStack {
									Image(historyListViewModel.getCallIconResId(callStatus: historyListViewModel.callLogs[index].status, callDir: historyListViewModel.callLogs[index].dir))
									 .resizable()
									 .frame(
										width: historyListViewModel.getCallIconResId(callStatus: historyListViewModel.callLogs[index].status, callDir: historyListViewModel.callLogs[index].dir).contains("rejected") ? 12 : 8,
										height: historyListViewModel.getCallIconResId(callStatus: historyListViewModel.callLogs[index].status, callDir: historyListViewModel.callLogs[index].dir).contains("rejected") ? 6 : 8)
									
									Text(historyListViewModel.getCallTime(startDate: historyListViewModel.callLogs[index].startDate))
									 .default_text_style_300(styleSize: 12)
									 .frame(maxWidth: .infinity, alignment: .leading)
									
									Spacer()
								}
								
								Spacer()
							}
							
							Image("phone")
								.resizable()
								.frame(width: 25, height: 25)
								.padding(.trailing, 5)
						}
					}
					.buttonStyle(.borderless)
					.listRowInsets(EdgeInsets(top: 5, leading: 20, bottom: 5, trailing: 20))
					.listRowSeparator(.hidden)
					.simultaneousGesture(
						LongPressGesture()
							.onEnded { _ in
								historyViewModel.selectedCall = historyListViewModel.callLogs[index]
								showingSheet.toggle()
							}
					)
					.highPriorityGesture(
						TapGesture()
							.onEnded { _ in
								withAnimation {
									//historyViewModel.indexDisplayedCall = index
								}
							}
					)
				}
			}
			.listStyle(.plain)
			.overlay(
				VStack {
					if historyListViewModel.callLogs.isEmpty {
						Spacer()
						Image("illus-belledonne")
							.resizable()
							.scaledToFit()
							.clipped()
							.padding(.all)
						Text("No call for the moment...")
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
	HistoryListFragment(historyListViewModel: HistoryListViewModel(), historyViewModel: HistoryViewModel(), showingSheet: .constant(false))
}
