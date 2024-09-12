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

// swiftlint:disable line_length

import SwiftUI
import linphonesw

struct HistoryListFragment: View {
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject private var telecomManager = TelecomManager.shared
	
	@ObservedObject var historyListViewModel: HistoryListViewModel
	@ObservedObject var historyViewModel: HistoryViewModel
	
	@Binding var showingSheet: Bool
	@Binding var text: String
	
	var body: some View {
		VStack {
			List {
				ForEach(0..<historyListViewModel.callLogs.count, id: \.self) { index in
					HStack {
						HStack {
							if !historyListViewModel.callLogs[index].isConf {
								if historyListViewModel.callLogs[index].avatarModel != nil {
									Avatar(contactAvatarModel: historyListViewModel.callLogs[index].avatarModel!, avatarSize: 50)
								} else {
									if !historyListViewModel.callLogs[index].addressName.isEmpty {
										Image(uiImage: contactsManager.textToImage(
											firstName: historyListViewModel.callLogs[index].addressName,
											lastName: historyListViewModel.callLogs[index].addressName.components(separatedBy: " ").count > 1
											? historyListViewModel.callLogs[index].addressName.components(separatedBy: " ")[1]
											: ""))
										.resizable()
										.frame(width: 50, height: 50)
										.clipShape(Circle())
									} else {
										VStack {
											Image("profil-picture-default")
												.renderingMode(.template)
												.resizable()
												.frame(width: 28, height: 28)
												.foregroundStyle(Color.grayMain2c600)
										}
										.frame(width: 50, height: 50)
										.background(Color.grayMain2c200)
										.clipShape(Circle())
									}
								}
							} else {
								VStack {
									Image("users-three-square")
										.renderingMode(.template)
										.resizable()
										.frame(width: 28, height: 28)
										.foregroundStyle(Color.grayMain2c600)
								}
								.frame(width: 50, height: 50)
								.background(Color.grayMain2c200)
								.clipShape(Circle())
							}
							
							VStack(spacing: 0) {
								Spacer()
								if !historyListViewModel.callLogs[index].isConf {
									Text(historyListViewModel.callLogs[index].addressName)
										.default_text_style(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
										.lineLimit(1)
								} else {
									Text(historyListViewModel.callLogs[index].subject)
										.default_text_style(styleSize: 14)
										.frame(maxWidth: .infinity, alignment: .leading)
										.lineLimit(1)
								}
								
								HStack {
									Image(historyListViewModel.getCallIconResId(callStatus: historyListViewModel.callLogs[index].status, isOutgoing: historyListViewModel.callLogs[index].isOutgoing))
										.resizable()
										.frame(
											width: historyListViewModel.getCallIconResId(callStatus: historyListViewModel.callLogs[index].status, isOutgoing: historyListViewModel.callLogs[index].isOutgoing).contains("rejected") ? 12 : 8,
											height: historyListViewModel.getCallIconResId(callStatus: historyListViewModel.callLogs[index].status, isOutgoing: historyListViewModel.callLogs[index].isOutgoing).contains("rejected") ? 6 : 8)
									Text(historyListViewModel.getCallTime(startDate: historyListViewModel.callLogs[index].startDate))
										.default_text_style_300(styleSize: 12)
										.frame(maxWidth: .infinity, alignment: .leading)
									
									Spacer()
								}
								
								Spacer()
							}
							
							if !historyListViewModel.callLogs[index].isConf {
								Image("phone")
									.resizable()
									.frame(width: 25, height: 25)
									.padding(.all, 10)
									.padding(.trailing, 5)
									.highPriorityGesture(
										TapGesture()
											.onEnded { _ in
												withAnimation {
													doCall(index: index)
													historyViewModel.displayedCall = nil
												}
											}
									)
							}
						}
					}
					.frame(height: 50)
					.buttonStyle(.borderless)
					.listRowInsets(EdgeInsets(top: 6, leading: 20, bottom: 6, trailing: 20))
					.listRowSeparator(.hidden)
					.background(.white)
					.onTapGesture {
						withAnimation {
							historyViewModel.displayedCall = historyListViewModel.callLogs[index]
						}
					}
					.onLongPressGesture(minimumDuration: 0.2) {
						historyViewModel.selectedCall = historyListViewModel.callLogs[index]
						showingSheet.toggle()
					}
				}
			}
			.safeAreaInset(edge: .top, content: {
				Spacer()
					.frame(height: 12)
			})
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
						Text(!text.isEmpty ? "list_filter_no_result_found" : "history_list_empty_history")
							.default_text_style_800(styleSize: 16)
							.multilineTextAlignment(.center)
						Spacer()
						Spacer()
					}
				}
					.padding(.all)
			)
		}
		.navigationTitle("")
		.navigationBarHidden(true)
	}
	
	func doCall(index: Int) {
		telecomManager.doCallOrJoinConf(address: historyListViewModel.callLogs[index].addressLinphone)
	}
}

#Preview {
	HistoryListFragment(historyListViewModel: HistoryListViewModel(), historyViewModel: HistoryViewModel(), showingSheet: .constant(false), text: .constant(""))
}

// swiftlint:enable line_length
