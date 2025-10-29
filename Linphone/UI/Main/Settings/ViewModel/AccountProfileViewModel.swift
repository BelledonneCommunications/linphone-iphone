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

class AccountProfileViewModel: ObservableObject {
	
	static let TAG = "[AccountProfileViewModel]"
	
	@Published var dialPlanValueSelected: String = ""
	var dialPlanSelected: DialPlan?
	
    @Published var accountModelIndex: Int? = 0
    @Published var defaultAccountModelIndex: Int? = 0
	@Published var accountError: Bool = false
	
	init() {
		SharedMainViewModel.shared.getDialPlansList()
	}
	
	func saveChangesWhenLeaving() {
		if accountModelIndex != nil {
			CoreContext.shared.doOnCoreQueue { _ in
				if self.accountModelIndex! < CoreContext.shared.accounts.count {
					let displayNameAccountModel = CoreContext.shared.accounts[self.accountModelIndex!].displayNameAvatar
					let newParams = CoreContext.shared.accounts[self.accountModelIndex!].account.params?.clone()
					if (displayNameAccountModel != newParams?.identityAddress?.displayName)
						&& (newParams?.identityAddress?.displayName != nil || !displayNameAccountModel.isEmpty) {
						if let newIdentityAddress = newParams?.identityAddress?.clone() {
							try? newIdentityAddress.setDisplayname(newValue: displayNameAccountModel)
							try? newParams?.setIdentityaddress(newValue: newIdentityAddress)
						}
						
						if self.getImagePath().lastPathComponent.contains("-default") || self.getImagePath().lastPathComponent == "Documents" {
							let usernameTmp = CoreContext.shared.accounts[self.accountModelIndex!].usernaneAvatar
							
							self.saveImage(
								image: ContactsManager.shared.textToImage(
									firstName: displayNameAccountModel.isEmpty ? usernameTmp : displayNameAccountModel, lastName: ""),
								name: usernameTmp,
								prefix: "-default")
						}
					}
					
					if self.dialPlanSelected != nil
						&& (self.dialPlanSelected!.countryCallingCode != newParams?.internationalPrefix || self.dialPlanSelected!.isoCountryCode != newParams?.internationalPrefixIsoCountryCode) {
						newParams?.internationalPrefix = self.dialPlanSelected?.countryCallingCode
						newParams?.internationalPrefixIsoCountryCode = self.dialPlanSelected?.isoCountryCode
						newParams?.useInternationalPrefixForCallsAndChats = true
					} else if self.dialPlanSelected == nil && newParams?.useInternationalPrefixForCallsAndChats == true {
						newParams?.internationalPrefix = nil
						newParams?.internationalPrefixIsoCountryCode = nil
						newParams?.useInternationalPrefixForCallsAndChats = false
					}
					
					CoreContext.shared.accounts[self.accountModelIndex!].account.params = newParams
				}
			}
		}
	}
	
	func setAvatarModel() {
		CoreContext.shared.doOnCoreQueue { _ in
			CoreContext.shared.accounts.forEach { accountTmp in
				let displayNameTmp = accountTmp.account.params?.identityAddress?.displayName ?? ""
				let contactAddressTmp = accountTmp.account.params?.identityAddress?.asStringUriOnly() ?? ""
				
				let prefix = accountTmp.account.params?.internationalPrefix ?? ""
				let isoCountryCode = accountTmp.account.params?.internationalPrefixIsoCountryCode ?? ""
				
				var dialPlanValueSelectedTmp = ""
				if !prefix.isEmpty || !isoCountryCode.isEmpty {
					Log.info(
						"\(AccountProfileViewModel.TAG) Account \(accountTmp.account.params?.identityAddress?.asStringUriOnly() ?? "") prefix is \(prefix) \(isoCountryCode)"
					)
					
					let dialPlansList = SharedMainViewModel.shared.dialPlansList
					if let dialPlan = dialPlansList.first(where: { $0?.isoCountryCode == isoCountryCode }) ??
						dialPlansList.first(where: { $0?.countryCallingCode == prefix }) {
						dialPlanValueSelectedTmp = "\(dialPlan?.flag ?? "") \(dialPlan?.country ?? "") | +\(dialPlan?.countryCallingCode ?? "")"
					} else {
						dialPlanValueSelectedTmp = "No country code"
					}
				} else {
					dialPlanValueSelectedTmp = "No country code"
				}
				
				self.updateDialPlan(newDialPlan: dialPlanValueSelectedTmp)
				
				let accountDisplayName = accountTmp.account.displayName()
				
				let defaultAccountModelIndexTmp = CoreContext.shared.accounts.firstIndex(where: {$0.isDefaultAccount})
				
				DispatchQueue.main.async {
					accountTmp.avatarModel = ContactAvatarModel(
						friend: nil,
						name: displayNameTmp.isEmpty ? accountDisplayName : displayNameTmp,
						address: contactAddressTmp,
						withPresence: false
					)
					
					self.defaultAccountModelIndex = defaultAccountModelIndexTmp
					
					self.dialPlanValueSelected = dialPlanValueSelectedTmp
				}
			}
		}
	}
	
	func updateDialPlan(newDialPlan: String) {
		let dialPlansList = SharedMainViewModel.shared.dialPlansList
		if let dialPlan = dialPlansList.first(where: { newDialPlan.contains($0?.isoCountryCode ?? "") }) ??
			dialPlansList.first(where: { newDialPlan.contains($0?.countryCallingCode ?? "") }) {
			self.dialPlanSelected = dialPlan
		} else {
			self.dialPlanSelected = nil
		}
	}
	
	func saveImage(image: UIImage, name: String, prefix: String) {
		guard let data = image.jpegData(compressionQuality: 1) ?? image.pngData() else {
			return
		}
		
		let photoAvatarModelKey = CoreContext.shared.accounts[self.accountModelIndex!].usernaneAvatar
		
		ContactsManager.shared.awaitDataWrite(data: data, name: name, prefix: prefix) { result in
			UserDefaults.standard.set(result, forKey: photoAvatarModelKey)
			
			DispatchQueue.main.async {
				CoreContext.shared.accounts[self.accountModelIndex ?? 0].photoAvatarModel = ""
				CoreContext.shared.accounts[self.accountModelIndex ?? 0].imagePathAvatar = nil
				NotificationCenter.default.post(name: NSNotification.Name("ImageChanged"), object: nil)
			}
			
			DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
				CoreContext.shared.accounts[self.accountModelIndex ?? 0].photoAvatarModel = result
				CoreContext.shared.accounts[self.accountModelIndex ?? 0].imagePathAvatar = CoreContext.shared.accounts[self.accountModelIndex ?? 0].getImagePath()
				NotificationCenter.default.post(name: NSNotification.Name("ImageChanged"), object: nil)
			}
		}
	}
	
	func getImagePath() -> URL {
		let imagePath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!.appendingPathComponent(
			CoreContext.shared.accounts[self.accountModelIndex ?? 0].photoAvatarModel ?? "Error"
		)
		
		return imagePath
	}
	
	func toggleRegister() {
		CoreContext.shared.doOnCoreQueue { _ in
			let account = CoreContext.shared.accounts[self.accountModelIndex ?? 0].account
			if let params = account.params {
				if let copy = params.clone() {
					copy.registerEnabled = !params.registerEnabled
					Log.info(
						"\(AccountProfileViewModel.TAG) Account registration is now \(copy.registerEnabled ? "enabled" : "disabled") for account \(params.identityAddress?.asStringUriOnly())"
					)
					account.params = copy
				}
			}
		}
	}
}
