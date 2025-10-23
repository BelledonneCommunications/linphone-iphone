/*
 * Copyright (c) 2010-2025 Belledonne Communications SARL.
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
import Combine
import SwiftUI
import linphonesw

class CardDavViewModel: ObservableObject {
	static let TAG = "[CardDAV ViewModel]"
	
	private var coreContext = CoreContext.shared
	
	let linphoneAddressBookFriendList = "Linphone address-book"
	let tempRemoteAddressBookFriendList = "TempRemoteDirectoryContacts address-book"
	
	@Published var isEdit: Bool = false
	@Published var displayName: String = ""
	@Published var serverUrl: String = ""
	@Published var username: String = ""
	@Published var password: String = ""
	@Published var realm: String = ""
	@Published var storeNewContactsInIt: Bool = false
	@Published var isReadOnly: Bool = false
	
	var isFormComplete: Bool {
		!displayName.isEmpty &&
		!serverUrl.isEmpty &&
		!username.isEmpty &&
		!realm.isEmpty
	}
	
	@Published var cardDavServerOperationInProgress = false
	@Published var cardDavServerOperationSuccessful = false
	
	private var friendList: FriendList?
	private var friendListDelegate: FriendListDelegate?
	
	init(name: String? = "") {
		isEdit = false
		cardDavServerOperationInProgress = false
		storeNewContactsInIt = false
		
		if let name = name, !name.isEmpty {
			loadcardDav(name: name)
		}
	}
	
	deinit {
		if let friendList = self.friendList, let friendListDelegate = self.friendListDelegate {
			self.coreContext.doOnCoreQueue { core in
				friendList.removeDelegate(delegate: friendListDelegate)
			}
		}
	}
	
	func loadcardDav(name: String) {
		self.coreContext.doOnCoreQueue { core in
			let found = core.getFriendListByName(name: name)
			guard let found = found else {
				Log.error("\(CardDavViewModel.TAG) Failed to find friend list with display name \(name)!")
				return
			}
			
			self.friendList = found
			
			guard let friendList = self.friendList else {
				return
			}
			
			let isReadOnlyTmp = friendList.isReadOnly
			let friendListInWhichStoreNewlyCreatedFriendsTmp = CorePreferences.friendListInWhichStoreNewlyCreatedFriends
			let uriTmp = friendList.uri ?? ""
			DispatchQueue.main.async {
				self.isEdit = true
				self.isReadOnly = isReadOnlyTmp
				
				self.displayName = name
				self.storeNewContactsInIt = name == friendListInWhichStoreNewlyCreatedFriendsTmp
				
				self.serverUrl = uriTmp
			}
			Log.info("\(CardDavViewModel.TAG) Existing friend list CardDAV values loaded")
		}
	}
	
	func delete() {
		self.coreContext.doOnCoreQueue { core in
			if self.isEdit, let friendList = self.friendList {
				let name = friendList.displayName
				if name == CorePreferences.friendListInWhichStoreNewlyCreatedFriends {
					Log.info("\(CardDavViewModel.TAG) Deleting friend list configured to be used to store newly created friends, updating default friend list back to \(self.linphoneAddressBookFriendList)")
					CorePreferences.friendListInWhichStoreNewlyCreatedFriends = self.linphoneAddressBookFriendList
				}
				
				if let tempRemoteFriendList = core.getFriendListByName(name: self.tempRemoteAddressBookFriendList) {
					tempRemoteFriendList.friends.forEach { friend in
						if let friendAddress = friend.address,
						   friendList.friends.contains(where: { $0.address?.weakEqual(address2: friendAddress) == true }) {
							_ = tempRemoteFriendList.removeFriend(linphoneFriend: friend)
						}
					}
				}
				
				core.removeFriendList(list: friendList)
				Log.info("\(CardDavViewModel.TAG) Removed friends list with display name \(name ?? "")")
				
				Log.info("\(CardDavViewModel.TAG) Notifying contacts manager that contacts have changed")
				MagicSearchSingleton.shared.searchForContacts()
				
				DispatchQueue.main.async {
					NotificationCenter.default.post(name: NSNotification.Name("ContactLoaded"), object: nil)
					self.cardDavServerOperationSuccessful = true
					
					ToastViewModel.shared.toastMessage = "Success_settings_contacts_carddav_deleted_toast"
					ToastViewModel.shared.displayToast = true
				}
			}
		}
	}
	
	func addAddressBook() {
		let name = displayName
		let server = serverUrl
		if name.isEmpty || server.isEmpty {
			return
		}
		
		let user = username
		let pwd = password
		let authRealm = realm
		
		self.coreContext.doOnCoreQueue { core in
			// TODO: add dialog to ask user before removing existing friend list & auth info ?
			if !self.isEdit == false {
				let foundFriendList = core.getFriendListByName(name: name)
				if let foundFriendList = foundFriendList {
					Log.warn("\(CardDavViewModel.TAG) Friend list \(name) already exists, removing it first")
					core.removeFriendList(list: foundFriendList)
				}
			}
			
			if !user.isEmpty && !authRealm.isEmpty {
				let foundAuthInfo = core.findAuthInfo(realm: authRealm, username: user, sipDomain: nil)
				if let foundAuthInfo = foundAuthInfo {
					Log.warn("\(CardDavViewModel.TAG) Auth info with username \(user) already exists, removing it first")
					core.removeAuthInfo(info: foundAuthInfo)
				}
				
				Log.info("\(CardDavViewModel.TAG) Adding auth info with username \(user)")
				if let authInfo = try? Factory.Instance.createAuthInfo(
					username: user,
					userid: nil,
					passwd: pwd,
					ha1: nil,
					realm: authRealm,
					domain: nil
				) {
					core.addAuthInfo(info: authInfo)
				}
			}
			
			if self.isEdit && self.friendList != nil {
				Log.info("\(CardDavViewModel.TAG) Changes were made to CardDAV friend list \(name), synchronizing it")
			} else {
				self.friendList = try? core.createFriendList()
				
				guard let friendList = self.friendList else {
					Log.error("\(CardDavViewModel.TAG) Failed to create CardDAV friend list")
					return
				}
				
				friendList.displayName = name
				friendList.type = .CardDAV
				friendList.uri = if (server.hasPrefix("http://") || server.hasPrefix("https://")) {
					server
				} else {
					"https://$server"
				}
				friendList.databaseStorageEnabled = true
				
				self.addFriendListDelegate(friendList: friendList)
				
				core.addFriendList(list: friendList)
				
				Log.info("\(CardDavViewModel.TAG) CardDAV friend list \(name) created with server URL \(server), synchronizing it")
			}
			
			if !self.storeNewContactsInIt && CorePreferences.friendListInWhichStoreNewlyCreatedFriends == name {
				Log.info("\(CardDavViewModel.TAG) No longer using friend list \(name) as default friend list, switching back to \(self.linphoneAddressBookFriendList)")
				CorePreferences.friendListInWhichStoreNewlyCreatedFriends = self.linphoneAddressBookFriendList
			}
			
			if let friendList = self.friendList {
				friendList.synchronizeFriendsFromServer()
			}
			
			DispatchQueue.main.async {
				self.cardDavServerOperationInProgress = true
			}
		}
	}
	
	func addFriendListDelegate(friendList: FriendList) {
		self.coreContext.doOnCoreQueue { core in
			let delegate = FriendListDelegateStub(
				onSyncStatusChanged: { (friendList: FriendList, status: FriendList.SyncStatus, message: String?) in
					Log.info("\(CardDavViewModel.TAG) Friend list \(friendList.displayName ?? "") sync status changed to \(status) with message \(message ?? "")")
					switch status {
					case .Successful:
						DispatchQueue.main.async {
							self.cardDavServerOperationInProgress = false
							
							ToastViewModel.shared.toastMessage = "Success_settings_contacts_carddav_sync_successful_toast"
							ToastViewModel.shared.displayToast = true
						}
						
						let name = self.displayName
						if self.storeNewContactsInIt {
							let previous = CorePreferences.friendListInWhichStoreNewlyCreatedFriends
							if friendList.isReadOnly {
								Log.warn("\(CardDavViewModel.TAG) User asked to add newly created contacts in this friend list but it is read only, keep currently default friend list \(previous)")
								self.storeNewContactsInIt = false
							} else {
								Log.info("\(CardDavViewModel.TAG) Updating default friend list to store newly created contacts from \(previous) to \(name)")
								CorePreferences.friendListInWhichStoreNewlyCreatedFriends = name
							}
							
							DispatchQueue.main.async {
								self.isReadOnly = friendList.isReadOnly
							}
						}
						
						Log.info("\(CardDavViewModel.TAG) Notifying contacts manager that contacts have changed")
						
						DispatchQueue.main.async {
							self.cardDavServerOperationSuccessful = true
						}
					case .Failure:
						DispatchQueue.main.async {
							self.cardDavServerOperationInProgress = false
							
							ToastViewModel.shared.toastMessage = "settings_contacts_carddav_sync_error_toast"
							ToastViewModel.shared.displayToast = true
						}
						if !self.isEdit {
							Log.error("\(CardDavViewModel.TAG) Synchronization failed, removing Friend list from Core")
							if let friendListDelegate = self.friendListDelegate {
								friendList.removeDelegate(delegate: friendListDelegate)
							}
							core.removeFriendList(list: friendList)
						}
					default: break
					}
					
				})
			
			self.friendListDelegate = delegate
			
			if let friendList = self.friendList {
				friendList.addDelegate(delegate: delegate)
			}
		}
	}
}

