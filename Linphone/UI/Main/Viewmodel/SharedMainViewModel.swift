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

import linphonesw

class SharedMainViewModel: ObservableObject {
	
	static let shared = SharedMainViewModel()
	
	@Published var welcomeViewDisplayed = false
	@Published var generalTermsAccepted = false
	@Published var displayProfileMode = false
	
	let welcomeViewKey = "welcome_view"
	let generalTermsKey = "general_terms"
	let displayProfileModeKey = "display_profile_mode"
	
	var maxWidth = 400.0
	
	private init() {
		let preferences = UserDefaults.standard
		
		if preferences.object(forKey: welcomeViewKey) == nil {
			preferences.set(welcomeViewDisplayed, forKey: welcomeViewKey)
		} else {
			welcomeViewDisplayed = preferences.bool(forKey: welcomeViewKey)
		}
		
		if preferences.object(forKey: generalTermsKey) == nil {
			preferences.set(generalTermsAccepted, forKey: generalTermsKey)
		} else {
			generalTermsAccepted = preferences.bool(forKey: generalTermsKey)
		}
		
		if preferences.object(forKey: displayProfileModeKey) == nil {
			preferences.set(displayProfileMode, forKey: displayProfileModeKey)
		} else {
			displayProfileMode = preferences.bool(forKey: displayProfileModeKey)
		}
	}
	
	func changeWelcomeView() {
		let preferences = UserDefaults.standard
		
		welcomeViewDisplayed = true
		preferences.set(welcomeViewDisplayed, forKey: welcomeViewKey)
	}
	
	func changeGeneralTerms() {
		let preferences = UserDefaults.standard
		
		generalTermsAccepted = true
		preferences.set(generalTermsAccepted, forKey: generalTermsKey)
	}
	
	func changeDisplayProfileMode() {
		let preferences = UserDefaults.standard
		
		displayProfileMode = true
		preferences.set(displayProfileMode, forKey: displayProfileModeKey)
	}
	
	func changeHideProfileMode() {
		let preferences = UserDefaults.standard
		
		displayProfileMode = false
		preferences.set(displayProfileMode, forKey: displayProfileModeKey)
	}
}
