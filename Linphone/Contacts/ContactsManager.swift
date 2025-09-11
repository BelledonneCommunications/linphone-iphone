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
	
	static let TAG = "[ContactsManager]"
	
	static let shared = ContactsManager()
	
	private var coreContext = CoreContext.shared
	
	private let nativeAddressBookFriendList = "Native address-book"
	let linphoneAddressBookFriendList = "Linphone address-book"
    let tempRemoteAddressBookFriendList = "TempRemoteDirectoryContacts address-book"
	
	var friendList: FriendList?
	var linphoneFriendList: FriendList?
    var tempRemoteFriendList: FriendList?
	
	@Published var lastSearch: [SearchResult] = []
	@Published var lastSearchSuggestions: [SearchResult] = []
	@Published var avatarListModel: [ContactAvatarModel] = [] {
		didSet {
			setupSubscriptions()
		}
	}
	
	@Published var starredChangeTrigger = UUID()
	private var cancellables = Set<AnyCancellable>()
	
	private var coreDelegate: CoreDelegate?
	private var friendListDelegate: FriendListDelegate?
	private var magicSearchDelegate: MagicSearchDelegate?
	
	private init() {}
	
	func fetchContacts() {
		self.coreContext.doOnCoreQueue { core in
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
				}
                
                do {
                    self.tempRemoteFriendList = try core.getFriendListByName(name: self.tempRemoteAddressBookFriendList) ?? core.createFriendList()
                } catch let error {
                    print("\(#function) - Failed to enumerate contacts: \(error)")
                }
                
                if let tempRemoteFriendList = self.tempRemoteFriendList {
                    if tempRemoteFriendList.displayName == nil || tempRemoteFriendList.displayName!.isEmpty {
                        print("\(#function) - Friend list \(self.tempRemoteAddressBookFriendList) didn't exist yet, let's create it")
                        tempRemoteFriendList.databaseStorageEnabled = true
                        tempRemoteFriendList.displayName = self.tempRemoteAddressBookFriendList
                        core.addFriendList(list: tempRemoteFriendList)
                    }
                }
			}
			
			let store = CNContactStore()
			store.requestAccess(for: .contacts) { (granted, error) in
				if let error = error {
					print("\(#function) - failed to request access", error)
					self.addFriendListDelegate()
					self.addCoreDelegate(core: core)
					MagicSearchSingleton.shared.searchForContacts()
					return
				}
				if granted {
					let keys = [CNContactEmailAddressesKey, CNContactPhoneNumbersKey,
								CNContactFamilyNameKey, CNContactGivenNameKey, CNContactNicknameKey,
								CNContactPostalAddressesKey, CNContactIdentifierKey,
								CNInstantMessageAddressUsernameKey, CNContactInstantMessageAddressesKey,
								CNContactOrganizationNameKey, CNContactImageDataAvailableKey, CNContactImageDataKey, CNContactThumbnailImageDataKey]
					
					let request = CNContactFetchRequest(keysToFetch: keys as [CNKeyDescriptor])
					
					let dispatchGroup = DispatchGroup()
					
					do {
						try store.enumerateContacts(with: request, usingBlock: { (contact, _) in
							
							dispatchGroup.enter()
							
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
							if let image = imageThumbnail {
								self.saveImage(
									image: image,
									name: contact.givenName + contact.familyName,
									prefix: "",
									contact: newContact, linphoneFriend: self.nativeAddressBookFriendList, existingFriend: nil) {
										dispatchGroup.leave()
									}
							} else {
								let image = self.textToImage(firstName: contact.givenName, lastName: contact.familyName)
								self.saveImage(
									image: image,
									name: contact.givenName + contact.familyName,
									prefix: "-default",
									contact: newContact, linphoneFriend: self.nativeAddressBookFriendList, existingFriend: nil) {
										dispatchGroup.leave()
									}
							}
						})
						
						dispatchGroup.notify(queue: .main) {
							self.addFriendListDelegate()
							self.addCoreDelegate(core: core)
							MagicSearchSingleton.shared.searchForContacts()
						}
					} catch let error {
						print("\(#function) - Failed to enumerate contact", error)
						self.addFriendListDelegate()
						self.addCoreDelegate(core: core)
						MagicSearchSingleton.shared.searchForContacts()
					}
				} else {
					print("\(#function) - access denied")
					self.addFriendListDelegate()
					self.addCoreDelegate(core: core)
					MagicSearchSingleton.shared.searchForContacts()
				}
			}
		}
	}
	
	func textToImage(firstName: String?, lastName: String?) -> UIImage {
		let firstInitial = firstName?.first.map { String($0) } ?? ""
		let lastInitial = lastName?.first.map { String($0) } ?? ""
		let textToDisplay = (firstInitial + lastInitial).uppercased()

		let size = CGSize(width: 200, height: 200)
		let renderer = UIGraphicsImageRenderer(size: size)

		return renderer.image { _ in
			let rect = CGRect(origin: .zero, size: size)
			
			UIColor(Color.grayMain2c200).setFill()
			UIBezierPath(roundedRect: rect, cornerRadius: 10).fill()
			
			let paragraph = NSMutableParagraphStyle()
			paragraph.alignment = .center

			let attributes: [NSAttributedString.Key: Any] = [
				.font: UIFont(name: "NotoSans-ExtraBold", size: 80) ?? UIFont.boldSystemFont(ofSize: 80),
				.foregroundColor: UIColor(Color.grayMain2c600),
				.paragraphStyle: paragraph
			]

			let textSize = textToDisplay.size(withAttributes: attributes)
			let textRect = CGRect(
				x: (size.width - textSize.width) / 2,
				y: (size.height - textSize.height) / 2,
				width: textSize.width,
				height: textSize.height
			)

			textToDisplay.draw(in: textRect, withAttributes: attributes)
		}
	}
	
	func saveImage(image: UIImage, name: String, prefix: String, contact: Contact, linphoneFriend: String, existingFriend: Friend?, completion: @escaping () -> Void) {
		guard let data = image.jpegData(compressionQuality: 1) ?? image.pngData() else {
			return
		}
		
		awaitDataWrite(data: data, name: name, prefix: prefix) { result in
			self.saveFriend(result: result, contact: contact, existingFriend: existingFriend) { resultFriend in
				if resultFriend != nil {
					if linphoneFriend != self.nativeAddressBookFriendList && existingFriend == nil {
                        if let linphoneFL = self.linphoneFriendList, linphoneFriend == linphoneFL.displayName {
							_ = linphoneFL.addFriend(linphoneFriend: resultFriend!)
                        } else if let linphoneFL = self.tempRemoteFriendList {
                            _ = linphoneFL.addFriend(linphoneFriend: resultFriend!)
                        }
					} else if existingFriend == nil {
						if let friendListTmp = self.friendList {
							_ = friendListTmp.addLocalFriend(linphoneFriend: resultFriend!)
						}
					}
				}
				DispatchQueue.main.async { completion() }
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
				let existingAddresses = Array(friend.addresses)
				for address in existingAddresses {
					friend.removeAddress(address: address)
				}
				
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
				let existingPhoneNumbers = Array(friend.phoneNumbersWithLabel)
				for phoneNumber in existingPhoneNumbers {
					friend.removePhoneNumberWithLabel(phoneNumber: phoneNumber)
				}
				
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
	
	func awaitDataWrite(data: Data, name: String, prefix: String, completion: @escaping (String) -> Void) {
		guard let directory = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask).first else {
			completion("")
			return
		}
		
		do {
			let fileName = name + prefix + ".png"
			let fileURL = directory.appendingPathComponent(fileName)
			
			try data.write(to: fileURL)
			completion(fileName)
		} catch {
			print("Error writing image: \(error)")
			completion("")
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
		guard let address = address, let clonedAddress = address.clone() else {
			return nil
		}
		clonedAddress.clean()
		let sipUri = clonedAddress.asStringUriOnly()
		
		var friend: Friend?
		
		if let friendList = self.friendList {
			friend = friendList.friends.first(where: { $0.addresses.contains(where: { $0.asStringUriOnly() == sipUri }) })
		}
		if friend == nil, let linphoneFriendList = self.linphoneFriendList {
			friend = linphoneFriendList.friends.first(where: { $0.addresses.contains(where: { $0.asStringUriOnly() == sipUri }) })
        }
        if friend == nil, let tempRemoteFriendList = self.tempRemoteFriendList {
            friend = tempRemoteFriendList.friends.first(where: { $0.addresses.contains(where: { $0.asStringUriOnly() == sipUri }) })
        }
		
		return friend
	}
	
	func getFriendWithAddressInCoreQueue(address: Address?, completion: @escaping (Friend?) -> Void) {
		self.coreContext.doOnCoreQueue { _ in
			completion(self.getFriendWithAddress(address: address))
		}
	}
	
	func addFriendListDelegate() {
		self.coreContext.doOnCoreQueue { _ in
			CoreContext.shared.mCore.friendListSubscriptionEnabled = true
			
			CoreContext.shared.mCore.friendsLists.forEach { friendList in
				friendList.updateSubscriptions()
			}
			
			let friendListDelegateTmp = FriendListDelegateStub(
				onContactCreated: { (friendList: FriendList, linphoneFriend: Friend) in
					Log.info("\(ContactsManager.TAG) FriendListDelegateStub onContactCreated")
				},
				onContactDeleted: { (friendList: FriendList, linphoneFriend: Friend) in
					Log.info("\(ContactsManager.TAG) FriendListDelegateStub onContactDeleted")
				},
				onContactUpdated: { (friendList: FriendList, newFriend: Friend, oldFriend: Friend) in
					Log.info("\(ContactsManager.TAG) FriendListDelegateStub onContactUpdated")
				},
				onSyncStatusChanged: { (friendList: FriendList, status: FriendList.SyncStatus?, message: String?) in
					Log.info("\(ContactsManager.TAG) FriendListDelegateStub onSyncStatusChanged")
					if status == .Successful {
                        if friendList.displayName != self.nativeAddressBookFriendList && friendList.displayName != self.linphoneAddressBookFriendList {
                            if let tempRemoteFriendList = self.tempRemoteFriendList {
								tempRemoteFriendList.friends.forEach { friend in
									if let friendAddress = friend.address,
									   friendList.friends.contains(where: { $0.address?.weakEqual(address2: friendAddress) == true }) {
										_ = tempRemoteFriendList.removeFriend(linphoneFriend: friend)
									}
                                }
                            }
                        }
                        
                        let dispatchGroup = DispatchGroup()
                        
						friendList.friends.forEach { friend in
                            dispatchGroup.enter()
							let addressTmp = friend.address?.clone()?.asStringUriOnly() ?? ""
							
							let newContact = Contact(
								identifier: UUID().uuidString,
								firstName: friend.name ?? addressTmp,
								lastName: "",
								organizationName: "",
								jobTitle: "",
								displayName: friend.address?.displayName ?? "",
								sipAddresses: friend.addresses.map { $0.asStringUriOnly() },
								phoneNumbers: [],
								imageData: ""
							)
							
							let image = self.textToImage(firstName: friend.name ?? addressTmp, lastName: "")
							self.saveImage(
								image: image,
								name: friend.name ?? addressTmp,
								prefix: "-default",
								contact: newContact, linphoneFriend: friendList.displayName ?? "No Display Name", existingFriend: nil) {
									dispatchGroup.leave()
								}
						}
                        
                        dispatchGroup.notify(queue: .main) {
                            MagicSearchSingleton.shared.searchForContacts()
                            if let linphoneFL = self.tempRemoteFriendList {
                                linphoneFL.updateSubscriptions()
                            }
                        }
					}
				},
				onPresenceReceived: { (friendList: FriendList, friends: [Friend?]) in
					Log.info("\(ContactsManager.TAG) FriendListDelegateStub onPresenceReceived \(friends.count)")
					if (friendList.isSubscriptionBodyless) {
						Log.info("\(ContactsManager.TAG) Bodyless friendlist \(friendList.displayName ?? "No Display Name") presence received")
						
						if friendList.displayName != self.nativeAddressBookFriendList && friendList.displayName != self.linphoneAddressBookFriendList {
							if let tempRemoteFriendList = self.tempRemoteFriendList {
								tempRemoteFriendList.friends.forEach { friend in
									if let friendAddress = friend.address,
									   friends.contains(where: { $0?.address?.weakEqual(address2: friendAddress) == true }) {
										_ = tempRemoteFriendList.removeFriend(linphoneFriend: friend)
									}
								}
							}
						}
						
						let dispatchGroup = DispatchGroup()
						
						friends.forEach { friend in
							dispatchGroup.enter()
							if let friend = friend {
								let addressTmp = friend.address?.clone()?.asStringUriOnly() ?? ""
								Log.debug("\(ContactsManager.TAG) Newly discovered SIP Address \(addressTmp) for friend \(friend.name ?? "No Name") in bodyless list \(friendList.displayName ?? "No Display Name")")
								
								let newContact = Contact(
									identifier: UUID().uuidString,
									firstName: friend.name ?? addressTmp,
									lastName: "",
									organizationName: "",
									jobTitle: "",
									displayName: friend.address?.displayName ?? "",
									sipAddresses: friend.addresses.map { $0.asStringUriOnly() },
									phoneNumbers: [],
									imageData: ""
								)
								
								let image = self.textToImage(firstName: friend.name ?? addressTmp, lastName: "")
								self.saveImage(
									image: image,
									name: friend.name ?? addressTmp,
									prefix: "-default",
									contact: newContact, linphoneFriend: friendList.displayName ?? "No Display Name", existingFriend: nil) {
										dispatchGroup.leave()
									}
							}
						}
						
						dispatchGroup.notify(queue: .main) {
							MagicSearchSingleton.shared.searchForContacts()
							if let linphoneFL = self.tempRemoteFriendList {
								linphoneFL.updateSubscriptions()
							}
						}
					}
				},
				onNewSipAddressDiscovered: { (friendList: FriendList, linphoneFriend: Friend, sipUri: String) in
					Log.info("\(ContactsManager.TAG) FriendListDelegateStub onNewSipAddressDiscovered \(linphoneFriend.name ?? "")")
					var addedAvatarListModel: [ContactAvatarModel] = []
					if !self.avatarListModel.contains(where: {$0.friend?.name == linphoneFriend.name}) {
						if let address = try? Factory.Instance.createAddress(addr: sipUri) {
							linphoneFriend.edit()
							linphoneFriend.addAddress(address: address)
							linphoneFriend.done()
							
							let addressTmp = linphoneFriend.address?.clone()?.asStringUriOnly() ?? ""
							addedAvatarListModel.append(
								ContactAvatarModel(
									friend: linphoneFriend,
									name: linphoneFriend.name ?? "",
									address: addressTmp,
									withPresence: true
								)
							)
							
							addedAvatarListModel += self.avatarListModel
							addedAvatarListModel = addedAvatarListModel.sorted { $0.name < $1.name }
							
							DispatchQueue.main.async {
								self.avatarListModel = addedAvatarListModel
								
								NotificationCenter.default.post(
									name: NSNotification.Name("ContactAdded"),
									object: nil,
									userInfo: ["address": addressTmp]
								)
							}
						}
					}
				}
			)
			
			CoreContext.shared.mCore.friendsLists.forEach { friendList in
				friendList.addDelegate(delegate: friendListDelegateTmp)
			}
		}
	}
	
	func addCoreDelegate(core: Core) {
		self.coreContext.doOnCoreQueue { _ in
			self.coreDelegate = CoreDelegateStub(
				onFriendListCreated: { (_: Core, friendList: FriendList) in
					Log.info("\(ContactsManager.TAG) Friend list \(friendList.displayName) created")
					if self.friendListDelegate != nil {
						friendList.addDelegate(delegate: self.friendListDelegate!)
					}
				}, onFriendListRemoved: { (_: Core, friendList: FriendList) in
					Log.info("\(ContactsManager.TAG) Friend list \(friendList.displayName) removed")
					if self.friendListDelegate != nil {
						friendList.removeDelegate(delegate: self.friendListDelegate!)
					}
				}, onDefaultAccountChanged: { (_: Core, _: Account?) in
					Log.info("\(ContactsManager.TAG) Default account changed, update all contacts' model showTrust value")
					//updateContactsModelDependingOnDefaultAccountMode()
				}
			)
			
			if self.coreDelegate != nil {
				core.addDelegate(delegate: self.coreDelegate!)
			}
		}
	}
	
	private func setupSubscriptions() {
		cancellables.removeAll()
		for contact in avatarListModel {
			contact.$starred
				.sink { [weak self] _ in
					self?.starredChangeTrigger = UUID()
				}
				.store(in: &cancellables)
		}
	}
	
	func updateSubscriptionsLinphoneList() {
		if let linphoneFL = self.linphoneFriendList {
			linphoneFL.updateSubscriptions()
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
