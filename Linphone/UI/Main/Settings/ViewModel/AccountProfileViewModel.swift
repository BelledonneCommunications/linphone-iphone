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

class AccountProfileViewModel: ObservableObject {
	
	let photoAvatarModelKey = "photo_avatar_model"
	
	@Published var avatarModel: ContactAvatarModel?
	@Published var photoAvatarModel: String?
	@Published var displayName: String = ""
	
	@Published var dialPlanValueSelected: String = "🇫🇷 France | +33"
	var dialPlanSelected: DialPlan?
	var dialPlansList: [DialPlan] = []
	
	init() {}
	
	func saveChangesWhenLeaving() {
		CoreContext.shared.doOnCoreQueue { core in
			let newParams = core.defaultAccount!.params?.clone()
			if (self.displayName != newParams?.identityAddress?.displayName)
				&& (newParams?.identityAddress?.displayName != nil || !self.displayName.isEmpty) {
				if let newIdentityAddress = newParams?.identityAddress?.clone() {
					try? newIdentityAddress.setDisplayname(newValue: self.displayName)
					try? newParams?.setIdentityaddress(newValue: newIdentityAddress)
				}
				
				if self.getImagePath().lastPathComponent.contains("-default") || self.getImagePath().lastPathComponent == "Documents" {
					DispatchQueue.main.async {
						self.saveImage(
							image: ContactsManager.shared.textToImage(
								firstName: self.displayName.isEmpty ? core.defaultAccount!.displayName() : self.displayName, lastName: ""),
							name: self.displayName.isEmpty ? core.defaultAccount!.displayName() : self.displayName,
							prefix: "-default")
					}
				}
			}
			
			if self.dialPlanSelected != nil
				&& (self.dialPlanSelected!.countryCallingCode != newParams?.internationalPrefix || self.dialPlanSelected!.isoCountryCode != newParams?.internationalPrefixIsoCountryCode) {
				newParams?.internationalPrefix = self.dialPlanSelected?.countryCallingCode
				newParams?.internationalPrefixIsoCountryCode = self.dialPlanSelected?.isoCountryCode
				newParams?.useInternationalPrefixForCallsAndChats = true
			}
			
			core.defaultAccount!.params = newParams
		}
	}
	 
	func setAvatarModel() {
		CoreContext.shared.doOnCoreQueue { core in
			if core.defaultAccount != nil {
				let displayNameTmp = core.defaultAccount!.params?.identityAddress?.displayName ?? ""
				let contactAddressTmp = core.defaultAccount!.params?.identityAddress?.asStringUriOnly() ?? ""
				var photoAvatarModelTmp = ""
				
				let prefix = core.defaultAccount!.params?.internationalPrefix ?? ""
				let isoCountryCode = core.defaultAccount!.params?.internationalPrefixIsoCountryCode ?? ""
				
				var dialPlanValueSelectedTmp = ""
				if !prefix.isEmpty || !isoCountryCode.isEmpty {
					Log.info(
						"$TAG Account \(core.defaultAccount!.params?.identityAddress?.asStringUriOnly() ?? "") prefix is \(prefix) \(isoCountryCode)"
					)
					
					self.dialPlansList = Factory.Instance.dialPlans
					if let dialPlan = self.dialPlansList.first(where: { $0.isoCountryCode == isoCountryCode }) ??
						self.dialPlansList.first(where: { $0.countryCallingCode == prefix }) {
						dialPlanValueSelectedTmp = "\(dialPlan.flag) \(dialPlan.country) | +\(dialPlan.countryCallingCode)"
					}
				}
				
				let preferences = UserDefaults.standard
				
				if preferences.object(forKey: self.photoAvatarModelKey) == nil {
					preferences.set(self.photoAvatarModel ?? "", forKey: self.photoAvatarModelKey)
				} else {
					photoAvatarModelTmp = preferences.string(forKey: self.photoAvatarModelKey)!
				}
				
				DispatchQueue.main.async {
					self.avatarModel = ContactAvatarModel(friend: nil, name: displayNameTmp.isEmpty ? core.defaultAccount!.displayName() : displayNameTmp, address: contactAddressTmp, withPresence: false)
					self.photoAvatarModel = photoAvatarModelTmp
					self.displayName = displayNameTmp
					self.dialPlanValueSelected = dialPlanValueSelectedTmp
				}
			}
		}
	}
	
	func updateDialPlan(newDialPlan: String) {
		if let dialPlan = self.dialPlansList.first(where: { newDialPlan.contains($0.isoCountryCode) }) ??
			self.dialPlansList.first(where: { newDialPlan.contains($0.countryCallingCode) }) {
			self.dialPlanSelected = dialPlan
		}
	}
	
	func saveImage(image: UIImage, name: String, prefix: String) {
		guard let data = image.jpegData(compressionQuality: 1) ?? image.pngData() else {
			return
		}
		
		ContactsManager.shared.awaitDataWrite(data: data, name: name, prefix: prefix) { _, result in
			UserDefaults.standard.set(result, forKey: self.photoAvatarModelKey)
			self.photoAvatarModel = result
		}
	}
	
	func getImagePath() -> URL {
		let imagePath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!.appendingPathComponent(self.photoAvatarModel ?? "Error")
		
		return imagePath
	}
}
