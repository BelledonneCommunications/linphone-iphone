/*
 * Copyright (c) 2010-2023 Belledonne Communications SARL.
 *
 * This file is part of Linphone
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

struct HistoryView: View {
	
	@ObservedObject var historyListViewModel: HistoryListViewModel
	@ObservedObject var historyViewModel: HistoryViewModel
	@ObservedObject var contactViewModel: ContactViewModel
	@ObservedObject var editContactViewModel: EditContactViewModel
	
	@Binding var index: Int
	@Binding var isShowStartCallFragment: Bool
	@Binding var isShowEditContactFragment: Bool
	@Binding var text: String
	
	var body: some View {
		NavigationView {
			ZStack(alignment: .bottomTrailing) {
				HistoryFragment(
					historyListViewModel: historyListViewModel,
					historyViewModel: historyViewModel,
					contactViewModel: contactViewModel,
					editContactViewModel: editContactViewModel,
					index: $index,
					isShowEditContactFragment: $isShowEditContactFragment,
					text: $text
				)
				
				Button {
					withAnimation {
						isShowStartCallFragment.toggle()
					}
					
					DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
						MagicSearchSingleton.shared.searchForSuggestions()
					}
				} label: {
					Image("phone-plus")
						.renderingMode(.template)
						.foregroundStyle(.white)
						.padding()
						.background(Color.orangeMain500)
						.clipShape(Circle())
						.shadow(color: .black.opacity(0.2), radius: 4)
					
				}
				.padding()
			}
		}
		.navigationViewStyle(.stack)
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
