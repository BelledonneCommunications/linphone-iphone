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
	
	let friend: Friend?
	
	let name: String
	
	let address: String
	
	@Published var addresses: [String]
	
	let nativeUri: String
	
	let withPresence: Bool?
	
	@Published var lastPresenceInfo: String
	
	@Published var presenceStatus: ConsolidatedPresence
	
	private var friendDelegate: FriendDelegate?
	
	init(friend: Friend?, name: String, address: String, withPresence: Bool?) {
		self.friend = friend
		self.name = name
		self.address = address
		var addressesTmp: [String] = []
		if friend != nil {
			friend!.addresses.forEach { address in
				addressesTmp.append(address.asStringUriOnly())
			}
		}
		self.addresses = addressesTmp
		self.nativeUri = friend?.nativeUri ?? ""
		self.withPresence = withPresence
		if friend != nil &&
			withPresence == true {
			self.lastPresenceInfo = ""
			
			self.presenceStatus = friend!.consolidatedPresence
			
			if friend!.consolidatedPresence == .Online || friend!.consolidatedPresence == .Busy {
				if friend!.consolidatedPresence == .Online || friend!.presenceModel!.latestActivityTimestamp != -1 {
					self.lastPresenceInfo = (friend!.consolidatedPresence == .Online) ?
					"Online" : getCallTime(startDate: friend!.presenceModel!.latestActivityTimestamp)
				} else {
					self.lastPresenceInfo = "Away"
				}
			} else {
				self.lastPresenceInfo = ""
			}
			
			if let delegate = friendDelegate {
				self.friend?.removeDelegate(delegate: delegate)
				self.friendDelegate = nil
			}
			
			addFriendDelegate()
		} else {
			self.lastPresenceInfo = ""
			self.presenceStatus = .Offline
		}
	}
	
	func addFriendDelegate() {
		friendDelegate = FriendDelegateStub(onPresenceReceived: { (friend: Friend) in
			let latestActivityTimestamp = friend.presenceModel?.latestActivityTimestamp ?? -1
			DispatchQueue.main.async {
				self.presenceStatus = friend.consolidatedPresence
				if friend.consolidatedPresence == .Online || friend.consolidatedPresence == .Busy {
					if friend.consolidatedPresence == .Online || latestActivityTimestamp != -1 {
						self.lastPresenceInfo = friend.consolidatedPresence == .Online ?
						"Online" : self.getCallTime(startDate: latestActivityTimestamp)
					} else {
						self.lastPresenceInfo = "Away"
					}
				} else {
					self.lastPresenceInfo = ""
				}
			}
		})
		friend?.addDelegate(delegate: friendDelegate!)
	}
	
	func removeFriendDelegate() {
		if let delegate = friendDelegate {
			presenceStatus = .Offline
			friend?.removeDelegate(delegate: delegate)
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
					let name = address.displayName != nil ? address.displayName! : address.username!
					completion(ContactAvatarModel(friend: nil, name: name, address: address.asStringUriOnly(), withPresence: false))
				}
			} else {
				let name = address.displayName != nil ? address.displayName! : address.username!
				completion(ContactAvatarModel(friend: nil, name: name, address: address.asStringUriOnly(), withPresence: false))
			}
		}
	}
}
