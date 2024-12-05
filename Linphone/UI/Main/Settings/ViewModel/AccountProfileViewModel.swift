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
	
	init() {}
	
	func setAvatarModel() {
		CoreContext.shared.doOnCoreQueue { core in
			if core.defaultAccount != nil {
				let displayNameTmp = core.defaultAccount!.displayName()
				let contactAddressTmp = core.defaultAccount!.contactAddress?.asStringUriOnly() ?? ""
				var photoAvatarModelTmp = ""
				
				let preferences = UserDefaults.standard
				
				if preferences.object(forKey: self.photoAvatarModelKey) == nil {
					preferences.set(self.photoAvatarModel ?? "", forKey: self.photoAvatarModelKey)
				} else {
					photoAvatarModelTmp = preferences.string(forKey: self.photoAvatarModelKey)!
				}
				
				DispatchQueue.main.async {
					self.avatarModel = ContactAvatarModel(friend: nil, name: displayNameTmp, address: contactAddressTmp, withPresence: false)
					self.photoAvatarModel = photoAvatarModelTmp
				}
			}
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
