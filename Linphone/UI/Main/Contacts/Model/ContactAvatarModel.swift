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

import Foundation
import linphonesw
import Combine

class ContactAvatarModel: ObservableObject, Identifiable {
	let id = UUID()
	
	var friend: Friend?
	
	@Published var name: String = ""
	@Published var address: String = ""
	@Published var addresses: [String] = []
	@Published var phoneNumbersWithLabel: [(label: String, phoneNumber: String)] = []
	
	var nativeUri: String = ""
	var editable: Bool = true
	var isReadOnly: Bool = false
	var withPresence: Bool?
	
	@Published var starred: Bool = false
	
	var vcard: Vcard?
	var organization: String = ""
	var jobTitle: String = ""
	
	@Published var photo: String = ""
	@Published var lastPresenceInfo: String = ""
	@Published var presenceStatus: ConsolidatedPresence = .Offline
	
	private var friendDelegate: FriendDelegate?
	
	init(friend: Friend?, name: String, address: String, withPresence: Bool?) {
		self.name = name
	 	self.address = address
		self.resetContactAvatarModel(friend: friend, name: name, address: address, withPresence: withPresence)
	}
	
	func resetContactAvatarModel(friend: Friend?, name: String, address: String, withPresence: Bool?) {
		CoreContext.shared.doOnCoreQueue { _ in
			self.friend = friend
			let nameTmp = name
			let addressTmp = address
			var addressesTmp: [String] = []
			if let friend = friend {
				friend.addresses.forEach { address in
					addressesTmp.append(address.asStringUriOnly())
				}
			}
			var phoneNumbersWithLabelTmp: [(label: String, phoneNumber: String)] = []
			if let friend = friend {
				friend.phoneNumbersWithLabel.forEach { phoneNum in
					phoneNumbersWithLabelTmp.append((label: phoneNum.label ?? "", phoneNumber: phoneNum.phoneNumber))
				}
			}
			let nativeUriTmp = friend?.nativeUri ?? ""
			let editableTmp = friend?.friendList?.type == .CardDAV || nativeUriTmp.isEmpty
			let isReadOnlyTmp = (friend?.isReadOnly == true) || (friend?.inList() == false)
			let withPresenceTmp = withPresence
			let starredTmp = friend?.starred ?? false
			let vcardTmp = friend?.vcard ?? nil
			let organizationTmp = friend?.organization ?? ""
			let jobTitleTmp = friend?.jobTitle ?? ""
			var photoTmp = friend?.photo ?? ""
			
			if friend?.friendList?.type == .CardDAV && friend?.photo?.isEmpty == false {
				let fileName = "file:/" + name + ".png"
				photoTmp = fileName.replacingOccurrences(of: " ", with: "")
			}
			
			var lastPresenceInfoTmp = ""
			var presenceStatusTmp: ConsolidatedPresence = .Offline
			
			if let friend = friend, withPresence == true {
                
				lastPresenceInfoTmp = ""
				
				presenceStatusTmp = friend.consolidatedPresence
                
				if friend.consolidatedPresence == .Online || friend.consolidatedPresence == .Busy {
					if friend.consolidatedPresence == .Online || friend.presenceModel?.latestActivityTimestamp != -1 {
						lastPresenceInfoTmp = (friend.consolidatedPresence == .Online) ?
						"Online" : self.getCallTime(startDate: friend.presenceModel!.latestActivityTimestamp)
					} else {
						lastPresenceInfoTmp = "Away"
					}
				}
				
				if let delegate = self.friendDelegate {
					self.friend?.removeDelegate(delegate: delegate)
					self.friendDelegate = nil
				}
				
				self.addFriendDelegate()
			}
			
			DispatchQueue.main.async {
				self.name = nameTmp
				self.address = addressTmp
				self.addresses = addressesTmp
				self.phoneNumbersWithLabel = phoneNumbersWithLabelTmp
				self.nativeUri = nativeUriTmp
				self.editable = editableTmp
				self.isReadOnly = isReadOnlyTmp
				self.withPresence = withPresenceTmp
				self.starred = starredTmp
				self.vcard = vcardTmp
				self.organization = organizationTmp
				self.jobTitle = jobTitleTmp
				self.photo = photoTmp
				self.lastPresenceInfo = lastPresenceInfoTmp
				self.presenceStatus = presenceStatusTmp
			}
		}
	}
	
	func addFriendDelegate() {
		friendDelegate = FriendDelegateStub(onPresenceReceived: { (friend: Friend) in
			let latestActivityTimestamp = friend.presenceModel?.latestActivityTimestamp ?? -1
			let consolidatedPresenceTmp = friend.consolidatedPresence
			DispatchQueue.main.async {
				self.presenceStatus = consolidatedPresenceTmp
				if consolidatedPresenceTmp == .Online || consolidatedPresenceTmp == .Busy {
					if consolidatedPresenceTmp == .Online || latestActivityTimestamp != -1 {
						self.lastPresenceInfo = consolidatedPresenceTmp == .Online ?
						"Online" : self.getCallTime(startDate: latestActivityTimestamp)
					} else {
						self.lastPresenceInfo = "Away"
					}
				} else {
					self.lastPresenceInfo = ""
				}
			}
		})
		
		if friend != nil && friendDelegate != nil {
			friend!.addDelegate(delegate: friendDelegate!)
		}
	}
	
	func removeFriendDelegate() {
		if let delegate = friendDelegate {
			DispatchQueue.main.async {
				self.presenceStatus = .Offline
			}
			if let friendTmp = friend {
				friendTmp.removeDelegate(delegate: delegate)
			}
			friendDelegate = nil
		}
	}
	
	func getCallTime(startDate: time_t) -> String {
		let timeInterval = TimeInterval(startDate)
		
		let myNSDate = Date(timeIntervalSince1970: timeInterval)
		
		if Calendar.current.isDateInToday(myNSDate) {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
			return "Online today at " + formatter.string(from: myNSDate)
		} else if Calendar.current.isDateInYesterday(myNSDate) {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "HH:mm" : "h:mm a"
			return "Online yesterday at " + formatter.string(from: myNSDate)
		} else if Calendar.current.isDate(myNSDate, equalTo: .now, toGranularity: .year) {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "dd/MM | HH:mm" : "MM/dd | h:mm a"
			return "Online on " + formatter.string(from: myNSDate)
		} else {
			let formatter = DateFormatter()
			formatter.dateFormat = Locale.current.identifier == "fr_FR" ? "dd/MM/yy | HH:mm" : "MM/dd/yy | h:mm a"
			return "Online on " + formatter.string(from: myNSDate)
		}
	}
	
	static func getAvatarModelFromAddress(address: Address, completion: @escaping (ContactAvatarModel) -> Void) {
		ContactsManager.shared.getFriendWithAddressInCoreQueue(address: address) { resultFriend in
			if let addressFriend = resultFriend {
				if addressFriend.address != nil {
					var avatarModel = ContactsManager.shared.avatarListModel.first(where: {
						$0.friend != nil && $0.friend!.name == addressFriend.name && $0.friend!.address != nil
						&& $0.friend!.address!.asStringUriOnly() == addressFriend.address!.asStringUriOnly()
					})
					
					if avatarModel == nil {
						avatarModel = ContactAvatarModel(friend: nil, name: addressFriend.name!, address: addressFriend.address!.asStringUriOnly(), withPresence: false)
					}
					completion(avatarModel!)
				} else {
					var name = ""
					if address.displayName != nil {
						name = address.displayName!
					} else if address.username != nil {
						name = address.username!
					} else {
						name = String(address.asStringUriOnly().dropFirst(4))
					}
					completion(ContactAvatarModel(friend: nil, name: name, address: address.asStringUriOnly(), withPresence: false))
				}
			} else {
				var name = ""
				if address.displayName != nil {
					name = address.displayName!
				} else if address.username != nil {
					name = address.username!
				} else {
					name = String(address.asStringUriOnly().dropFirst(4))
				}
				completion(ContactAvatarModel(friend: nil, name: name, address: address.asStringUriOnly(), withPresence: false))
			}
		}
	}
}
