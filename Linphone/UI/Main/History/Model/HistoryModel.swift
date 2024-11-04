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
	
	var callLog: CallLog
	
	var id: String
	@Published var subject: String
	@Published var isConf: Bool
	@Published var addressLinphone: Address
	@Published var address: String
	@Published var addressName: String
	@Published var isOutgoing: Bool
	@Published var status: Call.Status
	@Published var startDate: time_t
	@Published var duration: Int
	@Published var addressFriend: Friend?
	@Published var avatarModel: ContactAvatarModel?
	
	init(callLog: CallLog) {
		self.callLog = callLog
		self.id = ""
		self.subject = ""
		self.isConf = false
		
		self.addressLinphone = callLog.dir == .Outgoing && callLog.toAddress != nil ? callLog.toAddress! : callLog.fromAddress!
		self.address = ""
		
		self.addressName = ""
		
		self.isOutgoing = false
		
		self.status = .Success
		
		self.startDate = 0
		
		self.duration = 0
		
		self.initValue(callLog: callLog)
	}
	
	func initValue(callLog: CallLog) {
		coreContext.doOnCoreQueue { _ in
			let callLogTmp = callLog
			let idTmp = callLog.callId ?? ""
			let subjectTmp = callLog.conferenceInfo != nil && callLog.conferenceInfo!.subject != nil ? callLog.conferenceInfo!.subject! : ""
			let isConfTmp = callLog.conferenceInfo != nil
			
			let addressLinphoneTmp = callLog.dir == .Outgoing && callLog.toAddress != nil ? callLog.toAddress! : callLog.fromAddress!
			
			let addressNameTmp = callLog.conferenceInfo != nil && callLog.conferenceInfo!.subject != nil
			? callLog.conferenceInfo!.subject!
			: (addressLinphoneTmp.username != nil ? addressLinphoneTmp.username ?? "" : addressLinphoneTmp.displayName ?? "")
			
			let addressTmp = addressLinphoneTmp.asStringUriOnly()
			
			let isOutgoingTmp = callLog.dir == .Outgoing
			
			let statusTmp = callLog.status
			
			let startDateTmp = callLog.startDate
			
			let durationTmp = callLog.duration
			
			DispatchQueue.main.async {
				self.callLog = callLogTmp
				self.id = idTmp
				self.subject = subjectTmp
				self.isConf = isConfTmp
				
				self.addressLinphone = addressLinphoneTmp
				self.address = addressTmp
				
				self.addressName = addressNameTmp
				
				self.isOutgoing = isOutgoingTmp
				
				self.status = statusTmp
				
				self.startDate = startDateTmp
				
				self.duration = durationTmp
			}
			
			self.refreshAvatarModel()
		}
	}
	
	func refreshAvatarModel() {
		coreContext.doOnCoreQueue { _ in
			let addressFriendTmp = ContactsManager.shared.getFriendWithAddress(
				address: self.callLog.dir == .Outgoing ? self.callLog.toAddress! : self.callLog.fromAddress!
			)
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
