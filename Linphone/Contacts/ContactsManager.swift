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
import Contacts
import SwiftUI

final class ContactsManager: ObservableObject {
	
	static let shared = ContactsManager()
	
	private var coreContext = CoreContext.shared
	private var magicSearch = MagicSearchSingleton.shared
	
	private let nativeAddressBookFriendList = "Native address-book"
	let linphoneAddressBookFirendList = "Linphone address-book"
	
	@Published var friendList: FriendList?
	@Published var linphoneFriendList: FriendList?
	
	private init() {
		fetchContacts()
	}
	
	func fetchContacts() {
		DispatchQueue.global().async {
			if self.coreContext.mCore.globalState == GlobalState.Shutdown || self.coreContext.mCore.globalState == GlobalState.Off {
				print("$TAG Core is being stopped or already destroyed, abort")
			} else {
				print("$TAG ${friends.size} friends created")
				
				self.friendList = self.coreContext.mCore.getFriendListByName(name: self.nativeAddressBookFriendList)
				if self.friendList == nil {
					do {
						self.friendList = try self.coreContext.mCore.createFriendList()
					} catch let error {
						print("Failed to enumerate contact", error)
					}
				}
				
				if self.friendList!.displayName == nil || self.friendList!.displayName!.isEmpty {
					print(
						"$TAG Friend list [$nativeAddressBookFriendList] didn't exist yet, let's create it"
					)
					
					self.friendList!.databaseStorageEnabled = false // We don't want to store local address-book in DB
					
					self.friendList!.displayName = self.nativeAddressBookFriendList
					self.coreContext.mCore.addFriendList(list: self.friendList!)
				} else {
					print(
						"$TAG Friend list [$LINPHONE_ADDRESS_BOOK_FRIEND_LIST] found, removing existing friends if any"
					)
					self.friendList!.friends.forEach { friend in
						_ = self.friendList!.removeFriend(linphoneFriend: friend)
					}
				}
				
				self.linphoneFriendList = self.coreContext.mCore.getFriendListByName(name: self.linphoneAddressBookFirendList)
				if self.linphoneFriendList == nil {
					do {
						self.linphoneFriendList = try self.coreContext.mCore.createFriendList()
					} catch let error {
						print("Failed to enumerate contact", error)
					}
				}
				
				if self.linphoneFriendList!.displayName == nil || self.linphoneFriendList!.displayName!.isEmpty {
					print(
						"$TAG Friend list [$linphoneAddressBookFirendList] didn't exist yet, let's create it"
					)
					
					self.linphoneFriendList!.databaseStorageEnabled = true
					
					self.linphoneFriendList!.displayName = self.linphoneAddressBookFirendList
					self.coreContext.mCore.addFriendList(list: self.linphoneFriendList!)
				}
			}
			
			let store = CNContactStore()
			store.requestAccess(for: .contacts) { (granted, error) in
				if let error = error {
					print("failed to request access", error)
					return
				}
				if granted {
					let keys = [CNContactEmailAddressesKey, CNContactPhoneNumbersKey,
								CNContactFamilyNameKey, CNContactGivenNameKey, CNContactNicknameKey,
								CNContactPostalAddressesKey, CNContactIdentifierKey,
								CNInstantMessageAddressUsernameKey, CNContactInstantMessageAddressesKey,
								CNContactImageDataKey, CNContactThumbnailImageDataKey, CNContactOrganizationNameKey]
					let request = CNContactFetchRequest(keysToFetch: keys as [CNKeyDescriptor])
					do {
						try store.enumerateContacts(with: request, usingBlock: { (contact, _) in
							DispatchQueue.main.sync {
								let newContact = Contact(
									firstName: contact.givenName,
									lastName: contact.familyName,
									organizationName: contact.organizationName,
									jobTitle: "",
									displayName: contact.nickname,
									sipAddresses: contact.instantMessageAddresses.map { $0.value.service == "SIP" ? $0.value.username : "" },
									phoneNumbers: contact.phoneNumbers.map { PhoneNumber(numLabel: $0.label ?? "", num: $0.value.stringValue)},
									imageData: ""
								)
								
								let imageThumbnail = UIImage(data: contact.thumbnailImageData ?? Data())
								self.saveImage(
									image: imageThumbnail
									?? self.textToImage(
										firstName: contact.givenName.isEmpty
										&& contact.familyName.isEmpty
										&& contact.phoneNumbers.first?.value.stringValue != nil
										? contact.phoneNumbers.first!.value.stringValue
										: contact.givenName, lastName: contact.familyName),
									name: contact.givenName + contact.familyName + String(Int.random(in: 1...1000)) + ((imageThumbnail == nil) ? "-default" : ""),
									contact: newContact, linphoneFriend: false, existingFriend: nil)
							}
						})
						
					} catch let error {
						print("Failed to enumerate contact", error)
					}
					
				} else {
					print("access denied")
				}
			}
			self.magicSearch.searchForContacts(sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
		}
	}
	
	func textToImage(firstName: String, lastName: String) -> UIImage {
		let lblNameInitialize = UILabel()
		lblNameInitialize.frame.size = CGSize(width: 100.0, height: 100.0)
		lblNameInitialize.font = UIFont(name: "NotoSans-ExtraBold", size: 40)
		lblNameInitialize.textColor = UIColor(Color.grayMain2c600)
		
		var textToDisplay = ""
		if firstName.first != nil {
			textToDisplay += String(firstName.first!)
		}
		if lastName.first != nil {
			textToDisplay += String(lastName.first!)
		}
		
		lblNameInitialize.text = textToDisplay.uppercased()
		lblNameInitialize.textAlignment = .center
		lblNameInitialize.backgroundColor = UIColor(Color.grayMain2c200)
		lblNameInitialize.layer.cornerRadius = 10.0
		
		var IBImgViewUserProfile = UIImage()
		UIGraphicsBeginImageContext(lblNameInitialize.frame.size)
		lblNameInitialize.layer.render(in: UIGraphicsGetCurrentContext()!)
		IBImgViewUserProfile = UIGraphicsGetImageFromCurrentImageContext()!
		UIGraphicsEndImageContext()
		
		return IBImgViewUserProfile
	}
	
	func saveImage(image: UIImage, name: String, contact: Contact, linphoneFriend: Bool, existingFriend: Friend?) {
		guard let data = image.jpegData(compressionQuality: 1) ?? image.pngData() else {
			return
		}
		
		awaitDataWrite(data: data, name: name) { _, result in
			let resultFriend = self.saveFriend(result: result, contact: contact, existingFriend: existingFriend)
			
			if resultFriend != nil {
				if linphoneFriend && existingFriend == nil {
					_ = self.linphoneFriendList!.addLocalFriend(linphoneFriend: resultFriend!)
					
					self.linphoneFriendList!.updateSubscriptions()
				} else if existingFriend == nil {
					_ = self.friendList!.addLocalFriend(linphoneFriend: resultFriend!)
					
					self.friendList!.updateSubscriptions()
				}
			}
		}
	}
	
	func saveFriend(result: String, contact: Contact, existingFriend: Friend?) -> Friend? {
		do {
			let friend = (existingFriend != nil) ? existingFriend : try self.coreContext.mCore.createFriend()
			
			if friend != nil {
				friend!.edit()
				
				try friend!.setName(newValue: contact.firstName + " " + contact.lastName)
				
				let friendvCard = friend!.vcard
				
				if friendvCard != nil {
					friendvCard!.givenName = contact.firstName
					friendvCard!.familyName = contact.lastName
				}
				
				friend!.organization = contact.organizationName
				
				var friendAddresses: [Address] = []
				friend?.addresses.forEach({ address in
					friend?.removeAddress(address: address)
				})
				contact.sipAddresses.forEach { sipAddress in
					let address = self.coreContext.mCore.interpretUrl(url: sipAddress, applyInternationalPrefix: true)
					
					if address != nil && ((friendAddresses.firstIndex(where: {$0.asString() == address?.asString()})) == nil) {
						friend!.addAddress(address: address!)
						friendAddresses.append(address!)
					}
				}
				
				var friendPhoneNumbers: [PhoneNumber] = []
				friend?.phoneNumbersWithLabel.forEach({ phoneNumber in
					friend?.removePhoneNumberWithLabel(phoneNumber: phoneNumber)
				})
				contact.phoneNumbers.forEach { phone in
					do {
						if (friendPhoneNumbers.firstIndex(where: {$0.num == phone.num})) == nil {
							let labelDrop = String(phone.numLabel.dropFirst(4).dropLast(4))
							let phoneNumber = try Factory.Instance.createFriendPhoneNumber(phoneNumber: phone.num, label: labelDrop)
							friend!.addPhoneNumberWithLabel(phoneNumber: phoneNumber)
							friendPhoneNumbers.append(phone)
						}
					} catch let error {
						print("Failed to enumerate contact", error)
					}
				}
				
				friend!.photo = "file:/" + result
				
				friend!.organization = contact.organizationName
				friend!.jobTitle = contact.jobTitle
				
				friend!.done()
				return friend
			}
		} catch let error {
			print("Failed to enumerate contact", error)
			return nil
		}
		return nil
	}
	
	func getImagePath(friendPhotoPath: String) -> URL {
		let friendPath = String(friendPhotoPath.dropFirst(6))
		
		let imagePath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!.appendingPathComponent(friendPath)
		
		return imagePath
	}
	
	func awaitDataWrite(data: Data, name: String, completion: @escaping ((), String) -> Void) {
		let directory = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first
		
		if directory != nil {
			DispatchQueue.main.async {
				do {
					let urlName = URL(string: name)
					let imagePath = urlName != nil ? urlName!.absoluteString.replacingOccurrences(of: "%", with: "") : String(Int.random(in: 1...1000))
					let decodedData: () = try data.write(to: directory!.appendingPathComponent(imagePath + ".png"))
					completion(decodedData, imagePath + ".png")
				} catch {
					print("Error: ", error)
					completion((), "")
				}
			}
		}
	}
}

struct PhoneNumber {
	var numLabel: String
	var num: String
}

struct Contact: Identifiable {
	var id = UUID()
	var firstName: String
	var lastName: String
	var organizationName: String
	var jobTitle: String
	var displayName: String
	var sipAddresses: [String] = []
	var phoneNumbers: [PhoneNumber] = []
	var imageData: String
}
