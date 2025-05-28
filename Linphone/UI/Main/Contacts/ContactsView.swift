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

struct ContactsView: View {
	
	@EnvironmentObject var contactsListViewModel: ContactsListViewModel
	
	@Binding var isShowEditContactFragment: Bool
	@Binding var isShowDeletePopup: Bool
	@Binding var text: String
	
	var body: some View {
		NavigationView {
			ZStack(alignment: .bottomTrailing) {
				ContactsFragment(isShowDeletePopup: $isShowDeletePopup, text: $text)
				
				Button {
					withAnimation {
						contactsListViewModel.selectedEditFriend = nil
						isShowEditContactFragment.toggle()
					}
				} label: {
					Image("user-plus")
						.renderingMode(.template)
						.foregroundStyle(.white)
						.padding()
						.background(Color.orangeMain500)
						.clipShape(Circle())
						.shadow(color: .black.opacity(0.2), radius: 4)
					
				}
				.padding()
				
				// For testing crashlytics
				/*Button(action: CoreContext.shared.crashForCrashlytics, label: {
					Text("CRASH ME")
				})*/
			}
		}
		.navigationViewStyle(.stack)
	}
}

#Preview {
	ContactsView(
		isShowEditContactFragment: .constant(false),
		isShowDeletePopup: .constant(false),
		text: .constant("")
	)
}
