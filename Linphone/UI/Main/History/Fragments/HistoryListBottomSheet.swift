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
	
	@ObservedObject var contactsManager = ContactsManager.shared
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	
	@EnvironmentObject var historyListViewModel: HistoryListViewModel
	
	@State private var orientation = UIDevice.current.orientation
	
	@Binding var showingSheet: Bool
	@Binding var isShowEditContactFragment: Bool
	@Binding var isShowEditContactFragmentAddress: String
	
	var body: some View {
		VStack(alignment: .leading) {
			if idiom != .pad && (orientation == .landscapeLeft
								 || orientation == .landscapeRight
								 || UIScreen.main.bounds.size.width > UIScreen.main.bounds.size.height) {
				Spacer()
				HStack {
					Spacer()
					Button("dialog_close") {
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
				
				sharedMainViewModel.changeIndexView(indexViewInt: 0)
				
				if let selectedCall = historyListViewModel.selectedCall, selectedCall.isFriend {
					let friendIndex = contactsManager.avatarListModel.first(where: {$0.addresses.contains(where: {$0 == selectedCall.address})})
					if friendIndex != nil {
						withAnimation {
							SharedMainViewModel.shared.displayedFriend = friendIndex
						}
					}
				} else if let selectedCall = historyListViewModel.selectedCall {
					withAnimation {
						isShowEditContactFragment.toggle()
						isShowEditContactFragmentAddress = String(selectedCall.address.dropFirst(4))
					}
				}
			} label: {
				HStack {
					if let selectedCall = historyListViewModel.selectedCall, selectedCall.isFriend {
						Image("user-circle")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c500)
							.frame(width: 25, height: 25, alignment: .leading)
							.padding(.all, 10)
						Text("menu_see_existing_contact")
						.default_text_style(styleSize: 16)
						Spacer()
					} else {
						Image("plus-circle")
							.renderingMode(.template)
							.resizable()
							.foregroundStyle(Color.grayMain2c500)
							.frame(width: 25, height: 25, alignment: .leading)
							.padding(.all, 10)
						Text("menu_add_address_to_contacts")
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
				if historyListViewModel.selectedCall != nil && historyListViewModel.selectedCall!.isOutgoing {
					UIPasteboard.general.setValue(
						historyListViewModel.selectedCall!.address.dropFirst(4),
						forPasteboardType: UTType.plainText.identifier
					)
				} else {
					UIPasteboard.general.setValue(
						historyListViewModel.selectedCall!.address.dropFirst(4),
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
				
				ToastViewModel.shared.toastMessage = "Success_address_copied_into_clipboard"
				ToastViewModel.shared.displayToast.toggle()
				
			} label: {
				HStack {
					Image("copy")
						.renderingMode(.template)
						.resizable()
						.foregroundStyle(Color.grayMain2c500)
						.frame(width: 25, height: 25, alignment: .leading)
						.padding(.all, 10)
					Text("menu_copy_sip_address")
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
				if historyListViewModel.selectedCall != nil {
					historyListViewModel.removeCallLog(historyModel: historyListViewModel.selectedCall!)
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
						.padding(.all, 10)
					Text("menu_delete_selected_item")
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
		showingSheet: .constant(false),
		isShowEditContactFragment: .constant(false),
		isShowEditContactFragmentAddress: .constant("")
	)
}
