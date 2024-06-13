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

import Foundation
import linphonesw

class RegisterViewModel: ObservableObject {
	
	private var coreContext = CoreContext.shared
	
	@Published var username: String = ""
	@Published var phoneNumber: String = ""
	@Published var passwd: String = ""
	@Published var domain: String = "sip.linphone.org"
	@Published var displayName: String = ""
	@Published var transportType: String = "TLS"
	
	@Published var dialPlanSelected: String = "🇫🇷 +33"
	@Published var dialPlansList: [DialPlan] = []
	@Published var dialPlansLabelList: [String] = []
	@Published var dialPlansShortLabelList: [String] = []
	
	init() {
		getDialPlansList()
	}
	
	func getDialPlansList() {
		coreContext.doOnCoreQueue { core in
			let dialPlans = Factory.Instance.dialPlans
			dialPlans.forEach { dialPlan in
				self.dialPlansList.append(dialPlan)
				self.dialPlansLabelList.append(
					"\(dialPlan.flag) \(dialPlan.country) | +\(dialPlan.countryCallingCode)"
				)
				self.dialPlansShortLabelList.append(
					"\(dialPlan.flag) +\(dialPlan.countryCallingCode)"
				)
			}
		}
	}
}
