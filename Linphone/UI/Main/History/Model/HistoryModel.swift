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

class HistoryModel: ObservableObject {
	
	private var coreContext = CoreContext.shared
	
	static let TAG = "[History Model]"
	
	let callLog: CallLog
	
	let id: String
	@Published var subject: String
	@Published var isConf: Bool
	@Published var addressLinphone: Address
	@Published var address: String
	@Published var addressName: String
	@Published var isOutgoing: Bool
	@Published var status: Call.Status
	@Published var startDate: time_t
	@Published var duration: Int
	@Published var addressFriend: Friend? = nil
	@Published var avatarModel: ContactAvatarModel? = nil
	
	init(callLog: CallLog) {
		self.callLog = callLog
		self.id = callLog.callId ?? ""
		self.subject = callLog.conferenceInfo != nil && callLog.conferenceInfo!.subject != nil ? callLog.conferenceInfo!.subject! : ""
		self.isConf = callLog.conferenceInfo != nil
		
		let addressLinphoneTmp = callLog.dir == .Outgoing && callLog.toAddress != nil ? callLog.toAddress! : callLog.fromAddress!
		self.addressLinphone = addressLinphoneTmp
		//let addressLinphone = callLog.dir == .Outgoing && callLog.toAddress != nil ? callLog.toAddress! : callLog.fromAddress!
		self.address = addressLinphoneTmp.asStringUriOnly()
		
		let addressNameTmp = callLog.conferenceInfo != nil && callLog.conferenceInfo!.subject != nil 
		? callLog.conferenceInfo!.subject!
		: (addressLinphoneTmp.username != nil ? addressLinphoneTmp.username ?? "" : addressLinphoneTmp.displayName ?? "")
		
		self.addressName = addressNameTmp
		
		self.isOutgoing = callLog.dir == .Outgoing
		
		self.status = callLog.status
		
		self.startDate = callLog.startDate
		
		self.duration = callLog.duration
		
		refreshAvatarModel()
	}
	
	func refreshAvatarModel() {
		coreContext.doOnCoreQueue { _ in
			let addressFriendTmp = ContactsManager.shared.getFriendWithAddress(address: self.callLog.dir == .Outgoing ? self.callLog.toAddress! : self.callLog.fromAddress!)
			if addressFriendTmp != nil {
				self.addressFriend = addressFriendTmp
				
				let addressNameTmp = self.addressName
				
				let avatarModelTmp = addressFriendTmp != nil
				? ContactsManager.shared.avatarListModel.first(where: {
					$0.friend!.name == addressFriendTmp!.name
					&& $0.friend!.address!.asStringUriOnly() == addressFriendTmp!.address!.asStringUriOnly()
				}) ?? ContactAvatarModel(friend: nil, name: self.addressName, address: self.address, withPresence: false)
				: ContactAvatarModel(friend: nil, name: self.addressName, address: self.address, withPresence: false)
				
				DispatchQueue.main.async {
					self.addressFriend = addressFriendTmp
					self.addressName = addressFriendTmp!.name ?? addressNameTmp
					self.avatarModel = avatarModelTmp
				}
			} else {
				DispatchQueue.main.async {
					self.avatarModel = ContactAvatarModel(friend: nil, name: self.addressName, address: self.address, withPresence: false)
				}
			}
		}
	}
}
