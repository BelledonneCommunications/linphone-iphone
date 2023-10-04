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

struct ContentView: View {
	
	@ObservedObject var sharedMainViewModel : SharedMainViewModel
	@ObservedObject private var coreContext = CoreContext.shared
	
	var body: some View {
		if UserDefaults.standard.bool(forKey: "general_terms") == false {
			WelcomeView(sharedMainViewModel: sharedMainViewModel)
        } else if coreContext.mCore.defaultAccount == nil || sharedMainViewModel.displayProfileMode {
            AssistantView(sharedMainViewModel: sharedMainViewModel)
        } else {
			TabView {
				ContactsView()
					.tabItem {
						Label("Contacts", image: "address-book")
					}
				
				HistoryView()
					.tabItem {
						Label("Calls", image: "phone")
					}
			}
		}
	}
}

#Preview {
    ContentView(sharedMainViewModel: SharedMainViewModel())
}
