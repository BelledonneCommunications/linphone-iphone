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
import UniformTypeIdentifiers

struct HistoryListBottomSheet: View {
	
	@Environment(\.dismiss) var dismiss
	
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@ObservedObject var historyViewModel: HistoryViewModel
	@ObservedObject var contactViewModel: ContactViewModel
	@ObservedObject var editContactViewModel: EditContactViewModel
	@ObservedObject var historyListViewModel: HistoryListViewModel
	
	@State private var orientation = UIDevice.current.orientation
	
	@Binding var showingSheet: Bool
	@Binding var index: Int
	@Binding var isShowEditContactFragment: Bool
	
	var body: some View {
		VStack(alignment: .leading) {
			if idiom != .pad && (orientation == .landscapeLeft
								 || orientation == .landscapeRight
								 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
				Spacer()
				HStack {
					Spacer()
					Button("Close") {
						if #available(iOS 16.0, *) {
							showingSheet.toggle()
						} else {
							showingSheet.toggle()
							dismiss()
						}
					}
				}
				.padding(.trailing)
			}
			
			Spacer()
			Button {
				
				if #available(iOS 16.0, *) {
					if idiom != .pad {
						showingSheet.toggle()
					} else {
						showingSheet.toggle()
						dismiss()
					}
				} else {
					showingSheet.toggle()
					dismiss()
				}
				
				index = 0
				
				if ContactsManager.shared.getFriendWithAddress(
					address: historyViewModel.selectedCall != nil && historyViewModel.selectedCall!.dir == .Outgoing
					   ? historyViewModel.selectedCall!.toAddress!
					   : historyViewModel.selectedCall!.fromAddress!
				) != nil {
					let addressCall = historyViewModel.selectedCall != nil && historyViewModel.selectedCall!.dir == .Outgoing
					? historyViewModel.selectedCall!.toAddress!
					: historyViewModel.selectedCall!.fromAddress!
					
					let friendIndex = MagicSearchSingleton.shared.lastSearch.firstIndex(where: {$0.friend!.addresses.contains(where: {$0.asStringUriOnly() == addressCall.asStringUriOnly()})})
					if friendIndex != nil {
						withAnimation {
							contactViewModel.indexDisplayedFriend = friendIndex
						}
					}
				} else {
					let addressCall = historyViewModel.selectedCall != nil && historyViewModel.selectedCall!.dir == .Outgoing
					? historyViewModel.selectedCall!.toAddress!
					: historyViewModel.selectedCall!.fromAddress!
					
					withAnimation {
						isShowEditContactFragment.toggle()
						editContactViewModel.sipAddresses.removeAll()
						editContactViewModel.sipAddresses.append(String(addressCall.asStringUriOnly().dropFirst(4)))
						editContactViewModel.sipAddresses.append("")
					}
				}
			} label: {
				HStack {
					if ContactsManager.shared.getFriendWithAddress(
						address: historyViewModel.selectedCall != nil && historyViewModel.selectedCall!.dir == .Outgoing
						   ? historyViewModel.selectedCall!.toAddress!
						   : historyViewModel.selectedCall!.fromAddress!
					) != nil {
						Image("user-circle")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c500)
							.frame(width: 25, height: 25, alignment: .leading)
						Text("See contact")
						.default_text_style(styleSize: 16)
						Spacer()
					} else {
						Image("plus-circle")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c500)
							.frame(width: 25, height: 25, alignment: .leading)
						Text("Add the contact")
						.default_text_style(styleSize: 16)
						Spacer()
					}
				}
				.frame(maxHeight: .infinity)
			}
			.padding(.horizontal, 30)
			.background(Color.gray100)
			
			VStack {
				Divider()
			}
			.frame(maxWidth: .infinity)
			
			Button {
				if historyViewModel.selectedCall != nil && historyViewModel.selectedCall!.dir == .Outgoing {
					UIPasteboard.general.setValue(
						historyViewModel.selectedCall!.toAddress!.asStringUriOnly().dropFirst(4),
						forPasteboardType: UTType.plainText.identifier
					)
				} else {
					UIPasteboard.general.setValue(
						historyViewModel.selectedCall!.fromAddress!.asStringUriOnly().dropFirst(4),
						forPasteboardType: UTType.plainText.identifier
					)
				}
				
				if #available(iOS 16.0, *) {
					if idiom != .pad {
						showingSheet.toggle()
					} else {
						showingSheet.toggle()
						dismiss()
					}
				} else {
					showingSheet.toggle()
					dismiss()
				}
			} label: {
				HStack {
					Image("copy")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.grayMain2c500)
						.frame(width: 25, height: 25, alignment: .leading)
					Text("Copy SIP address")
						.default_text_style(styleSize: 16)
					Spacer()
				}
				.frame(maxHeight: .infinity)
			}
			.padding(.horizontal, 30)
			.background(Color.gray100)
			
			VStack {
				Divider()
			}
			.frame(maxWidth: .infinity)
			
			Button {
				CoreContext.shared.doOnCoreQueue { core in
					if historyViewModel.selectedCall != nil {
						core.removeCallLog(callLog: historyViewModel.selectedCall!)
						historyListViewModel.removeCallLog(callLog: historyViewModel.selectedCall!)
					}
				}
				
				if #available(iOS 16.0, *) {
					if idiom != .pad {
						showingSheet.toggle()
					} else {
						showingSheet.toggle()
						dismiss()
					}
				} else {
					showingSheet.toggle()
					dismiss()
				}
			} label: {
				HStack {
					Image("trash-simple")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.redDanger500)
						.frame(width: 25, height: 25, alignment: .leading)
					Text("Delete")
						.foregroundStyle(Color.redDanger500)
						.default_text_style(styleSize: 16)
					Spacer()
				}
				.frame(maxHeight: .infinity)
			}
			.padding(.horizontal, 30)
			.background(Color.gray100)
			
		}
		.background(Color.gray100)
		.frame(maxWidth: .infinity)
		.onRotate { newOrientation in
			orientation = newOrientation
		}
	}
}

#Preview {
	HistoryListBottomSheet(
		historyViewModel: HistoryViewModel(),
		contactViewModel: ContactViewModel(),
		editContactViewModel: EditContactViewModel(),
		historyListViewModel: HistoryListViewModel(),
		showingSheet: .constant(false),
		index: .constant(1),
		isShowEditContactFragment: .constant(false)
	)
}
