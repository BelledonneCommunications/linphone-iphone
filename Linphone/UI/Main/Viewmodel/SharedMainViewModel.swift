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

class SharedMainViewModel : ObservableObject {
    
    @Published var displayProfileMode : Bool = false
    
	@Published var generalTermsAccepted = false
	
	init() {
		let preferences = UserDefaults.standard

		let generalTermsKey = "general_terms"
		
		if preferences.object(forKey: generalTermsKey) == nil {
			preferences.set(generalTermsAccepted, forKey: generalTermsKey)
		} else {
			generalTermsAccepted = preferences.bool(forKey: generalTermsKey)
		}
	}
	
	func changeGeneralTerms(){
		let preferences = UserDefaults.standard

		generalTermsAccepted = true
		let generalTermsKey = "general_terms"
		preferences.set(generalTermsAccepted, forKey: generalTermsKey)
	}
}
