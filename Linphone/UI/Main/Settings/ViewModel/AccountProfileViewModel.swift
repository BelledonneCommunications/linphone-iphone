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
	
	@Published var dialPlanValueSelected: String = "🇫🇷 France | +33"
	var dialPlanSelected: DialPlan?
	var dialPlansList: [DialPlan] = []
	
	var accountModelIndex: Int? = 0
	
	init() {}
	
	func saveChangesWhenLeaving() {
		if accountModelIndex != nil {
			CoreContext.shared.doOnCoreQueue { _ in
				let displayNameAccountModel = CoreContext.shared.accounts[self.accountModelIndex!].displayNameAvatar
				let newParams = CoreContext.shared.accounts[self.accountModelIndex!].account.params?.clone()
				if (displayNameAccountModel != newParams?.identityAddress?.displayName)
					&& (newParams?.identityAddress?.displayName != nil || !displayNameAccountModel.isEmpty) {
					if let newIdentityAddress = newParams?.identityAddress?.clone() {
						try? newIdentityAddress.setDisplayname(newValue: displayNameAccountModel)
						try? newParams?.setIdentityaddress(newValue: newIdentityAddress)
					}
					
					if self.getImagePath().lastPathComponent.contains("-default") || self.getImagePath().lastPathComponent == "Documents" {
						DispatchQueue.main.async {
							self.saveImage(
								image: ContactsManager.shared.textToImage(
									firstName: displayNameAccountModel.isEmpty ? CoreContext.shared.accounts[self.accountModelIndex!].account.displayName() : displayNameAccountModel, lastName: ""),
								name: displayNameAccountModel.isEmpty ? CoreContext.shared.accounts[self.accountModelIndex!].account.displayName() : displayNameAccountModel,
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
				
				CoreContext.shared.accounts[self.accountModelIndex!].account.params = newParams
			}
		}
	}
	 
	func setAvatarModel() {
		if accountModelIndex != nil {
			CoreContext.shared.doOnCoreQueue { _ in
				let photoAvatarAccountModel = CoreContext.shared.accounts[self.accountModelIndex!].photoAvatarModel
				let displayNameTmp = CoreContext.shared.accounts[self.accountModelIndex!].account.params?.identityAddress?.displayName ?? ""
				let contactAddressTmp = CoreContext.shared.accounts[self.accountModelIndex!].account.params?.identityAddress?.asStringUriOnly() ?? ""
				var photoAvatarModelTmp = ""
				
				let prefix = CoreContext.shared.accounts[self.accountModelIndex!].account.params?.internationalPrefix ?? ""
				let isoCountryCode = CoreContext.shared.accounts[self.accountModelIndex!].account.params?.internationalPrefixIsoCountryCode ?? ""
				
				var dialPlanValueSelectedTmp = ""
				if !prefix.isEmpty || !isoCountryCode.isEmpty {
					Log.info(
						"$TAG Account \(CoreContext.shared.accounts[self.accountModelIndex!].account.params?.identityAddress?.asStringUriOnly() ?? "") prefix is \(prefix) \(isoCountryCode)"
					)
					
					self.dialPlansList = Factory.Instance.dialPlans
					if let dialPlan = self.dialPlansList.first(where: { $0.isoCountryCode == isoCountryCode }) ??
						self.dialPlansList.first(where: { $0.countryCallingCode == prefix }) {
						dialPlanValueSelectedTmp = "\(dialPlan.flag) \(dialPlan.country) | +\(dialPlan.countryCallingCode)"
					}
				}
				
				let preferences = UserDefaults.standard
				
				let accountDisplayName = CoreContext.shared.accounts[self.accountModelIndex!].account.displayName()
				
				let photoAvatarModelKey = "photo_avatar_model" + CoreContext.shared.accounts[self.accountModelIndex!].address
				if preferences.object(forKey: photoAvatarModelKey) == nil {
					preferences.set(photoAvatarAccountModel ?? "", forKey: photoAvatarModelKey)
				} else {
					photoAvatarModelTmp = preferences.string(forKey: photoAvatarModelKey)!
				}
				
				DispatchQueue.main.async {
					CoreContext.shared.accounts[self.accountModelIndex!].avatarModel = ContactAvatarModel(
						friend: nil,
						name: displayNameTmp.isEmpty ? accountDisplayName : displayNameTmp,
						address: contactAddressTmp,
						withPresence: false
					)
					
					CoreContext.shared.accounts[self.accountModelIndex!].photoAvatarModel = photoAvatarModelTmp
					CoreContext.shared.accounts[self.accountModelIndex!].displayNameAvatar = displayNameTmp
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
		
		let photoAvatarModelKey = "photo_avatar_model" + CoreContext.shared.accounts[self.accountModelIndex ?? 0].address
		
		ContactsManager.shared.awaitDataWrite(data: data, name: name, prefix: prefix) { _, result in
			UserDefaults.standard.set(result, forKey: photoAvatarModelKey)
			CoreContext.shared.accounts[self.accountModelIndex ?? 0].photoAvatarModel = result
		}
	}
	
	func getImagePath() -> URL {
		let imagePath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!.appendingPathComponent(
			CoreContext.shared.accounts[self.accountModelIndex ?? 0].photoAvatarModel ?? "Error"
		)
		
		return imagePath
	}
}
