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

// swiftlint:disable line_length
// swiftlint:disable function_parameter_count

import linphonesw
import Contacts
import SwiftUI
import ContactsUI
import Combine

final class ContactsManager: ObservableObject {
	
	static let shared = ContactsManager()
	
	private var coreContext = CoreContext.shared
	
	private let nativeAddressBookFriendList = "Native address-book"
	let linphoneAddressBookFriendList = "Linphone address-book"
	
	var friendList: FriendList?
	var linphoneFriendList: FriendList?
	
	@Published var lastSearch: [SearchResult] = []
	@Published var lastSearchSuggestions: [SearchResult] = []
	@Published var avatarListModel: [ContactAvatarModel] = []
	
	private var friendListDelegate: FriendListDelegate?
	
	private init() {}
	
	func fetchContacts() {
		coreContext.doOnCoreQueue { core in
			if core.globalState == GlobalState.Shutdown || core.globalState == GlobalState.Off {
				print("\(#function) - Core is being stopped or already destroyed, abort")
			} else {
				do {
					self.friendList = try core.getFriendListByName(name: self.nativeAddressBookFriendList) ?? core.createFriendList()
				} catch let error {
					print("\(#function) - Failed to enumerate contacts: \(error)")
				}
				
				if let friendList = self.friendList {
					if friendList.displayName == nil || friendList.displayName!.isEmpty {
						print("\(#function) - Friend list '\(self.nativeAddressBookFriendList)' didn't exist yet, let's create it")
						friendList.databaseStorageEnabled = false // We don't want to store local address-book in DB
						friendList.displayName = self.nativeAddressBookFriendList
						core.addFriendList(list: friendList)
					} else {
						print("\(#function) - Friend list '\(friendList.displayName!) found, removing existing friends if any")
						friendList.friends.forEach { friend in
							_ = friendList.removeFriend(linphoneFriend: friend)
						}
					}
				}
				
				do {
					self.linphoneFriendList = try core.getFriendListByName(name: self.linphoneAddressBookFriendList) ?? core.createFriendList()
				} catch let error {
					print("\(#function) - Failed to enumerate contacts: \(error)")
				}
				
				if let linphoneFriendList = self.linphoneFriendList {
					if linphoneFriendList.displayName == nil || linphoneFriendList.displayName!.isEmpty {
						print("\(#function) - Friend list \(self.linphoneAddressBookFriendList) didn't exist yet, let's create it")
						linphoneFriendList.databaseStorageEnabled = true
						linphoneFriendList.displayName = self.linphoneAddressBookFriendList
						core.addFriendList(list: linphoneFriendList)
					}
					linphoneFriendList.subscriptionsEnabled = true
				}
			}
			
			let store = CNContactStore()
            
			store.requestAccess(for: .contacts) { (granted, error) in
				if let error = error {
					print("\(#function) - failed to request access", error)
					return
				}
				if granted {
					let keys = [CNContactEmailAddressesKey, CNContactPhoneNumbersKey,
								CNContactFamilyNameKey, CNContactGivenNameKey, CNContactNicknameKey,
								CNContactPostalAddressesKey, CNContactIdentifierKey,
								CNInstantMessageAddressUsernameKey, CNContactInstantMessageAddressesKey,
								CNContactOrganizationNameKey, CNContactImageDataAvailableKey, CNContactImageDataKey, CNContactThumbnailImageDataKey]
					let request = CNContactFetchRequest(keysToFetch: keys as [CNKeyDescriptor])
					do {
						var contactCounter = 0
						try store.enumerateContacts(with: request, usingBlock: { (contact, _) in
							DispatchQueue.main.async {
								let newContact = Contact(
									identifier: contact.identifier,
									firstName: contact.givenName,
									lastName: contact.familyName,
									organizationName: contact.organizationName,
									jobTitle: "",
									displayName: contact.nickname,
									sipAddresses: contact.instantMessageAddresses.map { $0.value.service.lowercased() == "SIP".lowercased() ? $0.value.username : "" },
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
									name: contact.givenName + contact.familyName,
									prefix: ((imageThumbnail == nil) ? "-default" : ""),
									contact: newContact, linphoneFriend: false, existingFriend: nil) {
										if (self.friendList?.friends.count ?? 0) + (self.linphoneFriendList?.friends.count ?? 0) == contactCounter {
											// Every contact properly added, proceed
											self.linphoneFriendList?.updateSubscriptions()
											self.friendList?.updateSubscriptions()
											
											if let friendListDelegate = self.friendListDelegate {
												self.friendList?.removeDelegate(delegate: friendListDelegate)
											}
											self.friendListDelegate = FriendListDelegateStub(onNewSipAddressDiscovered: { (_: FriendList, linphoneFriend: Friend, sipUri: String) in
												
												var addedAvatarListModel: [ContactAvatarModel] = []
												linphoneFriend.phoneNumbers.forEach { phone in
													let address = core.interpretUrl(url: phone, applyInternationalPrefix: true)
													
													let presence = linphoneFriend.getPresenceModelForUriOrTel(uriOrTel: address?.asStringUriOnly() ?? "")
													if address != nil && presence != nil {
														linphoneFriend.edit()
														linphoneFriend.addAddress(address: address!)
														linphoneFriend.done()
														
														addedAvatarListModel.append(
															ContactAvatarModel(
																friend: linphoneFriend,
																name: linphoneFriend.name ?? "",
																address: linphoneFriend.address?.clone()?.asStringUriOnly() ?? "",
																withPresence: true
															)
														)
													}
												}
												
												DispatchQueue.main.async {
													self.avatarListModel += addedAvatarListModel
												}
												
												MagicSearchSingleton.shared.searchForContacts(sourceFlags: MagicSearch.Source.Friends.rawValue | MagicSearch.Source.LdapServers.rawValue)
											})
											self.friendList?.addDelegate(delegate: self.friendListDelegate!)
										}
									}
							}
							
							if !(contact.givenName.isEmpty && contact.familyName.isEmpty) {
								contactCounter += 1
							}
						})
						
					} catch let error {
						print("\(#function) - Failed to enumerate contact", error)
					}
					
				} else {
					print("\(#function) - access denied")
				}
			}
		}
	}
	
	func textToImage(firstName: String, lastName: String) -> UIImage {
		let lblNameInitialize = UILabel()
		lblNameInitialize.frame.size = CGSize(width: 200.0, height: 200.0)
		lblNameInitialize.font = UIFont(name: "NotoSans-ExtraBold", size: 80)
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
	
	func saveImage(image: UIImage, name: String, prefix: String, contact: Contact, linphoneFriend: Bool, existingFriend: Friend?, completion: @escaping () -> Void) {
		guard let data = image.jpegData(compressionQuality: 1) ?? image.pngData() else {
			return
		}
		
		awaitDataWrite(data: data, name: name, prefix: prefix) { _, result in
			self.saveFriend(result: result, contact: contact, existingFriend: existingFriend) { resultFriend in
				if resultFriend != nil {
					if linphoneFriend && existingFriend == nil {
						_ = self.linphoneFriendList?.addFriend(linphoneFriend: resultFriend!)
					} else if existingFriend == nil {
						_ = self.friendList?.addLocalFriend(linphoneFriend: resultFriend!)
					}
				}
				completion()
			}
		}
	}
	
	func saveFriend(result: String, contact: Contact, existingFriend: Friend?, completion: @escaping (Friend?) -> Void) {
		self.coreContext.doOnCoreQueue { core in
			do {
				let friend = try existingFriend ?? core.createFriend()
                
				friend.edit()
				friend.nativeUri = contact.identifier
				try friend.setName(newValue: contact.firstName + " " + contact.lastName)
				
				let friendvCard = friend.vcard
				
				if friendvCard != nil {
					friendvCard!.givenName = contact.firstName
					friendvCard!.familyName = contact.lastName
				}
				
				friend.organization = contact.organizationName
				
				var friendAddresses: [Address] = []
				friend.addresses.forEach({ address in
					friend.removeAddress(address: address)
				})
				contact.sipAddresses.forEach { sipAddress in
					if !sipAddress.isEmpty {
						let address = core.interpretUrl(url: sipAddress, applyInternationalPrefix: true)
						
						if address != nil && ((friendAddresses.firstIndex(where: {$0.asString() == address?.asString()})) == nil) {
							friend.addAddress(address: address!)
							friendAddresses.append(address!)
						}
					}
				}
				
				var friendPhoneNumbers: [PhoneNumber] = []
				friend.phoneNumbersWithLabel.forEach({ phoneNumber in
					friend.removePhoneNumberWithLabel(phoneNumber: phoneNumber)
				})
				contact.phoneNumbers.forEach { phone in
					do {
						if (friendPhoneNumbers.firstIndex(where: {$0.num == phone.num})) == nil {
							let labelDrop = String(phone.numLabel.dropFirst(4).dropLast(4))
							let phoneNumber = try Factory.Instance.createFriendPhoneNumber(phoneNumber: phone.num, label: labelDrop)
							friend.addPhoneNumberWithLabel(phoneNumber: phoneNumber)
							friendPhoneNumbers.append(phone)
						}
					} catch let error {
						print("\(#function) - Failed to create friend phone number for \(phone.numLabel):", error)
					}
				}
				
				friend.photo = "file:/" + result
				friend.organization = contact.organizationName
				friend.jobTitle = contact.jobTitle
				
				try friend.setSubscribesenabled(newValue: false)
				try friend.setIncsubscribepolicy(newValue: .SPDeny)
				
				friend.done()
				
				completion(friend)
			} catch let error {
				print("Failed to enumerate contact", error)
				completion(nil)
			}
		}
	}
	
	func getImagePath(friendPhotoPath: String) -> URL {
		let friendPath = String(friendPhotoPath.dropFirst(6))
		
		let imagePath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first!.appendingPathComponent(friendPath)
		
		return imagePath
	}
	
	func awaitDataWrite(data: Data, name: String, prefix: String, completion: @escaping ((), String) -> Void) {
		let directory = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first
		
		if directory != nil {
			DispatchQueue.main.async {
				do {
					let urlName = URL(string: name + prefix)
					let imagePath = urlName != nil ? urlName!.absoluteString.replacingOccurrences(of: "%", with: "") : "ImageError"
					
					let decodedData: () = try data.write(to: directory!.appendingPathComponent(imagePath + ".png"))
					
					completion(decodedData, imagePath + ".png")
				} catch {
					print("Error: ", error)
					completion((), "")
				}
			}
		}
	}
	
	func getFriendWithContact(contact: Contact) -> Friend? {
		if friendList != nil {
			let friend = friendList!.friends.first(where: {$0.nativeUri == contact.identifier})
			if friend == nil && friendList != nil {
				return linphoneFriendList!.friends.first(where: {$0.nativeUri == contact.identifier})
			}
			return friend
		} else {
			return nil
		}
	}
	
	func getFriendWithAddress(address: Address?) -> Friend? {
		if address != nil {
			let clonedAddress = address!.clone()
			clonedAddress!.clean()
			let sipUri = clonedAddress!.asStringUriOnly()
			
			if self.friendList != nil && !self.friendList!.friends.isEmpty {
				var friend: Friend?
				friend = self.friendList!.friends.first(where: {$0.addresses.contains(where: {$0.asStringUriOnly() == sipUri})})
				if friend == nil && self.linphoneFriendList != nil && !self.linphoneFriendList!.friends.isEmpty {
					friend = self.linphoneFriendList!.friends.first(where: {$0.addresses.contains(where: {$0.asStringUriOnly() == sipUri})})
				}
				
				return friend
			} else {
				return nil
			}
		} else {
			return nil
		}
	}
	
	func getFriendWithAddressInCoreQueue(address: Address?, completion: @escaping (Friend?) -> Void) {
		self.coreContext.doOnCoreQueue { _ in
			completion(self.getFriendWithAddress(address: address))
		}
	}
}

struct PhoneNumber {
	var numLabel: String
	var num: String
}

struct Contact: Identifiable {
	var id = UUID()
	var identifier: String
	var firstName: String
	var lastName: String
	var organizationName: String
	var jobTitle: String
	var displayName: String
	var sipAddresses: [String] = []
	var phoneNumbers: [PhoneNumber] = []
	var imageData: String
}

// swiftlint:enable line_length
// swiftlint:enable function_parameter_count
