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

import linphonesw
import SwiftUI

class EditContactViewModel: ObservableObject {
	
	let selectedEditFriend: ContactAvatarModel?
	
	@Published var identifier: String = ""
	@Published var firstName: String = ""
	@Published var lastName: String = ""
	@Published var sipAddresses: [String] = []
	@Published var phoneNumbers: [String] = []
	@Published var company: String = ""
	@Published var jobTitle: String = ""
	
	init(contactAvatarModel: ContactAvatarModel? = nil) {
		self.selectedEditFriend = contactAvatarModel
		resetValues()
	}
	
	func resetValues() {
		CoreContext.shared.doOnCoreQueue { _ in
			let nativeUriTmp = self.selectedEditFriend == nil ? "" : self.selectedEditFriend!.nativeUri
			let givenNameTmp = (self.selectedEditFriend == nil ? "" : self.selectedEditFriend!.vcard?.givenName) ?? ""
			let familyNameTmp = (self.selectedEditFriend == nil ? "" : self.selectedEditFriend!.vcard?.familyName) ?? ""
			let organizationTmp = self.selectedEditFriend == nil ? "" : self.selectedEditFriend!.organization
			let jobTitleTmp = self.selectedEditFriend == nil ? "" : self.selectedEditFriend!.jobTitle
			
			var sipAddressesTmp: [String] = []
			var phoneNumbersTmp: [String] = []
			
			if self.selectedEditFriend != nil {
				self.selectedEditFriend?.addresses.forEach({ address in
					sipAddressesTmp.append(String(address.dropFirst(4)))
				})
				
				self.selectedEditFriend?.phoneNumbersWithLabel.forEach({ phoneNumber in
					phoneNumbersTmp.append(phoneNumber.phoneNumber)
				})
			}
			
			DispatchQueue.main.async {
				self.identifier = nativeUriTmp
				self.firstName = givenNameTmp
				self.lastName = familyNameTmp
				self.sipAddresses = []
				self.phoneNumbers = []
				self.company = organizationTmp
				self.jobTitle = jobTitleTmp
				
				self.sipAddresses = sipAddressesTmp
				self.phoneNumbers = phoneNumbersTmp
				
				self.sipAddresses.append("")
				self.phoneNumbers.append("")
			}
		}
	}
}
