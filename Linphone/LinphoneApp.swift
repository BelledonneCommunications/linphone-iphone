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

@main
struct LinphoneApp: App {
	
	@ObservedObject private var coreContext = CoreContext.shared
	@ObservedObject private var sharedMainViewModel = SharedMainViewModel.shared
	
	@State private var contactViewModel: ContactViewModel?
	@State private var editContactViewModel: EditContactViewModel?
	@State private var historyViewModel: HistoryViewModel?
	@State private var historyListViewModel: HistoryListViewModel?
	
	@State private var isActive = true
	
	var body: some Scene {
		WindowGroup {
			if isActive && coreContext.coreIsStarted {
				if !sharedMainViewModel.welcomeViewDisplayed {
					WelcomeView()
				} else if coreContext.defaultAccount == nil || sharedMainViewModel.displayProfileMode {
					AssistantView()
				} else if coreContext.defaultAccount != nil 
							&& contactViewModel != nil
							&& editContactViewModel != nil
							&& historyViewModel != nil
							&& historyListViewModel != nil {
					ContentView(
						contactViewModel: contactViewModel!,
						editContactViewModel: editContactViewModel!,
						historyViewModel: historyViewModel!,
						historyListViewModel: historyListViewModel!
					)
				}
			} else {
				SplashScreen(isActive: $isActive)
					.onDisappear {
						contactViewModel = ContactViewModel()
						editContactViewModel = EditContactViewModel()
						historyViewModel = HistoryViewModel()
						historyListViewModel = HistoryListViewModel()
					}
			}
		}
	}
}
