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

struct HistoryFragment: View {
	private var idiom: UIUserInterfaceIdiom { UIDevice.current.userInterfaceIdiom }
	
	@ObservedObject var historyListViewModel: HistoryListViewModel
	@ObservedObject var historyViewModel: HistoryViewModel
	@ObservedObject var contactViewModel: ContactViewModel
	@ObservedObject var editContactViewModel: EditContactViewModel
	
	@State private var showingSheet = false
	@Binding var index: Int
	@Binding var isShowEditContactFragment: Bool
	@Binding var text: String
	
	var body: some View {
		ZStack {
			if #available(iOS 16.0, *), idiom != .pad {
				HistoryListFragment(historyListViewModel: historyListViewModel, historyViewModel: historyViewModel, showingSheet: $showingSheet, text: $text)
					.sheet(isPresented: $showingSheet) {
						HistoryListBottomSheet(
							historyViewModel: historyViewModel,
							contactViewModel: contactViewModel,
							editContactViewModel: editContactViewModel,
							historyListViewModel: historyListViewModel,
							showingSheet: $showingSheet,
							index: $index,
							isShowEditContactFragment: $isShowEditContactFragment
						)
						.presentationDetents([.fraction(0.2)])
					}
			} else {
				HistoryListFragment(historyListViewModel: historyListViewModel, historyViewModel: historyViewModel, showingSheet: $showingSheet, text: $text)
					.halfSheet(showSheet: $showingSheet) {
						HistoryListBottomSheet(
							historyViewModel: historyViewModel,
							contactViewModel: contactViewModel,
							editContactViewModel: editContactViewModel,
							historyListViewModel: historyListViewModel,
							showingSheet: $showingSheet,
							index: $index,
							isShowEditContactFragment: $isShowEditContactFragment
						)
					} onDismiss: {}
			}
		}
	}
}

#Preview {
	HistoryFragment(
		historyListViewModel: HistoryListViewModel(),
		historyViewModel: HistoryViewModel(),
		contactViewModel: ContactViewModel(),
		editContactViewModel: EditContactViewModel(),
		index: .constant(1),
		isShowEditContactFragment: .constant(false),
		text: .constant("")
	)
}
